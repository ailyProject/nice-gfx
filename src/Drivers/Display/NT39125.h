#pragma once
#include "../../Display/ControllerProfile.h"

// NT39125 initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_NT39125(PanelSequence &s) {
{
  s.writecommand(0x11);
  s.delay(250);
  s.delay(250);
  s.delay(250);
  s.delay(250);
  s.delay(200);
  s.writecommand(0x34);
  s.writecommand(0xB1);
  s.writedata(0x11);
  s.writedata(0x1B);
  s.writecommand(0xB2);
  s.writedata(0x11);
  s.writedata(0x1B);
  s.writecommand(0xB3);
  s.writedata(0x11);
  s.writedata(0x1B);
  s.writecommand(0xB4);
  s.writedata(0x02);
  s.writecommand(0xB6);
  s.writedata(0x01);
  s.writedata(0x02);
  s.writecommand(0xC0);
  s.writedata(0x24);
  s.writecommand(0xC1);
  s.writedata(0x02);
  s.writedata(0x00);
  s.writecommand(0xC2);
  s.writedata(0x05);
  s.writedata(0x01);
  s.writecommand(0xC3);
  s.writedata(0x02);
  s.writedata(0x05);
  s.writecommand(0xC4);
  s.writedata(0x02);
  s.writedata(0x04);
  s.writecommand(0xC5);
  s.writedata(0x14);
  s.writedata(0x2E);
  s.writecommand(0xEA);
  s.writedata(0x01);
  s.writecommand(0xE0);
  s.writedata(0x27);
  s.writedata(0x2B);
  s.writedata(0x2E);
  s.writedata(0x06);
  s.writedata(0x0D);
  s.writedata(0x11);
  s.writedata(0x28);
  s.writedata(0x7B);
  s.writedata(0x35);
  s.writedata(0x0C);
  s.writedata(0x20);
  s.writedata(0x26);
  s.writedata(0x25);
  s.writedata(0x28);
  s.writedata(0x3C);
  s.writecommand(0xE1);
  s.writedata(0x08);
  s.writedata(0x21);
  s.writedata(0x26);
  s.writedata(0x09);
  s.writedata(0x0F);
  s.writedata(0x12);
  s.writedata(0x1F);
  s.writedata(0x38);
  s.writedata(0x31);
  s.writedata(0x0D);
  s.writedata(0x23);
  s.writedata(0x29);
  s.writedata(0x2C);
  s.writedata(0x2F);
  s.writedata(0x33);
  s.writecommand(0xE2);
  s.writedata(0x27);
  s.writedata(0x2C);
  s.writedata(0x2F);
  s.writedata(0x07);
  s.writedata(0x0E);
  s.writedata(0x11);
  s.writedata(0x29);
  s.writedata(0x7A);
  s.writedata(0x35);
  s.writedata(0x0C);
  s.writedata(0x20);
  s.writedata(0x26);
  s.writedata(0x24);
  s.writedata(0x29);
  s.writedata(0x3C);
  s.writecommand(0xE3);
  s.writedata(0x08);
  s.writedata(0x20);
  s.writedata(0x26);
  s.writedata(0x09);
  s.writedata(0x0F);
  s.writedata(0x12);
  s.writedata(0x1F);
  s.writedata(0x48);
  s.writedata(0x30);
  s.writedata(0x0D);
  s.writedata(0x22);
  s.writedata(0x28);
  s.writedata(0x2B);
  s.writedata(0x2E);
  s.writedata(0x33);
  s.writecommand(0xE4);
  s.writedata(0x1F);
  s.writedata(0x24);
  s.writedata(0x27);
  s.writedata(0x08);
  s.writedata(0x0F);
  s.writedata(0x12);
  s.writedata(0x25);
  s.writedata(0x7B);
  s.writedata(0x32);
  s.writedata(0x0C);
  s.writedata(0x20);
  s.writedata(0x26);
  s.writedata(0x20);
  s.writedata(0x25);
  s.writedata(0x3C);
  s.writecommand(0xE5);
  s.writedata(0x08);
  s.writedata(0x24);
  s.writedata(0x2B);
  s.writedata(0x09);
  s.writedata(0x0F);
  s.writedata(0x12);
  s.writedata(0x22);
  s.writedata(0x38);
  s.writedata(0x35);
  s.writedata(0x0C);
  s.writedata(0x21);
  s.writedata(0x27);
  s.writedata(0x33);
  s.writedata(0x36);
  s.writedata(0x3B);
  s.writecommand(0x3A);
  s.writedata(0x55);
  s.writecommand(0x11);
  s.delay(250);
  s.delay(250);
  s.delay(250);
  s.delay(250);
  s.delay(200);
  s.writecommand(0x29);
  s.writecommand(0x20);
}
}
inline void rotate_NT39125(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  s.writecommand(0x36);
  switch (s.rotation) {
    case 0:
      s.writedata(uint8_t(0x40 | s.colorOrder));
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 1:
      s.writedata(uint8_t(0x20 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 2:
      s.writedata(uint8_t(0x80 | s.colorOrder));
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 3:
      s.writedata(uint8_t(0xE0 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
  }
}
inline ControllerProfile profile_NT39125() {
  return {"NT39125",240,432,0x39125,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0x08,0x00,0x08,init_NT39125,rotate_NT39125,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class NT39125 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit NT39125(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_NT39125(), reset, options) {}
};
} // namespace nice_gfx
