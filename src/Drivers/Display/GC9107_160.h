#pragma once
#include "../../Display/ControllerProfile.h"

// The common 128 x 128 sequence is based on:
// https://github.com/moononournation/Arduino_GFX/tree/master/src/display
// The 128 x 160 sequence is based on displaylib_16bit_PICO, where it is used
// https://github.com/gavinlyonsrepo/displaylib_16bit_PICO

namespace nice_gfx { namespace controllers {
inline void init_GC9107_160(PanelSequence &s) {
{
  s.writecommand(0xFE);
  s.writecommand(0xEF);
  s.writecommand(0xB0);
  s.writedata(0xC0);
  s.writecommand(0xB2);
  s.writedata(0x2F);
  s.writecommand(0xB3);
  s.writedata(0x03);
  s.writecommand(0xB6);
  s.writedata(0x19);
  s.writecommand(0xB7);
  s.writedata(0x01);
  s.writecommand(0xAC);
  s.writedata(0xCB);
  s.writecommand(0xAB);
  s.writedata(0x07);
  s.writecommand(0x36);
  s.writedata(s.colorOrder);
  s.writecommand(0xB4);
  s.writedata(0x00);
  s.writecommand(0xA8);
  s.writedata(0x0C);
  s.writecommand(0xE8);
  s.writedata(0x23);
  s.writecommand(0xE9);
  s.writedata(0x47);
  s.writecommand(0xEA);
  s.writedata(0x44);
  s.writecommand(0xEB);
  s.writedata(0xE0);
  s.writecommand(0xED);
  s.writedata(0x03);
  s.writecommand(0xC6);
  s.writedata(0x19);
  s.writecommand(0xC7);
  s.writedata(0x10);
  s.writecommand(0xF0);
  s.writedata(0x0B);
  s.writedata(0x2F);
  s.writedata(0x10);
  s.writedata(0x4B);
  s.writedata(0x28);
  s.writedata(0x3F);
  s.writedata(0x3E);
  s.writedata(0x60);
  s.writedata(0x00);
  s.writedata(0x12);
  s.writedata(0x12);
  s.writedata(0x0F);
  s.writedata(0x00);
  s.writedata(0x1F);
  s.writecommand(0xF1);
  s.writedata(0x0E);
  s.writedata(0x3D);
  s.writedata(0x2A);
  s.writedata(0x40);
  s.writedata(0xFA);
  s.writedata(0x00);
  s.writedata(0x02);
  s.writedata(0x60);
  s.writedata(0x00);
  s.writedata(0x03);
  s.writedata(0x13);
  s.writedata(0x00);
  s.writedata(0x10);
  s.writedata(0x1F);
  s.writecommand(0x3A);
  s.writedata(0x05);
  s.writecommand(0xB8);
  s.writedata(0x08);
  s.writecommand(0x11);
  s.delay(120);
  s.writecommand(0x29);
  s.delay(10);
}
}
inline void rotate_GC9107_160(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  s.writecommand(0x36);
  switch (s.rotation) {
    case 0:
      s.writedata(s.colorOrder);
      s._width  = s._init_width;
      s._height = s._init_height;
      s.colstart = s.offsetX;
      s.rowstart = s.offsetY;
      break;
    case 1:
      s.writedata(0x10 | 0x20 | uint8_t(0x40 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
      s.colstart = s.offsetY;
      s.rowstart = s.offsetX;
      break;
    case 2:
      s.writedata(0x80 | uint8_t(0x40 | s.colorOrder));
      s._width  = s._init_width;
      s._height = s._init_height;
      s.colstart = 128 - s._init_width - s.offsetX;
      s.rowstart = 160 - s._init_height - s.offsetY;
      break;
    case 3:
      s.writedata(0x80 | uint8_t(0x20 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
      s.colstart = 160 - s._init_height - s.offsetY;
      s.rowstart = 128 - s._init_width - s.offsetX;
      break;
  }
}
inline ControllerProfile profile_GC9107_160() {
  return {"GC9107_160",128,160,0x9107,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0x08,0x00,0x08,init_GC9107_160,rotate_GC9107_160,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class GC9107_160 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit GC9107_160(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_GC9107_160(), reset, options) {}
};
} // namespace nice_gfx
