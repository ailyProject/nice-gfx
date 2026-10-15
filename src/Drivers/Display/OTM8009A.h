#pragma once
#include "../../Display/ControllerProfile.h"

// OTM8009A 16-bit register initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_OTM8009A(PanelSequence &s) {
{
  uint8_t ini01[] = {0x80, 0x09, 0x01, 0x01};
  for (uint16_t i = 0; i < sizeof(ini01); ++i) { s.commandMode(); s.raw16(0xFF00 + i); s.dataMode(); s.raw16(ini01[i]); }
  uint8_t ini02[] = {0x80, 0x09};
  for (uint16_t i = 0; i < sizeof(ini02); ++i) { s.commandMode(); s.raw16(0xFF80 + i); s.dataMode(); s.raw16(ini02[i]); }
  s.commandMode(); s.raw16(0xF5B6);
  s.dataMode(); s.raw16(0x0006);
  uint8_t ini03[] = {0x30, 0x83};
  for (uint16_t i = 0; i < sizeof(ini03); ++i) { s.commandMode(); s.raw16(0xC480 + i); s.dataMode(); s.raw16(ini03[i]); }
  s.commandMode(); s.raw16(0xC48A);
  s.dataMode(); s.raw16(0x0040);
  s.commandMode(); s.raw16(0xC0A3);
  s.dataMode(); s.raw16(0x001B);
  s.commandMode(); s.raw16(0xC0BA);
  s.dataMode(); s.raw16(0x0050);
  s.commandMode(); s.raw16(0xC181);
  s.dataMode(); s.raw16(0x0066);
  s.commandMode(); s.raw16(0xC1A1);
  s.dataMode(); s.raw16(0x000E);
  s.commandMode(); s.raw16(0xC582);
  s.dataMode(); s.raw16(0x0083);
  uint8_t ini04[] = {0x96, 0x2B, 0x01, 0x33, 0x34};
  for (uint16_t i = 0; i < sizeof(ini04); ++i) { s.commandMode(); s.raw16(0xC590 + i); s.dataMode(); s.raw16(ini04[i]); }
  s.commandMode(); s.raw16(0xC5B1);
  s.dataMode(); s.raw16(0x00A9);
  uint8_t ini05[] = {0x86, 0x01, 0x00, 0x85, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  for (uint16_t i = 0; i < sizeof(ini05); ++i) { s.commandMode(); s.raw16(0xCE80 + i); s.dataMode(); s.raw16(ini05[i]); }
  uint8_t ini06[] = {0x18, 0x04, 0x03, 0x21, 0x00, 0x00, 0x00};
  for (uint16_t i = 0; i < sizeof(ini06); ++i) { s.commandMode(); s.raw16(0xCEA0 + i); s.dataMode(); s.raw16(ini06[i]); }
  uint8_t ini07[] = {0x18, 0x03, 0x03, 0x22, 0x00, 0x00, 0x00};
  for (uint16_t i = 0; i < sizeof(ini07); ++i) { s.commandMode(); s.raw16(0xCEA7 + i); s.dataMode(); s.raw16(ini07[i]); }
  uint8_t ini08[] = {0x18, 0x02, 0x03, 0x23, 0x00, 0x00, 0x00};
  for (uint16_t i = 0; i < sizeof(ini08); ++i) { s.commandMode(); s.raw16(0xCEB0 + i); s.dataMode(); s.raw16(ini08[i]); }
  uint8_t ini09[] = {0x18, 0x01, 0x03, 0x24, 0x00, 0x00, 0x00};
  for (uint16_t i = 0; i < sizeof(ini09); ++i) { s.commandMode(); s.raw16(0xCEB7 + i); s.dataMode(); s.raw16(ini09[i]); }
  uint8_t ini10[] = {0x01, 0x01, 0x20, 0x20, 0x00, 0x00};
  for (uint16_t i = 0; i < sizeof(ini10); ++i) { s.commandMode(); s.raw16(0xCFC0 + i); s.dataMode(); s.raw16(ini10[i]); }
  s.commandMode(); s.raw16(0xCFC6);
  s.dataMode(); s.raw16(0x0001);
  uint8_t ini11[] = {0x00, 0x00, 0x00};
  for (uint16_t i = 0; i < sizeof(ini11); ++i) { s.commandMode(); s.raw16(0xCFC7 + i); s.dataMode(); s.raw16(ini11[i]); }
  s.commandMode(); s.raw16(0xCFD0);
  s.dataMode(); s.raw16(0x0000);
  uint8_t ini12[] = {0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  for (uint16_t i = 0; i < sizeof(ini12); ++i) { s.commandMode(); s.raw16(0xCBC0 + i); s.dataMode(); s.raw16(ini12[i]); }
  uint8_t ini13[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00};
  for (uint16_t i = 0; i < sizeof(ini13); ++i) { s.commandMode(); s.raw16(0xCBD0 + i); s.dataMode(); s.raw16(ini13[i]); }
  uint8_t ini14[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  for (uint16_t i = 0; i < sizeof(ini14); ++i) { s.commandMode(); s.raw16(0xCBE0 + i); s.dataMode(); s.raw16(ini14[i]); }
  uint8_t ini15[] = {0x00, 0x26, 0x09, 0x0B, 0x01, 0x25, 0x00, 0x00, 0x00, 0x00};
  for (uint16_t i = 0; i < sizeof(ini15); ++i) { s.commandMode(); s.raw16(0xCC80 + i); s.dataMode(); s.raw16(ini15[i]); }
  uint8_t ini16[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x0A, 0x0C, 0x02};
  for (uint16_t i = 0; i < sizeof(ini16); ++i) { s.commandMode(); s.raw16(0xCC90 + i); s.dataMode(); s.raw16(ini16[i]); }
  uint8_t ini17[] = {0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  for (uint16_t i = 0; i < sizeof(ini17); ++i) { s.commandMode(); s.raw16(0xCCA0 + i); s.dataMode(); s.raw16(ini17[i]); }
  s.commandMode(); s.raw16(0x3A00);
  s.dataMode(); s.raw16(0x0055);
  s.commandMode(); s.raw16(0x1100);
  s.delay(100);
  s.commandMode(); s.raw16(0x2900);
  s.delay(50);
}
}
inline void rotate_OTM8009A(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  static const uint16_t madctl[4] = {0x00, 0x60, 0xC0, 0xA0};
  s.commandMode(); s.raw16(0x3600); s.dataMode(); s.raw16(madctl[s.rotation] | s.colorOrder);
  if (s.rotation & 1) { s._width = s._init_height; s._height = s._init_width; }
  else { s._width = s._init_width; s._height = s._init_height; }
}
inline ControllerProfile profile_OTM8009A() {
  return {"OTM8009A",480,800,0x8009A,Protocol::Dcs16,PixelFormat::RGB565,
    0x2C00,0x2E00,0x0000,4,0,0,8,init_OTM8009A,rotate_OTM8009A,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class OTM8009A : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit OTM8009A(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_OTM8009A(), reset, options) {}
};
} // namespace nice_gfx
