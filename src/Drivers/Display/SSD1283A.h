#pragma once
#include "../../Display/ControllerProfile.h"

// SSD1283A 16-bit register initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_SSD1283A(PanelSequence &s) {
{
  s.commandMode(); s.raw16(0x0010);
  s.dataMode(); s.raw16(0x2F8E);
  s.commandMode(); s.raw16(0x0011);
  s.dataMode(); s.raw16(0x000C);
  s.commandMode(); s.raw16(0x0007);
  s.dataMode(); s.raw16(0x0021);
  s.commandMode(); s.raw16(0x0028);
  s.dataMode(); s.raw16(0x0006);
  s.commandMode(); s.raw16(0x0028);
  s.dataMode(); s.raw16(0x0005);
  s.commandMode(); s.raw16(0x0027);
  s.dataMode(); s.raw16(0x057F);
  s.commandMode(); s.raw16(0x0029);
  s.dataMode(); s.raw16(0x89A1);
  s.commandMode(); s.raw16(0x0000);
  s.dataMode(); s.raw16(0x0001);
  s.delay(100);
  s.commandMode(); s.raw16(0x0029);
  s.dataMode(); s.raw16(0x80B0);
  s.delay(30);
  s.commandMode(); s.raw16(0x0029);
  s.dataMode(); s.raw16(0xFFFE);
  s.commandMode(); s.raw16(0x0007);
  s.dataMode(); s.raw16(0x0223);
  s.delay(30);
  s.commandMode(); s.raw16(0x0007);
  s.dataMode(); s.raw16(0x0233);
  s.commandMode(); s.raw16(0x0001);
  s.dataMode(); s.raw16(0x2183);
  s.commandMode(); s.raw16(0x0003);
  s.dataMode(); s.raw16(0x6830);
  s.commandMode(); s.raw16(0x002F);
  s.dataMode(); s.raw16(0xFFFF);
  s.commandMode(); s.raw16(0x002C);
  s.dataMode(); s.raw16(0x8000);
  s.commandMode(); s.raw16(0x0027);
  s.dataMode(); s.raw16(0x0570);
  s.commandMode(); s.raw16(0x0002);
  s.dataMode(); s.raw16(0x0300);
  s.commandMode(); s.raw16(0x000B);
  s.dataMode(); s.raw16(0x580C);
  s.commandMode(); s.raw16(0x0012);
  s.dataMode(); s.raw16(0x0609);
  s.commandMode(); s.raw16(0x0013);
  s.dataMode(); s.raw16(0x3100);
}
}
inline void rotate_SSD1283A(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  static const uint16_t entryMode[4] = {0x6830, 0x6828, 0x6800, 0x6818};
  s.commandMode(); s.raw16(0x0003); s.dataMode(); s.raw16(entryMode[s.rotation]);
  if (s.rotation & 1) { s._width = s._init_height; s._height = s._init_width; }
  else { s._width = s._init_width; s._height = s._init_height; }
}
inline ControllerProfile profile_SSD1283A() {
  return {"SSD1283A",130,130,0x1283A,Protocol::Ssd1283,PixelFormat::RGB565,
    0x22,0x22,0x00,4,0,0,8,init_SSD1283A,rotate_SSD1283A,PixelReadFormat::RGB565,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class SSD1283A : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit SSD1283A(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_SSD1283A(), reset, options) {}
};
} // namespace nice_gfx
