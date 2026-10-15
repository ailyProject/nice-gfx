#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_GC9A01(PanelSequence &s) {
{
  s.writecommand(0xEF);
  s.writecommand(0xEB);
  s.writedata(0x14);
  s.writecommand(0xFE);
  s.writecommand(0xEF);
  s.writecommand(0xEB);
  s.writedata(0x14);
  s.writecommand(0x84);
  s.writedata(0x40);
  s.writecommand(0x85);
  s.writedata(0xFF);
  s.writecommand(0x86);
  s.writedata(0xFF);
  s.writecommand(0x87);
  s.writedata(0xFF);
  s.writecommand(0x88);
  s.writedata(0x0A);
  s.writecommand(0x89);
  s.writedata(0x21);
  s.writecommand(0x8A);
  s.writedata(0x00);
  s.writecommand(0x8B);
  s.writedata(0x80);
  s.writecommand(0x8C);
  s.writedata(0x01);
  s.writecommand(0x8D);
  s.writedata(0x01);
  s.writecommand(0x8E);
  s.writedata(0xFF);
  s.writecommand(0x8F);
  s.writedata(0xFF);
  s.writecommand(0xB6);
  s.writedata(0x00);
  s.writedata(0x20);
  s.writecommand(0x3A);
  s.writedata(0x05);
  s.writecommand(0x90);
  s.writedata(0x08);
  s.writedata(0x08);
  s.writedata(0x08);
  s.writedata(0x08);
  s.writecommand(0xBD);
  s.writedata(0x06);
  s.writecommand(0xBC);
  s.writedata(0x00);
  s.writecommand(0xFF);
  s.writedata(0x60);
  s.writedata(0x01);
  s.writedata(0x04);
  s.writecommand(0xC3);
  s.writedata(0x13);
  s.writecommand(0xC4);
  s.writedata(0x13);
  s.writecommand(0xC9);
  s.writedata(0x22);
  s.writecommand(0xBE);
  s.writedata(0x11);
  s.writecommand(0xE1);
  s.writedata(0x10);
  s.writedata(0x0E);
  s.writecommand(0xDF);
  s.writedata(0x21);
  s.writedata(0x0c);
  s.writedata(0x02);
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
  s.writecommand(0xED);
  s.writedata(0x1B);
  s.writedata(0x0B);
  s.writecommand(0xAE);
  s.writedata(0x77);
  s.writecommand(0xCD);
  s.writedata(0x63);
  s.writecommand(0x70);
  s.writedata(0x07);
  s.writedata(0x07);
  s.writedata(0x04);
  s.writedata(0x0E);
  s.writedata(0x0F);
  s.writedata(0x09);
  s.writedata(0x07);
  s.writedata(0x08);
  s.writedata(0x03);
  s.writecommand(0xE8);
  s.writedata(0x34);
  s.writecommand(0x62);
  s.writedata(0x18);
  s.writedata(0x0D);
  s.writedata(0x71);
  s.writedata(0xED);
  s.writedata(0x70);
  s.writedata(0x70);
  s.writedata(0x18);
  s.writedata(0x0F);
  s.writedata(0x71);
  s.writedata(0xEF);
  s.writedata(0x70);
  s.writedata(0x70);
  s.writecommand(0x63);
  s.writedata(0x18);
  s.writedata(0x11);
  s.writedata(0x71);
  s.writedata(0xF1);
  s.writedata(0x70);
  s.writedata(0x70);
  s.writedata(0x18);
  s.writedata(0x13);
  s.writedata(0x71);
  s.writedata(0xF3);
  s.writedata(0x70);
  s.writedata(0x70);
  s.writecommand(0x64);
  s.writedata(0x28);
  s.writedata(0x29);
  s.writedata(0xF1);
  s.writedata(0x01);
  s.writedata(0xF1);
  s.writedata(0x00);
  s.writedata(0x07);
  s.writecommand(0x66);
  s.writedata(0x3C);
  s.writedata(0x00);
  s.writedata(0xCD);
  s.writedata(0x67);
  s.writedata(0x45);
  s.writedata(0x45);
  s.writedata(0x10);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writecommand(0x67);
  s.writedata(0x00);
  s.writedata(0x3C);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x01);
  s.writedata(0x54);
  s.writedata(0x10);
  s.writedata(0x32);
  s.writedata(0x98);
  s.writecommand(0x74);
  s.writedata(0x10);
  s.writedata(0x85);
  s.writedata(0x80);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x4E);
  s.writedata(0x00);
  s.writecommand(0x98);
  s.writedata(0x3e);
  s.writedata(0x07);
  s.writecommand(0x35);
  s.writecommand(0x21);
  s.writecommand(0x11);
  s.delay(120);
  s.writecommand(0x29);
  s.delay(20);
}
}
inline void rotate_GC9A01(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  s.writecommand(0x36);
  switch (s.rotation) {
    case 0:
      s.writedata(s.colorOrder);
      s._width = s._init_width;
      s._height = s._init_height;
      if (s._init_width == 128)
      {
        s.colstart = 2;
        s.rowstart = 1;
      }
      break;
    case 1:
      s.writedata(0x40 | uint8_t(0x20 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
      if (s._init_width == 128)
      {
        s.colstart = 1;
        s.rowstart = 2;
      }
      break;
    case 2:
      s.writedata(0x40 | uint8_t(0x80 | s.colorOrder));
      s._width = s._init_width;
      s._height = s._init_height;
      if (s._init_width == 128)
      {
        s.colstart = 2;
        s.rowstart = 1;
      }
      break;
    case 3:
      s.writedata(0x20 | uint8_t(0x80 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
      if (s._init_width == 128)
      {
        s.colstart = 1;
        s.rowstart = 2;
      }
      break;
  }
}
inline ControllerProfile profile_GC9A01() {
  return {"GC9A01",240,240,0x9A01,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0x08,0x00,0x08,init_GC9A01,rotate_GC9A01,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class GC9A01 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit GC9A01(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_GC9A01(), reset, options) {}
};
} // namespace nice_gfx
