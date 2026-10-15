#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_ST7789_Sequence3(PanelSequence &s) {
{
  s.writecommand(0x11);
  s.delay(120);
  s.writecommand(0x13);
  s.writecommand(0x36);
  s.writedata(s.colorOrder);
  s.writecommand(0x3A);
  s.writedata(0x55);
  s.delay(10);
  s.writecommand(0xB2);
  s.writedata(0x0b);
  s.writedata(0x0b);
  s.writedata(0x00);
  s.writedata(0x33);
  s.writedata(0x33);
  s.writecommand(0xB7);
  s.writedata(0x75);
  s.writecommand(0xBB);
  s.writedata(0x28);
  s.writecommand(0xC0);
  s.writedata(0x2C);
  s.writecommand(0xC2);
  s.writedata(0x01);
  s.writecommand(0xC3);
  s.writedata(0x1F);
  s.writecommand(0xC6);
  s.writedata(0x13);
  s.writecommand(0xD0);
  s.writedata(0xa7);
  s.writecommand(0xD0);
  s.writedata(0xa4);
  s.writedata(0xa1);
  s.writecommand(0xD6);
  s.writedata(0xa1);
  s.writecommand(0xE0);
  s.writedata(0xf0);
  s.writedata(0x05);
  s.writedata(0x0a);
  s.writedata(0x06);
  s.writedata(0x06);
  s.writedata(0x03);
  s.writedata(0x2b);
  s.writedata(0x32);
  s.writedata(0x43);
  s.writedata(0x36);
  s.writedata(0x11);
  s.writedata(0x10);
  s.writedata(0x2b);
  s.writedata(0x32);
  s.writecommand(0xE1);
  s.writedata(0xf0);
  s.writedata(0x08);
  s.writedata(0x0c);
  s.writedata(0x0b);
  s.writedata(0x09);
  s.writedata(0x24);
  s.writedata(0x2b);
  s.writedata(0x22);
  s.writedata(0x43);
  s.writedata(0x38);
  s.writedata(0x15);
  s.writedata(0x16);
  s.writedata(0x2f);
  s.writedata(0x37);
  s.endPanelWrite();
  s.delay(120);
  s.beginPanelWrite();
  s.writecommand(0x29);
  s.delay(120);
}
}
inline void rotate_ST7789_Sequence3(PanelSequence &s, uint8_t m) {
s.writecommand(0x36);
  s.rotation = m % 4;
  switch (s.rotation) {
    case 0:
      if (s._init_width == 135)
      {
        s.colstart = 52;
        s.rowstart = 40;
      }
      else if(s._init_height == 280)
      {
        s.colstart = 0;
        s.rowstart = 20;
      }
      else if(s._init_width == 172)
      {
        s.colstart = 34;
        s.rowstart = 0;
      }
      else if(s._init_width == 170)
      {
        s.colstart = 35;
        s.rowstart = 0;
      }
      else
      {
        s.colstart = 0;
        s.rowstart = 0;
      }
      s.writedata(s.colorOrder);
      s._width  = s._init_width;
      s._height = s._init_height;
      break;
    case 1:
      if (s._init_width == 135)
      {
        s.colstart = 40;
        s.rowstart = 53;
      }
      else if(s._init_height == 280)
      {
        s.colstart = 20;
        s.rowstart = 0;
      }
      else if(s._init_width == 172)
      {
        s.colstart = 0;
        s.rowstart = 34;
      }
      else if(s._init_width == 170)
      {
        s.colstart = 0;
        s.rowstart = 35;
      }
      else
      {
        s.colstart = 0;
        s.rowstart = 0;
      }
      s.writedata(0x40 | uint8_t(0x20 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
      break;
      case 2:
      if (s._init_width == 135)
      {
        s.colstart = 53;
        s.rowstart = 40;
      }
      else if(s._init_height == 280)
      {
        s.colstart = 0;
        s.rowstart = 20;
      }
      else if(s._init_width == 172)
      {
        s.colstart = 34;
        s.rowstart = 0;
      }
      else if(s._init_width == 170)
      {
        s.colstart = 35;
        s.rowstart = 0;
      }
      else
      {
        s.colstart = 0;
        s.rowstart = 320 - s._init_height;
      }
      s.writedata(0x40 | uint8_t(0x80 | s.colorOrder));
      s._width  = s._init_width;
      s._height = s._init_height;
       break;
    case 3:
      if (s._init_width == 135)
      {
        s.colstart = 40;
        s.rowstart = 52;
      }
      else if(s._init_height == 280)
      {
        s.colstart = 20;
        s.rowstart = 0;
      }
      else if(s._init_width == 172)
      {
        s.colstart = 0;
        s.rowstart = 34;
      }
      else if(s._init_width == 170)
      {
        s.colstart = 0;
        s.rowstart = 35;
      }
      else
      {
        s.colstart = 320 - s._init_height;
        s.rowstart = 0;
      }
      s.writedata(0x20 | uint8_t(0x80 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
      break;
  }
}
inline ControllerProfile profile_ST7789_Sequence3() {
  return {"ST7789_Sequence3",240,320,0x7789,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0x08,0x00,0x08,init_ST7789_Sequence3,rotate_ST7789_Sequence3,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class ST7789_Sequence3 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit ST7789_Sequence3(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ST7789_Sequence3(), reset, options) {}
};
} // namespace nice_gfx
