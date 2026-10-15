#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_ILI9488_Parallel(PanelSequence &s) {
s.writecommand(0xE0);
    s.writedata(0x00);
    s.writedata(0x03);
    s.writedata(0x09);
    s.writedata(0x08);
    s.writedata(0x16);
    s.writedata(0x0A);
    s.writedata(0x3F);
    s.writedata(0x78);
    s.writedata(0x4C);
    s.writedata(0x09);
    s.writedata(0x0A);
    s.writedata(0x08);
    s.writedata(0x16);
    s.writedata(0x1A);
    s.writedata(0x0F);
    s.writecommand(0XE1);
    s.writedata(0x00);
    s.writedata(0x16);
    s.writedata(0x19);
    s.writedata(0x03);
    s.writedata(0x0F);
    s.writedata(0x05);
    s.writedata(0x32);
    s.writedata(0x45);
    s.writedata(0x46);
    s.writedata(0x04);
    s.writedata(0x0E);
    s.writedata(0x0D);
    s.writedata(0x35);
    s.writedata(0x37);
    s.writedata(0x0F);
    s.writecommand(0XC0);
    s.writedata(0x17);
    s.writedata(0x15);
    s.writecommand(0xC1);
    s.writedata(0x41);
    s.writecommand(0xC5);
    s.writedata(0x00);
    s.writedata(0x12);
    s.writedata(0x80);
    s.writecommand(0x36);
    s.writedata(0x48);
    s.writecommand(0x3A);
    s.writedata(0x55);
    s.writecommand(0xB0);
    s.writedata(0x00);
    s.writecommand(0xB1);
    s.writedata(0xA0);
    s.writecommand(0xB4);
    s.writedata(0x02);
    s.writecommand(0xB6);
    s.writedata(0x02);
    s.writedata(0x02);
    s.writedata(0x3B);
    s.writecommand(0xB7);
    s.writedata(0xC6);
    s.writecommand(0xF7);
    s.writedata(0xA9);
    s.writedata(0x51);
    s.writedata(0x2C);
    s.writedata(0x82);
    s.writecommand(0x11);
s.delay(120);
    s.writecommand(0x29);
s.delay(25);
}
inline void rotate_ILI9488_Parallel(PanelSequence &s, uint8_t m) {
s.writecommand(0x36);
  s.rotation = m % 8;
  switch (s.rotation) {
   case 0:
     s.writedata(uint8_t(0x40 | s.colorOrder));
      s._width = s._init_width;
      s._height = s._init_height;
     break;
   case 1:
     s.writedata(uint8_t(0x20 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
     break;
   case 2:
     s.writedata(uint8_t(0x80 | s.colorOrder));
      s._width = s._init_width;
      s._height = s._init_height;
     break;
   case 3:
     s.writedata(0x40 | 0x80 | uint8_t(0x20 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
     break;
   case 4:
     s.writedata(0x40 | uint8_t(0x80 | s.colorOrder));
      s._width = s._init_width;
      s._height = s._init_height;
     break;
   case 5:
     s.writedata(0x20 | uint8_t(0x40 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
     break;
   case 6:
     s.writedata(s.colorOrder);
      s._width = s._init_width;
      s._height = s._init_height;
     break;
   case 7:
     s.writedata(0x80 | uint8_t(0x20 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
     break;
  }
}
inline ControllerProfile profile_ILI9488_Parallel() {
  return {"ILI9488_Parallel",320,480,0x9488,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,8,0x08,0x00,0x08,init_ILI9488_Parallel,rotate_ILI9488_Parallel,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class ILI9488_Parallel : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit ILI9488_Parallel(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ILI9488_Parallel(), reset, options) {}
};
} // namespace nice_gfx
