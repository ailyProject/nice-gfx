#pragma once
#include "../../Display/ControllerProfile.h"

// SSD1357 initialization based on Arduino_GFX and the WiseChip reference table.

namespace nice_gfx { namespace controllers {
inline void init_SSD1357(PanelSequence &s) {
{
  const uint8_t gamma_lut[63] = {
    0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,
    0x12,0x13,0x15,0x17,0x19,0x1B,0x1D,0x1F,0x21,0x23,0x25,0x27,0x2A,0x2D,0x30,0x33,
    0x36,0x39,0x3C,0x3F,0x42,0x45,0x48,0x4C,0x50,0x54,0x58,0x5C,0x60,0x64,0x68,0x6C,
    0x70,0x74,0x78,0x7D,0x82,0x87,0x8C,0x91,0x96,0x9B,0xA0,0xA5,0xAA,0xAF,0xB4
  };
  s.writecommand(0xFD); s.writedata(0x12);
  s.writecommand(0xAE);
  s.writecommand(0xB3); s.writedata(0xB0);
  s.writecommand(0xCA); s.writedata(s._init_height - 1);
  s.writecommand(0xA2); s.writedata(128 - s._init_height);
  s.writecommand(0xA1); s.writedata(0x00);
  s.writecommand(0xA0); s.writedata(uint8_t(0x70 | s.colorOrder)); s.writedata(0x00);
  s.writecommand(0xC1); s.writedata(0x88); s.writedata(0x32); s.writedata(0x88);
  s.writecommand(0xC7); s.writedata(0x0F);
  s.writecommand(0xB1); s.writedata(0x32);
  s.writecommand(0xB6); s.writedata(0x01);
  s.writecommand(0xB8); for (uint8_t i = 0; i < 63; ++i) s.writedata(gamma_lut[i]);
  s.writecommand(0xBB); s.writedata(0x17);
  s.writecommand(0xBE); s.writedata(0x05);
  s.writecommand(0xA6);
  s.writecommand(0xAF);
}
}
inline void rotate_SSD1357(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  static const uint8_t remap[4] = {0x72, 0x71, 0x60, 0x63};
  s.writecommand(0xA0);
  s.writedata(remap[s.rotation] | s.colorOrder);
  s.writedata(0x00);
  s.writecommand(0xA2);
  s.writedata(s.rotation < 2 ? (128 - s._init_height) : 0);
  switch (s.rotation) {
    case 1:
      s._width = s._init_height; s._height = s._init_width;
      s.colstart = 0; s.rowstart = ((128 - s._init_width) / 2);
      break;
    case 2:
      s._width = s._init_width; s._height = s._init_height;
      s.colstart = ((128 - s._init_width) / 2); s.rowstart = 0;
      break;
    case 3:
      s._width = s._init_height; s._height = s._init_width;
      s.colstart = 0; s.rowstart = ((128 - s._init_width) / 2);
      break;
    default:
      s._width = s._init_width; s._height = s._init_height;
      s.colstart = ((128 - s._init_width) / 2); s.rowstart = 0;
      break;
  }
}
inline ControllerProfile profile_SSD1357() {
  return {"SSD1357",128,128,0x1357,Protocol::Ssd1351,PixelFormat::RGB565,
    0x5C,0x5D,0x00,4,0,0,4,init_SSD1357,rotate_SSD1357,PixelReadFormat::RGB565,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class SSD1357 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit SSD1357(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_SSD1357(), reset, options) {}
};
} // namespace nice_gfx
