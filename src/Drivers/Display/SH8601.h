#pragma once
#include "../../Display/ControllerProfile.h"

// SH8601 initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_SH8601(PanelSequence &s) {
{
  s.writecommand(0x11);
  s.delay(120);
  s.writecommand(0x13);
  s.writecommand(0x20);
  s.writecommand(0x3A);
  s.writedata(0x05);
  s.writecommand(0x29);
  s.writecommand(0x53);
  s.writedata(0x28);
  s.writecommand(0x51);
  s.writedata(0xD0);
  s.writecommand(0x58);
  s.writedata(0x00);
  s.delay(10);
  s.writecommand(0x20);
}
}
inline void rotate_SH8601(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  s.writecommand(0x36);
  switch (s.rotation) {
    case 0:
      s.writedata(0x00);
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 1:
      s.writedata(0x00);
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 2:
      s.writedata(0x07);
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 3:
      s.writedata(0x00);
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
  }
}
inline ControllerProfile profile_SH8601() {
  return {"SH8601",480,480,0x8601,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,1,0,0x00,0x08,init_SH8601,rotate_SH8601,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class SH8601 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit SH8601(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_SH8601(), reset, options, 2, 2) {}
};
} // namespace nice_gfx
