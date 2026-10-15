#pragma once
#include "../../Display/ControllerProfile.h"

// R61529 initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_R61529(PanelSequence &s) {
{
  s.writecommand(0x11);
  s.delay(200);
  s.writecommand(0xB0);
  s.writedata(0x04);
  s.writecommand(0xB8);
  s.writedata(0x02);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writecommand(0xB9);
  s.writedata(0x01);
  s.writedata(0x00);
  s.writedata(0xFF);
  s.writedata(0x18);
  s.writecommand(0xB3);
  s.writedata(0x02);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.delay(2);
  s.writecommand(0xB4);
  s.writedata(0x00);
  s.writecommand(0xC0);
  s.writedata(0x03);
  s.writedata(0xDF);
  s.writedata(0x40);
  s.writedata(0x10);
  s.writedata(0x00);
  s.writedata(0x01);
  s.writedata(0x00);
  s.writedata(0x55);
  s.writecommand(0xC1);
  s.writedata(0x07);
  s.writedata(0x27);
  s.writedata(0x08);
  s.writedata(0x08);
  s.writedata(0x00);
  s.writecommand(0xC4);
  s.writedata(0x57);
  s.writedata(0x00);
  s.writedata(0x05);
  s.writedata(0x03);
  s.writecommand(0xC6);
  s.writedata(0x04);
  s.writecommand(0xC8);
  s.writedata(0x03);
  s.writedata(0x12);
  s.writedata(0x1A);
  s.writedata(0x24);
  s.writedata(0x32);
  s.writedata(0x4B);
  s.writedata(0x3B);
  s.writedata(0x29);
  s.writedata(0x1F);
  s.writedata(0x18);
  s.writedata(0x12);
  s.writedata(0x04);
  s.writedata(0x03);
  s.writedata(0x12);
  s.writedata(0x1A);
  s.writedata(0x24);
  s.writedata(0x32);
  s.writedata(0x4B);
  s.writedata(0x3B);
  s.writedata(0x29);
  s.writedata(0x1F);
  s.writedata(0x18);
  s.writedata(0x12);
  s.writedata(0x04);
  s.writecommand(0xC9);
  s.writedata(0x03);
  s.writedata(0x12);
  s.writedata(0x1A);
  s.writedata(0x24);
  s.writedata(0x32);
  s.writedata(0x4B);
  s.writedata(0x3B);
  s.writedata(0x29);
  s.writedata(0x1F);
  s.writedata(0x18);
  s.writedata(0x12);
  s.writedata(0x04);
  s.writedata(0x03);
  s.writedata(0x12);
  s.writedata(0x1A);
  s.writedata(0x24);
  s.writedata(0x32);
  s.writedata(0x4B);
  s.writedata(0x3B);
  s.writedata(0x29);
  s.writedata(0x1F);
  s.writedata(0x18);
  s.writedata(0x12);
  s.writedata(0x04);
  s.writecommand(0xCA);
  s.writedata(0x03);
  s.writedata(0x12);
  s.writedata(0x1A);
  s.writedata(0x24);
  s.writedata(0x32);
  s.writedata(0x4B);
  s.writedata(0x3B);
  s.writedata(0x29);
  s.writedata(0x1F);
  s.writedata(0x18);
  s.writedata(0x12);
  s.writedata(0x04);
  s.writedata(0x03);
  s.writedata(0x12);
  s.writedata(0x1A);
  s.writedata(0x24);
  s.writedata(0x32);
  s.writedata(0x4B);
  s.writedata(0x3B);
  s.writedata(0x29);
  s.writedata(0x1F);
  s.writedata(0x18);
  s.writedata(0x12);
  s.writedata(0x04);
  s.writecommand(0xD0);
  s.writedata(0x99);
  s.writedata(0x06);
  s.writedata(0x08);
  s.writedata(0x20);
  s.writedata(0x29);
  s.writedata(0x04);
  s.writedata(0x01);
  s.writedata(0x00);
  s.writedata(0x08);
  s.writedata(0x01);
  s.writedata(0x00);
  s.writedata(0x06);
  s.writedata(0x01);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x20);
  s.writecommand(0xD1);
  s.writedata(0x00);
  s.writedata(0x20);
  s.writedata(0x20);
  s.writedata(0x15);
  s.writecommand(0xE0);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writecommand(0xE1);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writecommand(0xE2);
  s.writedata(0x00);
  s.writecommand(0x3A);
  s.writedata(0x55);
  s.writecommand(0x29);
  s.delay(20);
}
}
inline void rotate_R61529(PanelSequence &s, uint8_t m) {
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
      s.writedata(uint8_t(0x41 | s.colorOrder));
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 3:
      s.writedata(uint8_t(0xA1 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
  }
}
inline ControllerProfile profile_R61529() {
  return {"R61529",320,480,0x61529,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0x00,0x00,0x08,init_R61529,rotate_R61529,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class R61529 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit R61529(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_R61529(), reset, options) {}
};
} // namespace nice_gfx
