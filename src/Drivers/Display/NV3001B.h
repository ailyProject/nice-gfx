#pragma once
#include "../../Display/ControllerProfile.h"

// NV3001B initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_NV3001B(PanelSequence &s) {
{
  s.writecommand(0xFF);
  s.writedata(0xA5);
  s.writecommand(0x41);
  s.writedata(0x00);
  s.writecommand(0x50);
  s.writedata(0x02);
  s.writecommand(0x52);
  s.writedata(0x6E);
  s.writecommand(0x57);
  s.writedata(0x02);
  s.writecommand(0x46);
  s.writedata(0x11);
  s.writecommand(0x47);
  s.writedata(0x00);
  s.writedata(0x01);
  s.writecommand(0x8F);
  s.writedata(0x22);
  s.writedata(0x03);
  s.writecommand(0x9A);
  s.writedata(0x78);
  s.writecommand(0x9B);
  s.writedata(0x78);
  s.writecommand(0x9C);
  s.writedata(0xA0);
  s.writecommand(0x9D);
  s.writedata(0x17);
  s.writecommand(0x9E);
  s.writedata(0xC1);
  s.writecommand(0x83);
  s.writedata(0x5A);
  s.writecommand(0x84);
  s.writedata(0xB6);
  s.writecommand(0xFF);
  s.writedata(0xA5);
  s.writecommand(0x85);
  s.writedata(0x5F);
  s.writecommand(0x6E);
  s.writedata(0x0F);
  s.writecommand(0x7E);
  s.writedata(0x0F);
  s.writecommand(0x60);
  s.writedata(0x00);
  s.writecommand(0x70);
  s.writedata(0x00);
  s.writecommand(0x6D);
  s.writedata(0x33);
  s.writecommand(0x7D);
  s.writedata(0x37);
  s.writecommand(0x61);
  s.writedata(0x09);
  s.writecommand(0x71);
  s.writedata(0x0A);
  s.writecommand(0x6C);
  s.writedata(0x2A);
  s.writecommand(0x7C);
  s.writedata(0x36);
  s.writecommand(0x62);
  s.writedata(0x11);
  s.writecommand(0x72);
  s.writedata(0x10);
  s.writecommand(0x68);
  s.writedata(0x4E);
  s.writecommand(0x78);
  s.writedata(0x4E);
  s.writecommand(0x66);
  s.writedata(0x36);
  s.writecommand(0x76);
  s.writedata(0x3C);
  s.writecommand(0x1A);
  s.writedata(0x1C);
  s.writecommand(0x7B);
  s.writedata(0x14);
  s.writecommand(0x63);
  s.writedata(0x0D);
  s.writecommand(0x73);
  s.writedata(0x0A);
  s.writecommand(0x6A);
  s.writedata(0x16);
  s.writecommand(0x7A);
  s.writedata(0x12);
  s.writecommand(0x64);
  s.writedata(0x0B);
  s.writecommand(0x74);
  s.writedata(0x0A);
  s.writecommand(0x69);
  s.writedata(0x08);
  s.writecommand(0x79);
  s.writedata(0x0A);
  s.writecommand(0x65);
  s.writedata(0x06);
  s.writecommand(0x75);
  s.writedata(0x07);
  s.writecommand(0x67);
  s.writedata(0x23);
  s.writecommand(0x77);
  s.writedata(0x44);
  s.writecommand(0xE0);
  s.writedata(0x00);
  s.writecommand(0xE9);
  s.writedata(0x30);
  s.writecommand(0xEB);
  s.writedata(0xB7);
  s.writecommand(0xEC);
  s.writedata(0x00);
  s.writecommand(0xED);
  s.writedata(0x11);
  s.writecommand(0xF0);
  s.writedata(0xB7);
  s.writecommand(0x53);
  s.writedata(0x04);
  s.writecommand(0x54);
  s.writedata(0x04);
  s.writecommand(0x55);
  s.writedata(0x40);
  s.writecommand(0x56);
  s.writedata(0x40);
  s.writecommand(0xA0);
  s.writedata(0x60);
  s.writedata(0x01);
  s.writecommand(0xA1);
  s.writedata(0x84);
  s.writecommand(0xA2);
  s.writedata(0x85);
  s.writecommand(0xAB);
  s.writedata(0x00);
  s.writedata(0x02);
  s.writecommand(0xAC);
  s.writedata(0x00);
  s.writedata(0x06);
  s.writecommand(0xAD);
  s.writedata(0x00);
  s.writedata(0x03);
  s.writecommand(0xAE);
  s.writedata(0x00);
  s.writedata(0x07);
  s.writecommand(0xC7);
  s.writedata(0x01);
  s.writecommand(0xB9);
  s.writedata(0x82);
  s.writecommand(0xBA);
  s.writedata(0x83);
  s.writecommand(0xBB);
  s.writedata(0x00);
  s.writecommand(0xBC);
  s.writedata(0x81);
  s.writecommand(0xBD);
  s.writedata(0x02);
  s.writecommand(0xBE);
  s.writedata(0x01);
  s.writecommand(0xBF);
  s.writedata(0x04);
  s.writecommand(0xC0);
  s.writedata(0x03);
  s.writecommand(0xC8);
  s.writedata(0x55);
  s.writecommand(0xC9);
  s.writedata(0xC9);
  s.writecommand(0xCA);
  s.writedata(0xC8);
  s.writecommand(0xCB);
  s.writedata(0xCB);
  s.writecommand(0xCC);
  s.writedata(0xCA);
  s.writecommand(0xCD);
  s.writedata(0x55);
  s.writecommand(0xCE);
  s.writedata(0xCE);
  s.writecommand(0xCF);
  s.writedata(0xCD);
  s.writecommand(0xD0);
  s.writedata(0xD0);
  s.writecommand(0xD1);
  s.writedata(0xCF);
  s.writecommand(0xF2);
  s.writedata(0x46);
  s.writecommand(0xA8);
  s.writedata(0x04);
  s.writecommand(0xA9);
  s.writedata(0xB0);
  s.writecommand(0xAA);
  s.writedata(0xA3);
  s.writecommand(0xB6);
  s.writedata(0x00);
  s.writecommand(0xB7);
  s.writedata(0xB0);
  s.writecommand(0xB8);
  s.writedata(0xA3);
  s.writecommand(0xC4);
  s.writedata(0x03);
  s.writecommand(0xC5);
  s.writedata(0xB0);
  s.writecommand(0xC6);
  s.writedata(0xA3);
  s.writecommand(0x80);
  s.writedata(0x10);
  s.writecommand(0xFF);
  s.writedata(0x00);
  s.writecommand(0x35);
  s.writedata(0x00);
  s.writecommand(0x11);
  s.delay(120);
  s.writecommand(0x3A);
  s.writedata(0x05);
  s.writecommand(0x36);
  s.writedata(0x00);
  s.writecommand(0x29);
  s.delay(10);
  s.writecommand(0x21);
}
}
inline void rotate_NV3001B(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
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
  }
}
inline ControllerProfile profile_NV3001B() {
  return {"NV3001B",128,220,0x3001B,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0x00,0x00,0x08,init_NV3001B,rotate_NV3001B,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class NV3001B : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit NV3001B(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_NV3001B(), reset, options) {}
};
} // namespace nice_gfx
