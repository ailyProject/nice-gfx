#pragma once
#include "../../Display/ControllerProfile.h"

// NT35510 16-bit register initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_NT35510(PanelSequence &s) {
{
  uint8_t ini01[] = {0x55, 0xAA, 0x52, 0x08, 0x01};
  for (uint16_t i = 0; i < sizeof(ini01); ++i) { s.commandMode(); s.raw16(0xF000 + i); s.dataMode(); s.raw16(ini01[i]); }
  uint8_t ini03[] = {0x34, 0x34, 0x34};
  for (uint16_t i = 0; i < sizeof(ini03); ++i) { s.commandMode(); s.raw16(0xB600 + i); s.dataMode(); s.raw16(ini03[i]); }
  uint8_t ini02[] = {0x0D, 0x0D, 0x0D};
  for (uint16_t i = 0; i < sizeof(ini02); ++i) { s.commandMode(); s.raw16(0xB000 + i); s.dataMode(); s.raw16(ini02[i]); }
  uint8_t ini05[] = {0x34, 0x34, 0x34};
  for (uint16_t i = 0; i < sizeof(ini05); ++i) { s.commandMode(); s.raw16(0xB700 + i); s.dataMode(); s.raw16(ini05[i]); }
  uint8_t ini04[] = {0x0D, 0x0D, 0x0D};
  for (uint16_t i = 0; i < sizeof(ini04); ++i) { s.commandMode(); s.raw16(0xB100 + i); s.dataMode(); s.raw16(ini04[i]); }
  uint8_t ini07[] = {0x24, 0x24, 0x24};
  for (uint16_t i = 0; i < sizeof(ini07); ++i) { s.commandMode(); s.raw16(0xB800 + i); s.dataMode(); s.raw16(ini07[i]); }
  uint8_t ini10[] = {0x34, 0x34, 0x34};
  for (uint16_t i = 0; i < sizeof(ini10); ++i) { s.commandMode(); s.raw16(0xB900 + i); s.dataMode(); s.raw16(ini10[i]); }
  uint8_t ini09[] = {0x0F, 0x0F, 0x0F};
  for (uint16_t i = 0; i < sizeof(ini09); ++i) { s.commandMode(); s.raw16(0xB300 + i); s.dataMode(); s.raw16(ini09[i]); }
  uint8_t ini14[] = {0x24, 0x24, 0x24};
  for (uint16_t i = 0; i < sizeof(ini14); ++i) { s.commandMode(); s.raw16(0xBA00 + i); s.dataMode(); s.raw16(ini14[i]); }
  uint8_t ini12[] = {0x08, 0x08};
  for (uint16_t i = 0; i < sizeof(ini12); ++i) { s.commandMode(); s.raw16(0xB500 + i); s.dataMode(); s.raw16(ini12[i]); }
  uint8_t ini15[] = {0x00, 0x78, 0x00};
  for (uint16_t i = 0; i < sizeof(ini15); ++i) { s.commandMode(); s.raw16(0xBC00 + i); s.dataMode(); s.raw16(ini15[i]); }
  uint8_t ini16[] = {0x00, 0x78, 0x00};
  for (uint16_t i = 0; i < sizeof(ini16); ++i) { s.commandMode(); s.raw16(0xBD00 + i); s.dataMode(); s.raw16(ini16[i]); }
  uint8_t ini17[] = {0x00, 0x89};
  for (uint16_t i = 0; i < sizeof(ini17); ++i) { s.commandMode(); s.raw16(0xBE00 + i); s.dataMode(); s.raw16(ini17[i]); }
  uint8_t ini20[] = {
  0x00, 0x2D, 0x00, 0x2E, 0x00, 0x32, 0x00, 0x44, 0x00, 0x53, 0x00, 0x88, 0x00, 0xB6, 0x00, 0xF3, 0x01, 0x22, 0x01, 0x64,
  0x01, 0x92, 0x01, 0xD4, 0x02, 0x07, 0x02, 0x08, 0x02, 0x34, 0x02, 0x5F, 0x02, 0x78, 0x02, 0x94, 0x02, 0xA6, 0x02, 0xBB,
  0x02, 0xCA, 0x02, 0xDB, 0x02, 0xE8, 0x02, 0xF9, 0x03, 0x1F, 0x03, 0x7F};
  for (uint16_t i = 0; i < sizeof(ini20); ++i) { s.commandMode(); s.raw16(0xD100 + i); s.dataMode(); s.raw16(ini20[i]); }
  for (uint16_t i = 0; i < sizeof(ini20); ++i) { s.commandMode(); s.raw16(0xD400 + i); s.dataMode(); s.raw16(ini20[i]); }
  for (uint16_t i = 0; i < sizeof(ini20); ++i) { s.commandMode(); s.raw16(0xD200 + i); s.dataMode(); s.raw16(ini20[i]); }
  for (uint16_t i = 0; i < sizeof(ini20); ++i) { s.commandMode(); s.raw16(0xD500 + i); s.dataMode(); s.raw16(ini20[i]); }
  for (uint16_t i = 0; i < sizeof(ini20); ++i) { s.commandMode(); s.raw16(0xD300 + i); s.dataMode(); s.raw16(ini20[i]); }
  for (uint16_t i = 0; i < sizeof(ini20); ++i) { s.commandMode(); s.raw16(0xD600 + i); s.dataMode(); s.raw16(ini20[i]); }
  uint8_t ini21[] = {0x55, 0xAA, 0x52, 0x08, 0x00};
  for (uint16_t i = 0; i < sizeof(ini21); ++i) { s.commandMode(); s.raw16(0xF000 + i); s.dataMode(); s.raw16(ini21[i]); }
  uint8_t ini22[] = {0x08, 0x05, 0x02, 0x05, 0x02};
  for (uint16_t i = 0; i < sizeof(ini22); ++i) { s.commandMode(); s.raw16(0xB000 + i); s.dataMode(); s.raw16(ini22[i]); }
  s.commandMode(); s.raw16(0xB600);
  s.dataMode(); s.raw16(0x0008);
  s.commandMode(); s.raw16(0xB500);
  s.dataMode(); s.raw16(0x0050);
  uint8_t ini24[] = {0x00, 0x00};
  for (uint16_t i = 0; i < sizeof(ini24); ++i) { s.commandMode(); s.raw16(0xB700 + i); s.dataMode(); s.raw16(ini24[i]); }
  uint8_t ini25[] = {0x01, 0x05, 0x05, 0x05};
  for (uint16_t i = 0; i < sizeof(ini25); ++i) { s.commandMode(); s.raw16(0xB800 + i); s.dataMode(); s.raw16(ini25[i]); }
  uint8_t ini26[] = {0x00, 0x00, 0x00};
  for (uint16_t i = 0; i < sizeof(ini26); ++i) { s.commandMode(); s.raw16(0xBC00 + i); s.dataMode(); s.raw16(ini26[i]); }
  uint8_t ini27[] = {0x03, 0x00, 0x00};
  for (uint16_t i = 0; i < sizeof(ini27); ++i) { s.commandMode(); s.raw16(0xCC00 + i); s.dataMode(); s.raw16(ini27[i]); }
  uint8_t ini28[] = {0x01, 0x84, 0x07, 0x31, 0x00, 0x01};
  for (uint16_t i = 0; i < sizeof(ini28); ++i) { s.commandMode(); s.raw16(0xBD00 + i); s.dataMode(); s.raw16(ini28[i]); }
  uint8_t ini30[] = {0xAA, 0x55, 0x25, 0x01};
  for (uint16_t i = 0; i < sizeof(ini30); ++i) { s.commandMode(); s.raw16(0xFF00 + i); s.dataMode(); s.raw16(ini30[i]); }
  s.commandMode(); s.raw16(0x3500);
  s.dataMode(); s.raw16(0x0000);
  s.commandMode(); s.raw16(0x3A00);
  s.dataMode(); s.raw16(0x0055);
  s.commandMode(); s.raw16(0x1100);
  s.delay(120);
  s.commandMode(); s.raw16(0x2900);
  s.delay(120);
}
}
inline void rotate_NT35510(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  static const uint16_t madctl[4] = {0x00, 0x60, 0xC0, 0xA0};
  s.commandMode(); s.raw16(0x3600); s.dataMode(); s.raw16(madctl[s.rotation] | s.colorOrder);
  if (s.rotation & 1) { s._width = s._init_height; s._height = s._init_width; }
  else { s._width = s._init_width; s._height = s._init_height; }
}
inline ControllerProfile profile_NT35510() {
  return {"NT35510",480,800,0x35510,Protocol::Dcs16,PixelFormat::RGB565,
    0x2C00,0x2E00,0x0000,4,0,0,8,init_NT35510,rotate_NT35510,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class NT35510 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit NT35510(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_NT35510(), reset, options) {}
};
} // namespace nice_gfx
