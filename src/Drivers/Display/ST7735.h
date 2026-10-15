#pragma once
#include "../../Display/ControllerProfile.h"

  // Frame control init for RobotLCD, taken from https://github.com/arduino-libraries/TFT, Adafruit_ST7735.cpp l. 263, commit 61b8a7e

namespace nice_gfx { namespace controllers {
inline void init_ST7735(PanelSequence &s) {
{
  const uint8_t
  Bcmd[] = {
    18,
    0x01,   0x80,
      50,
    0x11 ,   0x80,
      255,
    0x3A , 1+0x80,
      0x05,
      10,
    0xB1, 3+0x80,
      0x00,
      0x06,
      0x03,
      10,
    0x36 , 1      ,
      uint8_t(0x40 | s.colorOrder),
    0xB6, 2      ,
      0x15,
      0x02,
    0xB4 , 1      ,
      0x0,
    0xC0 , 2+0x80,
      0x02,
      0x70,
      10,
    0xC1 , 1      ,
      0x05,
    0xC2 , 2      ,
      0x01,
      0x02,
    0xC5 , 2+0x80,
      0x3C,
      0x38,
      10,
    0xFC , 2      ,
      0x11, 0x15,
    0xE0,16      ,
      0x09, 0x16, 0x09, 0x20,
      0x21, 0x1B, 0x13, 0x19,
      0x17, 0x15, 0x1E, 0x2B,
      0x04, 0x05, 0x02, 0x0E,
    0xE1,16+0x80,
      0x0B, 0x14, 0x08, 0x1E,
      0x22, 0x1D, 0x18, 0x1E,
      0x1B, 0x1A, 0x24, 0x2B,
      0x06, 0x06, 0x02, 0x0F,
      10,
    0x2A  , 4      ,
      0x00, 0x02,
      0x00, 0x81,
    0x2B  , 4      ,
      0x00, 0x02,
      0x00, 0x81,
    0x13  ,   0x80,
      10,
    0x29 ,   0x80,
      255 },
  Rcmd1[] = {
    15,
    0x01,   0x80,
      150,
    0x11 ,   0x80,
      255,
    0xB1, 3      ,
      0x01, 0x2C, 0x2D,
    0xB2, 3      ,
      0x01, 0x2C, 0x2D,
    0xB3, 6      ,
      0x01, 0x2C, 0x2D,
      0x01, 0x2C, 0x2D,
    0xB4 , 1      ,
      0x07,
    0xC0 , 3      ,
      0xA2,
      0x02,
      0x84,
    0xC1 , 1      ,
      0xC5,
    0xC2 , 2      ,
      0x0A,
      0x00,
    0xC3 , 2      ,
      0x8A,
      0x2A,
    0xC4 , 2      ,
      0x8A, 0xEE,
    0xC5 , 1      ,
      0x0E,
    0x20 , 0      ,
    0x36 , 1      ,
      uint8_t(0xC0 | s.colorOrder),
    0x3A , 1      ,
      0x05 },
  Rcmd2green[] = {
    2,
    0x2A  , 4      ,
      0x00, 0x02,
      0x00, 0x7F+0x02,
    0x2B  , 4      ,
      0x00, 0x01,
      0x00, 0x9F+0x01 },
  Rcmd2red[] = {
    2,
    0x2A  , 4      ,
      0x00, 0x00,
      0x00, 0x7F,
    0x2B  , 4      ,
      0x00, 0x00,
      0x00, 0x9F },
  Rcmd3RobotLCD[] = {
      3,
      0xB1, 2    ,
        0x0B, 0x14,
      0xB2, 2    ,
        0x0B, 0x14,
      0xB3, 4    ,
        0x0B, 0x14,
        0x0B, 0x14 },
  Rcmd3[] = {
    4,
    0xE0, 16      ,
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    0xE1, 16      ,
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2D,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    0x13  ,    0x80,
      10,
    0x29 ,    0x80,
      100 };
     if (s.tabcolor == 0xB)
     {
       s.commandList(Bcmd);
     }
     else
     {
	     s.commandList(Rcmd1);
       if (s.tabcolor == 0x0)
       {
         s.commandList(Rcmd2green);
         s.colstart = 2;
         s.rowstart = 1;
       }
       else if (s.tabcolor == 0x3)
       {
         s.commandList(Rcmd2green);
         s.writecommand(0x36);
         s.writedata(uint8_t(0xC0 | s.colorOrder));
         s.colstart = 2;
         s.rowstart = 1;
       }
       else if (s.tabcolor == 0x4)
       {
         s.commandList(Rcmd2green);
         s.colstart = 2;
         s.rowstart = 3;
       }
       else if (s.tabcolor == 0x5)
       {
         s.commandList(Rcmd2green);
         s.colstart = 0;
         s.rowstart = 32;
       }
       else if (s.tabcolor == 0x6)
       {
         s.commandList(Rcmd2green);
         s.writecommand(0x21);
         s.colstart = 26;
         s.rowstart = 1;
       }
       else if (s.tabcolor == 0x8)
       {
         s.commandList(Rcmd2green);
         s.commandList(Rcmd3RobotLCD);
       }
       else if (s.tabcolor == 0x7)
       {
         s.commandList(Rcmd2green);
         s.colstart = 24;
         s.rowstart = 0;
       }
       else if (s.tabcolor == 0x1)
       {
         s.commandList(Rcmd2red);
       }
       else if (s.tabcolor == 0x2)
       {
         s.writecommand(0x36);
         s.writedata(uint8_t(0xC0 | s.colorOrder));
       }
       s.commandList(Rcmd3);
     }
}
}
inline void rotate_ST7735(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  s.writecommand(0x36);
  switch (s.rotation) {
    case 0:
     if (s.tabcolor == 0x2) {
       s.writedata(0x40 | uint8_t(0x80 | s.colorOrder));
     } else if(s.tabcolor == 0x3) {
       s.writedata(0x40 | uint8_t(0x80 | s.colorOrder));
       s.colstart = 2;
       s.rowstart = 1;
     } else if(s.tabcolor == 0x4) {
       s.writedata(0x40 | uint8_t(0x80 | s.colorOrder));
       s.colstart = 2;
       s.rowstart = 3;
     } else if(s.tabcolor == 0x5) {
       s.writedata(0x40 | 0x80 | uint8_t(0x04 | s.colorOrder));
       s.colstart = 0;
       s.rowstart = 32;
     } else if(s.tabcolor == 0x6) {
       s.writedata(0x40 | 0x80 | uint8_t(0x04 | s.colorOrder));
       s.colstart = 26;
       s.rowstart = 1;
     } else if(s.tabcolor == 0x7) {
       s.writedata(0x40 | 0x80 | uint8_t(0x04 | s.colorOrder));
       s.colstart = 24;
       s.rowstart = 0;
     } else if(s.tabcolor == 0xB) {
       s.writedata(uint8_t(0x40 | s.colorOrder));
     } else {
       s.writedata(0x40 | uint8_t(0x80 | s.colorOrder));
     }
      s._width  = s._init_width;
      s._height = s._init_height;
      break;
    case 1:
     if (s.tabcolor == 0x2) {
       s.writedata(0x80 | uint8_t(0x20 | s.colorOrder));
     } else if(s.tabcolor == 0x3) {
       s.writedata(0x80 | uint8_t(0x20 | s.colorOrder));
       s.colstart = 1;
       s.rowstart = 2;
     } else if(s.tabcolor == 0x4) {
       s.writedata(0x80 | uint8_t(0x20 | s.colorOrder));
       s.colstart = 3;
       s.rowstart = 2;
     } else if(s.tabcolor == 0x5) {
       s.writedata(0x20 | uint8_t(0x80 | s.colorOrder));
       s.colstart = 32;
       s.rowstart = 0;
     } else if(s.tabcolor == 0x6) {
       s.writedata(0x20 | uint8_t(0x80 | s.colorOrder));
       s.colstart = 1;
       s.rowstart = 26;
     } else if(s.tabcolor == 0x7) {
       s.writedata(0x20 | uint8_t(0x80 | s.colorOrder));
       s.colstart = 0;
       s.rowstart = 24;
     } else if(s.tabcolor == 0xB) {
       s.writedata(0x20 | 0x40 | uint8_t(0x80 | s.colorOrder));
     } else {
       s.writedata(0x80 | uint8_t(0x20 | s.colorOrder));
     }
      s._width  = s._init_height;
      s._height = s._init_width;
      break;
    case 2:
     if (s.tabcolor == 0x2) {
       s.writedata(s.colorOrder);
     } else if(s.tabcolor == 0x3) {
       s.writedata(s.colorOrder);
       s.colstart = 2;
       s.rowstart = 1;
     } else if(s.tabcolor == 0x4) {
       s.writedata(s.colorOrder);
       s.colstart = 2;
       s.rowstart = 1;
     } else if(s.tabcolor == 0x5) {
       s.writedata(s.colorOrder);
       s.colstart = 0;
       s.rowstart = 0;
     } else if(s.tabcolor == 0x6) {
       s.writedata(s.colorOrder);
       s.colstart = 26;
       s.rowstart = 1;
     } else if(s.tabcolor == 0x7) {
       s.writedata(s.colorOrder);
       s.colstart = 24;
       s.rowstart = 0;
     } else if(s.tabcolor == 0xB) {
       s.writedata(uint8_t(0x80 | s.colorOrder));
     } else {
       s.writedata(s.colorOrder);
     }
      s._width  = s._init_width;
      s._height = s._init_height;
      break;
    case 3:
     if (s.tabcolor == 0x2) {
       s.writedata(0x40 | uint8_t(0x20 | s.colorOrder));
     } else if(s.tabcolor == 0x3) {
       s.writedata(0x40 | uint8_t(0x20 | s.colorOrder));
       s.colstart = 1;
       s.rowstart = 2;
     } else if(s.tabcolor == 0x4) {
       s.writedata(0x40 | uint8_t(0x20 | s.colorOrder));
       s.colstart = 1;
       s.rowstart = 2;
     } else if(s.tabcolor == 0x5) {
       s.writedata(0x40 | uint8_t(0x20 | s.colorOrder));
       s.colstart = 0;
       s.rowstart = 0;
     } else if(s.tabcolor == 0x6) {
       s.writedata(0x40 | uint8_t(0x20 | s.colorOrder));
       s.colstart = 1;
       s.rowstart = 26;
     } else if(s.tabcolor == 0x7) {
       s.writedata(0x40 | uint8_t(0x20 | s.colorOrder));
       s.colstart = 0;
       s.rowstart = 24;
     } else if(s.tabcolor == 0xB) {
       s.writedata(uint8_t(0x20 | s.colorOrder));
     } else {
       s.writedata(0x40 | uint8_t(0x20 | s.colorOrder));
     }
      s._width  = s._init_height;
      s._height = s._init_width;
      break;
  }
}
inline ControllerProfile profile_ST7735() {
  return {"ST7735",128,160,0x7735,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0x08,0x00,0x08,init_ST7735,rotate_ST7735,PixelReadFormat::RGB777,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class ST7735 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit ST7735(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ST7735(), reset, options) {}
};
} // namespace nice_gfx
