#pragma once
#include "../../Display/Panel.h"
#include "Common/LegacyRotation.h"

namespace nice_gfx { namespace controllers {
// ST7781R register values and power-up delays from Seeed's TFT Shield V1.
// Adapted from TFT.cpp (2011 Seeed Technology Inc., Albert.Miao/Visweswara R).
// SPDX-License-Identifier: LGPL-2.1-or-later
// Original licensing notice and LGPL text: LICENSES/Seeed-TFT-LGPL-2.1.txt.
inline void init_ST7781R(PanelSequence &s) {
  const uint16_t setup[][2] = {
    {1,0x0100},{2,0x0700},{3,0x1030},{4,0},{8,0x0302},{0xA,0},{0xC,0},{0xD,0},{0xF,0}};
  s.delay(100);
  for (const auto &v : setup) s.writeRegister16(v[0], v[1]);
  s.delay(100);
  const uint16_t gamma[][2] = {
    {0x30,0},{0x31,0x0405},{0x32,0x0203},{0x35,4},{0x36,0x0B07},
    {0x37,0},{0x38,0x0405},{0x39,0x0203},{0x3C,4},{0x3D,0x0B07},
    {0x20,0},{0x21,0},{0x50,0},{0x51,239},{0x52,0},{0x53,319}};
  for (const auto &v : gamma) s.writeRegister16(v[0], v[1]);
  s.delay(100);
  const uint16_t panel[][2] = {
    {0x60,0xA700},{0x61,1},{0x90,0x003A},{0x95,0x021E},
    {0x80,0},{0x81,0},{0x82,0},{0x83,0},{0x84,0},{0x85,0},
    {0xFF,1},{0xB0,0x140D},{0xFF,0}};
  for (const auto &v : panel) s.writeRegister16(v[0], v[1]);
  s.delay(100); s.writeRegister16(7,0x0133); s.delay(50);
  s.writeRegister16(0x10,0x14E0); s.delay(100); s.writeRegister16(7,0x0133);
}

inline ControllerProfile profile_ST7781R() {
  return {"ST7781R",240,320,0x7781,Protocol::Ili9331,PixelFormat::RGB565,
    0x22,0,0,1,0,0,0,init_ST7781R,rotate_SeeedLegacy,PixelReadFormat::RGB565,1,0};
}
} }

namespace nice_gfx {
class ST7781R : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  ST7781R(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ST7781R(), reset, options) {}
};
}
