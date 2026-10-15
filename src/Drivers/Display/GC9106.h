#pragma once
#include "../../Display/ControllerProfile.h"

// GC9106 initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_GC9106(PanelSequence &s) {
{
  s.writecommand(0xFE);
  s.writecommand(0xFE);
  s.writecommand(0xFE);
  s.writecommand(0xEF);
  s.writecommand(0xB3);
  s.writedata(0x03);
  s.writecommand(0x36);
  s.writedata(0xD8);
  s.writecommand(0x3A);
  s.writedata(0x05);
  s.writecommand(0xB6);
  s.writedata(0x11);
  s.writecommand(0xAC);
  s.writedata(0x0B);
  s.writecommand(0xB4);
  s.writedata(0x21);
  s.writecommand(0xB1);
  s.writedata(0xC0);
  s.writecommand(0xE6);
  s.writedata(0x50);
  s.writedata(0x43);
  s.writecommand(0xE7);
  s.writedata(0x56);
  s.writedata(0x43);
  s.writecommand(0xF0);
  s.writedata(0x1F);
  s.writedata(0x41);
  s.writedata(0x1B);
  s.writedata(0x55);
  s.writedata(0x36);
  s.writedata(0x3D);
  s.writedata(0x3E);
  s.writedata(0x00);
  s.writedata(0x16);
  s.writedata(0x08);
  s.writedata(0x09);
  s.writedata(0x15);
  s.writedata(0x14);
  s.writedata(0x0F);
  s.writecommand(0xF1);
  s.writedata(0x1F);
  s.writedata(0x41);
  s.writedata(0x1B);
  s.writedata(0x55);
  s.writedata(0x36);
  s.writedata(0x3D);
  s.writedata(0x3E);
  s.writedata(0x00);
  s.writedata(0x16);
  s.writedata(0x08);
  s.writedata(0x09);
  s.writedata(0x15);
  s.writedata(0x14);
  s.writedata(0x0F);
  s.writecommand(0xFE);
  s.writecommand(0xFF);
  s.writecommand(0x35);
  s.writedata(0x00);
  s.writecommand(0x44);
  s.writedata(0x00);
  s.writecommand(0x11);
  s.delay(120);
  s.writecommand(0x29);
  s.delay(20);
  s.writecommand(0x20);
}
}
inline void rotate_GC9106(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  s.writecommand(0x36);
  switch (s.rotation) {
    case 0:
      s.writedata(s.colorOrder);
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 24; s.rowstart = 0;
      break;
    case 1:
      s.writedata(uint8_t(0x60 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 24;
      break;
    case 2:
      s.writedata(uint8_t(0xC0 | s.colorOrder));
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 24; s.rowstart = 0;
      break;
    case 3:
      s.writedata(uint8_t(0xA0 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 24;
      break;
  }
}
inline ControllerProfile profile_GC9106() {
  return {"GC9106",80,160,0x9106,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0x08,0x00,0x08,init_GC9106,rotate_GC9106,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class GC9106 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit GC9106(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_GC9106(), reset, options) {}
};
} // namespace nice_gfx
