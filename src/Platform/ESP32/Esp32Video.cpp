#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include "Esp32Video.h"
#if (defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)) && (SOC_LCD_RGB_SUPPORTED || SOC_MIPI_DSI_SUPPORTED)
#include "../../Drivers/Display/Video/VideoControllers.h"
#include "Esp32Platform.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include <string.h>
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#include "esp_cache.h"
#endif

namespace nice_gfx {

Status VideoTransport::convert(esp_err_t error) {
  switch (error) {
    case ESP_OK: return Status::Ok;
    case ESP_ERR_INVALID_ARG: return Status::InvalidArgument;
    case ESP_ERR_NO_MEM: return Status::NoMemory;
    case ESP_ERR_NOT_SUPPORTED: return Status::Unsupported;
    case ESP_ERR_INVALID_STATE: return Status::Busy;
    default: return Status::IoError;
  }
}

bool IRAM_ATTR VideoTransport::complete(void *context) {
  SemaphoreHandle_t semaphore = static_cast<SemaphoreHandle_t>(context);
  if (xPortInIsrContext()) {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(semaphore, &woken);
    return woken == pdTRUE;
  }
  xSemaphoreGive(semaphore);
  return false;
}
#if SOC_LCD_RGB_SUPPORTED
bool IRAM_ATTR VideoTransport::rgbDone(esp_lcd_panel_handle_t,
                                      const esp_lcd_rgb_panel_event_data_t *, void *context) {
  return complete(static_cast<VideoTransport*>(context)->completed_);
}
bool IRAM_ATTR VideoTransport::rgbVsync(esp_lcd_panel_handle_t,
                                      const esp_lcd_rgb_panel_event_data_t *,void *context){
  auto &s=*static_cast<VideoTransport*>(context);
  const uint32_t now=uint32_t(esp_timer_get_time());
  if(s.scanStats_)s.scanStats_->record(s.lastVsync_?uint32_t(now-s.lastVsync_):0);
  s.lastVsync_=now;return false;
}
#endif
#if SOC_MIPI_DSI_SUPPORTED
bool IRAM_ATTR VideoTransport::dpiDone(esp_lcd_panel_handle_t,
                                      esp_lcd_dpi_panel_event_data_t *, void *context) {
  return complete(context);
}
#endif

Status VideoTransport::wait() {
  if (!pending_) return Status::Ok;
  // Accepted DMA copies must finish before their staging memory can be reused.
  if (!completed_ || xSemaphoreTake(completed_, portMAX_DELAY) != pdTRUE)
    return result_ = Status::IoError;
  pending_ = false;
  return result_;
}

Status VideoTransport::present() {
  if (!panel_ || !rasterReady()) return Status::NotInitialized;
  const Status s = wait();
  if (s != Status::Ok) return s;
#if SOC_LCD_RGB_SUPPORTED
  if (interface_ == VideoInterface::RGB && refreshOnDemand_)
    return result_ = convert(esp_lcd_rgb_panel_refresh(panel_));
#endif
  return result_;
}

Status VideoTransport::shutdown() {
  const Status first = wait();
  if (first != Status::Ok && pending_) return result_ = first;
#if SOC_PPA_SUPPORTED && defined(CONFIG_IDF_TARGET_ESP32S31)
  if(ppaFill_){const Status s=convert(ppa_unregister_client(ppaFill_));if(s!=Status::Ok)return result_=s;ppaFill_=nullptr;}
  if(ppaCopy_){const Status s=convert(ppa_unregister_client(ppaCopy_));if(s!=Status::Ok)return result_=s;ppaCopy_=nullptr;}
  ppaVerified_=ppaPartialVerified_=false;
  ppaFillVerified_=ppaFillPartialVerified_=ppaFillRejected_=false;
#endif
#if SOC_LCD_RGB_SUPPORTED
  if(copyStats_){copyStats_->ppaReady=false;copyStats_->ppaFillReady=false;}
#endif
  stopRaster();
  if (panel_) {
    const Status s = convert(esp_lcd_panel_del(panel_));
    // A failed delete may leave DMA running. Its resources and callback
    // semaphore must survive, and a later explicit shutdown can retry.
    if (s != Status::Ok) return result_ = first == Status::Ok ? s : first;
    panel_ = nullptr; rgbFramebuffer_ = nullptr;
  }
  if (io_) {
    const Status s = convert(esp_lcd_panel_io_del(io_));
    if (s != Status::Ok) return result_ = first == Status::Ok ? s : first;
    io_ = nullptr;
  }
#if SOC_MIPI_DSI_SUPPORTED
  if (dsiBus_) {
    const Status s = convert(esp_lcd_del_dsi_bus(static_cast<esp_lcd_dsi_bus_handle_t>(dsiBus_)));
    if (s != Status::Ok) return result_ = first == Status::Ok ? s : first;
    dsiBus_ = nullptr;
  }
  if (ldo_) {
    const Status s = convert(esp_ldo_release_channel(static_cast<esp_ldo_channel_handle_t>(ldo_)));
    if (s != Status::Ok) return result_ = first == Status::Ok ? s : first;
    ldo_ = nullptr;
  }
#endif
  if (completed_) { vSemaphoreDelete(completed_); completed_ = nullptr; }
  if (scratch_) { heap_caps_free(scratch_); scratch_ = nullptr; }
  hasSpi_ = hasDisplayPin_ = refreshOnDemand_ = swapRedBlue_ = false;
  return result_ = first;
}
void VideoTransport::end() { shutdown(); }
Status VideoTransport::fail(Status reason) { shutdown(); return result_ = reason; }
Status VideoTransport::fail(esp_err_t error) { return fail(convert(error)); }

Status VideoTransport::prepareCommandSpi(const VideoSpiConfig &config) {
  if (config.spi_mode > 3 || !config.expect_clk_speed ||
      config.lcd_cmd_bytes != 1 || config.lcd_param_bytes != 1 || !config.flags.use_dc_bit)
    return Status::InvalidArgument;
  const int pins[] = {config.line_config.cs_gpio_num, config.line_config.scl_gpio_num,
                      config.line_config.sda_gpio_num};
  for (size_t i = 0; i < 3; ++i) {
    if (i == 0 && pins[i] == -1) continue;
    if (!validOutputPin(pins[i])) return Status::InvalidArgument;
    for (size_t j = 0; j < i; ++j) if (pins[i] == pins[j]) return Status::InvalidArgument;
  }
  spi_ = config;
  halfPeriodUs_ = 500000UL / config.expect_clk_speed + (500000UL % config.expect_clk_speed != 0);
  if (!halfPeriodUs_) halfPeriodUs_ = 1;
  if (pins[0] >= 0) output(pins[0], !spi_.flags.cs_high_active);
  output(pins[1], (spi_.spi_mode & 2) != 0);
  output(pins[2], false);
  hasSpi_ = true;
  return Status::Ok;
}

void VideoTransport::spiBit(bool high) {
  const bool idle = (spi_.spi_mode & 2) != 0;
  const int clock = spi_.line_config.scl_gpio_num, data = spi_.line_config.sda_gpio_num;
  if (spi_.spi_mode & 1) {
    esp32_platform::write(clock, !idle);
    esp32_platform::write(data, high);
    esp32_platform::delayMicros(halfPeriodUs_);
    esp32_platform::write(clock, idle);
    esp32_platform::delayMicros(halfPeriodUs_);
  } else {
    esp32_platform::write(data, high);
    esp32_platform::delayMicros(halfPeriodUs_);
    esp32_platform::write(clock, !idle);
    esp32_platform::delayMicros(halfPeriodUs_);
    esp32_platform::write(clock, idle);
  }
}
void VideoTransport::spiByte(uint8_t byte, bool data) {
  const bool dc = spi_.flags.dc_zero_on_data ? !data : data;
  spiBit(dc);
  if (spi_.flags.lsb_first) for (unsigned bit = 0; bit < 8; ++bit) spiBit((byte >> bit) & 1);
  else for (unsigned bit = 0; bit < 8; ++bit) spiBit((byte >> (7 - bit)) & 1);
}
Status VideoTransport::transmitCommand(uint8_t c, const uint8_t *data, size_t bytes) {
  if (bytes && !data) return Status::InvalidArgument;
  if (interface_ == VideoInterface::MIPI) {
    if (!io_) return Status::NotInitialized;
    return convert(esp_lcd_panel_io_tx_param(io_, c, data, bytes));
  }
  if (!hasSpi_) return Status::NotInitialized;
  const int cs = spi_.line_config.cs_gpio_num;
  if (cs >= 0) esp32_platform::write(cs, spi_.flags.cs_high_active);
  spiByte(c, false);
  for (size_t i = 0; i < bytes; ++i) spiByte(data[i], true);
  if (cs >= 0) esp32_platform::write(cs, !spi_.flags.cs_high_active);
  return Status::Ok;
}
Status VideoTransport::writeCommand(uint8_t c, const uint8_t *data, size_t bytes, void *context) {
  return static_cast<VideoTransport *>(context)->transmitCommand(c, data, bytes);
}
void VideoTransport::controllerDelay(uint32_t ms, void *context) {
  static_cast<VideoTransport *>(context)->delayMs(ms);
}

Status VideoTransport::initializeHardware(VideoController controller, const VideoPanelConfig &options,
                                          const VideoInitOverride &init, int reset,
                                          size_t width, size_t height, unsigned lanes,
                                          bool displayOnInit, bool hasDisplayPin) {
  if (!panel_ || !width || !height || width > INT16_MAX || height > INT16_MAX)
    return fail(Status::InvalidArgument);
  const auto *profile = videoControllerProfile(controller, interface_);
  if (!profile) return fail(Status::Unsupported);
  controller_ = controller; hasDisplayPin_ = hasDisplayPin;
  swapRedBlue_ = interface_ == VideoInterface::RGB && profile->noCommand &&
                 options.rgb_ele_order == LCD_RGB_ELEMENT_ORDER_BGR;
  const size_t bytes = (width * sizeof(uint16_t) + 63) & ~size_t(63);
  scratch_ = static_cast<uint16_t *>(heap_caps_aligned_alloc(64, bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
  if (!scratch_) return fail(ESP_ERR_NO_MEM);
  completed_ = xSemaphoreCreateBinary();
  if (!completed_) return fail(ESP_ERR_NO_MEM);
  esp_err_t error = ESP_OK;
#if SOC_LCD_RGB_SUPPORTED
  if (interface_ == VideoInterface::RGB) {
    esp_lcd_rgb_panel_event_callbacks_t callbacks = {};
    callbacks.on_color_trans_done = rgbDone;
    callbacks.on_vsync = rgbVsync;
    error = esp_lcd_rgb_panel_register_event_callbacks(panel_, &callbacks, this);
  }
#endif
#if SOC_MIPI_DSI_SUPPORTED
  if (interface_ == VideoInterface::MIPI) {
    esp_lcd_dpi_panel_event_callbacks_t callbacks = {};
    callbacks.on_color_trans_done = dpiDone;
    error = esp_lcd_dpi_panel_register_event_callbacks(panel_, &callbacks, completed_);
  }
#endif
  if (error != ESP_OK) return fail(error);
  // RGB exposes a peripheral reset operation. Native DPI exposes no reset
  // handler: its external controller is reset below through GPIO or commands.
  if (interface_ == VideoInterface::RGB) {
    error = esp_lcd_panel_reset(panel_);
    if (error != ESP_OK) return fail(error);
  }
  if (reset >= 0) {
    const bool active = options.flags.reset_active_high;
    output(reset, !active); delayMs(profile->resetPreMs);
    output(reset, active); delayMs(profile->resetActiveMs);
    output(reset, !active); delayMs(profile->resetIdleMs);
  } else {
    const Status resetStatus = videoReset(controller, interface_, writeCommand, controllerDelay, this);
    if (resetStatus != Status::Ok) return fail(resetStatus);
  }
  if (profile->initRequiresVideo) {
    error = esp_lcd_panel_init(panel_);
    if (error != ESP_OK) return fail(error);
  }
  Status s = videoInitialize(controller, interface_, options.bits_per_pixel, lanes,
                             options.rgb_ele_order == LCD_RGB_ELEMENT_ORDER_BGR,
                             init, writeCommand, controllerDelay, this, width, height);
  if (s != Status::Ok) return fail(s);
  if (!profile->initRequiresVideo) {
    error = esp_lcd_panel_init(panel_);
    if (error != ESP_OK) return fail(error);
  }
  if (displayOnInit && (!profile->noCommand || hasDisplayPin)) {
    s = display(true);
    if (s != Status::Ok) return fail(s);
  }
  s = startRaster(width, height, scratch_, width);
  if (s != Status::Ok) return fail(s);
  if (refreshOnDemand_ && displayOnInit) {
    s = present();
    if (s != Status::Ok) return fail(s);
  }
  return result_ = Status::Ok;
}

Status VideoTransport::drawBitmap(int x0, int y0, int x1, int y1, const uint16_t *pixels) {
  if (!panel_ || !rasterReady()) return Status::NotInitialized;
  const Status s = wait();
  if (s != Status::Ok) return s;
  while (xSemaphoreTake(completed_, 0) == pdTRUE) {}
  if (swapRedBlue_) {
    const size_t count = size_t(x1 - x0) * size_t(y1 - y0);
    for (size_t i = 0; i < count; ++i) {
      const uint16_t color = pixels[i];
      scratch_[i] = uint16_t((color & 0x07E0) | ((color & 0x001F) << 11) | (color >> 11));
    }
    pixels = scratch_;
  }
  pending_ = true;
#if SOC_LCD_RGB_SUPPORTED
  if(copyStats_)copyStats_->cpuPixels+=uint64_t(x1-x0)*(y1-y0);
#endif
  result_ = convert(esp_lcd_panel_draw_bitmap(panel_, x0, y0, x1, y1, pixels));
  if (result_ != Status::Ok) { pending_ = false; return result_; }
  return wait();
}

#if defined(CONFIG_IDF_TARGET_ESP32S31)
Status VideoTransport::synchronizeFramebuffer(int x, int y, size_t width, size_t height, bool cpuWritten) {
  // Only a fixed, single RGB framebuffer is mapped. CPU stores and blocking
  // PPA copies have already consumed their source; no driver job remains to
  // wait for and no framebuffer/DMA link needs to change. present() retains
  // responsibility for starting refresh-on-demand panels.
  if (!cpuWritten || !esp_cache_get_line_size_by_addr(rgbFramebuffer_)) return result_ = Status::Ok;
  auto *first = rgbFramebuffer_ + size_t(y) * rasterWidth() + size_t(x);
  const int flags = ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED;
  // A narrow tile must not write back every LCD scanline it intersects.
  // C2M preserves neighboring pixels sharing either boundary cache line.
  if (height == 1 || width * 2 >= rasterWidth())
    return result_ = convert(esp_cache_msync(first, ((height - 1) * rasterWidth() + width) * 2, flags));
  for (size_t row = 0; row < height; ++row) {
    const Status s = convert(esp_cache_msync(first + row * rasterWidth(), width * 2, flags));
    if (s != Status::Ok) return result_ = s;
  }
  return result_ = Status::Ok;
}

Status VideoTransport::drawFillRect(int x, int y, size_t width, size_t height, uint16_t color) {
  if (!rgbFramebuffer_) return Status::NotInitialized;
  const Status s = wait();
  if (s != Status::Ok) return s;
  if (swapRedBlue_) color = uint16_t((color & 0x07E0) | ((color & 0x001F) << 11) | (color >> 11));
#if SOC_PPA_SUPPORTED
  if (fillWithPpa(x, y, width, height, color)) return result_ = Status::Ok;
#endif
  const auto fillSpan = [color](uint16_t *out, size_t count) {
#if defined(__GNUC__)
    // An aligned word store writes two PSRAM pixels. Avoid both staging and
    // the peripheral driver's halfword copy for every individual scanline.
    typedef uint32_t FillWord __attribute__((__may_alias__));
    if ((reinterpret_cast<uintptr_t>(out) & 3) && count) { *out++ = color; --count; }
    auto *words = reinterpret_cast<FillWord *>(out);
    const uint32_t pair = uint32_t(color) | (uint32_t(color) << 16);
    while (count >= 8) {
      words[0] = pair; words[1] = pair; words[2] = pair; words[3] = pair;
      words += 4; count -= 8;
    }
    while (count >= 2) { *words++ = pair; count -= 2; }
    out = reinterpret_cast<uint16_t *>(words);
#endif
    while (count--) *out++ = color;
  };
  uint16_t *out = rgbFramebuffer_ + size_t(y) * rasterWidth() + size_t(x);
  if (width == rasterWidth()) fillSpan(out, width * height);
  else for (size_t row = 0; row < height; ++row) fillSpan(out + row * rasterWidth(), width);
#if SOC_LCD_RGB_SUPPORTED
  if (copyStats_) copyStats_->cpuPixels += uint64_t(width) * height;
#endif
  return synchronizeFramebuffer(x, y, width, height, true);
}
#endif

Status VideoTransport::drawPixelView(int x, int y, PixelView view) {
  if (!rgbFramebuffer_) return Status::NotInitialized;
  const Status s = wait();
  if (s != Status::Ok) return s;
  bool copied=false;
#if SOC_PPA_SUPPORTED && defined(CONFIG_IDF_TARGET_ESP32S31)
  copied=copyWithPpa(x,y,view);
#endif
#if SOC_LCD_RGB_SUPPORTED
  if(!copied&&copyStats_)copyStats_->cpuPixels+=uint64_t(view.width)*view.height;
#endif
  if(!copied)for (size_t row = 0; row < view.height; ++row) {
    uint16_t *out = rgbFramebuffer_ + (size_t(y) + row) * rasterWidth() + size_t(x);
    const uint8_t *in = view.data + row * view.stride * 2;
    if (view.order == PixelOrder::Native && !swapRedBlue_) memcpy(out, in, view.width * 2);
    else {
      size_t i=0;
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      // PSRAM halfword stores are expensive on S31. Stream aligned words,
      // converting two wire pixels per store without a second staging copy.
      typedef uint32_t CopyWord __attribute__((__may_alias__));
      if (!(reinterpret_cast<uintptr_t>(out)&3) && !(reinterpret_cast<uintptr_t>(in)&3)) {
        auto *dst=reinterpret_cast<CopyWord *>(out);
        const auto *src=reinterpret_cast<const CopyWord *>(in);
        const auto convertPair=[&](uint32_t p){
          if(view.order==PixelOrder::Wire)p=((p&0x00FF00FFu)<<8)|((p>>8)&0x00FF00FFu);
          if(swapRedBlue_)p=(p&0x07E007E0u)|((p&0x001F001Fu)<<11)|((p>>11)&0x001F001Fu);
          return p;
        };
        for(;i+7<view.width;i+=8,src+=4,dst+=4){
          const uint32_t a=src[0],b=src[1],c=src[2],d=src[3];
          dst[0]=convertPair(a);dst[1]=convertPair(b);dst[2]=convertPair(c);dst[3]=convertPair(d);
        }
        for(;i+1<view.width;i+=2)*dst++=convertPair(*src++);
      }
#endif
      for (; i < view.width; ++i) {
      uint16_t c;
      if (view.order == PixelOrder::Wire) c = uint16_t(uint16_t(in[2*i]) << 8 | in[2*i+1]);
      else { const uint8_t *p=in+2*i; c=uint16_t(p[0] | uint16_t(p[1]) << 8); }
      if (swapRedBlue_) c=uint16_t((c & 0x07E0) | ((c & 0x001F) << 11) | (c >> 11));
      out[i]=c;
      }
    }
  }
#if defined(CONFIG_IDF_TARGET_ESP32S31)
  // Blocking PPA already synchronized its output; CPU writes need only the
  // dirty pixels written back, rather than every complete LCD scanline.
  return synchronizeFramebuffer(x, y, view.width, view.height, !copied);
#else
  // The IDF recognizes its own framebuffer and skips its pixel-by-pixel copy.
  // This still performs the required cache writeback without bounce buffers,
  // and preserves source-completion/on-demand semantics on IDF 5 and IDF 6.
  while (xSemaphoreTake(completed_, 0) == pdTRUE) {}
  pending_=true;
  result_=convert(esp_lcd_panel_draw_bitmap(panel_, x, y, x+int(view.width), y+int(view.height), rgbFramebuffer_));
  if (result_ != Status::Ok) { pending_=false; return result_; }
  return wait();
#endif
}

#if SOC_PPA_SUPPORTED && defined(CONFIG_IDF_TARGET_ESP32S31)
bool VideoTransport::fillWithPpa(int x,int y,size_t width,size_t height,uint16_t color){
  // Keep small rectangles on the CPU to avoid DMA submission/wakeup overhead.
  // The hardware descriptor and fill block dimensions are 14-bit on S31.
  if(!ppaFill_||ppaFillRejected_||width*height<65536||rasterWidth()>0x3FFF||rasterHeight()>0x3FFF)return false;
  const size_t bytes=rasterWidth()*rasterHeight()*2;
  const size_t line=esp_cache_get_line_size_by_addr(rgbFramebuffer_);
  if(!line||bytes%line||reinterpret_cast<uintptr_t>(rgbFramebuffer_)%line)return false;
  const size_t begin=(size_t(y)*rasterWidth()*2)&~(line-1);
  const size_t end=((size_t(y)+height)*rasterWidth()*2+line-1)&~(line-1);
  if(end>bytes)return false;
  auto *region=reinterpret_cast<uint8_t*>(rgbFramebuffer_)+begin;
  ppa_fill_oper_config_t operation={};
  operation.out.buffer=rgbFramebuffer_;operation.out.buffer_size=bytes;
  operation.out.pic_w=rasterWidth();operation.out.pic_h=rasterHeight();
  operation.out.block_offset_x=x;operation.out.block_offset_y=y;
  operation.out.fill_cm=PPA_FILL_COLOR_MODE_RGB565;
  operation.fill_block_w=width;operation.fill_block_h=height;
  // The fill register takes ARGB8888 channels, including for RGB565 output.
  // Truncation by the hardware recovers every original 5/6/5 component exactly.
  operation.fill_argb_color.a=255;
  operation.fill_argb_color.r=uint8_t((color>>11)<<3);
  operation.fill_argb_color.g=uint8_t(((color>>5)&63)<<2);
  operation.fill_argb_color.b=uint8_t((color&31)<<3);
  operation.mode=PPA_TRANS_MODE_BLOCKING;
  // Unlike SRM, the SDK fill path writes back and invalidates the complete
  // output rows before DMA, preserving dirty neighbors outside the rectangle.
  if(ppa_do_fill(ppaFill_,&operation)!=ESP_OK){if(copyStats_)++copyStats_->ppaFillFailures;return false;}
  // Scanout/bounce readers may have re-cached those rows while DMA was writing.
  if(esp_cache_msync(region,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_M2C)!=ESP_OK){if(copyStats_)++copyStats_->ppaFillFailures;return false;}
  const bool partial=x!=0||width!=rasterWidth();
  if(!ppaFillVerified_||(partial&&!ppaFillPartialVerified_)){
    bool exact=true;
    for(size_t row=0;row<height&&exact;++row)for(size_t col=0;col<width;++col)
      if(rgbFramebuffer_[(size_t(y)+row)*rasterWidth()+size_t(x)+col]!=color){exact=false;break;}
    if(!exact){
      ppaFillRejected_=true;
      if(copyStats_){++copyStats_->ppaFillVerificationFailures;copyStats_->ppaFillReady=false;}
      if(ppa_unregister_client(ppaFill_)==ESP_OK)ppaFill_=nullptr;
      return false;
    }
    ppaFillVerified_=true;if(partial)ppaFillPartialVerified_=true;
  }
  if(copyStats_){++copyStats_->ppaFillTransfers;copyStats_->ppaFillPixels+=uint64_t(width)*height;}
  return true;
}

bool VideoTransport::copyWithPpa(int x,int y,PixelView view){
  // S31 applies RGB swap before byte swap. With wire RGB565 input the two
  // operations do not commute; keep this combination on the exact CPU path.
  if(swapRedBlue_&&view.order==PixelOrder::Wire)return false;
  if(!ppaCopy_||view.width*view.height<256||view.stride>UINT16_MAX)return false;
  const size_t bytes=rasterWidth()*rasterHeight()*2;
  const size_t line=esp_cache_get_line_size_by_addr(rgbFramebuffer_);
  if(!line||bytes%line||reinterpret_cast<uintptr_t>(rgbFramebuffer_)%line)return false;
  const size_t begin=(size_t(y)*rasterWidth()*2)&~(line-1);
  const size_t end=((size_t(y)+view.height)*rasterWidth()*2+line-1)&~(line-1);
  if(end>bytes)return false;
  auto *region=reinterpret_cast<uint8_t*>(rgbFramebuffer_)+begin;
  // Preserve dirty CPU-rendered neighbors before DMA invalidates whole rows.
  if(esp_cache_msync(region,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_C2M)!=ESP_OK)return false;
  ppa_srm_oper_config_t operation={};
  operation.in.buffer=view.data;operation.in.pic_w=view.stride;operation.in.pic_h=view.height;
  operation.in.block_w=view.width;operation.in.block_h=view.height;operation.in.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
  operation.out.buffer=rgbFramebuffer_;operation.out.buffer_size=bytes;
  operation.out.pic_w=rasterWidth();operation.out.pic_h=rasterHeight();
  operation.out.block_offset_x=x;operation.out.block_offset_y=y;operation.out.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
  operation.scale_x=operation.scale_y=1.0f;operation.byte_swap=view.order==PixelOrder::Wire;
  operation.rgb_swap=swapRedBlue_;operation.mode=PPA_TRANS_MODE_BLOCKING;
  if(ppa_do_scale_rotate_mirror(ppaCopy_,&operation)!=ESP_OK){if(copyStats_)++copyStats_->ppaFailures;return false;}
  // A bounce ISR could have cached these rows while DMA was writing them.
  if(esp_cache_msync(region,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_M2C)!=ESP_OK)return false;
  const bool partial=x!=0||view.width!=rasterWidth();
  if(!ppaVerified_||(partial&&!ppaPartialVerified_)){
    bool exact=true;
    for(size_t row=0;row<view.height&&exact;++row)for(size_t col=0;col<view.width;++col){
      const uint8_t *p=view.data+(row*view.stride+col)*2;
      uint16_t expected=view.order==PixelOrder::Wire?uint16_t(uint16_t(p[0])<<8|p[1]):uint16_t(p[0]|uint16_t(p[1])<<8);
      if(swapRedBlue_)expected=uint16_t((expected&0x07E0)|((expected&31)<<11)|(expected>>11));
      if(rgbFramebuffer_[(size_t(y)+row)*rasterWidth()+size_t(x)+col]!=expected){exact=false;break;}
    }
    if(!exact){
      if(copyStats_){++copyStats_->ppaVerificationFailures;copyStats_->ppaReady=false;}
      if(ppa_unregister_client(ppaCopy_)==ESP_OK)ppaCopy_=nullptr;
      return false;
    }
    ppaVerified_=true;if(partial)ppaPartialVerified_=true;
  }
  if(copyStats_){++copyStats_->ppaTransfers;copyStats_->ppaPixels+=uint64_t(view.width)*view.height;}
  return true;
}
#endif

Status VideoTransport::command(uint8_t c, const uint8_t *data, size_t bytes) {
  if (!rasterReady()) return Status::NotInitialized;
  if (bytes && !data) return Status::InvalidArgument;
  const Status s = wait();
  if (s != Status::Ok) return s;
  if (c == 0x2A || c == 0x2B || c == 0x2C)
    return result_ = FramebufferTransport::command(c, data, bytes);
  if (bytes) return result_ = Status::Unsupported;
  if (c == 0x28 || c == 0x29) return result_ = display(c == 0x29);
  if (c == 0x20 || c == 0x21) {
    const auto *profile = videoControllerProfile(controller_, interface_);
    if (profile && profile->noCommand)
      return result_ = convert(esp_lcd_panel_invert_color(panel_, c == 0x21));
    return result_ = videoInvert(controller_, interface_, c == 0x21, writeCommand, this);
  }
  return result_ = Status::Unsupported;
}
Status VideoTransport::display(bool on) {
  const auto *profile = videoControllerProfile(controller_, interface_);
  if (!profile) return Status::Unsupported;
  // An external DISP signal gates the RGB output independently of controller
  // DCS state. Use the native engine to drive its configured active polarity.
  if (interface_ == VideoInterface::RGB && hasDisplayPin_)
    return convert(esp_lcd_panel_disp_on_off(panel_, on));
  if (profile->noCommand) return Status::Unsupported;
  return videoDisplay(controller_, interface_, on, writeCommand, controllerDelay, this);
}
bool VideoTransport::validOutputPin(int pin) const { return GPIO_IS_VALID_OUTPUT_GPIO(pin); }
void VideoTransport::output(int pin, bool high) { esp32_platform::output(pin,high); }
void VideoTransport::delayMs(uint32_t ms) { esp32_platform::delayMillis(ms); }

#if SOC_LCD_RGB_SUPPORTED
RgbConfig::RgbConfig() {
  rgb.data_width = 16;
#if defined(ESP_IDF_VERSION_MAJOR) && ESP_IDF_VERSION_MAJOR >= 6
  rgb.in_color_format = rgb.out_color_format = LCD_COLOR_FMT_RGB565;
#else
  rgb.bits_per_pixel = 16;
#endif
  rgb.clk_src = LCD_CLK_SRC_DEFAULT;
  rgb.hsync_gpio_num = rgb.vsync_gpio_num = rgb.de_gpio_num = rgb.pclk_gpio_num = rgb.disp_gpio_num = decltype(rgb.disp_gpio_num)(-1);
  for (size_t i = 0; i < sizeof(rgb.data_gpio_nums) / sizeof(rgb.data_gpio_nums[0]); ++i)
    rgb.data_gpio_nums[i] = decltype(rgb.disp_gpio_num)(-1);
}
Status Esp32Rgb::begin() {
  if (rasterReady()) return wait();
  const Status cleaned = shutdown();
  if (cleaned != Status::Ok) return cleaned;
  const auto *profile = videoControllerProfile(config_.controller, VideoInterface::RGB);
  if (!profile || !profile->supportsRgb) return fail(Status::Unsupported);
#if defined(ESP_IDF_VERSION_MAJOR) && ESP_IDF_VERSION_MAJOR >= 6
  // IDF 6 replaces bits_per_pixel with input/output color formats. Zero output
  // means the SDK uses the input format; other outputs require conversion.
  if (config_.rgb.in_color_format != LCD_COLOR_FMT_RGB565 ||
      (config_.rgb.out_color_format && config_.rgb.out_color_format != LCD_COLOR_FMT_RGB565))
    return fail(Status::Unsupported);
#else
  const size_t bpp = config_.rgb.bits_per_pixel ? config_.rgb.bits_per_pixel : config_.rgb.data_width;
  if (bpp != 16) return fail(Status::Unsupported);
#endif
  if (config_.panel.bits_per_pixel != 16 || config_.rgb.flags.no_fb ||
      !(profile->rgbBitsMask & (1UL << 16))) return fail(Status::Unsupported);
  if (config_.rgb.data_width != 8 && config_.rgb.data_width != 16) return fail(Status::Unsupported);
  if (config_.reset < -1 || (config_.reset >= 0 && !validOutputPin(config_.reset)) ||
      !config_.rgb.timings.h_res || !config_.rgb.timings.v_res ||
      config_.rgb.timings.h_res > INT16_MAX || config_.rgb.timings.v_res > INT16_MAX || !config_.rgb.timings.pclk_hz ||
      (config_.panel.rgb_ele_order != LCD_RGB_ELEMENT_ORDER_RGB && config_.panel.rgb_ele_order != LCD_RGB_ELEMENT_ORDER_BGR) ||
      (!config_.init.commands && config_.init.count)) return fail(Status::InvalidArgument);
  const int controlPins[] = {config_.rgb.hsync_gpio_num, config_.rgb.vsync_gpio_num,
    config_.rgb.de_gpio_num, config_.rgb.pclk_gpio_num, config_.rgb.disp_gpio_num};
  for (size_t i = 0; i < sizeof(controlPins) / sizeof(controlPins[0]); ++i)
    if (config_.reset >= 0 && config_.reset == controlPins[i]) return fail(Status::InvalidArgument);
  for (size_t i = 0; i < config_.rgb.data_width; ++i)
    if (config_.reset >= 0 && config_.reset == config_.rgb.data_gpio_nums[i]) return fail(Status::InvalidArgument);
  if (!profile->noCommand) {
    if (!config_.createIo) return fail(Status::InvalidArgument);
    const int commandPins[] = {config_.io3wire.line_config.cs_gpio_num,
      config_.io3wire.line_config.scl_gpio_num, config_.io3wire.line_config.sda_gpio_num};
    for (size_t i = 0; i < 3; ++i) {
      if (commandPins[i] < 0 && i == 0) continue;
      if (commandPins[i] == config_.reset) return fail(Status::InvalidArgument);
      for (size_t j = 0; j < sizeof(controlPins) / sizeof(controlPins[0]); ++j)
        if (commandPins[i] >= 0 && commandPins[i] == controlPins[j]) return fail(Status::InvalidArgument);
      for (size_t j = 0; j < config_.rgb.data_width; ++j)
        if (commandPins[i] >= 0 && commandPins[i] == config_.rgb.data_gpio_nums[j]) return fail(Status::InvalidArgument);
    }
    const Status prepared = prepareCommandSpi(config_.io3wire);
    if (prepared != Status::Ok) return fail(prepared);
  }
  esp_lcd_panel_handle_t created = nullptr;
  const esp_err_t error = esp_lcd_new_rgb_panel(&config_.rgb, &created);
  if (error != ESP_OK) return fail(error);
  if (!created) return fail(Status::Busy);
  panel_ = created; refreshOnDemand_ = config_.rgb.flags.refresh_on_demand;
  const Status initialized = initializeHardware(config_.controller, config_.panel, config_.init, config_.reset,
    config_.rgb.timings.h_res, config_.rgb.timings.v_res, 0, config_.displayOnInit,
    config_.rgb.disp_gpio_num >= 0);
  // Keep multi-buffer configurations on the established IDF copy path.
  if (initialized == Status::Ok && config_.rgb.num_fbs <= 1 && !config_.rgb.flags.double_fb) {
    void *frame = nullptr;
    if (esp_lcd_rgb_panel_get_frame_buffer(panel_, 1, &frame) == ESP_OK)
      rgbFramebuffer_ = static_cast<uint16_t *>(frame);
  }
  copyStats_=config_.copyStats;
  if(copyStats_)*copyStats_=VideoCopyStats{};
  scanStats_=config_.scanStats;if(scanStats_)scanStats_->reset();
#if SOC_PPA_SUPPORTED && defined(CONFIG_IDF_TARGET_ESP32S31)
  if(initialized==Status::Ok&&rgbFramebuffer_&&config_.usePpa){
    ppa_client_config_t client={};client.oper_type=PPA_OPERATION_SRM;client.max_pending_trans_num=1;
    if(ppa_register_client(&client,&ppaCopy_)!=ESP_OK)ppaCopy_=nullptr;
    if(copyStats_)copyStats_->ppaReady=ppaCopy_!=nullptr;
    client.oper_type=PPA_OPERATION_FILL;
    if(ppa_register_client(&client,&ppaFill_)!=ESP_OK)ppaFill_=nullptr;
    if(copyStats_)copyStats_->ppaFillReady=ppaFill_!=nullptr;
  }
#endif
  return initialized;
}
Status Esp32Rgb::setPixelClockHz(uint32_t hz){
  if(!panel_||!rasterReady())return Status::NotInitialized;
  if(hz<100000||hz>80000000)return Status::InvalidArgument;
  const Status s=convert(esp_lcd_rgb_panel_set_pclk(panel_,hz));
  if(s==Status::Ok)config_.rgb.timings.pclk_hz=hz;
  return s;
}
TransportInfo Esp32Rgb::info() const {
  TransportInfo result;
  result.frequency = config_.rgb.timings.pclk_hz; result.clock = config_.rgb.pclk_gpio_num;
  result.lanes = uint8_t(config_.rgb.data_width);
  for (size_t i = 0; i < 16 && i < config_.rgb.data_width; ++i) result.data[i] = config_.rgb.data_gpio_nums[i];
  return result;
}
#endif

#if SOC_MIPI_DSI_SUPPORTED
MipiConfig::MipiConfig() {
  dbi.lcd_cmd_bits = dbi.lcd_param_bits = 8;
  dpi.pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565;
  dpi.in_color_format = dpi.out_color_format = LCD_COLOR_FMT_RGB565;
  dpi.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;
}
Status Esp32Mipi::begin() {
  if (rasterReady()) return wait();
  const Status cleaned = shutdown();
  if (cleaned != Status::Ok) return cleaned;
  const auto *profile = videoControllerProfile(config_.controller, VideoInterface::MIPI);
  if (!profile || !profile->supportsMipi) return fail(Status::Unsupported);
  if (config_.panel.rgb_ele_order == LCD_RGB_ELEMENT_ORDER_BGR && !profile->colorOrderSupported)
    return fail(Status::Unsupported);
  // ST77922 has one physical data pair. Keep its explicit-one-lane contract;
  // SDK zero would otherwise select two lanes, contrary to that hardware.
  if (config_.controller == VideoController::ST77922 && config_.dsi.num_data_lanes != 1)
    return fail(Status::InvalidArgument);
  const unsigned lanes = config_.dsi.num_data_lanes ? config_.dsi.num_data_lanes : profile->defaultLanes;
  if (config_.reset < -1 || (config_.reset >= 0 && !validOutputPin(config_.reset)) ||
      lanes > 2 || !(profile->laneMask & (1U << lanes)) ||
      config_.dbi.lcd_cmd_bits != 8 || config_.dbi.lcd_param_bits != 8 ||
      (config_.panel.rgb_ele_order != LCD_RGB_ELEMENT_ORDER_RGB && config_.panel.rgb_ele_order != LCD_RGB_ELEMENT_ORDER_BGR) ||
      (!config_.init.commands && config_.init.count)) return fail(Status::InvalidArgument);
  if ((config_.dpi.in_color_format && config_.dpi.in_color_format != LCD_COLOR_FMT_RGB565) ||
      (!config_.dpi.in_color_format && config_.dpi.pixel_format != LCD_COLOR_PIXEL_FORMAT_RGB565))
    return fail(Status::Unsupported);
  const auto output = config_.dpi.out_color_format ? config_.dpi.out_color_format : LCD_COLOR_FMT_RGB565;
  unsigned bits;
  switch (output) {
    case LCD_COLOR_FMT_RGB565: bits = 16; break;
    case LCD_COLOR_FMT_RGB666: bits = 18; break;
    case LCD_COLOR_FMT_RGB888: bits = 24; break;
    default: return fail(Status::Unsupported);
  }
  if (config_.panel.bits_per_pixel != bits) return fail(Status::InvalidArgument);
  if (!(profile->mipiBitsMask & (1UL << bits))) return fail(Status::Unsupported);
#if !defined(CONFIG_ESP_REV_MIN_FULL) || CONFIG_ESP_REV_MIN_FULL < 300
  if (bits != 16) return fail(Status::Unsupported);
#endif
  if (!config_.dpi.video_timing.h_size || !config_.dpi.video_timing.v_size ||
      config_.dpi.video_timing.h_size > INT16_MAX || config_.dpi.video_timing.v_size > INT16_MAX ||
      !(config_.dpi.dpi_clock_freq_mhz > 0 && config_.dpi.dpi_clock_freq_mhz < 1000000.0f) ||
      !(config_.dsi.lane_bit_rate_mbps > 0 && config_.dsi.lane_bit_rate_mbps < 1000000.0f))
    return fail(Status::InvalidArgument);
  config_.dsi.num_data_lanes = lanes;
  esp_err_t error;
  if (config_.acquireLdo) {
    esp_ldo_channel_handle_t handle = nullptr;
    error = esp_ldo_acquire_channel(&config_.ldo, &handle);
    if (error != ESP_OK) return fail(error);
    ldo_ = handle;
  }
  esp_lcd_dsi_bus_handle_t bus = nullptr;
  error = esp_lcd_new_dsi_bus(&config_.dsi, &bus);
  if (error != ESP_OK) return fail(error);
  dsiBus_ = bus;
  esp_lcd_panel_io_handle_t createdIo = nullptr;
  error = esp_lcd_new_panel_io_dbi(bus, &config_.dbi, &createdIo);
  if (error != ESP_OK) return fail(error);
  io_ = createdIo;
  esp_lcd_panel_handle_t created = nullptr;
  error = esp_lcd_new_panel_dpi(bus, &config_.dpi, &created);
  if (error != ESP_OK) return fail(error);
  if (!created) return fail(Status::Busy);
  panel_ = created;
  return initializeHardware(config_.controller, config_.panel, config_.init, config_.reset,
    config_.dpi.video_timing.h_size, config_.dpi.video_timing.v_size, lanes,
    config_.displayOnInit, false);
}
TransportInfo Esp32Mipi::info() const {
  TransportInfo result;
  result.frequency = uint32_t(config_.dpi.dpi_clock_freq_mhz * 1000000.0f);
  const auto *profile = videoControllerProfile(config_.controller, VideoInterface::MIPI);
  result.lanes = config_.dsi.num_data_lanes ? config_.dsi.num_data_lanes : profile ? profile->defaultLanes : 0;
  return result;
}
#endif
} // namespace nice_gfx
#endif

#endif
