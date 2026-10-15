#pragma once
#include "../../Display/ControllerProfile.h"

// GC9C01 initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_GC9C01(PanelSequence &s) {
{
  s.writecommand(0xFE);
  s.writecommand(0xEF);
  s.writecommand(0x80);
  s.writedata(0x11);
  s.writecommand(0x81);
  s.writedata(0x70);
  s.writecommand(0x82);
  s.writedata(0x09);
  s.writecommand(0x83);
  s.writedata(0x03);
  s.writecommand(0x84);
  s.writedata(0x20);
  s.writecommand(0x85);
  s.writedata(0x42);
  s.writecommand(0x86);
  s.writedata(0xFC);
  s.writecommand(0x87);
  s.writedata(0x09);
  s.writecommand(0x89);
  s.writedata(0x10);
  s.writecommand(0x8A);
  s.writedata(0x4F);
  s.writecommand(0x8C);
  s.writedata(0x59);
  s.writecommand(0x8D);
  s.writedata(0x51);
  s.writecommand(0x8E);
  s.writedata(0xAE);
  s.writecommand(0x8F);
  s.writedata(0xF3);
  s.writecommand(0x36);
  s.writedata(0x00);
  s.writecommand(0x3A);
  s.writedata(0x05);
  s.writecommand(0xEC);
  s.writedata(0x77);
  s.writecommand(0x74);
  s.writedata(0x01);
  s.writedata(0x80);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writecommand(0x98);
  s.writedata(0x3E);
  s.writecommand(0x99);
  s.writedata(0x3E);
  s.writecommand(0xC3);
  s.writedata(0x2A);
  s.writecommand(0xC4);
  s.writedata(0x18);
  s.writecommand(0xA1);
  s.writedata(0x01);
  s.writedata(0x04);
  s.writecommand(0xA2);
  s.writedata(0x01);
  s.writedata(0x04);
  s.writecommand(0xA9);
  s.writedata(0x1C);
  s.writecommand(0xA5);
  s.writedata(0x11);
  s.writedata(0x09);
  s.writecommand(0xB9);
  s.writedata(0x8A);
  s.writecommand(0xA8);
  s.writedata(0x5E);
  s.writecommand(0xA7);
  s.writedata(0x40);
  s.writecommand(0xAF);
  s.writedata(0x73);
  s.writecommand(0xAE);
  s.writedata(0x44);
  s.writecommand(0xAD);
  s.writedata(0x38);
  s.writecommand(0xA3);
  s.writedata(0x5D);
  s.writecommand(0xC2);
  s.writedata(0x02);
  s.writecommand(0xC5);
  s.writedata(0x11);
  s.writecommand(0xC6);
  s.writedata(0x0E);
  s.writecommand(0xC7);
  s.writedata(0x13);
  s.writecommand(0xC8);
  s.writedata(0x0D);
  s.writecommand(0xCB);
  s.writedata(0x02);
  s.writecommand(0x7C);
  s.writedata(0xB6);
  s.writedata(0x26);
  s.writecommand(0xAC);
  s.writedata(0x24);
  s.writecommand(0xF6);
  s.writedata(0x80);
  s.writecommand(0xB5);
  s.writedata(0x09);
  s.writedata(0x09);
  s.writecommand(0x60);
  s.writedata(0x38);
  s.writedata(0x0B);
  s.writedata(0x5B);
  s.writedata(0x56);
  s.writecommand(0x63);
  s.writedata(0x3A);
  s.writedata(0xE0);
  s.writedata(0x5B);
  s.writedata(0x56);
  s.writecommand(0x64);
  s.writedata(0x38);
  s.writedata(0x0D);
  s.writedata(0x72);
  s.writedata(0xDD);
  s.writedata(0x5B);
  s.writedata(0x56);
  s.writecommand(0x66);
  s.writedata(0x38);
  s.writedata(0x11);
  s.writedata(0x72);
  s.writedata(0xE1);
  s.writedata(0x5B);
  s.writedata(0x56);
  s.writecommand(0x68);
  s.writedata(0x3B);
  s.writedata(0x08);
  s.writedata(0x08);
  s.writedata(0x00);
  s.writedata(0x08);
  s.writedata(0x29);
  s.writedata(0x5B);
  s.writecommand(0x6E);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x07);
  s.writedata(0x01);
  s.writedata(0x13);
  s.writedata(0x11);
  s.writedata(0x0B);
  s.writedata(0x09);
  s.writedata(0x16);
  s.writedata(0x15);
  s.writedata(0x1D);
  s.writedata(0x1E);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x1E);
  s.writedata(0x1D);
  s.writedata(0x15);
  s.writedata(0x16);
  s.writedata(0x0A);
  s.writedata(0x0C);
  s.writedata(0x12);
  s.writedata(0x14);
  s.writedata(0x02);
  s.writedata(0x08);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writecommand(0xBE);
  s.writedata(0x11);
  s.writecommand(0x6C);
  s.writedata(0xCC);
  s.writedata(0x0C);
  s.writedata(0xCC);
  s.writedata(0x84);
  s.writedata(0xCC);
  s.writedata(0x04);
  s.writedata(0x50);
  s.writecommand(0x7D);
  s.writedata(0x72);
  s.writecommand(0x7E);
  s.writedata(0x38);
  s.writecommand(0x70);
  s.writedata(0x02);
  s.writedata(0x03);
  s.writedata(0x09);
  s.writedata(0x05);
  s.writedata(0x0C);
  s.writedata(0x06);
  s.writedata(0x09);
  s.writedata(0x05);
  s.writedata(0x0C);
  s.writedata(0x06);
  s.writecommand(0x90);
  s.writedata(0x06);
  s.writedata(0x06);
  s.writedata(0x05);
  s.writedata(0x06);
  s.writecommand(0x93);
  s.writedata(0x45);
  s.writedata(0xFF);
  s.writedata(0x00);
  s.writecommand(0xF0);
  s.writedata(0x45);
  s.writedata(0x09);
  s.writedata(0x08);
  s.writedata(0x08);
  s.writedata(0x26);
  s.writedata(0x2A);
  s.writecommand(0xF1);
  s.writedata(0x43);
  s.writedata(0x70);
  s.writedata(0x72);
  s.writedata(0x36);
  s.writedata(0x37);
  s.writedata(0x6F);
  s.writecommand(0xF2);
  s.writedata(0x45);
  s.writedata(0x09);
  s.writedata(0x08);
  s.writedata(0x08);
  s.writedata(0x26);
  s.writedata(0x2A);
  s.writecommand(0xF3);
  s.writedata(0x43);
  s.writedata(0x70);
  s.writedata(0x72);
  s.writedata(0x36);
  s.writedata(0x37);
  s.writedata(0x6F);
  s.writecommand(0x11);
  s.delay(120);
  s.writecommand(0x29);
  s.delay(10);
  s.writecommand(0x20);
}
}
inline void rotate_GC9C01(PanelSequence &s, uint8_t m) {
s.rotation = m % 8;
  s.writecommand(0x36);
  switch (s.rotation) {
    case 0:
      s.writedata(s.colorOrder);
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 1:
      s.writedata(uint8_t(0x60 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 2:
      s.writedata(uint8_t(0xC0 | s.colorOrder));
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 3:
      s.writedata(uint8_t(0xA0 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 4:
      s.writedata(uint8_t(0x40 | s.colorOrder));
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 5:
      s.writedata(uint8_t(0xE0 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 6:
      s.writedata(uint8_t(0x80 | s.colorOrder));
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 7:
      s.writedata(uint8_t(0x20 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
  }
}
inline ControllerProfile profile_GC9C01() {
  return {"GC9C01",360,360,0x9C01,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,8,0x00,0x00,0x08,init_GC9C01,rotate_GC9C01,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class GC9C01 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit GC9C01(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_GC9C01(), reset, options) {}
};
} // namespace nice_gfx
