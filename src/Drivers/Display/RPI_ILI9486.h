#pragma once
#include "../../Display/ControllerProfile.h"

// From https://github.com/notro/fbtft/blob/master/fb_ili9486.c

namespace nice_gfx { namespace controllers {
inline void init_RPI_ILI9486(PanelSequence &s) {
{
    s.writecommand(0x01);
    s.delay(120);
    s.writecommand(0x11);
    s.delay(120);
    s.writecommand(0x3A);
      s.writedata(0x55);
    s.writecommand(0xC0);
    s.writedata(0x0E);
    s.writedata(0x0E);
    s.writecommand(0xC1);
    s.writedata(0x41);
    s.writedata(0x00);
    s.writecommand(0xC2);
    s.writedata(0x55);
    s.writecommand(0xC5);
    s.writedata(0x00);
    s.writedata(0x00);
    s.writedata(0x00);
    s.writedata(0x00);
    s.writecommand(0xE0);
    s.writedata(0x0F);
    s.writedata(0x1F);
    s.writedata(0x1C);
    s.writedata(0x0C);
    s.writedata(0x0F);
    s.writedata(0x08);
    s.writedata(0x48);
    s.writedata(0x98);
    s.writedata(0x37);
    s.writedata(0x0A);
    s.writedata(0x13);
    s.writedata(0x04);
    s.writedata(0x11);
    s.writedata(0x0D);
    s.writedata(0x00);
    s.writecommand(0xE1);
    s.writedata(0x0F);
    s.writedata(0x32);
    s.writedata(0x2E);
    s.writedata(0x0B);
    s.writedata(0x0D);
    s.writedata(0x05);
    s.writedata(0x47);
    s.writedata(0x75);
    s.writedata(0x37);
    s.writedata(0x06);
    s.writedata(0x10);
    s.writedata(0x03);
    s.writedata(0x24);
    s.writedata(0x20);
    s.writedata(0x00);
      s.writecommand(0x20);
    s.writecommand(0x36);
    s.writedata(0x48);
    s.writecommand(0x29);
    s.delay(150);
}
}
inline void rotate_RPI_ILI9486(PanelSequence &s, uint8_t m) {
s.writecommand(0x36);
  s.rotation = m % 8;
  switch (s.rotation) {
   case 0:
     s.writedata(s.colorOrder | 0x40);
     s._width = s._init_width;
     s._height = s._init_height;
     break;
   case 1:
     s.writedata(s.colorOrder | 0x20);
     s._width = s._init_height;
     s._height = s._init_width;
     break;
   case 2:
     s.writedata( s.colorOrder | 0x80);
     s._width = s._init_width;
     s._height = s._init_height;
    break;
   case 3:
     s.writedata(s.colorOrder | 0x20 | 0x40 | 0x80);
     s._width = s._init_height;
     s._height = s._init_width;
     break;
   case 4:
     s.writedata(s.colorOrder | 0x40 | 0x80);
     s._width = s._init_width;
     s._height = s._init_height;
     break;
   case 5:
     s.writedata(s.colorOrder | 0x20 | 0x40);
     s._width = s._init_height;
     s._height = s._init_width;
     break;
   case 6:
     s.writedata( s.colorOrder);
     s._width = s._init_width;
     s._height = s._init_height;
     break;
   case 7:
     s.writedata(s.colorOrder | 0x20 | 0x80);
     s._width = s._init_height;
     s._height = s._init_width;
     break;
  }
}
inline ControllerProfile profile_RPI_ILI9486() {
  return {"RPI_ILI9486",320,480,0x9486,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,8,0x08,0x00,0x08,init_RPI_ILI9486,rotate_RPI_ILI9486,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class RPI_ILI9486 : public Panel {
 public:
  static constexpr bool paddedCommands = true;
  static constexpr uint8_t spiMode = 0;
  explicit RPI_ILI9486(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_RPI_ILI9486(), reset, options) {}
};
} // namespace nice_gfx
