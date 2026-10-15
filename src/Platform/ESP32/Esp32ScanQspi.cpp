#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
// not an esp_lcd chip factory or an external component's API.
#include "Esp32ScanQspi.h"
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <atomic>
#include <new>
#include <string.h>
#if SOC_SPI_SCT_SUPPORTED
#include "esp_private/spi_master_internal.h"
#include "soc/soc.h"
#include "../../Drivers/Display/Video/ST77903_QSPI.h"
#endif

namespace nice_gfx {
#if SOC_SPI_SCT_SUPPORTED
namespace scan_qspi_detail {
static Status statusOf(esp_err_t e) {
  switch (e) {
    case ESP_OK: return Status::Ok;
    case ESP_ERR_INVALID_ARG: return Status::InvalidArgument;
    case ESP_ERR_NO_MEM: return Status::NoMemory;
    case ESP_ERR_NOT_SUPPORTED: return Status::Unsupported;
    case ESP_ERR_INVALID_STATE: return Status::Busy;
    default: return Status::IoError;
  }
}
} // namespace scan_qspi_detail

struct Esp32ScanQspi::State {
  struct Batch {
    spi_multi_transaction_t *segments;
    uint8_t *wire;
    bool queued;
  };
  ScanQspiConfig config;
  spi_device_handle_t device = nullptr;
  bool ownsBus = false, sct = false, on = false, resetOwned = false;
  uint16_t *framebuffer = nullptr, *scratch = nullptr;
  Batch *batches = nullptr;
  spi_multi_transaction_t *back = nullptr, *front = nullptr;
  spi_multi_transaction_t control = {};
  SemaphoreHandle_t frameLock = nullptr, stopped = nullptr;
  TaskHandle_t worker = nullptr;
  std::atomic<bool> stopping{false};
  std::atomic<Status> scanStatus{Status::Ok};
  size_t pending = 0;
  explicit State(const ScanQspiConfig &c) : config(c) {}

