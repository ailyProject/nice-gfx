#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_HX8357C(PanelSequence &s) {
s.writecommand(0xB9);
    s.writedata(0xFF);
    s.writedata(0x83);
    s.writedata(0x57);
    s.delay(50);
    s.writecommand(0xB6);
    s.writedata(0x2C);
    s.writecommand(0x11);
    s.delay(200);
    s.writecommand(0x35);
    s.writedata(0x00);
    s.writecommand(0x3A);
    s.writedata(0x55);
    s.writecommand(0xB1);
    s.writedata(0x00);
    s.writedata(0x15);
    s.writedata(0x0D);
    s.writedata(0x0D);
    s.writedata(0x83);
    s.writedata(0x48);
    s.writecommand(0xC0);
    s.writedata(0x24);
    s.writedata(0x24);
    s.writedata(0x01);
    s.writedata(0x3C);
    s.writedata(0xC8);
    s.writedata(0x08);
    s.writecommand(0xB4);
    s.writedata(0x02);
    s.writedata(0x40);
    s.writedata(0x00);
    s.writedata(0x2A);
    s.writedata(0x2A);
    s.writedata(0x0D);
    s.writedata(0x4F);
    s.writecommand(0xE0);
    s.writedata(0x00);
    s.writedata(0x15);
    s.writedata(0x1D);
    s.writedata(0x2A);
    s.writedata(0x31);
    s.writedata(0x42);
    s.writedata(0x4C);
    s.writedata(0x53);
    s.writedata(0x45);
    s.writedata(0x40);
    s.writedata(0x3B);
    s.writedata(0x32);
    s.writedata(0x2E);
    s.writedata(0x28);
    s.writedata(0x24);
    s.writedata(0x03);
    s.writedata(0x00);
    s.writedata(0x15);
    s.writedata(0x1D);
    s.writedata(0x2A);
    s.writedata(0x31);
    s.writedata(0x42);
    s.writedata(0x4C);
    s.writedata(0x53);
    s.writedata(0x45);
    s.writedata(0x40);
    s.writedata(0x3B);
    s.writedata(0x32);
    s.writedata(0x2E);
    s.writedata(0x28);
    s.writedata(0x24);
    s.writedata(0x03);
    s.writedata(0x00);
    s.writedata(0x01);
    s.writecommand(0x36);
    s.writedata(0x48);
    s.delay(20);
    s.writecommand(0x21);
    s.delay(20);
    s.writecommand(0x29);
    s.delay(120);
}
inline void rotate_HX8357C(PanelSequence &s, uint8_t m) {
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
inline ControllerProfile profile_HX8357C() {
  return {"HX8357C",320,480,0x8357C,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,8,0x08,0x00,0x08,init_HX8357C,rotate_HX8357C,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class HX8357C : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit HX8357C(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_HX8357C(), reset, options) {}
};
} // namespace nice_gfx
