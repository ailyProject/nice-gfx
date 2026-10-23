#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include "Esp32Qspi.h"
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include <driver/gpio.h>
#include <esp_heap_caps.h>
#include <esp_memory_utils.h>
#include <string.h>

namespace nice_gfx {
// IDF 5.5 adds per-transaction frequency changes; SFINAE keeps older Arduino
// cores source-compatible without guessing the SDK feature from version macros.
template<class Transaction> static auto overrideFrequency(Transaction &t,uint32_t frequency,int)
    -> decltype(t.override_freq_hz=frequency,bool()) { t.override_freq_hz=frequency; return true; }
template<class Transaction> static bool overrideFrequency(Transaction &,uint32_t,long) { return false; }
template<class Bus> static auto disableOctalPins(Bus &bus,int)
    -> decltype(bus.data4_io_num=-1,void()) {
  bus.data4_io_num=bus.data5_io_num=bus.data6_io_num=bus.data7_io_num=-1;
}
template<class Bus> static void disableOctalPins(Bus &,long) {}
static Status statusOf(esp_err_t error) {
  switch (error) {
    case ESP_OK: return Status::Ok;
    case ESP_ERR_NO_MEM: return Status::NoMemory;
    case ESP_ERR_INVALID_ARG: return Status::InvalidArgument;
    case ESP_ERR_INVALID_STATE: return Status::Busy;
    default: return Status::IoError;
  }
}
static bool dmaTxFailed(const spi_transaction_t *transaction) {
#ifdef SPI_TRANS_DMA_TX_FAIL
  return transaction && (transaction->flags & SPI_TRANS_DMA_TX_FAIL);
#else
  (void)transaction;return false;
#endif
}
static esp_err_t checkedPolling(spi_device_handle_t device,spi_transaction_t *transaction) {
  const esp_err_t error=spi_device_polling_transmit(device,transaction);
  return error==ESP_OK&&dmaTxFailed(transaction) ? ESP_FAIL : error;
}
Status Esp32Qspi::begin() {
  if (device_) return Status::Ok;
  const int logicalPins[] = {config_.pins.clock, config_.pins.d0, config_.pins.d1,
                            config_.pins.d2, config_.pins.d3, config_.pins.cs, config_.dc};
  int pins[7];
  for (unsigned i=0; i<7; ++i) pins[i]=Esp32Pin::gpioNumber(logicalPins[i]);
  for (unsigned i=0; i<7; ++i) {
    if (logicalPins[i] < 0 && ((!config_.quad && i >= 2 && i <= 5) || (config_.quad && i == 6))) continue;
    if (i == 2 && !config_.quad) {
      if (!GPIO_IS_VALID_GPIO(pins[i])) return Status::InvalidArgument;
    } else if (!GPIO_IS_VALID_OUTPUT_GPIO(pins[i])) return Status::InvalidArgument;
    for (unsigned j=0; j<i; ++j) if (pins[i] == pins[j]) return Status::InvalidArgument;
  }
  if (config_.writeStrobePin >= 0) {
    const int strobe=Esp32Pin::gpioNumber(config_.writeStrobePin);
    if (config_.quad || !config_.paddedCommands || !GPIO_IS_VALID_OUTPUT_GPIO(strobe))
      return Status::InvalidArgument;
    for (int pin : pins) if (pin == strobe) return Status::InvalidArgument;
  }
#if defined(SOC_SPI_PERIPH_NUM)
  if (int(config_.host) < int(SPI2_HOST) || int(config_.host) >= SOC_SPI_PERIPH_NUM) return Status::InvalidArgument;
#else
  if (config_.host != SPI2_HOST && config_.host != SPI3_HOST) return Status::InvalidArgument;
#endif
  if (config_.frequency == 0 || config_.frequency > 80000000 || !config_.readFrequency || config_.readFrequency>80000000 || config_.mode > 3 ||
      config_.chunkBytes < 4 || config_.chunkBytes > 32768 || config_.chunkBytes % 4 ||
      config_.stagingBytes < 4 || config_.stagingBytes > 32768 || config_.stagingBytes % 4)
    return Status::InvalidArgument;
  transferBytes_=config_.chunkBytes;
#if defined(CONFIG_IDF_TARGET_ESP32S31)
  // S31 SPI_LL_DMA_MAX_BIT_LEN=0x3ffff. 32768 bytes is one bit too
  // large; round down to whole 32-bit DMA units, including direct RAM input.
  if(transferBytes_>32764)transferBytes_=32764;
#endif
  stagingBytes_=config_.stagingBytes<transferBytes_?config_.stagingBytes:transferBytes_;
  if (config_.quad && (!config_.protocol.pixelQuantum || config_.protocol.pixelQuantum>4 ||
      stagingBytes_<size_t(config_.protocol.pixelQuantum)*wirePixelBytes(format_))) return Status::InvalidArgument;
  stopping_ = false;
  result_ = Status::Ok;
  // Direct DMA needs descriptors, not equally large copies of both buffers.
  // Keep the default bounded 4 KiB staging; dedicated animation applications
  // may explicitly trade internal RAM for fewer queued copy transactions.
  for (auto &buffer : staging_) {
    buffer = static_cast<uint8_t *>(heap_caps_malloc(stagingBytes_, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    if (!buffer) { end(); return Status::NoMemory; }
  }
  done_ = xSemaphoreCreateBinary();
  if (!done_) { end(); return Status::NoMemory; }
  xSemaphoreGive(done_);
  spi_bus_config_t bus = {};
  disableOctalPins(bus,0);
  bus.sclk_io_num = pins[0];
  bus.mosi_io_num = pins[1];
  bus.miso_io_num = pins[2];
  bus.quadwp_io_num = config_.quad ? pins[3] : -1;
  bus.quadhd_io_num = config_.quad ? pins[4] : -1;
  bus.max_transfer_sz = static_cast<int>(config_.chunkBytes);
  esp_err_t error = spi_bus_initialize(config_.host, &bus, SPI_DMA_CH_AUTO);
  // Only resource contention triggers automatic selection. A wiring or memory
  // error must remain visible rather than silently selecting another peripheral.
#if !defined(SOC_SPI_PERIPH_NUM) || SOC_SPI_PERIPH_NUM > 2
  if (error==ESP_ERR_INVALID_STATE && config_.autoHost) {
    config_.host=config_.host==SPI2_HOST ? SPI3_HOST : SPI2_HOST;
    error=spi_bus_initialize(config_.host,&bus,SPI_DMA_CH_AUTO);
  }
#endif
  // Do not silently attach to a host whose pins/DMA limits we cannot verify.
  if (error != ESP_OK) { end(); return statusOf(error); }
  ownsBus_ = true;
  spi_device_interface_config_t device = {};
  device.command_bits = config_.quad ? 8 : 0;
  device.address_bits = config_.quad ? 24 : 0;
  device.clock_speed_hz = config_.frequency;
  device.mode = config_.mode;
  device.spics_io_num = pins[5];
  device.queue_size = 2;
  device.flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_NO_DUMMY | (config_.threeWire ? SPI_DEVICE_3WIRE : 0);
  error = spi_bus_add_device(config_.host, &device, &device_);
  if (error != ESP_OK) { end(); return statusOf(error); }
  currentFrequency_=config_.frequency;
  if (!config_.quad) esp32_platform::output(config_.dc,true);
  if (config_.writeStrobePin >= 0) writeStrobe_.attach(config_.writeStrobePin,true);
  // IDF installs the SPI interrupt on the initializing core. Keep the DMA
  // worker there too: a returned transaction can wake a worker on the other
  // core before spi_intr has finished its bus-lock bookkeeping. Releasing the
  // acquired bus at that point can race bg_exit_core (observed on IDF 5.5.5).
  if (xTaskCreatePinnedToCore(workerEntry, "gfx-qspi", 3072, this, 2, &worker_,
                            xPortGetCoreID()) != pdPASS) {
    worker_ = nullptr; end(); return Status::NoMemory;
  }
  return Status::Ok;
}
void Esp32Qspi::end() {
  if (worker_) {
    wait();
    xSemaphoreTake(done_, portMAX_DELAY);
    stopping_ = true;
    xTaskNotifyGive(worker_);
    xSemaphoreTake(done_, portMAX_DELAY);
    worker_ = nullptr;
  }
  if (device_) { spi_bus_remove_device(device_); device_ = nullptr; }
  currentFrequency_=0; pendingFrequency_=0;
  if (ownsBus_) { spi_bus_free(config_.host); ownsBus_ = false; }
  if (done_) { vSemaphoreDelete(done_); done_ = nullptr; }
  for (auto &buffer : staging_) { if (buffer) heap_caps_free(buffer); buffer = nullptr; }
  stagingBytes_=0;transferBytes_=0;
}
uint32_t Esp32Qspi::actualFrequency() const {
#if defined(CONFIG_IDF_TARGET_ESP32S31)
  int khz=0;
  return device_&&spi_device_get_actual_freq(device_,&khz)==ESP_OK&&khz>0 ? uint32_t(khz)*1000 : 0;
#else
  return 0;
#endif
}
Status Esp32Qspi::wait() {
  if (!done_) return device_ ? Status::IoError : Status::Ok;
  xSemaphoreTake(done_, portMAX_DELAY);
  Status result = result_;
  xSemaphoreGive(done_);
  return result;
}
bool Esp32Qspi::busy() const { return done_ && uxSemaphoreGetCount(done_) == 0; }
bool Esp32Qspi::canFrameBlit(PixelView view) const {
  if (!view.valid() || view.height > SIZE_MAX / view.width ||
      (view.order != PixelOrder::Native && view.order != PixelOrder::Wire)) return false;
  if (!config_.quad) return true;
  // image() can stage unaligned/PSRAM sources, but RAMWR/RAMWRC and every
  // independently staged row must still obey the controller's pixel quantum.
  const size_t quantum=config_.protocol.pixelQuantum;
  return quantum && quantum<=4 &&
      config_.chunkBytes>=quantum*wirePixelBytes(format_) &&
      (view.width*view.height)%quantum==0 &&
      (view.stride==view.width || view.width%quantum==0);
}
Status Esp32Qspi::command(uint8_t command, const uint8_t *data, size_t bytes) {
  return sendCommand(command,8,data,bytes);
}
Status Esp32Qspi::command16(uint16_t command, const uint8_t *data, size_t bytes) {
  return sendCommand(command,16,data,bytes);
}
Status Esp32Qspi::data(const uint8_t *data, size_t bytes) { return sendCommand(0,0,data,bytes); }
Status Esp32Qspi::setPixelFormat(PixelFormat format) {
  if (format != PixelFormat::RGB565 && format != PixelFormat::RGB666 && format != PixelFormat::RGB888)
    return Status::InvalidArgument;
  const size_t capacity=stagingBytes_?stagingBytes_:(config_.stagingBytes<config_.chunkBytes?config_.stagingBytes:config_.chunkBytes);
  if (config_.quad && capacity<size_t(config_.protocol.pixelQuantum)*wirePixelBytes(format))
    return Status::InvalidArgument;
  if (config_.writeStrobePin >= 0 && format != PixelFormat::RGB565) return Status::Unsupported;
  Status status=wait(); if (status==Status::Ok) format_=format; return status;
}
Status Esp32Qspi::prefix(uint16_t command, uint8_t bits, bool keepActive) {
  if (!bits || config_.quad) return Status::Ok;
  esp32_platform::write(config_.dc,false);
  uint8_t data[2]={uint8_t(command >> 8),uint8_t(command)};
  spi_transaction_t transaction = {};
  const bool wide=bits==16 || config_.paddedCommands;
  transaction.length=wide ? 16 : bits; transaction.tx_buffer=wide ? data : data+1;
  if (keepActive) transaction.flags=SPI_TRANS_CS_KEEP_ACTIVE;
  configureFrequency(transaction);
  Status s=statusOf(checkedPolling(device_,&transaction));
  esp32_platform::write(config_.dc,true); return s;
}
Status Esp32Qspi::sendCommand(uint16_t command, uint8_t bits, const uint8_t *data, size_t bytes) {
  if (!device_) return Status::NotInitialized;
  if (bytes && !data) return Status::InvalidArgument;
  if (config_.quad && bits!=8) return Status::Unsupported;
  Status s = wait();
  if (s != Status::Ok) return s;
  esp_err_t error=spi_device_acquire_bus(device_,portMAX_DELAY);
  if (error!=ESP_OK) return statusOf(error);
  s=prefix(command,bits,bytes!=0);
  bool first=true;
  if (s==Status::Ok && (bytes || config_.quad)) do {
    const bool padded=!config_.quad && config_.paddedCommands;
    size_t capacity=stagingBytes_/(padded ? 2 : 1);
    size_t count=bytes < capacity ? bytes : capacity;
    if (padded) for(size_t i=0;i<count;++i) { staging_[0][2*i]=0; staging_[0][2*i+1]=data[i]; }
    else if (count) memcpy(staging_[0],data,count);
    spi_transaction_ext_t t={};
    t.base.flags=SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    t.command_bits=config_.quad && first ? 8 : 0; t.address_bits=config_.quad && first ? 24 : 0;
    t.base.cmd=config_.protocol.commandOpcode; t.base.addr=config_.protocol.address(command);
    t.base.length=count*(padded ? 16 : 8); t.base.tx_buffer=count ? staging_[0] : nullptr;
    if (bytes > count) t.base.flags |= SPI_TRANS_CS_KEEP_ACTIVE;
    configureFrequency(t.base);
    error=checkedPolling(device_,&t.base);
    if (error!=ESP_OK) { s=statusOf(error); break; }
    first=false; if (count) data+=count; bytes-=count;
  } while (bytes);
  if (s!=Status::Ok) {
    spi_transaction_ext_t release={}; release.base.flags=SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    spi_device_polling_transmit(device_,&release.base);
  }
  spi_device_release_bus(device_); return result_=s;
}
Status Esp32Qspi::pixels(uint8_t command, PixelView view, bool async) {
  return image(command,8,view,async);
}
Status Esp32Qspi::pixels16(uint16_t command, PixelView view, bool async) { return image(command,16,view,async); }
Status Esp32Qspi::dataPixels(PixelView view, bool async) { return image(0,0,view,async); }
Status Esp32Qspi::image(uint16_t command, uint8_t bits, PixelView view, bool async) {
  if (!device_) return Status::NotInitialized;
  if (!view.valid() || view.height > SIZE_MAX / view.width) return Status::InvalidArgument;
  if (config_.quad && bits!=8 && !(bits==0 && config_.protocol.pixelContinuation)) return Status::Unsupported;
  if (config_.quad && ((view.width*view.height)%config_.protocol.pixelQuantum ||
      (view.stride!=view.width && view.width%config_.protocol.pixelQuantum))) return Status::InvalidArgument;
  Status s = wait();
  if (s != Status::Ok) return s;
  const size_t bpp=wirePixelBytes(format_);
  if (!async && view.width * view.height <= stagingBytes_ / bpp) {
    for (size_t row=0; row<view.height; ++row)
      encodePixels(staging_[0] + bpp*row*view.width, view.data + 2*row*view.stride, view.width, view.order,format_);
    return transmitSmall(command,bits,staging_[0],view.width*view.height*bpp);
  }
  job_.view = view; job_.command = command; job_.commandBits=bits; job_.solid = false;
  job_.count = view.width * view.height;
  return submit(async);
}
Status Esp32Qspi::fill(uint8_t command, uint16_t color, size_t count) {
  return solid(command,8,color,count);
}
Status Esp32Qspi::fill16(uint16_t command, uint16_t color, size_t count) { return solid(command,16,color,count); }
Status Esp32Qspi::dataFill(uint16_t color, size_t count) { return solid(0,0,color,count); }
Status Esp32Qspi::solid(uint16_t command, uint8_t bits, uint16_t color, size_t count) {
  if (!device_) return Status::NotInitialized;
  if (config_.quad && bits!=8 && !(bits==0 && config_.protocol.pixelContinuation)) return Status::Unsupported;
  if (!count) return Status::Ok;
  if (config_.quad && count%config_.protocol.pixelQuantum) return Status::InvalidArgument;
  Status s = wait();
  if (s != Status::Ok) return s;
  if (config_.writeStrobePin >= 0 && count > 1) return solidStrobe(command,bits,color,count);
  const size_t bpp=wirePixelBytes(format_);
  if (count <= stagingBytes_ / bpp) {
    encodePixels(staging_[0],reinterpret_cast<const uint8_t *>(&color),1,PixelOrder::Native,format_);
    for (size_t i=1; i<count; ++i) memcpy(staging_[0]+i*bpp,staging_[0],bpp);
    return transmitSmall(command,bits,staging_[0],count*bpp);
  }
  job_.command = command; job_.commandBits=bits; job_.solid = true; job_.color = color; job_.count = count;
  return submit(false);
}
Status Esp32Qspi::solidStrobe(uint16_t command, uint8_t bits, uint16_t color, size_t count) {
  // wait() above drains every asynchronous descriptor before GPIO can pulse WR.
  // The bridge latches the first RGB565 word and repeats it on each WR edge.
  esp_err_t error=spi_device_acquire_bus(device_,portMAX_DELAY);
  if (error!=ESP_OK) return result_=statusOf(error);
  Status status=prefix(command,bits,true);
  if (status==Status::Ok) {
    uint8_t pixel[]={uint8_t(color >> 8),uint8_t(color)};
    spi_transaction_t transaction={};
    transaction.flags=SPI_TRANS_CS_KEEP_ACTIVE;
    transaction.length=16; transaction.tx_buffer=pixel;
    configureFrequency(transaction);
    status=statusOf(checkedPolling(device_,&transaction));
    if (status==Status::Ok) {
      while (--count) { writeStrobe_.write(false); writeStrobe_.write(true); }
    }
  }
  // A transaction without data clocks releases the selected hardware CS.
  spi_transaction_t release={};
  Status released=statusOf(spi_device_polling_transmit(device_,&release));
  spi_device_release_bus(device_);
  return result_=status==Status::Ok ? released : status;
}
Status Esp32Qspi::transmitSmall(uint16_t command, uint8_t bits, const uint8_t *data, size_t bytes) {
  esp_err_t error=spi_device_acquire_bus(device_,portMAX_DELAY);
  if (error!=ESP_OK) return result_=statusOf(error);
  Status s=prefix(command,bits,true);
  spi_transaction_t transaction = {};
  transaction.flags = config_.quad ? SPI_TRANS_MODE_QIO : 0;
  transaction.cmd = config_.protocol.pixelOpcode;
  transaction.addr = config_.protocol.address(config_.protocol.pixelCommand(command,bits));
  transaction.length = bytes * 8; transaction.tx_buffer = data;
  configureFrequency(transaction);
  if (s==Status::Ok) s=statusOf(checkedPolling(device_, &transaction));
  if (s!=Status::Ok) {
    spi_transaction_ext_t release={}; release.base.flags=SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    spi_device_polling_transmit(device_,&release.base);
  }
  spi_device_release_bus(device_); return result_=s;
}
Status Esp32Qspi::submit(bool async) {
  xSemaphoreTake(done_, portMAX_DELAY);
  xTaskNotifyGive(worker_);
  return async ? Status::Ok : wait();
}
void Esp32Qspi::workerEntry(void *self) { static_cast<Esp32Qspi *>(self)->worker(); }
void Esp32Qspi::worker() {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (stopping_) { xSemaphoreGive(done_); vTaskDelete(nullptr); return; }
    result_ = transfer();
    xSemaphoreGive(done_);
  }
}
Status Esp32Qspi::transfer() {
  esp_err_t error = spi_device_acquire_bus(device_, portMAX_DELAY);
  if (error != ESP_OK) return statusOf(error);
  Status header=prefix(job_.command,job_.commandBits,true);
  if (header!=Status::Ok) {
    spi_transaction_ext_t release={}; release.base.flags=SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    spi_device_polling_transmit(device_,&release.base);
    spi_device_release_bus(device_); return header;
  }
  const size_t bpp=wirePixelBytes(format_);
  size_t remaining = job_.count, row = 0, column = 0;
  unsigned slot = 0, pending = 0;
  bool first = true;
  if (job_.solid) {
    for (unsigned b=0; b<2; ++b) {
      encodePixels(staging_[b],reinterpret_cast<const uint8_t *>(&job_.color),1,PixelOrder::Native,format_);
      for (size_t i=1; i<stagingBytes_/bpp; ++i) memcpy(staging_[b]+i*bpp,staging_[b],bpp);
    }
  }
  while (remaining && error == ESP_OK) {
    if (pending == 2) {
      spi_transaction_t *completed = nullptr;
      error = spi_device_get_trans_result(device_, &completed, portMAX_DELAY);
      if (error != ESP_OK) break;
      --pending;
      if(dmaTxFailed(completed)){error=ESP_FAIL;break;}
    }
    size_t count = remaining < transferBytes_/bpp ? remaining : transferBytes_/bpp;
    if (job_.solid && count > stagingBytes_/bpp) count = stagingBytes_/bpp;
    const uint8_t *source = nullptr;
    const bool strided=!job_.solid && job_.view.stride!=job_.view.width;
    size_t directCount=count;
    if (!job_.solid) {
      if (strided && directCount > job_.view.width-column)directCount=job_.view.width-column;
      source = job_.view.data + 2 * (row * job_.view.stride + column);
    }
    const bool direct = source && format_==PixelFormat::RGB565 && job_.view.order == PixelOrder::Wire &&
                        (reinterpret_cast<uintptr_t>(source) & 3) == 0 &&
                        (directCount & 1) == 0 && esp_ptr_dma_capable(source) &&
                        (!strided || directCount>=stagingBytes_/bpp);
    if(direct)count=directCount;
    else if(source && count>stagingBytes_/bpp)count=stagingBytes_/bpp;
    if(config_.quad)count-=count%config_.protocol.pixelQuantum;
    if(!count) { error=ESP_ERR_INVALID_ARG; break; }
    if (source && !direct) {
      if(!strided)encodePixels(staging_[slot],source,count,job_.view.order,format_);
      else {
        // Cached slide slices are commonly only a few hundred pixels wide.
        // Pack their visible rows into one bounded DMA buffer instead of
        // paying an interrupt/queue transaction for every short source row.
        // Neither source padding nor the next row is sent to the controller.
        size_t copied=0,sourceRow=row,sourceColumn=column;
        while(copied<count){
          size_t span=job_.view.width-sourceColumn;
          if(span>count-copied)span=count-copied;
          encodePixels(staging_[slot]+copied*bpp,
                       job_.view.data+2*(sourceRow*job_.view.stride+sourceColumn),
                       span,job_.view.order,format_);
          copied+=span;sourceColumn+=span;
          if(sourceColumn==job_.view.width){sourceColumn=0;++sourceRow;}
        }
      }
    }
    auto &transaction = transactions_[slot];
    memset(&transaction, 0, sizeof(transaction));
    transaction.base.flags = (config_.quad ? SPI_TRANS_MODE_QIO : 0) | SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    const bool split=config_.quad && config_.protocol.splitPixelTransactions;
    transaction.command_bits = config_.quad && (first || split) ? 8 : 0;
    transaction.address_bits = config_.quad && (first || split) ? 24 : 0;
    transaction.base.cmd = config_.protocol.pixelOpcode;
    const uint8_t pixelCommand=split && !first ? config_.protocol.continuationCommand : config_.protocol.pixelCommand(job_.command,job_.commandBits);
    transaction.base.addr = config_.protocol.address(pixelCommand);
    transaction.base.length = count * bpp * 8;
    transaction.base.tx_buffer = direct ? source : staging_[slot];
    if (remaining > count && !split) transaction.base.flags |= SPI_TRANS_CS_KEEP_ACTIVE;
    configureFrequency(transaction.base);
    error = spi_device_queue_trans(device_, &transaction.base, portMAX_DELAY);
    if (error != ESP_OK) break;
    ++pending; first = false;
    remaining -= count;
    column += count;
    if(strided){row+=column/job_.view.width;column%=job_.view.width;}
    slot ^= 1;
  }
  // A failed submission must not free/reuse buffers still owned by the driver.
  while (pending) {
    spi_transaction_t *completed = nullptr;
    esp_err_t drained = spi_device_get_trans_result(device_, &completed, portMAX_DELAY);
    if (drained != ESP_OK) {
      // With a valid handle and portMAX_DELAY IDF cannot time out; retain ownership
      // and retry instead of returning while DMA might still read the source.
      if (error == ESP_OK) error = drained;
      continue;
    }
    --pending;
    if(error==ESP_OK&&dmaTxFailed(completed))error=ESP_FAIL;
  }
  if (error != ESP_OK) {
    // Release a CS left active by the last successfully queued non-final chunk.
    spi_transaction_ext_t release = {};
    release.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    spi_device_polling_transmit(device_, &release.base);
  }
  spi_device_release_bus(device_);
  return statusOf(error);
}
Status Esp32Qspi::read(uint16_t command, uint8_t *out, size_t count, uint8_t dummy, uint8_t bits) {
  if (!device_) return Status::NotInitialized;
  if ((count && !out) || (bits!=8 && bits!=16 && bits!=0)) return Status::InvalidArgument;
  if ((config_.quad && bits!=8) || (!config_.quad && !config_.threeWire && config_.pins.d1 < 0)) return Status::Unsupported;
  Status s=wait(); if (s!=Status::Ok || !count) return s;
  s=setFrequency(config_.readFrequency); if (s!=Status::Ok) return result_=s;
  esp_err_t error=spi_device_acquire_bus(device_,portMAX_DELAY);
  if (error!=ESP_OK) return statusOf(error);
  if (!config_.quad && bits) {
    uint8_t bytes[2]={uint8_t(command >> 8),uint8_t(command)};
    spi_transaction_t t={}; t.flags=SPI_TRANS_CS_KEEP_ACTIVE;
    const bool wide=bits==16 || config_.paddedCommands;
    t.length=wide ? 16 : bits; t.tx_buffer=wide ? bytes : bytes+1;
    configureFrequency(t);
    esp32_platform::write(config_.dc,false); error=spi_device_polling_transmit(device_,&t); esp32_platform::write(config_.dc,true);
  }
  bool first=true; size_t skip=dummy;
  while ((count || skip) && error==ESP_OK) {
    size_t discarded=skip < stagingBytes_ ? skip : stagingBytes_;
    size_t capacity=stagingBytes_-discarded;
    size_t n=count < capacity ? count : capacity;
    spi_transaction_ext_t t={}; t.base.flags=SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    t.command_bits=config_.quad && first ? 8 : 0; t.address_bits=config_.quad && first ? 24 : 0;
    t.base.cmd=config_.protocol.readOpcode; t.base.addr=config_.protocol.address(command);
    t.base.rxlength=(n+discarded)*8; t.base.rx_buffer=staging_[0];
    if (count > n || skip > discarded) t.base.flags |= SPI_TRANS_CS_KEEP_ACTIVE;
    configureFrequency(t.base);
    error=spi_device_polling_transmit(device_,&t.base);
    if (error!=ESP_OK) break;
    memcpy(out,staging_[0]+discarded,n); out+=n; count-=n; skip-=discarded; first=false;
  }
  if (error!=ESP_OK) {
    spi_transaction_ext_t release={}; release.base.flags=SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    spi_device_polling_transmit(device_,&release.base);
  }
  spi_device_release_bus(device_);
  s=setFrequency(config_.frequency);
  return result_=error==ESP_OK ? s : statusOf(error);
}
Status Esp32Qspi::setFrequency(uint32_t frequency) {
  if (currentFrequency_==frequency) { pendingFrequency_=0; return Status::Ok; }
  spi_transaction_t probe={};
  if (overrideFrequency(probe,frequency,0)) { pendingFrequency_=frequency; return Status::Ok; }
  // Keep exactly one IDF device attached to this CS pin. Two handles using the
  // same GPIO would re-route that pin to the second hardware CS signal.
  esp_err_t error=spi_bus_remove_device(device_);
  if (error!=ESP_OK) return statusOf(error);
  device_=nullptr; currentFrequency_=0;
  spi_device_interface_config_t device={};
  device.command_bits=config_.quad ? 8 : 0; device.address_bits=config_.quad ? 24 : 0;
  device.clock_speed_hz=frequency; device.mode=config_.mode;
  device.spics_io_num=Esp32Pin::gpioNumber(config_.pins.cs); device.queue_size=2;
  device.flags=SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_NO_DUMMY | (config_.threeWire ? SPI_DEVICE_3WIRE : 0);
  error=spi_bus_add_device(config_.host,&device,&device_);
  if (error==ESP_OK) currentFrequency_=frequency;
  return statusOf(error);
}
void Esp32Qspi::configureFrequency(spi_transaction_t &transaction) {
  if (pendingFrequency_ && overrideFrequency(transaction,pendingFrequency_,0)) {
    currentFrequency_=pendingFrequency_; pendingFrequency_=0;
  }
}
void Esp32Qspi::output(int pin, bool high) { esp32_platform::output(pin,high); }
void Esp32Qspi::delayMs(uint32_t ms) { esp32_platform::delayMillis(ms); }
bool Esp32Qspi::validOutputPin(int pin) const {
  const auto &p = config_.pins;
  const int gpio=Esp32Pin::gpioNumber(pin);
  if (!GPIO_IS_VALID_OUTPUT_GPIO(gpio)) return false;
  const int reserved[]={p.clock,p.d0,p.d1,p.d2,p.d3,p.cs,config_.dc,config_.writeStrobePin};
  for (int used : reserved) if (gpio==Esp32Pin::gpioNumber(used)) return false;
  return true;
}
TransportInfo Esp32Qspi::info() const {
  TransportInfo result;
  result.frequency = config_.frequency; result.clock = config_.pins.clock; result.cs = config_.pins.cs;
  result.data[0] = config_.pins.d0; result.data[1] = config_.threeWire ? config_.pins.d0 : config_.pins.d1;
  result.data[2] = config_.pins.d2; result.data[3] = config_.pins.d3;
  result.dc=config_.dc; result.lanes = config_.quad ? 4 : 1;
  result.write=config_.writeStrobePin;
  return result;
}
} // namespace nice_gfx
#endif

#endif