  // SCT completion owns the DMA descriptors until the SDK returns this list.
  // This is deliberately not a post_cb semaphore: an ISR callback may run
  // before the driver's DMA/bus bookkeeping has completed.
  Status reap() {
    if (!pending) return Status::Ok;
    spi_multi_transaction_t *finished = nullptr;
    const Status s = scan_qspi_detail::statusOf(
      spi_device_get_multi_trans_result(device, &finished, portMAX_DELAY));
    if (s != Status::Ok) return s;
    --pending;
    if (finished != back && finished != front) for (size_t i = 0; i < config.batchCount; ++i)
      if (finished == batches[i].segments) { batches[i].queued = false; break; }
    return Status::Ok;
  }
  Status drain() {
    while (pending) {
      const Status s = reap();
      if (s != Status::Ok) return s;
    }
    return Status::Ok;
  }
  Status queue(spi_multi_transaction_t *segments, size_t count) {
    if (!count) return Status::Ok;
    while (pending >= size_t(config.batchCount) + 2) {
      const Status s = reap();
      if (s != Status::Ok) return s;
    }
    const Status s = scan_qspi_detail::statusOf(
      spi_device_queue_multi_trans(device, segments, uint32_t(count), portMAX_DELAY));
    if (s == Status::Ok) ++pending;
    return s;
  }
  Status frame() {
    Status s = queue(back, size_t(config.backPorch) + 1);
    if (s != Status::Ok) return s;
    size_t batchIndex = 0;
    for (size_t row = 0; row < config.height;) {
      Batch &batch = batches[batchIndex];
      while (batch.queued) {
        s = reap();
        if (s != Status::Ok) return s;
      }
      size_t lines = size_t(config.height) - row;
      if (lines > config.batchLines) lines = config.batchLines;
      if (xSemaphoreTake(frameLock, portMAX_DELAY) != pdTRUE) return Status::IoError;
      copyPixels(batch.wire, reinterpret_cast<const uint8_t *>(framebuffer + row * config.width),
                 lines * config.width, PixelOrder::Native);
      xSemaphoreGive(frameLock);
      s = queue(batch.segments, lines);
      if (s != Status::Ok) return s;
      batch.queued = true;
      row += lines;
      batchIndex = (batchIndex + 1) % config.batchCount;
    }
    s = queue(front, config.frontPorch);
    if (s != Status::Ok) return s;
    return drain();
  }
};

Status Esp32ScanQspi::begin() {
  if (state_) return rasterReady() ? status() : Status::Busy;
  const ScanQspiConfig &c = config_;
  // DISCN encodes horizontal groups of eight and vertical groups of two.
  if (!c.width || c.width > 400 || c.width % 8 || !c.height || c.height > 480 || c.height % 2 ||
      c.frequency < 1000000 || c.frequency > 80000000 || c.minimumLineUs < 41 || c.minimumLineUs > 100 ||
      !c.batchLines || c.batchLines > 64 || c.batchCount < 2 || c.batchCount > 8 ||
      c.taskStackBytes < 2048 || c.taskStackBytes > 65536 || !c.taskPriority ||
      c.taskPriority >= configMAX_PRIORITIES || c.init.count > 1024 ||
      (c.init.count && !c.init.commands) || (c.init.commands && !c.init.count))
    return result_ = Status::InvalidArgument;
  if (c.host != SPI2_HOST && c.host != SPI3_HOST) return result_ = Status::InvalidArgument;
  // SPI interrupt and scanner are kept on the same core, including shutdown.
  if (c.taskCore != -1 && c.taskCore != xPortGetCoreID()) return result_ = Status::InvalidArgument;
  const int pins[] = {c.clock, c.data[0], c.data[1], c.data[2], c.data[3], c.cs, c.reset};
  for (size_t i = 0; i < 7; ++i) {
    if (i == 6 && pins[i] == -1) continue;
    if (!GPIO_IS_VALID_OUTPUT_GPIO(pins[i])) return result_ = Status::InvalidArgument;
    for (size_t j = 0; j < i; ++j) if (pins[i] == pins[j]) return result_ = Status::InvalidArgument;
  }
  void *storage = heap_caps_malloc(sizeof(State), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  if (!storage) return result_ = Status::NoMemory;
  state_ = new(storage) State(c);
  State &s = *state_;
  auto fail = [this](Status reason) -> Status { shutdown(); return result_ = reason; };
  s.frameLock = xSemaphoreCreateMutex(); s.stopped = xSemaphoreCreateBinary();
  if (!s.frameLock || !s.stopped) return fail(Status::NoMemory);
  const size_t framebufferBytes = size_t(c.width) * c.height * sizeof(uint16_t);
  if (c.framebufferInPsram)
    s.framebuffer = static_cast<uint16_t *>(heap_caps_calloc(1, framebufferBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!s.framebuffer)
    s.framebuffer = static_cast<uint16_t *>(heap_caps_calloc(1, framebufferBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  s.scratch = static_cast<uint16_t *>(heap_caps_malloc(size_t(c.width) * 2, MALLOC_CAP_INTERNAL));
  s.batches = static_cast<State::Batch *>(heap_caps_calloc(c.batchCount, sizeof(State::Batch), MALLOC_CAP_INTERNAL));
  s.back = static_cast<spi_multi_transaction_t *>(heap_caps_calloc(size_t(c.backPorch) + 1, sizeof(spi_multi_transaction_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
  if (c.frontPorch)
    s.front = static_cast<spi_multi_transaction_t *>(heap_caps_calloc(c.frontPorch, sizeof(spi_multi_transaction_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
  if (!s.framebuffer || !s.scratch || !s.batches || !s.back || (c.frontPorch && !s.front)) return fail(Status::NoMemory);
  const uint32_t apbMHz = APB_CLK_FREQ / 1000000;
  const uint32_t porchGap = uint32_t(c.minimumLineUs) * apbMHz;
  // Command/address remain single-lane; only the pixel phase is quad-lane.
  // Round transmitted time down so the computed inactive gap never makes a
  // line shorter than requested. A full microsecond CS gap exceeds 500 ns.
  const uint32_t lineWireUs = uint32_t((uint64_t(32 + size_t(c.width) * 4) * 1000000) / c.frequency);
  const uint32_t gapUs = lineWireUs < c.minimumLineUs ? c.minimumLineUs - lineWireUs : 1;
  const uint32_t colorGap = (gapUs ? gapUs : 1) * apbMHz;
  for (size_t i = 0; i < size_t(c.backPorch) + 1; ++i) {
    s.back[i].base.cmd = 0xD8; s.back[i].base.addr = uint32_t(i ? 0x60 : 0x61) << 8;
    s.back[i].sct_gap_len = porchGap;
  }
  for (size_t i = 0; i < c.frontPorch; ++i) {
    s.front[i].base.cmd = 0xD8; s.front[i].base.addr = uint32_t(0x60) << 8;
    s.front[i].sct_gap_len = porchGap;
  }
  for (size_t i = 0; i < c.batchCount; ++i) {
    // Batch has trivial fields; allocation intentionally has zero state.
    State::Batch &b = s.batches[i];
    b.segments = static_cast<spi_multi_transaction_t *>(heap_caps_calloc(c.batchLines, sizeof(spi_multi_transaction_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    b.wire = static_cast<uint8_t *>(heap_caps_aligned_alloc(32, size_t(c.batchLines) * c.width * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    if (!b.segments || !b.wire) return fail(Status::NoMemory);
    for (size_t j = 0; j < c.batchLines; ++j) {
      b.segments[j].base.cmd = 0xDE; b.segments[j].base.addr = uint32_t(0x60) << 8;
      b.segments[j].base.length = size_t(c.width) * 16;
      b.segments[j].base.tx_buffer = b.wire + j * c.width * 2;
      b.segments[j].base.flags = SPI_TRANS_MODE_QIO;
      b.segments[j].sct_gap_len = colorGap;
    }
  }
  spi_bus_config_t bus = {};
  bus.sclk_io_num = c.clock; bus.mosi_io_num = c.data[0]; bus.miso_io_num = c.data[1];
  bus.quadwp_io_num = c.data[2]; bus.quadhd_io_num = c.data[3];
  bus.data4_io_num = bus.data5_io_num = bus.data6_io_num = bus.data7_io_num = -1;
  bus.flags = SPICOMMON_BUSFLAG_QUAD;
  // SCT needs DMA descriptors for the configuration and data of every queued
  // segment. max_transfer_sz reserves descriptors, not a framebuffer copy.
  bus.max_transfer_sz = int((size_t(c.batchLines) * c.batchCount + c.frontPorch + c.backPorch + 2) * 4 * 4092);
  Status r = scan_qspi_detail::statusOf(spi_bus_initialize(c.host, &bus, SPI_DMA_CH_AUTO));
  if (r != Status::Ok) return fail(r);
  s.ownsBus = true;
  spi_device_interface_config_t device = {};
  device.command_bits = 8; device.address_bits = 24; device.mode = 0;
  device.clock_speed_hz = int(c.frequency); device.spics_io_num = c.cs;
  device.queue_size = c.batchCount + 2; device.flags = SPI_DEVICE_HALFDUPLEX;
  r = scan_qspi_detail::statusOf(spi_bus_add_device(c.host, &device, &s.device));
  if (r != Status::Ok) return fail(r);
  r = scan_qspi_detail::statusOf(spi_bus_multi_trans_mode_enable(s.device, true));
  if (r != Status::Ok) return fail(r);
  s.sct = true;
  if (c.reset >= 0) {
    gpio_config_t io = {}; io.mode = GPIO_MODE_OUTPUT; io.pin_bit_mask = uint64_t(1) << c.reset;
    r = scan_qspi_detail::statusOf(gpio_config(&io));
    if (r != Status::Ok) return fail(r);
    s.resetOwned = true;
    output(c.reset, !c.resetActiveHigh); delayMs(10);
    output(c.reset, c.resetActiveHigh); delayMs(10);
    output(c.reset, !c.resetActiveHigh); delayMs(120);
  } else {
    r = sendRegister(0x01, nullptr, 0); if (r != Status::Ok) return fail(r);
    delayMs(120);
  }
  r = initializeController(); if (r != Status::Ok) return fail(r);
  r = startRaster(c.width, c.height, s.scratch, c.width);
  if (r != Status::Ok) return fail(r);
  result_ = Status::Ok;
  if (c.displayOnInit) { r = display(true); if (r != Status::Ok) return fail(r); }
  return Status::Ok;
}

Status Esp32ScanQspi::sendRegister(uint8_t command, const uint8_t *data, size_t bytes) {
  if (!state_ || !state_->device) return Status::NotInitialized;
  if ((bytes && !data) || bytes > 256) return Status::InvalidArgument;
  State &s = *state_;
  if (s.worker || s.pending) return Status::Busy;
  s.control = {};
  s.control.base.cmd = 0xD8; s.control.base.addr = uint32_t(command) << 8;
  s.control.base.tx_buffer = data; s.control.base.length = bytes * 8;
  s.control.sct_gap_len = uint32_t(s.config.minimumLineUs) * (APB_CLK_FREQ / 1000000);
  Status r = s.queue(&s.control, 1);
  if (r == Status::Ok) r = s.drain();
  return r;
}

Status Esp32ScanQspi::initializeController() {
  const uint8_t geometry[] = {uint8_t(config_.height / 2 - 1), uint8_t(config_.width / 8 - 1)};
  const uint8_t porches[] = {0, config_.frontPorch, 0, config_.backPorch};
  const uint8_t order = config_.bgr ? 8 : 0, format = 5;
  const VideoInitCommand *commands = config_.init.commands ? config_.init.commands : video::st77903ScanInit;
  const size_t count = config_.init.commands ? config_.init.count : sizeof(video::st77903ScanInit) / sizeof(VideoInitCommand);
  // Do not accept a custom format/geometry which invalidates the scan stream.
  for (size_t i = 0; i < count; ++i) {
    const VideoInitCommand &c = commands[i];
    if ((c.dataBytes && !c.data) || c.dataBytes > 256) return Status::InvalidArgument;
    if ((c.command == 0xB6 && (c.dataBytes != 2 || memcmp(c.data, geometry, 2))) ||
        (c.command == 0xB5 && (c.dataBytes != 4 || memcmp(c.data, porches, 4))) ||
        (c.command == 0x3A && (c.dataBytes != 1 || c.data[0] != format)) ||
        (c.command == 0x36 && (c.dataBytes != 1 || c.data[0] != order))) return Status::InvalidArgument;
  }
  const uint8_t keys[] = {0xC3, 0x96, 0xA5};
  for (uint8_t key : keys) { const Status r = sendRegister(0xF0, &key, 1); if (r != Status::Ok) return r; }
  const VideoInitCommand setup[] = {{0xB6, geometry, 2, 0}, {0xB5, porches, 4, 0},
                                    {0x36, &order, 1, 0}, {0x3A, &format, 1, 0}};
  for (const VideoInitCommand &c : setup) { const Status r = sendRegister(c.command, c.data, c.dataBytes); if (r != Status::Ok) return r; }
  for (size_t i = 0; i < count; ++i) {
    const VideoInitCommand &c = commands[i];
    const Status r = sendRegister(c.command, c.data, c.dataBytes);
    if (r != Status::Ok) return r;
    if (c.delayMs) delayMs(c.delayMs);
  }
  return Status::Ok;
}

void Esp32ScanQspi::workerEntry(void *context) {
  State &s = *static_cast<State *>(context);
  Status r = Status::Ok;
  while (!s.stopping.load(std::memory_order_acquire)) {
    r = s.frame();
    if (r != Status::Ok) break;
  }
  // Accepted DMA remains owned even after a later queue operation failed.
  const Status drained = s.drain();
  if (r == Status::Ok) r = drained;
  s.scanStatus.store(r, std::memory_order_release);
  SemaphoreHandle_t done = s.stopped;
  xSemaphoreGive(done);
  // No State access after the completion signal: shutdown can free it now.
  vTaskDelete(nullptr);
}
Status Esp32ScanQspi::startScan() {
  State &s = *state_;
  if (s.worker) return s.scanStatus.load();
  while (xSemaphoreTake(s.stopped, 0) == pdTRUE) {}
  s.stopping.store(false); s.scanStatus.store(Status::Ok);
  if (xTaskCreatePinnedToCore(workerEntry, "gfx-scan-qspi", config_.taskStackBytes, &s,
                            config_.taskPriority, &s.worker, xPortGetCoreID()) != pdPASS) {
    s.worker = nullptr; return Status::NoMemory;
  }
  return Status::Ok;
}
Status Esp32ScanQspi::stopScan() {
  if (!state_) return Status::Ok;
  State &s = *state_;
  if (s.worker) {
    s.stopping.store(true, std::memory_order_release);
    // One whole frame finishes; never kill a task while DMA owns its lists.
    if (xSemaphoreTake(s.stopped, portMAX_DELAY) != pdTRUE) return Status::IoError;
    s.worker = nullptr;
  }
  const Status r = s.drain();
  return r == Status::Ok ? s.scanStatus.load(std::memory_order_acquire) : r;
}
Status Esp32ScanQspi::display(bool on) {
  if (!state_) return Status::NotInitialized;
  Status r = stopScan();
  if (r != Status::Ok) return r;
  r = sendRegister(on ? 0x29 : 0x28, nullptr, 0);
  if (r == Status::Ok) state_->on = on;
  return r == Status::Ok && on ? startScan() : r;
}
Status Esp32ScanQspi::shutdown() {
  stopRaster();
  if (!state_) return result_;
  State &s = *state_;
  Status first = stopScan();
  // An SDK failure with outstanding DMA cannot justify freeing its memory.
  if (s.pending || s.worker) return result_ = first == Status::Ok ? Status::IoError : first;
  if (s.device && s.sct && s.on) {
    const Status off = sendRegister(0x28, nullptr, 0);
    if (first == Status::Ok) first = off;
    if (s.pending) return result_ = first;
    s.on = false;
  }
  if (s.sct) {
    const Status r = scan_qspi_detail::statusOf(spi_bus_multi_trans_mode_enable(s.device, false));
    if (r != Status::Ok) return result_ = r;
    s.sct = false;
  }
  if (s.device) {
    const Status r = scan_qspi_detail::statusOf(spi_bus_remove_device(s.device));
    if (r != Status::Ok) return result_ = r;
    s.device = nullptr;
  }
  if (s.ownsBus) {
    const Status r = scan_qspi_detail::statusOf(spi_bus_free(config_.host));
    if (r != Status::Ok) return result_ = r;
    s.ownsBus = false;
  }
  if (s.batches) {
    for (size_t i = 0; i < config_.batchCount; ++i) {
      heap_caps_free(s.batches[i].wire); heap_caps_free(s.batches[i].segments);
    }
    heap_caps_free(s.batches);
  }
  heap_caps_free(s.front); heap_caps_free(s.back);
  heap_caps_free(s.framebuffer); heap_caps_free(s.scratch);
  if (s.stopped) vSemaphoreDelete(s.stopped);
  if (s.frameLock) vSemaphoreDelete(s.frameLock);
  if (s.resetOwned) gpio_reset_pin(gpio_num_t(config_.reset));
  s.~State(); heap_caps_free(state_); state_ = nullptr;
  if (first != Status::Ok) result_ = first;
  return result_;
}
Status Esp32ScanQspi::drawBitmap(int x0, int y0, int x1, int y1, const uint16_t *pixels) {
  if (!state_ || !rasterReady()) return Status::NotInitialized;
  if (!pixels || x0 < 0 || y0 < 0 || x1 <= x0 || y1 <= y0 || x1 > config_.width || y1 > config_.height)
    return Status::InvalidArgument;
  State &s = *state_;
  const Status r = s.scanStatus.load(std::memory_order_acquire);
  if (r != Status::Ok) return result_ = r;
  if (xSemaphoreTake(s.frameLock, portMAX_DELAY) != pdTRUE) return result_ = Status::IoError;
  for (int y = y0; y < y1; ++y) {
    memcpy(s.framebuffer + size_t(y) * config_.width + x0, pixels, size_t(x1 - x0) * 2);
    pixels += x1 - x0;
  }
  xSemaphoreGive(s.frameLock);
  return Status::Ok;
}
Status Esp32ScanQspi::command(uint8_t c, const uint8_t *data, size_t bytes) {
  if (!state_ || !rasterReady()) return Status::NotInitialized;
  if (bytes && !data) return Status::InvalidArgument;
  if (c == 0x2A || c == 0x2B || c == 0x2C) return FramebufferTransport::command(c, data, bytes);
  if (bytes) return Status::Unsupported;
  if (c == 0x28 || c == 0x29) return result_ = display(c == 0x29);
  if (c == 0x20 || c == 0x21) {
    const bool resume = state_->on;
    Status r = stopScan();
    if (r == Status::Ok) r = sendRegister(c, nullptr, 0);
    if (r == Status::Ok && resume) r = startScan();
    return result_ = r;
  }
  return Status::Unsupported;
}
Status Esp32ScanQspi::status() const {
  if (state_) { const Status r = state_->scanStatus.load(std::memory_order_acquire); if (r != Status::Ok) return r; }
  return result_;
}
Status Esp32ScanQspi::wait() { return state_ && rasterReady() ? status() : Status::NotInitialized; }
const uint16_t *Esp32ScanQspi::framebuffer() const { return state_ ? state_->framebuffer : nullptr; }

#else // No SCT peripheral on this SoC.
struct Esp32ScanQspi::State {};
Status Esp32ScanQspi::begin() { return result_ = Status::Unsupported; }
Status Esp32ScanQspi::shutdown() { stopRaster(); return result_; }
Status Esp32ScanQspi::command(uint8_t, const uint8_t *, size_t) { return Status::Unsupported; }
Status Esp32ScanQspi::drawBitmap(int, int, int, int, const uint16_t *) { return Status::Unsupported; }
Status Esp32ScanQspi::status() const { return result_; }
Status Esp32ScanQspi::wait() { return Status::NotInitialized; }
const uint16_t *Esp32ScanQspi::framebuffer() const { return nullptr; }
#endif

void Esp32ScanQspi::end() { shutdown(); }
bool Esp32ScanQspi::validOutputPin(int pin) const { return GPIO_IS_VALID_OUTPUT_GPIO(pin); }
void Esp32ScanQspi::output(int pin, bool high) {
  if (!GPIO_IS_VALID_OUTPUT_GPIO(pin)) return;
  gpio_set_direction(gpio_num_t(pin), GPIO_MODE_OUTPUT);
  gpio_set_level(gpio_num_t(pin), high ? 1 : 0);
}
void Esp32ScanQspi::delayMs(uint32_t ms) {
  if (ms) { TickType_t ticks = pdMS_TO_TICKS(ms); vTaskDelay(ticks ? ticks : 1); }
}
TransportInfo Esp32ScanQspi::info() const {
  TransportInfo i; i.frequency = config_.frequency; i.clock = config_.clock; i.cs = config_.cs;
  i.lanes = 4; for (size_t p = 0; p < 4; ++p) i.data[p] = config_.data[p];
  return i;
}
} // namespace nice_gfx
#endif

#endif
