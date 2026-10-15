#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_ST7789_2(PanelSequence &s) {
{
const uint8_t
  st7789[] = {
    8,
    0x11, 0x80, 255,
    0x3A, 1+0x80, 0x55, 10,
    0x36, 1, 0x00,
    0x2A, 4, 0x00, 0x00, 0x00, 0xF0,
    0x2B, 4, 0x00, 0x00, 0x00, 0xF0,
    0x21, 0x80, 10,
    0x13, 0x80, 10,
    0x29, 0x80, 255
    };
    s.commandList(st7789);
}
}
inline void rotate_ST7789_2(PanelSequence &s, uint8_t m) {
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
inline ControllerProfile profile_ST7789_2() {
  return {"ST7789_2",240,320,0x77892,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0x08,0x00,0x08,init_ST7789_2,rotate_ST7789_2,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class ST7789_2 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit ST7789_2(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ST7789_2(), reset, options) {}
};
} // namespace nice_gfx
