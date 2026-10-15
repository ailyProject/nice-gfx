#pragma once
#include "../../Display/ControllerProfile.h"

// JBT6K71 initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_JBT6K71(PanelSequence &s) {
{
  s.commandMode(); s.raw16(0x0000);
  s.delay(10);
  s.commandMode(); s.raw16(0x0000);
  s.delay(10);
  s.commandMode(); s.raw16(0x0000);
  s.delay(10);
  s.commandMode(); s.raw16(0x001D);
  s.dataMode(); s.raw16(0x0005);
  s.delay(100);
  s.commandMode(); s.raw16(0x0000);
  s.dataMode(); s.raw16(0x0001);
  s.delay(100);
  s.commandMode(); s.raw16(0x0002);
  s.dataMode(); s.raw16(0x0200);
  s.commandMode(); s.raw16(0x000D);
  s.dataMode(); s.raw16(0x0011);
  s.delay(100);
  s.commandMode(); s.raw16(0x0012);
  s.dataMode(); s.raw16(0x0303);
  s.commandMode(); s.raw16(0x0013);
  s.dataMode(); s.raw16(0x0102);
  s.commandMode(); s.raw16(0x001C);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0102);
  s.dataMode(); s.raw16(0x00F6);
  s.delay(250);
  s.delay(250);
  s.commandMode(); s.raw16(0x0103);
  s.dataMode(); s.raw16(0x0007);
  s.delay(100);
  s.commandMode(); s.raw16(0x0105);
  s.dataMode(); s.raw16(0x0111);
  s.delay(100);
  s.commandMode(); s.raw16(0x0300);
  s.dataMode(); s.raw16(0x0200);
  s.commandMode(); s.raw16(0x0301);
  s.dataMode(); s.raw16(0x0002);
  s.commandMode(); s.raw16(0x0302);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0303);
  s.dataMode(); s.raw16(0x0300);
  s.commandMode(); s.raw16(0x0304);
  s.dataMode(); s.raw16(0x0700);
  s.commandMode(); s.raw16(0x0305);
  s.dataMode(); s.raw16(0x0070);
  s.commandMode(); s.raw16(0x0402);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0403);
  s.dataMode(); s.raw16(0x013F);
  s.commandMode(); s.raw16(0x0100);
  s.dataMode(); s.raw16(0xC010);
  s.delay(250);
  s.delay(250);
  s.commandMode(); s.raw16(0x0101);
  s.dataMode(); s.raw16(0x0001);
  s.commandMode(); s.raw16(0x0100);
  s.dataMode(); s.raw16(0xF7FE);
  s.delay(250);
  s.delay(250);
  s.delay(250);
}
}
inline void rotate_JBT6K71(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  static const uint16_t outputControl[4] = {0x0027, 0x0127, 0x0127, 0x0027};
  static const uint16_t entryMode[4] = {0x0010, 0x0018, 0x0030, 0x0038};
  s.commandMode(); s.raw16(0x0001); s.dataMode(); s.raw16(outputControl[s.rotation]);
  s.commandMode(); s.raw16(0x0003); s.dataMode(); s.raw16(entryMode[s.rotation]);
  if (s.rotation & 1) { s._width = s._init_height; s._height = s._init_width; }
  else { s._width = s._init_width; s._height = s._init_height; }
}
inline ControllerProfile profile_JBT6K71() {
  return {"JBT6K71",240,320,0x671,Protocol::Jbt6k71,PixelFormat::RGB565,
    0x0202,0x0202,0x0000,4,0,0,8,init_JBT6K71,rotate_JBT6K71,PixelReadFormat::RGB565,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class JBT6K71 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit JBT6K71(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_JBT6K71(), reset, options) {}
};
} // namespace nice_gfx
