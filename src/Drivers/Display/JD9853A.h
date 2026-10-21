#pragma once
#include "ST7789.h"

namespace nice_gfx { namespace controllers {
// Seeed's 172x320 JD9853A uses the ST7789-compatible startup, with inversion
// disabled and a 34-column crop. See docs/VendorTouchScreens.md for pinned sources.
inline void init_JD9853A_172(PanelSequence &s) {
  controllers::init_ST7789(s); s.writecommand(0x20);
}
inline void rotate_JD9853A_172(PanelSequence &s, uint8_t r) {
  const uint8_t scan[] = {0x40, 0x20, 0x80, 0xE0};
  s.writecommand(0x36); s.writedata(scan[r & 3] | s.colorOrder);
  s.colstart = r & 1 ? 0 : 34; s.rowstart = r & 1 ? 34 : 0;
}
inline ControllerProfile profile_JD9853A_172() {
  return {"JD9853A_172",172,320,0x9853,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,8,0,8,init_JD9853A_172,rotate_JD9853A_172,PixelReadFormat::RGB888,1,0};
}
// Waveshare's Arduino demo uses Arduino_ST7789, IPS=false, offsets 34/0.
// Keep its scan orientation distinct from Seeed's MX-mirrored JD9853A glass.
inline void rotate_JD9853_172(PanelSequence &s, uint8_t r) {
  const uint8_t scan[] = {0, 0x60, 0xC0, 0xA0};
  s.writecommand(0x36); s.writedata(scan[r & 3] | s.colorOrder);
  s.colstart = r & 1 ? 0 : 34; s.rowstart = r & 1 ? 34 : 0;
}
inline ControllerProfile profile_JD9853_172() {
  ControllerProfile p = profile_JD9853A_172();
  p.name = "JD9853_172"; p.defaultColorOrder = 0; p.rotate = rotate_JD9853_172; return p;
}
} }

namespace nice_gfx {
class JD9853A_172x320 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  JD9853A_172x320(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_JD9853A_172(), reset, options) {}
};
class JD9853_172x320 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  JD9853_172x320(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_JD9853_172(), reset, options) {}
};
}
