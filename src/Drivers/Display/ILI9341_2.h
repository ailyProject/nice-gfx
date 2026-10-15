#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_ILI9341_2(PanelSequence &s) {
{
s.writecommand(0xCF);
s.writedata(0x00);
s.writedata(0XC1);
s.writedata(0X30);
s.writecommand(0xED);
s.writedata(0x64);
s.writedata(0x03);
s.writedata(0X12);
s.writedata(0X81);
s.writecommand(0xE8);
s.writedata(0x85);
s.writedata(0x00);
s.writedata(0x78);
s.writecommand(0xCB);
s.writedata(0x39);
s.writedata(0x2C);
s.writedata(0x00);
s.writedata(0x34);
s.writedata(0x02);
s.writecommand(0xF7);
s.writedata(0x20);
s.writecommand(0xEA);
s.writedata(0x00);
s.writedata(0x00);
s.writecommand(0xC0);
s.writedata(0x10);
s.writecommand(0xC1);
s.writedata(0x00);
s.writecommand(0xC5);
s.writedata(0x30);
s.writedata(0x30);
s.writecommand(0xC7);
s.writedata(0xB7);
s.writecommand(0x3A);
s.writedata(0x55);
s.writecommand(0x36);
s.writedata(0x08);
s.writecommand(0xB1);
s.writedata(0x00);
s.writedata(0x1A);
s.writecommand(0xB6);
s.writedata(0x08);
s.writedata(0x82);
s.writedata(0x27);
s.writecommand(0xF2);
s.writedata(0x00);
s.writecommand(0x26);
s.writedata(0x01);
s.writecommand(0xE0);
s.writedata(0x0F);
s.writedata(0x2A);
s.writedata(0x28);
s.writedata(0x08);
s.writedata(0x0E);
s.writedata(0x08);
s.writedata(0x54);
s.writedata(0xA9);
s.writedata(0x43);
s.writedata(0x0A);
s.writedata(0x0F);
s.writedata(0x00);
s.writedata(0x00);
s.writedata(0x00);
s.writedata(0x00);
s.writecommand(0XE1);
s.writedata(0x00);
s.writedata(0x15);
s.writedata(0x17);
s.writedata(0x07);
s.writedata(0x11);
s.writedata(0x06);
s.writedata(0x2B);
s.writedata(0x56);
s.writedata(0x3C);
s.writedata(0x05);
s.writedata(0x10);
s.writedata(0x0F);
s.writedata(0x3F);
s.writedata(0x3F);
s.writedata(0x0F);
s.writecommand(0x2B);
s.writedata(0x00);
s.writedata(0x00);
s.writedata(0x01);
s.writedata(0x3f);
s.writecommand(0x2A);
s.writedata(0x00);
s.writedata(0x00);
s.writedata(0x00);
s.writedata(0xef);
s.writecommand(0x11);
s.endPanelWrite();
s.delay(120);
s.beginPanelWrite();
s.writecommand(0x29);
}
}
inline void rotate_ILI9341_2(PanelSequence &s, uint8_t m) {
s.rotation = m % 8;
  s.writecommand(0x36);
  switch (s.rotation) {
    case 0:
      s.writedata(uint8_t(0x40 | s.colorOrder));
      s._width  = s._init_width;
      s._height = s._init_height;
      break;
    case 1:
      s.writedata(uint8_t(0x20 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
      break;
    case 2:
      s.writedata(uint8_t(0x80 | s.colorOrder));
      s._width  = s._init_width;
      s._height = s._init_height;
      break;
    case 3:
      s.writedata(0x40 | 0x80 | uint8_t(0x20 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
      break;
    case 4:
      s.writedata(0x40 | uint8_t(0x80 | s.colorOrder));
      s._width  = s._init_width;
      s._height = s._init_height;
      break;
    case 5:
      s.writedata(0x20 | uint8_t(0x40 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
      break;
    case 6:
      s.writedata(s.colorOrder);
      s._width  = s._init_width;
      s._height = s._init_height;
      break;
    case 7:
      s.writedata(0x80 | uint8_t(0x20 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
      break;
  }
}
inline ControllerProfile profile_ILI9341_2() {
  return {"ILI9341_2",240,320,0x9341,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,8,0x08,0x00,0x08,init_ILI9341_2,rotate_ILI9341_2,PixelReadFormat::RGB888,1,0xD9};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class ILI9341_2 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit ILI9341_2(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ILI9341_2(), reset, options) {}
};
} // namespace nice_gfx
