#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_HX8357B(PanelSequence &s) {
s.writecommand(0x11);
    s.delay(20);
    s.writecommand(0xD0);
    s.writedata(0x07);
    s.writedata(0x42);
    s.writedata(0x18);
    s.writecommand(0xD1);
    s.writedata(0x00);
    s.writedata(0x07);
    s.writedata(0x10);
    s.writecommand(0xD2);
    s.writedata(0x01);
    s.writedata(0x02);
    s.writecommand(0xC0);
    s.writedata(0x10);
    s.writedata(0x3B);
    s.writedata(0x00);
    s.writedata(0x02);
    s.writedata(0x11);
    s.writecommand(0xC5);
    s.writedata(0x08);
    s.writecommand(0xC8);
    s.writedata(0x00);
    s.writedata(0x32);
    s.writedata(0x36);
    s.writedata(0x45);
    s.writedata(0x06);
    s.writedata(0x16);
    s.writedata(0x37);
    s.writedata(0x75);
    s.writedata(0x77);
    s.writedata(0x54);
    s.writedata(0x0C);
    s.writedata(0x00);
    s.writecommand(0x36);
    s.writedata(0x0a);
    s.writecommand(0x3A);
    s.writedata(0x55);
    s.writecommand(0x2A);
    s.writedata(0x00);
    s.writedata(0x00);
    s.writedata(0x01);
    s.writedata(0x3F);
    s.writecommand(0x2B);
    s.writedata(0x00);
    s.writedata(0x00);
    s.writedata(0x01);
    s.writedata(0xDF);
    s.delay(120);
    s.writecommand(0x29);
    s.delay(25);
}
inline void rotate_HX8357B(PanelSequence &s, uint8_t m) {
s.writecommand(0x36);
  s.rotation = m % 8;
  switch (s.rotation) {
   case 0:
     s.writedata(s.colorOrder | 0x40);
     s._width  = s._init_width;
     s._height = s._init_height;
     break;
   case 1:
     s.writedata(s.colorOrder | 0x20);
     s._width  = s._init_height;
     s._height = s._init_width;
     break;
   case 2:
     s.writedata( s.colorOrder | 0x80);
     s._width  = s._init_width;
     s._height = s._init_height;
    break;
   case 3:
     s.writedata(s.colorOrder | 0x20 | 0x40 | 0x80);
     s._width  = s._init_height;
     s._height = s._init_width;
     break;
   case 4:
     s.writedata(s.colorOrder | 0x40 | 0x80);
     s._width  = s._init_width;
     s._height = s._init_height;
     break;
   case 5:
     s.writedata(s.colorOrder | 0x20 | 0x40);
     s._width  = s._init_height;
     s._height = s._init_width;
     break;
   case 6:
     s.writedata( s.colorOrder);
     s._width  = s._init_width;
     s._height = s._init_height;
     break;
   case 7:
     s.writedata(s.colorOrder | 0x20 | 0x80);
     s._width  = s._init_height;
     s._height = s._init_width;
     break;
  }
}
inline ControllerProfile profile_HX8357B() {
  return {"HX8357B",320,480,0x8357B,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,8,0x08,0x00,0x08,init_HX8357B,rotate_HX8357B,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class HX8357B : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit HX8357B(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_HX8357B(), reset, options) {}
};
} // namespace nice_gfx
