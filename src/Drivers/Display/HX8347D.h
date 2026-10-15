#pragma once
#include "../../Display/ControllerProfile.h"

// HX8347D initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_HX8347D(PanelSequence &s) {
{
  s.writecommand(0xEA);
  s.writedata(0x00);
  s.writecommand(0xEB);
  s.writedata(0x20);
  s.writecommand(0xEC);
  s.writedata(0x0C);
  s.writecommand(0xED);
  s.writedata(0xC4);
  s.writecommand(0xE8);
  s.writedata(0x38);
  s.writecommand(0xE9);
  s.writedata(0x10);
  s.writecommand(0xF1);
  s.writedata(0x01);
  s.writecommand(0xF2);
  s.writedata(0x10);
  s.writecommand(0x40);
  s.writedata(0x01);
  s.writecommand(0x41);
  s.writedata(0x00);
  s.writecommand(0x42);
  s.writedata(0x00);
  s.writecommand(0x43);
  s.writedata(0x10);
  s.writecommand(0x44);
  s.writedata(0x0E);
  s.writecommand(0x45);
  s.writedata(0x24);
  s.writecommand(0x46);
  s.writedata(0x04);
  s.writecommand(0x47);
  s.writedata(0x50);
  s.writecommand(0x48);
  s.writedata(0x02);
  s.writecommand(0x49);
  s.writedata(0x13);
  s.writecommand(0x4A);
  s.writedata(0x19);
  s.writecommand(0x4B);
  s.writedata(0x19);
  s.writecommand(0x4C);
  s.writedata(0x16);
  s.writecommand(0x50);
  s.writedata(0x1B);
  s.writecommand(0x51);
  s.writedata(0x31);
  s.writecommand(0x52);
  s.writedata(0x2F);
  s.writecommand(0x53);
  s.writedata(0x3F);
  s.writecommand(0x54);
  s.writedata(0x3F);
  s.writecommand(0x55);
  s.writedata(0x3E);
  s.writecommand(0x56);
  s.writedata(0x2F);
  s.writecommand(0x57);
  s.writedata(0x7B);
  s.writecommand(0x58);
  s.writedata(0x09);
  s.writecommand(0x59);
  s.writedata(0x06);
  s.writecommand(0x5A);
  s.writedata(0x06);
  s.writecommand(0x5B);
  s.writedata(0x0C);
  s.writecommand(0x5C);
  s.writedata(0x1D);
  s.writecommand(0x5D);
  s.writedata(0xCC);
  s.writecommand(0x1B);
  s.writedata(0x1B);
  s.writecommand(0x1A);
  s.writedata(0x01);
  s.writecommand(0x24);
  s.writedata(0x2F);
  s.writecommand(0x25);
  s.writedata(0x57);
  s.writecommand(0x23);
  s.writedata(0x88);
  s.writecommand(0x18);
  s.writedata(0x34);
  s.writecommand(0x19);
  s.writedata(0x01);
  s.writecommand(0x1F);
  s.writedata(0x88);
  s.writecommand(0x1F);
  s.writedata(0x80);
  s.writecommand(0x1F);
  s.writedata(0x90);
  s.writecommand(0x1F);
  s.writedata(0xD0);
  s.writecommand(0x17);
  s.writedata(0x05);
  s.writecommand(0x28);
  s.writedata(0x38);
  s.writecommand(0x28);
  s.writedata(0x3F);
  s.writecommand(0x16);
  s.writedata(0x18);
}
}
inline void rotate_HX8347D(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  switch (s.rotation) {
    case 0: s.writecommand(0x36); s.writedata(0x03); s.writecommand(0x16); s.writedata(0x40); s._width = s._init_width; s._height = s._init_height; break;
    case 1: s.writecommand(0x36); s.writedata(0x03); s.writecommand(0x16); s.writedata(0x20); s._width = s._init_height; s._height = s._init_width; break;
    case 2: s.writecommand(0x36); s.writedata(0x07); s.writecommand(0x16); s.writedata(0x00); s._width = s._init_width; s._height = s._init_height; break;
    case 3: s.writecommand(0x36); s.writedata(0x07); s.writecommand(0x16); s.writedata(0x60); s._width = s._init_height; s._height = s._init_width; break;
  }
}
inline ControllerProfile profile_HX8347D() {
  return {"HX8347D",240,320,0x8347D,Protocol::Hx8347,PixelFormat::RGB565,
    0x22,0x22,0x00,4,0,0,8,init_HX8347D,rotate_HX8347D,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class HX8347D : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit HX8347D(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_HX8347D(), reset, options) {}
};
} // namespace nice_gfx
