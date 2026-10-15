#pragma once
#include "../../Display/ControllerProfile.h"

// SEPS525 initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_SEPS525(PanelSequence &s) {
{
  s.writecommand(0x04);
  s.writedata(0x01);
  s.delay(1);
  s.writecommand(0x04);
  s.writedata(0x00);
  s.delay(1);
  s.writecommand(0x06);
  s.writedata(0x00);
  s.writecommand(0x02);
  s.writedata(0x01);
  s.writecommand(0x03);
  s.writedata(0x30);
  s.writecommand(0x28);
  s.writedata(0x7F);
  s.writecommand(0x29);
  s.writedata(0x00);
  s.writecommand(0x14);
  s.writedata(0x31);
  s.writecommand(0x10);
  s.writedata(0x45);
  s.writecommand(0x11);
  s.writedata(0x34);
  s.writecommand(0x12);
  s.writedata(0x33);
  s.writecommand(0x08);
  s.writedata(0x04);
  s.writecommand(0x09);
  s.writedata(0x05);
  s.writecommand(0x0A);
  s.writedata(0x05);
  s.writecommand(0x0B);
  s.writedata(0x9D);
  s.writecommand(0x0C);
  s.writedata(0x8C);
  s.writecommand(0x0D);
  s.writedata(0x57);
  s.writecommand(0x80);
  s.writedata(0x00);
  s.writecommand(0x06);
  s.writedata(0x01);
}
}
inline void rotate_SEPS525(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  switch (s.rotation) {
    case 0: s.writecommand(0x13); s.writedata(0x00); s.writecommand(0x16); s.writedata(0x66); s._width = s._init_width; s._height = s._init_height; break;
    case 1: s.writecommand(0x13); s.writedata(0x10); s.writecommand(0x16); s.writedata(0x67); s._width = s._init_height; s._height = s._init_width; break;
    case 2: s.writecommand(0x13); s.writedata(0x30); s.writecommand(0x16); s.writedata(0x66); s._width = s._init_width; s._height = s._init_height; break;
    case 3: s.writecommand(0x13); s.writedata(0x20); s.writecommand(0x16); s.writedata(0x67); s._width = s._init_height; s._height = s._init_width; break;
  }
}
inline ControllerProfile profile_SEPS525() {
  return {"SEPS525",160,128,0x525,Protocol::Seps525,PixelFormat::RGB565,
    0x22,0x22,0x05,4,0,0,8,init_SEPS525,rotate_SEPS525,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class SEPS525 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit SEPS525(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_SEPS525(), reset, options) {}
};
} // namespace nice_gfx
