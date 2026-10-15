#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_ILI9481_Sequence7_Parallel(PanelSequence &s) {
s.writecommand(0x11);
     s.delay(20);
     s.writecommand(0xD0);
     s.writedata(0x07);
     s.writedata(0x42);
     s.writedata(0x1B);
     s.writecommand(0xD1);
     s.writedata(0x00);
     s.writedata(0x14);
     s.writedata(0x1B);
     s.writecommand(0xD2);
     s.writedata(0x01);
     s.writedata(0x12);
     s.writecommand(0xC0);
     s.writedata(0x10);
     s.writedata(0x3B);
     s.writedata(0x00);
     s.writedata(0x02);
     s.writedata(0x01);
     s.writecommand(0xC5);
     s.writedata(0x03);
     s.writecommand(0xC8);
     s.writedata(0x00);
     s.writedata(0x46);
     s.writedata(0x44);
     s.writedata(0x50);
     s.writedata(0x04);
     s.writedata(0x16);
     s.writedata(0x33);
     s.writedata(0x13);
     s.writedata(0x77);
     s.writedata(0x05);
     s.writedata(0x0F);
     s.writedata(0x00);
     s.writecommand(0x36);
     s.writedata(0x0A);
     s.writecommand(0x3A);
          s.writedata(0x55);
     s.writecommand(0x22);
     s.writedata(0x00);
     s.writedata(0x00);
     s.writedata(0x01);
     s.writedata(0x3F);
     s.writecommand(0x2B);
     s.writedata(0x00);
     s.writedata(0x00);
     s.writedata(0x01);
     s.writedata(0xE0);
     s.delay(120);
     s.writecommand(0x29);
}
inline void rotate_ILI9481_Sequence7_Parallel(PanelSequence &s, uint8_t m) {
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
inline ControllerProfile profile_ILI9481_Sequence7_Parallel() {
  return {"ILI9481_Sequence7_Parallel",320,480,0x9481,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0x08,0x00,0x08,init_ILI9481_Sequence7_Parallel,rotate_ILI9481_Sequence7_Parallel,PixelReadFormat::BGR565,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class ILI9481_Sequence7_Parallel : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit ILI9481_Sequence7_Parallel(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ILI9481_Sequence7_Parallel(), reset, options) {}
};
} // namespace nice_gfx
