#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_SSD1331(PanelSequence &s) {
{
  s.writecommand(0xAE);
  s.writecommand(0x81);
  s.writecommand(0x91);
  s.writecommand(0x82);
  s.writecommand(0x50);
  s.writecommand(0x83);
  s.writecommand(0x7D);
  s.writecommand(0x87);
  s.writecommand(0x06);
  s.writecommand(0x8A);
  s.writecommand(0x64);
  s.writecommand(0x8B);
  s.writecommand(0x78);
  s.writecommand(0x8C);
  s.writecommand(0x64);
  s.writecommand(0xA0);
  s.writecommand(uint8_t(0x72 | s.colorOrder));
  s.writecommand(0xA1);
  s.writecommand(0x00);
  s.writecommand(0xA2);
  s.writecommand(0x00);
  s.writecommand(0xA4);
  s.writecommand(0xA8);
  s.writecommand(0x3F);
  s.writecommand(0xAD);
  s.writecommand(0x8E);
  s.writecommand(0xB0);
  s.writecommand(0x0B);
  s.writecommand(0xB1);
  s.writecommand(0x31);
  s.writecommand(0xB3);
  s.writecommand(0xF0);
  s.writecommand(0xBB);
  s.writecommand(0x3A);
  s.writecommand(0xBE);
  s.writecommand(0x3E);
  s.writecommand(0xAF);
}
}
inline void rotate_SSD1331(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  uint8_t remap;
  switch (s.rotation) {
    case 0:
      remap = 0x72;
      s._width  = s._init_width;
      s._height = s._init_height;
      break;
    case 1:
      remap = 0x71;
      s._width  = s._init_height;
      s._height = s._init_width;
      break;
    case 2:
      remap = 0x60;
      s._width  = s._init_width;
      s._height = s._init_height;
      break;
    default:
      remap = 0x63;
      s._width  = s._init_height;
      s._height = s._init_width;
      break;
  }
  s.writecommand(0xA0);
  s.writecommand(remap | s.colorOrder);
}
inline ControllerProfile profile_SSD1331() {
  return {"SSD1331",96,64,0x1331,Protocol::Ssd1331,PixelFormat::RGB565,
    0x00,0x00,0x00,4,0,0,4,init_SSD1331,rotate_SSD1331,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class SSD1331 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit SSD1331(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_SSD1331(), reset, options) {}
};
} // namespace nice_gfx
