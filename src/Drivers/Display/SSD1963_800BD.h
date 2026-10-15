#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_SSD1963_800BD(PanelSequence &s) {
s.writecommand(0xE2);
  s.writedata(0x23);
  s.writedata(0x02);
  s.writedata(0x54);
  s.writecommand(0xE0);
  s.writedata(0x01);
  s.delay(10);
  s.writecommand(0xE0);
  s.writedata(0x03);
  s.delay(10);
  s.writecommand(0x01);
  s.delay(100);
  s.writecommand(0xE6);
  s.writedata(0x03);
  s.writedata(0x33);
  s.writedata(0x33);
  s.writecommand(0xB0);
  s.writedata(0x20);
  s.writedata(0x00);
  s.writedata(799 >> 8);
  s.writedata(799 & 0xFF);
  s.writedata(479 >> 8);
  s.writedata(479 & 0xFF);
  s.writedata(0x00);
  s.writecommand(0xB4);
  s.writedata(0x04);
  s.writedata(0x1F);
  s.writedata(0x00);
  s.writedata(0xD2);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writecommand(0xB6);
  s.writedata(0x02);
  s.writedata(0x0C);
  s.writedata(0x00);
  s.writedata(0x22);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writecommand(0xB8);
  s.writedata(0x0F);
  s.writedata(0x01);
  s.writecommand(0xBA);
  s.writedata(0x01);
  s.writecommand(0x36);
  s.writedata(uint8_t(0x21 | s.colorOrder));
  s.writecommand(0xF0);
  s.writedata(0x00);
  s.writecommand(0xBC);
  s.writedata(0x40);
  s.writedata(0x80);
  s.writedata(0x40);
  s.writedata(0x01);
  s.delay(10);
  s.writecommand(0x29);
  s.writecommand(0xBE);
  s.writedata(0x06);
  s.writedata(0x80);
  s.writedata(0x01);
  s.writedata(0xF0);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writecommand(0xD0);
  s.writedata(0x0D);
}
inline void rotate_SSD1963_800BD(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  s.writecommand(0x36);
  switch (s.rotation) {
    case 0:
      s.writedata(uint8_t(0x21 | s.colorOrder));
      s._width  = s._init_width;
      s._height = s._init_height;
      break;
    case 1:
      s.writedata(uint8_t(0x00 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
      break;
    case 2:
      s.writedata(uint8_t(0x22 | s.colorOrder));
      s._width  = s._init_width;
      s._height = s._init_height;
      break;
    case 3:
      s.writedata(uint8_t(0x03 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
      break;
  }
}
inline ControllerProfile profile_SSD1963_800BD() {
  return {"SSD1963_800BD",480,800,0x1963,Protocol::Ssd1963,PixelFormat::RGB888,
    0x2C,0x2E,0x01,4,0x08,0x00,0x08,init_SSD1963_800BD,rotate_SSD1963_800BD,PixelReadFormat::RGB888,0,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class SSD1963_800BD : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit SSD1963_800BD(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_SSD1963_800BD(), reset, options) {}
};
} // namespace nice_gfx
