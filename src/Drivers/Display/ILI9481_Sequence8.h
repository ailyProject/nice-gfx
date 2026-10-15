#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_ILI9481_Sequence8(PanelSequence &s) {
s.writecommand(0x01);
     s.delay(220);
     s.writecommand(0x11);
     s.delay(280);
     s.writecommand(0xd0);
     s.writedata(0x07);
     s.writedata(0x44);
     s.writedata(0x1E);
     s.delay(220);
     s.writecommand(0xd1);
     s.writedata(0x00);
     s.writedata(0x0C);
     s.writedata(0x1A);
     s.writecommand(0xC5);
     s.writedata(0x03);
     s.writecommand(0xd2);
     s.writedata(0x01);
     s.writedata(0x11);
     s.writecommand(0xE4);
     s.writedata(0xa0);
     s.writecommand(0xf3);
     s.writedata(0x00);
     s.writedata(0x2a);
     s.writecommand(0xc8);
     s.writedata(0x00);
     s.writedata(0x26);
     s.writedata(0x21);
     s.writedata(0x00);
     s.writedata(0x00);
     s.writedata(0x1f);
     s.writedata(0x65);
     s.writedata(0x23);
     s.writedata(0x77);
     s.writedata(0x00);
     s.writedata(0x0f);
     s.writedata(0x00);
     s.writecommand(0xC0);
     s.writedata(0x00);
     s.writedata(0x3B);
     s.writedata(0x00);
     s.writedata(0x02);
     s.writedata(0x11);
     s.writecommand(0xc6);
     s.writedata(0x83);
     s.writecommand(0xf0);
     s.writedata(0x01);
     s.writecommand(0xE4);
     s.writedata(0xa0);
     s.writecommand(0x36);
     s.writedata(0x0A);
     s.writecommand(0x3a);
          s.writedata(0x66);
     s.writecommand(0xb4);
     s.writedata(0x02);
     s.writedata(0x00);
     s.writedata(0x00);
     s.writedata(0x01);
     s.delay(280);
     s.writecommand(0x2a);
     s.writedata(0x00);
     s.writedata(0x00);
     s.writedata(0x01);
     s.writedata(0x3F);
     s.writecommand(0x2b);
     s.writedata(0x00);
     s.writedata(0x00);
     s.writedata(0x01);
     s.writedata(0xDf);
     s.writecommand(0x29);
     s.writecommand(0x2c);
}
inline void rotate_ILI9481_Sequence8(PanelSequence &s, uint8_t m) {
s.writecommand(0x36);
  s.rotation = m % 4;
  switch (s.rotation) {
   case 0:
     s.writedata(s.colorOrder | 0x02);
      s._width = s._init_width;
      s._height = s._init_height;
     break;
   case 1:
     s.writedata(uint8_t(0x20 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
     break;
   case 2:
     s.writedata(s.colorOrder | 0x01);
      s._width = s._init_width;
      s._height = s._init_height;
     break;
   case 3:
     s.writedata(uint8_t(0x20 | s.colorOrder) | 0x02 | 0x01);
      s._width = s._init_height;
      s._height = s._init_width;
     break;
  }
}
inline ControllerProfile profile_ILI9481_Sequence8() {
  return {"ILI9481_Sequence8",320,480,0x9481,Protocol::Dcs,PixelFormat::RGB666,
    0x2C,0x2E,0x01,4,0x08,0x00,0x08,init_ILI9481_Sequence8,rotate_ILI9481_Sequence8,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class ILI9481_Sequence8 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit ILI9481_Sequence8(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ILI9481_Sequence8(), reset, options) {}
};
} // namespace nice_gfx
