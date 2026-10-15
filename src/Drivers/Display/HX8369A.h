#pragma once
#include "../../Display/ControllerProfile.h"

// HX8369A initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_HX8369A(PanelSequence &s) {
{
  s.writecommand(0xB9);
  s.writedata(0xFF);
  s.writedata(0x83);
  s.writedata(0x69);
  s.writecommand(0xB1);
  s.writedata(0x01);
  s.writedata(0x00);
  s.writedata(0x34);
  s.writedata(0x07);
  s.writedata(0x00);
  s.writedata(0x01);
  s.writedata(0x0F);
  s.writedata(0x2A);
  s.writedata(0x32);
  s.writedata(0x3F);
  s.writedata(0x3F);
  s.writedata(0x07);
  s.writedata(0x10);
  s.writedata(0x01);
  s.writedata(0xE6);
  s.writedata(0xE6);
  s.writedata(0xE6);
  s.writedata(0xE6);
  s.writedata(0xE6);
  s.writecommand(0xB2);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x03);
  s.writedata(0x03);
  s.writedata(0x70);
  s.writedata(0x00);
  s.writedata(0xFF);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x03);
  s.writedata(0x03);
  s.writedata(0x00);
  s.writedata(0x01);
  s.writecommand(0xB4);
  s.writedata(0x00);
  s.writedata(0x0C);
  s.writedata(0xA0);
  s.writedata(0x0E);
  s.writedata(0x06);
  s.writecommand(0xB6);
  s.writedata(0x2C);
  s.writedata(0x2C);
  s.writecommand(0xD5);
  s.writedata(0x00);
  s.writedata(0x05);
  s.writedata(0x03);
  s.writedata(0x00);
  s.writedata(0x01);
  s.writedata(0x09);
  s.writedata(0x10);
  s.writedata(0x80);
  s.writedata(0x37);
  s.writedata(0x37);
  s.writedata(0x20);
  s.writedata(0x31);
  s.writedata(0x46);
  s.writedata(0x8A);
  s.writedata(0x57);
  s.writedata(0x9B);
  s.writedata(0x20);
  s.writedata(0x31);
  s.writedata(0x46);
  s.writedata(0x8A);
  s.writedata(0x57);
  s.writedata(0x9B);
  s.writedata(0x07);
  s.writedata(0x0F);
  s.writedata(0x02);
  s.writedata(0x00);
  s.writecommand(0xE0);
  s.writedata(0x00);
  s.writedata(0x08);
  s.writedata(0x0D);
  s.writedata(0x2D);
  s.writedata(0x34);
  s.writedata(0x3F);
  s.writedata(0x19);
  s.writedata(0x38);
  s.writedata(0x09);
  s.writedata(0x0E);
  s.writedata(0x0E);
  s.writedata(0x12);
  s.writedata(0x14);
  s.writedata(0x12);
  s.writedata(0x14);
  s.writedata(0x13);
  s.writedata(0x19);
  s.writedata(0x00);
  s.writedata(0x08);
  s.writedata(0x0D);
  s.writedata(0x2D);
  s.writedata(0x34);
  s.writedata(0x3F);
  s.writedata(0x19);
  s.writedata(0x38);
  s.writedata(0x09);
  s.writedata(0x0E);
  s.writedata(0x0E);
  s.writedata(0x12);
  s.writedata(0x14);
  s.writedata(0x12);
  s.writedata(0x14);
  s.writedata(0x13);
  s.writedata(0x19);
  s.writecommand(0xC1);
  s.writedata(0x01);
  s.writedata(0x02);
  s.writedata(0x08);
  s.writedata(0x12);
  s.writedata(0x1A);
  s.writedata(0x22);
  s.writedata(0x2A);
  s.writedata(0x31);
  s.writedata(0x36);
  s.writedata(0x3F);
  s.writedata(0x48);
  s.writedata(0x51);
  s.writedata(0x58);
  s.writedata(0x60);
  s.writedata(0x68);
  s.writedata(0x70);
  s.writedata(0x78);
  s.writedata(0x80);
  s.writedata(0x88);
  s.writedata(0x90);
  s.writedata(0x98);
  s.writedata(0xA0);
  s.writedata(0xA7);
  s.writedata(0xAF);
  s.writedata(0xB6);
  s.writedata(0xBE);
  s.writedata(0xC7);
  s.writedata(0xCE);
  s.writedata(0xD6);
  s.writedata(0xDE);
  s.writedata(0xE6);
  s.writedata(0xEF);
  s.writedata(0xF5);
  s.writedata(0xFB);
  s.writedata(0xFC);
  s.writedata(0xFE);
  s.writedata(0x8C);
  s.writedata(0xA4);
  s.writedata(0x19);
  s.writedata(0xEC);
  s.writedata(0x1B);
  s.writedata(0x4C);
  s.writedata(0x40);
  s.writedata(0x02);
  s.writedata(0x08);
  s.writedata(0x12);
  s.writedata(0x1A);
  s.writedata(0x22);
  s.writedata(0x2A);
  s.writedata(0x31);
  s.writedata(0x36);
  s.writedata(0x3F);
  s.writedata(0x48);
  s.writedata(0x51);
  s.writedata(0x58);
  s.writedata(0x60);
  s.writedata(0x68);
  s.writedata(0x70);
  s.writedata(0x78);
  s.writedata(0x80);
  s.writedata(0x88);
  s.writedata(0x90);
  s.writedata(0x98);
  s.writedata(0xA0);
  s.writedata(0xA7);
  s.writedata(0xAF);
  s.writedata(0xB6);
  s.writedata(0xBE);
  s.writedata(0xC7);
  s.writedata(0xCE);
  s.writedata(0xD6);
  s.writedata(0xDE);
  s.writedata(0xE6);
  s.writedata(0xEF);
  s.writedata(0xF5);
  s.writedata(0xFB);
  s.writedata(0xFC);
  s.writedata(0xFE);
  s.writedata(0x8C);
  s.writedata(0xA4);
  s.writedata(0x19);
  s.writedata(0xEC);
  s.writedata(0x1B);
  s.writedata(0x4C);
  s.writedata(0x40);
  s.writedata(0x02);
  s.writedata(0x08);
  s.writedata(0x12);
  s.writedata(0x1A);
  s.writedata(0x22);
  s.writedata(0x2A);
  s.writedata(0x31);
  s.writedata(0x36);
  s.writedata(0x3F);
  s.writedata(0x48);
  s.writedata(0x51);
  s.writedata(0x58);
  s.writedata(0x60);
  s.writedata(0x68);
  s.writedata(0x70);
  s.writedata(0x78);
  s.writedata(0x80);
  s.writedata(0x88);
  s.writedata(0x90);
  s.writedata(0x98);
  s.writedata(0xA0);
  s.writedata(0xA7);
  s.writedata(0xAF);
  s.writedata(0xB6);
  s.writedata(0xBE);
  s.writedata(0xC7);
  s.writedata(0xCE);
  s.writedata(0xD6);
  s.writedata(0xDE);
  s.writedata(0xE6);
  s.writedata(0xEF);
  s.writedata(0xF5);
  s.writedata(0xFB);
  s.writedata(0xFC);
  s.writedata(0xFE);
  s.writedata(0x8C);
  s.writedata(0xA4);
  s.writedata(0x19);
  s.writedata(0xEC);
  s.writedata(0x1B);
  s.writedata(0x4C);
  s.writedata(0x40);
  s.writecommand(0x2D);
  for (uint16_t i = 0; i <= 63; ++i) s.writedata(i * 8);
  for (uint16_t i = 0; i <= 63; ++i) s.writedata(i * 4);
  for (uint16_t i = 0; i <= 63; ++i) s.writedata(i * 8);
  s.writecommand(0x35);
  s.writedata(0x00);
  s.writecommand(0x3A);
  s.writedata(0x55);
  s.writecommand(0x11);
  s.delay(120);
  s.writecommand(0x29);
  s.writecommand(0x20);
}
}
inline void rotate_HX8369A(PanelSequence &s, uint8_t m) {
s.rotation = m % 8;
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
    case 4:
      s.writedata(0x40 | s.colorOrder);
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 5:
      s.writedata(0xE0 | s.colorOrder);
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 6:
      s.writedata(0x80 | s.colorOrder);
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 7:
      s.writedata(0x20 | s.colorOrder);
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
  }
}
inline ControllerProfile profile_HX8369A() {
  return {"HX8369A",480,864,0x8369A,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,8,0,0x00,0x08,init_HX8369A,rotate_HX8369A,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class HX8369A : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit HX8369A(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_HX8369A(), reset, options) {}
};
} // namespace nice_gfx
