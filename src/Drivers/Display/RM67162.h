#pragma once
#include "../../Display/ControllerProfile.h"

// RM67162 initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_RM67162(PanelSequence &s) {
{
  s.writecommand(0xFE);
  s.writedata(0x04);
  s.writecommand(0x6A);
  s.writedata(0x00);
  s.writecommand(0xFE);
  s.writedata(0x05);
  s.writecommand(0xFE);
  s.writedata(0x07);
  s.writecommand(0x07);
  s.writedata(0x4F);
  s.writecommand(0xFE);
  s.writedata(0x01);
  s.writecommand(0x2A);
  s.writedata(0x02);
  s.writecommand(0x2B);
  s.writedata(0x73);
  s.writecommand(0xFE);
  s.writedata(0x0A);
  s.writecommand(0x29);
  s.writedata(0x10);
  s.writecommand(0xFE);
  s.writedata(0x00);
  s.writecommand(0x51);
  s.writedata(0xBF);
  s.writecommand(0x53);
  s.writedata(0x20);
  s.writecommand(0x35);
  s.writedata(0x00);
  s.writecommand(0x3A);
  s.writedata(0x75);
  s.writecommand(0xC4);
  s.writedata(0x80);
  s.writecommand(0x11);
  s.delay(120);
  s.writecommand(0x29);
  s.delay(20);
  s.writecommand(0x20);
}
}
inline void rotate_RM67162(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  s.writecommand(0x36);
  switch (s.rotation) {
    case 0:
      s.writedata(0x00 | s.colorOrder);
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 1:
      s.writedata(0x60 | s.colorOrder);
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 2:
      s.writedata(0xC0 | s.colorOrder);
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 3:
      s.writedata(0xA0 | s.colorOrder);
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
  }
}
inline ControllerProfile profile_RM67162() {
  return {"RM67162",240,536,0x67162,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0,0x00,0x08,init_RM67162,rotate_RM67162,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class RM67162 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit RM67162(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_RM67162(), reset, options) {}
};
} // namespace nice_gfx
