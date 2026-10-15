#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_ILI9225(PanelSequence &s) {
{
	s.writecommand(0x10);
	s.writedata(0x00);s.writedata(0x00);
	s.writecommand(0x11);
	s.writedata(0x00);s.writedata(0x00);
	s.writecommand(0x12);
	s.writedata(0x00);s.writedata(0x00);
	s.writecommand(0x13);
	s.writedata(0x00);s.writedata(0x00);
	s.writecommand(0x14);
	s.writedata(0x00);s.writedata(0x00);
	s.delay(40);
	s.writecommand(0x11);
	s.writedata(0x00);s.writedata(0x18);
	s.writecommand(0x12);
	s.writedata(0x61);s.writedata(0x21);
	s.writecommand(0x13);
	s.writedata(0x00);s.writedata(0x6F);
	s.writecommand(0x14);
	s.writedata(0x49);s.writedata(0x5F);
	s.writecommand(0x10);
	s.writedata(0x08);s.writedata(0x00);
	s.delay(10);
	s.writecommand(0x11);
	s.writedata(0x10);s.writedata(0x3B);
	s.delay(50);
	s.writecommand(0x02);
	s.writedata(0x01);s.writedata(0x00);
	s.writecommand(0x07);
	s.writedata(0x00);s.writedata(0x00);
	s.writecommand(0x08);
	s.writedata(0x08);s.writedata(0x08);
	s.writecommand(0x0B);
	s.writedata(0x11);s.writedata(0x00);
	s.writecommand(0x0C);
	s.writedata(0x00);s.writedata(0x00);
	s.writecommand(0x0F);
	s.writedata(0x0D);s.writedata(0x01);
	s.writecommand(0x15);
	s.writedata(0x00);s.writedata(0x20);
	s.writecommand(0x20);
	s.writedata(0x00);s.writedata(0x00);
	s.writecommand(0x21);
	s.writedata(0x00);s.writedata(0x00);
	s.writecommand(0x30);
	s.writedata(0x00);s.writedata(0x00);
	s.writecommand(0x31);
	s.writedata(0x00);s.writedata(0xDB);
	s.writecommand(0x32);
	s.writedata(0x00);s.writedata(0x00);
	s.writecommand(0x33);
	s.writedata(0x00);s.writedata(0x00);
	s.writecommand(0x34);
	s.writedata(0x00);s.writedata(0xDB);
	s.writecommand(0x35);
	s.writedata(0x00);s.writedata(0x00);
	s.writecommand(0x36);
	s.writedata(0x00);s.writedata(0xAF);
	s.writecommand(0x37);
	s.writedata(0x00);s.writedata(0x00);
	s.writecommand(0x38);
	s.writedata(0x00);s.writedata(0xDB);
	s.writecommand(0x39);
	s.writedata(0x00);s.writedata(0x00);
	s.writecommand(0x50);
	s.writedata(0x00);s.writedata(0x00);
	s.writecommand(0x51);
	s.writedata(0x08);s.writedata(0x08);
	s.writecommand(0x52);
	s.writedata(0x08);s.writedata(0x0A);
	s.writecommand(0x53);
	s.writedata(0x00);s.writedata(0x0A);
	s.writecommand(0x54);
	s.writedata(0x0A);s.writedata(0x08);
	s.writecommand(0x55);
	s.writedata(0x08);s.writedata(0x08);
	s.writecommand(0x56);
	s.writedata(0x00);s.writedata(0x00);
	s.writecommand(0x57);
	s.writedata(0x0A);s.writedata(0x00);
	s.writecommand(0x58);
	s.writedata(0x07);s.writedata(0x10);
	s.writecommand(0x59);
	s.writedata(0x07);s.writedata(0x10);
	s.writecommand(0x07);
	s.writedata(0x00);s.writedata(0x12);
	s.delay(50);
	s.writecommand(0x07);
	s.writedata(0x10);s.writedata(0x17);
}
}
inline void rotate_ILI9225(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  switch (s.rotation) {
    case 0:
    	s.writecommand(0x01);
	    s.writedata(0x01);s.writedata(0x1C);
      s.writecommand(0x03);
    	s.writedata(s.colorOrder);s.writedata(0x30);
      s._width  = s._init_width;
      s._height = s._init_height;
      break;
    case 1:
    	s.writecommand(0x01);
	    s.writedata(0x00);s.writedata(0x1C);
      s.writecommand(0x03);
    	s.writedata(s.colorOrder);s.writedata(0x38);
      s._width  = s._init_height;
      s._height = s._init_width;
      break;
    case 2:
    	s.writecommand(0x01);
	    s.writedata(0x02);s.writedata(0x1C);
      s.writecommand(0x03);
    	s.writedata(s.colorOrder);s.writedata(0x30);
      s._width  = s._init_width;
      s._height = s._init_height;
      break;
    case 3:
    	s.writecommand(0x01);
	    s.writedata(0x03);s.writedata(0x1C);
      s.writecommand(0x03);
    	s.writedata(s.colorOrder);s.writedata(0x38);
      s._width  = s._init_height;
      s._height = s._init_width;
      break;
  }
}
inline ControllerProfile profile_ILI9225() {
  return {"ILI9225",176,220,0x9225,Protocol::Ili9225,PixelFormat::RGB565,
    0x22,0x00,0x28,4,0x10,0x00,0x10,init_ILI9225,rotate_ILI9225,PixelReadFormat::RGB565,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class ILI9225 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit ILI9225(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ILI9225(), reset, options) {}
};
} // namespace nice_gfx
