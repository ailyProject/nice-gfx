#pragma once
#include "Display.h"
#include "../Platform/ESP32/Esp32Video.h"
#include "../Platform/ESP32/Esp32ScanQspi.h"

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
namespace nice_gfx {
namespace detail {
// GFX controller programs initialize the chip; the renderer uses its normal
// RGB565 window interface while the transport writes the video framebuffer.
template<uint16_t Width, uint16_t Height> class VideoPanel : public Panel {
  static void initialize(PanelSequence &) {}
  static void rotate(PanelSequence &, uint8_t) {}
  static const ControllerProfile &profile() {
    static const ControllerProfile p = {"GFX video", Width, Height, 0,
      Protocol::Dcs, PixelFormat::RGB565, 0x2C, 0, 0, 1, 0, 0, 0,
      initialize, rotate, PixelReadFormat::RGB565, 0, 0};
    return p;
  }
 public:
  VideoPanel(Transport &bus, int reset=-1, PanelOptions options=PanelOptions())
    : Panel(bus, profile(), reset, options) {}
};
inline PanelOptions videoOptions(size_t width, size_t height) {
  PanelOptions options;
  if (width <= INT16_MAX && height <= INT16_MAX) {
    options.width = uint16_t(width); options.height = uint16_t(height);
  }
  return options;
}
}

#if SOC_LCD_RGB_SUPPORTED
template<VideoController Controller, uint16_t Width, uint16_t Height>
class RgbDisplay : public OwnedDisplay<Esp32Rgb,detail::VideoPanel<Width,Height>> {
  using Base=OwnedDisplay<Esp32Rgb,detail::VideoPanel<Width,Height>>;
  static RgbConfig config(RgbConfig value) { value.controller=Controller; return value; }
 public:
  explicit RgbDisplay(const RgbConfig &value)
    : Base(detail::videoOptions(value.rgb.timings.h_res,value.rgb.timings.v_res),-1,config(value)) {}
};
#endif

#if SOC_MIPI_DSI_SUPPORTED
template<VideoController Controller, uint16_t Width, uint16_t Height>
class MipiDisplay : public OwnedDisplay<Esp32Mipi,detail::VideoPanel<Width,Height>> {
  using Base=OwnedDisplay<Esp32Mipi,detail::VideoPanel<Width,Height>>;
  static MipiConfig config(MipiConfig value) { value.controller=Controller; return value; }
 public:
  explicit MipiDisplay(const MipiConfig &value)
    : Base(detail::videoOptions(value.dpi.video_timing.h_size,value.dpi.video_timing.v_size),-1,config(value)) {}
};
#endif

template<uint16_t Width, uint16_t Height>
class ScanQspiDisplay : public OwnedDisplay<Esp32ScanQspi,detail::VideoPanel<Width,Height>> {
  using Base=OwnedDisplay<Esp32ScanQspi,detail::VideoPanel<Width,Height>>;
 public:
  explicit ScanQspiDisplay(const ScanQspiConfig &value)
    : Base(detail::videoOptions(value.width,value.height),-1,value) {}
};
using ST77903_400x400_QSPI=ScanQspiDisplay<400,400>;
} // namespace nice_gfx
#endif
