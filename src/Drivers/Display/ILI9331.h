#pragma once
#include "../../Display/ControllerProfile.h"

// ILI9331 initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_ILI9331(PanelSequence &s) {
{
  s.commandMode(); s.raw16(0x00E7);
  s.dataMode(); s.raw16(0x1014);
  s.commandMode(); s.raw16(0x0001);
  s.dataMode(); s.raw16(0x0100);
  s.commandMode(); s.raw16(0x0002);
  s.dataMode(); s.raw16(0x0200);
  s.commandMode(); s.raw16(0x0003);
  s.dataMode(); s.raw16(0x1030);
  s.commandMode(); s.raw16(0x0008);
  s.dataMode(); s.raw16(0x0207);
  s.commandMode(); s.raw16(0x0009);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x000A);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x000C);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x000D);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x000F);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0010);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0011);
  s.dataMode(); s.raw16(0x0001);
  s.commandMode(); s.raw16(0x0012);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0013);
  s.dataMode(); s.raw16(0x0000);
  s.delay(200);
  s.commandMode(); s.raw16(0x0010);
  s.dataMode(); s.raw16(0x1690);
  s.commandMode(); s.raw16(0x0011);
  s.dataMode(); s.raw16(0x0001);
  s.delay(50);
  s.commandMode(); s.raw16(0x0012);
  s.dataMode(); s.raw16(0x000C);
  s.delay(50);
  s.commandMode(); s.raw16(0x0013);
  s.dataMode(); s.raw16(0x0700);
  s.commandMode(); s.raw16(0x0029);
  s.dataMode(); s.raw16(0x0005);
  s.commandMode(); s.raw16(0x002B);
  s.dataMode(); s.raw16(0x000D);
  s.delay(50);
  s.commandMode(); s.raw16(0x0020);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0021);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0030);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0031);
  s.dataMode(); s.raw16(0x0207);
  s.commandMode(); s.raw16(0x0032);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0035);
  s.dataMode(); s.raw16(0x0007);
  s.commandMode(); s.raw16(0x0036);
  s.dataMode(); s.raw16(0x0508);
  s.commandMode(); s.raw16(0x0037);
  s.dataMode(); s.raw16(0x0707);
  s.commandMode(); s.raw16(0x0038);
  s.dataMode(); s.raw16(0x0005);
  s.commandMode(); s.raw16(0x0039);
  s.dataMode(); s.raw16(0x0707);
  s.commandMode(); s.raw16(0x003C);
  s.dataMode(); s.raw16(0x0202);
  s.commandMode(); s.raw16(0x003D);
  s.dataMode(); s.raw16(0x0A09);
  s.commandMode(); s.raw16(0x0050);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0051);
  s.dataMode(); s.raw16(0x00EF);
  s.commandMode(); s.raw16(0x0052);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0053);
  s.dataMode(); s.raw16(0x013F);
  s.commandMode(); s.raw16(0x0060);
  s.dataMode(); s.raw16(0xA700);
  s.commandMode(); s.raw16(0x0061);
  s.dataMode(); s.raw16(0x0001);
  s.commandMode(); s.raw16(0x006A);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0080);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0081);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0082);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0083);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0084);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0085);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x0090);
  s.dataMode(); s.raw16(0x0010);
  s.commandMode(); s.raw16(0x0092);
  s.dataMode(); s.raw16(0x0600);
  s.commandMode(); s.raw16(0x0007);
  s.dataMode(); s.raw16(0x0133);
  s.commandMode(); s.raw16(0x00B3);
  s.dataMode(); s.raw16(0x0024);
  s.commandMode(); s.raw16(0x00B1);
  s.dataMode(); s.raw16(0x00FF);
}
}
inline void rotate_ILI9331(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  static const uint16_t gateScan[4] = {0x2700, 0x2700, 0xA700, 0xA700};
  static const uint16_t sourceScan[4] = {0x0100, 0x0000, 0x0000, 0x0100};
  static const uint16_t entryMode[4] = {0x1030, 0x1038, 0x1030, 0x1038};
  s.commandMode(); s.raw16(0x0060); s.dataMode(); s.raw16(gateScan[s.rotation]);
  s.commandMode(); s.raw16(0x0001); s.dataMode(); s.raw16(sourceScan[s.rotation]);
  s.commandMode(); s.raw16(0x0003); s.dataMode(); s.raw16(entryMode[s.rotation]);
  s.commandMode(); s.raw16(0x22); s.dataMode();
  if (s.rotation & 1) { s._width = s._init_height; s._height = s._init_width; }
  else { s._width = s._init_width; s._height = s._init_height; }
}
inline ControllerProfile profile_ILI9331() {
  return {"ILI9331",240,320,0x9331,Protocol::Ili9331,PixelFormat::RGB565,
    0x22,0x22,0x00,4,0,0,8,init_ILI9331,rotate_ILI9331,PixelReadFormat::RGB565,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class ILI9331 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit ILI9331(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ILI9331(), reset, options) {}
};
} // namespace nice_gfx
