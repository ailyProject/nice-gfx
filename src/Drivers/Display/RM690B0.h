#pragma once
#include "../../Display/ControllerProfile.h"

// RM690B0 initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_RM690B0(PanelSequence &s) {
{
  s.writecommand(0xFE);
  s.writedata(0x20);
  s.writecommand(0x26);
  s.writedata(0x0A);
  s.writecommand(0x24);
  s.writedata(0x80);
  s.writecommand(0x5A);
  s.writedata(0x51);
  s.writecommand(0x5B);
  s.writedata(0x2E);
  s.writecommand(0xFE);
  s.writedata(0x00);
  s.writecommand(0x3A);
  s.writedata(0x55);
  s.writecommand(0xC2);
  s.delay(10);
  s.writecommand(0x35);
  s.writedata(0x00);
  s.writecommand(0x51);
  s.writedata(0x00);
  s.writecommand(0x11);
  s.delay(120);
  s.writecommand(0x29);
  s.delay(10);
  s.writecommand(0x51);
  s.writedata(0xD0);
  s.writecommand(0x20);
}
}
inline void rotate_RM690B0(PanelSequence &s, uint8_t m) {
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
inline ControllerProfile profile_RM690B0() {
  return {"RM690B0",482,600,0x690B0,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0,0x00,0x08,init_RM690B0,rotate_RM690B0,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class RM690B0 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit RM690B0(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_RM690B0(), reset, options) {}
};
} // namespace nice_gfx
