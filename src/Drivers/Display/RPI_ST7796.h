#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_RPI_ST7796(PanelSequence &s) {
{
	s.delay(120);
	s.writecommand(0x01);
	s.delay(120);
	s.writecommand(0x11);
	s.delay(120);
	s.writecommand(0xF0);
	s.writedata(0xC3);
	s.writecommand(0xF0);
	s.writedata(0x96);
	s.writecommand(0x36);
	s.writedata(0x48);
	s.writecommand(0x3A);
	s.writedata(0x55);
	s.writecommand(0xB4);
	s.writedata(0x01);
	s.writecommand(0xB6);
	s.writedata(0x80);
	s.writedata(0x02);
	s.writedata(0x3B);
	s.writecommand(0xE8);
	s.writedata(0x40);
	s.writedata(0x8A);
	s.writedata(0x00);
	s.writedata(0x00);
	s.writedata(0x29);
	s.writedata(0x19);
	s.writedata(0xA5);
	s.writedata(0x33);
	s.writecommand(0xC1);
	s.writedata(0x06);
	s.writecommand(0xC2);
	s.writedata(0xA7);
	s.writecommand(0xC5);
	s.writedata(0x18);
	s.delay(120);
	s.writecommand(0xE0);
	s.writedata(0xF0);
	s.writedata(0x09);
	s.writedata(0x0b);
	s.writedata(0x06);
	s.writedata(0x04);
	s.writedata(0x15);
	s.writedata(0x2F);
	s.writedata(0x54);
	s.writedata(0x42);
	s.writedata(0x3C);
	s.writedata(0x17);
	s.writedata(0x14);
	s.writedata(0x18);
	s.writedata(0x1B);
	s.writecommand(0xE1);
	s.writedata(0xE0);
	s.writedata(0x09);
	s.writedata(0x0B);
	s.writedata(0x06);
	s.writedata(0x04);
	s.writedata(0x03);
	s.writedata(0x2B);
	s.writedata(0x43);
	s.writedata(0x42);
	s.writedata(0x3B);
	s.writedata(0x16);
	s.writedata(0x14);
	s.writedata(0x17);
	s.writedata(0x1B);
  s.delay(120);
	s.writecommand(0xF0);
	s.writedata(0x3C);
	s.writecommand(0xF0);
	s.writedata(0x69);
  s.endPanelWrite();
  s.delay(120);
  s.beginPanelWrite();
	s.writecommand(0x29);
}
}
inline void rotate_RPI_ST7796(PanelSequence &s, uint8_t m) {
s.rotation = m % 8;
  s.writecommand(0x36);
  switch (s.rotation) {
    case 0:
      s.writedata(uint8_t(0x40 | s.colorOrder));
      s._width  = s._init_width;
      s._height = s._init_height;
      if (s._init_width == 222)
      {
        s.colstart = 49;
        s.rowstart = 0;
      }
      break;
    case 1:
      s.writedata(uint8_t(0x20 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
      if (s._init_width == 222)
      {
        s.colstart = 0;
        s.rowstart = 49;
      }
      break;
    case 2:
      s.writedata(uint8_t(0x80 | s.colorOrder));
      s._width  = s._init_width;
      s._height = s._init_height;
      if (s._init_width == 222)
      {
        s.colstart = 49;
        s.rowstart = 0;
      }
      break;
    case 3:
      s.writedata(0x40 | 0x80 | uint8_t(0x20 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
      if (s._init_width == 222)
      {
        s.colstart = 0;
        s.rowstart = 49;
      }
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
inline ControllerProfile profile_RPI_ST7796() {
  return {"RPI_ST7796",320,480,0x7796,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,8,0x08,0x00,0x08,init_RPI_ST7796,rotate_RPI_ST7796,PixelReadFormat::RGB565,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class RPI_ST7796 : public Panel {
 public:
  static constexpr bool paddedCommands = true;
  static constexpr uint8_t spiMode = 0;
  explicit RPI_ST7796(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_RPI_ST7796(), reset, options) {}
};
} // namespace nice_gfx
