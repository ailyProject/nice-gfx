#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_HX8357D(PanelSequence &s) {
s.writecommand(0xB9);
    s.writedata(0xFF);
    s.writedata(0x83);
    s.writedata(0x57);
    s.delay(300);
    s.writecommand(0xB3);
    s.writedata(0x80);
    s.writedata(0x0);
    s.writedata(0x06);
    s.writedata(0x06);
    s.writecommand(0xB6);
    s.writedata(0x25);
    s.writecommand(0xB0);
    s.writedata(0x68);
    s.writecommand(0xCC);
    s.writedata(0x05);
    s.writecommand(0xB1);
    s.writedata(0x00);
    s.writedata(0x15);
    s.writedata(0x1C);
    s.writedata(0x1C);
    s.writedata(0x83);
    s.writedata(0xAA);
    s.writecommand(0xC0);
    s.writedata(0x50);
    s.writedata(0x50);
    s.writedata(0x01);
    s.writedata(0x3C);
    s.writedata(0x1E);
    s.writedata(0x08);
    s.writecommand(0xB4);
    s.writedata(0x02);
    s.writedata(0x40);
    s.writedata(0x00);
    s.writedata(0x2A);
    s.writedata(0x2A);
    s.writedata(0x0D);
    s.writedata(0x78);
    s.writecommand(0xE0);
    s.writedata(0x02);
    s.writedata(0x0A);
    s.writedata(0x11);
    s.writedata(0x1d);
    s.writedata(0x23);
    s.writedata(0x35);
    s.writedata(0x41);
    s.writedata(0x4b);
    s.writedata(0x4b);
    s.writedata(0x42);
    s.writedata(0x3A);
    s.writedata(0x27);
    s.writedata(0x1B);
    s.writedata(0x08);
    s.writedata(0x09);
    s.writedata(0x03);
    s.writedata(0x02);
    s.writedata(0x0A);
    s.writedata(0x11);
    s.writedata(0x1d);
    s.writedata(0x23);
    s.writedata(0x35);
    s.writedata(0x41);
    s.writedata(0x4b);
    s.writedata(0x4b);
    s.writedata(0x42);
    s.writedata(0x3A);
    s.writedata(0x27);
    s.writedata(0x1B);
    s.writedata(0x08);
    s.writedata(0x09);
    s.writedata(0x03);
    s.writedata(0x00);
    s.writedata(0x01);
    s.writecommand(0x3A);
    s.writedata(0x55);
    s.writecommand(0x36);
    s.writedata(0x40 | uint8_t(0x80 | s.colorOrder));
    s.writecommand(0x35);
    s.writedata(0x00);
    s.writecommand(0x44);
    s.writedata(0x00);
    s.writedata(0x02);
    s.writecommand(0x11);
    s.delay(150);
    s.writecommand(0x29);
    s.delay(50);
}
inline void rotate_HX8357D(PanelSequence &s, uint8_t m) {
s.writecommand(0x36);
  s.rotation = m % 4;
  switch (s.rotation) {
   case 0:
     s.writedata(0x40 | uint8_t(0x80 | s.colorOrder));
      s._width  = s._init_width;
      s._height = s._init_height;
     break;
   case 1:
     s.writedata(0x20 | uint8_t(0x80 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
     break;
   case 2:
     s.writedata(s.colorOrder);
      s._width  = s._init_width;
      s._height = s._init_height;
     break;
   case 3:
     s.writedata(0x40 | uint8_t(0x20 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
     break;
  }
}
inline ControllerProfile profile_HX8357D() {
  return {"HX8357D",320,480,0x8357D,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0x00,0x00,0x08,init_HX8357D,rotate_HX8357D,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class HX8357D : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit HX8357D(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_HX8357D(), reset, options) {}
};
} // namespace nice_gfx
