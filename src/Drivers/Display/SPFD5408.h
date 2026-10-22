#pragma once
#include "../../Display/Panel.h"
#include "Common/LegacyRotation.h"

namespace nice_gfx { namespace controllers {
// SPFD5408A / ILI932x startup from the author's SPFD5408 Arduino library.
// Graphics library by ladyada/adafruit with init code from Rossum, modified
// for SPFD5408 by Joao Lopes. Notices: LICENSES/SPFD5408-BSD.txt.
inline void init_SPFD5408(PanelSequence &s) {
  s.writeRegister16(0,1); s.delay(50);
  const uint16_t setup[][2] = {
    {1,0x0100},{2,0x0700},{3,0x1030},{4,0},{8,0x0202},{9,0},{0xA,0},
    {0xC,0},{0xD,0},{0xF,0},{0x10,0},{0x11,7},{0x12,0},{0x13,0}};
  for (const auto &v : setup) s.writeRegister16(v[0],v[1]);
  s.delay(200); s.writeRegister16(0x10,0x1690); s.writeRegister16(0x11,0x0227);
  s.delay(50); s.writeRegister16(0x12,0x001A); s.delay(50);
  s.writeRegister16(0x13,0x1800); s.writeRegister16(0x29,0x002A); s.delay(50);
  const uint16_t panel[][2] = {
    {0x30,0},{0x31,0},{0x32,0},{0x35,0x0206},{0x36,0x0808},{0x37,7},
    {0x38,0x0201},{0x39,0},{0x3C,0},{0x3D,0},
    {0x20,0},{0x21,0},{0x50,0},{0x51,239},{0x52,0},{0x53,319},
    {0x60,0xA700},{0x61,3},{0x6A,0},{0x90,0x0010},{0x92,0},
    {0x93,3},{0x95,0x1100},{0x97,0},{0x98,0},{7,0x0133}};
  for (const auto &v : panel) s.writeRegister16(v[0],v[1]);
}

inline ControllerProfile profile_SPFD5408() {
  return {"SPFD5408",240,320,0x5408,Protocol::Ili9331,PixelFormat::RGB565,
    0x22,0,0,1,0,0,0,init_SPFD5408,rotate_FixedEntryMode,PixelReadFormat::RGB565,1,0};
}
} }

namespace nice_gfx {
class SPFD5408 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  SPFD5408(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_SPFD5408(), reset, options) {}
};
}
