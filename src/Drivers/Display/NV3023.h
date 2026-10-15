#pragma once
#include "../../Display/ControllerProfile.h"

// NV3023 initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_NV3023(PanelSequence &s) {
{
  s.writecommand(0xFF);
  s.writedata(0xA5);
  s.writecommand(0x3E);
  s.writedata(0x09);
  s.writecommand(0x3A);
  s.writedata(0x65);
  s.writecommand(0x82);
  s.writedata(0x00);
  s.writecommand(0x98);
  s.writedata(0x00);
  s.writecommand(0x63);
  s.writedata(0x0F);
  s.writecommand(0x64);
  s.writedata(0x0F);
  s.writecommand(0xB4);
  s.writedata(0x34);
  s.writecommand(0xB5);
  s.writedata(0x30);
  s.writecommand(0x83);
  s.writedata(0x03);
  s.writecommand(0x86);
  s.writedata(0x04);
  s.writecommand(0x87);
  s.writedata(0x16);
  s.writecommand(0x88);
  s.writedata(0x0A);
  s.writecommand(0x89);
  s.writedata(0x27);
  s.writecommand(0x93);
  s.writedata(0x63);
  s.writecommand(0x96);
  s.writedata(0x81);
  s.writecommand(0xC3);
  s.writedata(0x10);
  s.writecommand(0xE6);
  s.writedata(0x00);
  s.writecommand(0x99);
  s.writedata(0x01);
  s.writecommand(0x70);
  s.writedata(0x09);
  s.writecommand(0x71);
  s.writedata(0x1D);
  s.writecommand(0x72);
  s.writedata(0x14);
  s.writecommand(0x73);
  s.writedata(0x0A);
  s.writecommand(0x74);
  s.writedata(0x11);
  s.writecommand(0x75);
  s.writedata(0x16);
  s.writecommand(0x76);
  s.writedata(0x38);
  s.writecommand(0x77);
  s.writedata(0x0B);
  s.writecommand(0x78);
  s.writedata(0x08);
  s.writecommand(0x79);
  s.writedata(0x3E);
  s.writecommand(0x7A);
  s.writedata(0x07);
  s.writecommand(0x7B);
  s.writedata(0x0D);
  s.writecommand(0x7C);
  s.writedata(0x16);
  s.writecommand(0x7D);
  s.writedata(0x0F);
  s.writecommand(0x7E);
  s.writedata(0x14);
  s.writecommand(0x7F);
  s.writedata(0x05);
  s.writecommand(0xA0);
  s.writedata(0x04);
  s.writecommand(0xA1);
  s.writedata(0x28);
  s.writecommand(0xA2);
  s.writedata(0x0C);
  s.writecommand(0xA3);
  s.writedata(0x11);
  s.writecommand(0xA4);
  s.writedata(0x0B);
  s.writecommand(0xA5);
  s.writedata(0x23);
  s.writecommand(0xA6);
  s.writedata(0x45);
  s.writecommand(0xA7);
  s.writedata(0x07);
  s.writecommand(0xA8);
  s.writedata(0x0A);
  s.writecommand(0xA9);
  s.writedata(0x3B);
  s.writecommand(0xAA);
  s.writedata(0x0D);
  s.writecommand(0xAB);
  s.writedata(0x18);
  s.writecommand(0xAC);
  s.writedata(0x14);
  s.writecommand(0xAD);
  s.writedata(0x0F);
  s.writecommand(0xAE);
  s.writedata(0x19);
  s.writecommand(0xAF);
  s.writedata(0x08);
  s.writecommand(0xFF);
  s.writedata(0x00);
  s.writecommand(0x11);
  s.delay(120);
  s.writecommand(0x29);
  s.delay(20);
  s.writecommand(0x20);
}
}
inline void rotate_NV3023(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  s.writecommand(0x36);
  switch (s.rotation) {
    case 0:
      s.writedata(uint8_t(0x80 | s.colorOrder));
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 1:
      s.writedata(uint8_t(0xE0 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 2:
      s.writedata(uint8_t(0x40 | s.colorOrder));
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 3:
      s.writedata(uint8_t(0x20 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
  }
}
inline ControllerProfile profile_NV3023() {
  return {"NV3023",128,128,0x3023,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0x08,0x00,0x08,init_NV3023,rotate_NV3023,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class NV3023 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit NV3023(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_NV3023(), reset, options) {}
};
} // namespace nice_gfx
