#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_SSD1351(PanelSequence &s) {
{
  s.writecommand(0xFD);
  s.writedata(0x12);
  s.writecommand(0xFD);
  s.writedata(0xB1);
  s.writecommand(0xAE);
  s.writecommand(0xB3);
  s.writedata(0xF1);
  s.writecommand(0xCA);
  s.writedata(127);
  s.writecommand(0xA2);
  s.writedata(0x00);
  s.writecommand(0xB5);
  s.writedata(0x00);
  s.writecommand(0xAB);
  s.writedata(0x01);
  s.writecommand(0xB1);
  s.writedata(0x32);
  s.writecommand(0xBE);
  s.writedata(0x05);
  s.writecommand(0xA6);
  s.writecommand(0xC1);
  s.writedata(0xC8);
  s.writedata(0x80);
  s.writedata(0xC8);
  s.writecommand(0xC7);
  s.writedata(0x0F);
  s.writecommand(0xB4);
  s.writedata(0xA0);
  s.writedata(0xB5);
  s.writedata(0x55);
  s.writecommand(0xB6);
  s.writedata(0x01);
  s.writecommand(0xAF);
}
}
inline void rotate_SSD1351(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  uint8_t madctl = uint8_t(0x60 | s.colorOrder);
  switch (s.rotation) {
    case 0:
      madctl |= 0x10;
      s._width = s._init_width;
      s._height = s._init_height;
      break;
    case 1:
      madctl |= 0x13;
      s._width = s._init_height;
      s._height = s._init_width;
      break;
    case 2:
      madctl |= 0x02;
      s._width = s._init_width;
      s._height = s._init_height;
      break;
    case 3:
      madctl |= 0x01;
      s._width = s._init_height;
      s._height = s._init_width;
      break;
  }
  s.writecommand(0xA0);
  s.writedata(madctl);
  s.writecommand(0xA1);
  s.writedata(s.rotation < 2 ? s._init_height : 0);
}
inline ControllerProfile profile_SSD1351() {
  return {"SSD1351",128,128,0x1351,Protocol::Ssd1351,PixelFormat::RGB565,
    0x5C,0x5D,0x00,4,4,0,4,init_SSD1351,rotate_SSD1351,PixelReadFormat::RGB565,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class SSD1351 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit SSD1351(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_SSD1351(), reset, options) {}
};
} // namespace nice_gfx
