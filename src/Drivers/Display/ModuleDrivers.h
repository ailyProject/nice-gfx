#pragma once
#include "../../Display/ControllerProfile.h"
#include "ST7789.h"

namespace nice_gfx { namespace controllers {
// Seeed's 172x320 JD9853A uses the ST7789-compatible startup, with inversion
// disabled and a 34-column crop. See docs/VendorTouchScreens.md for pinned sources.
inline void init_JD9853A_172(PanelSequence &s) {
  controllers::init_ST7789(s); s.writecommand(0x20);
}
inline void rotate_JD9853A_172(PanelSequence &s, uint8_t r) {
  const uint8_t scan[] = {0x40, 0x20, 0x80, 0xE0};
  s.writecommand(0x36); s.writedata(scan[r & 3] | s.colorOrder);
  s.colstart = r & 1 ? 0 : 34; s.rowstart = r & 1 ? 34 : 0;
}
inline ControllerProfile profile_JD9853A_172() {
  return {"JD9853A_172",172,320,0x9853,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,8,0,8,init_JD9853A_172,rotate_JD9853A_172,PixelReadFormat::RGB888,1,0};
}
// Waveshare's Arduino demo uses Arduino_ST7789, IPS=false, offsets 34/0.
// Keep its scan orientation distinct from Seeed's MX-mirrored JD9853A glass.
inline void rotate_JD9853_172(PanelSequence &s, uint8_t r) {
  const uint8_t scan[] = {0, 0x60, 0xC0, 0xA0};
  s.writecommand(0x36); s.writedata(scan[r & 3] | s.colorOrder);
  s.colstart = r & 1 ? 0 : 34; s.rowstart = r & 1 ? 34 : 0;
}
inline ControllerProfile profile_JD9853_172() {
  ControllerProfile p = profile_JD9853A_172();
  p.name = "JD9853_172"; p.defaultColorOrder = 0; p.rotate = rotate_JD9853_172; return p;
}
inline void rotate_ST7789P_284(PanelSequence &s, uint8_t r) {
  controllers::rotate_ST7789(s, r);
  s.colstart = r & 1 ? 18 : 0; s.rowstart = r & 1 ? 0 : 18;
}
inline ControllerProfile profile_ST7789P_284() {
  ControllerProfile p = controllers::profile_ST7789();
  p.name = "ST7789P"; p.height = 284; p.rotate = rotate_ST7789P_284; return p;
}

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
inline void rotate_SeeedLegacy(PanelSequence &s, uint8_t) {
  s.colstart = s.rowstart = 0;
  s.writeRegister16(3,0x1030);
}
inline ControllerProfile profile_ST7781R() {
  return {"ST7781R",240,320,0x7781,Protocol::Ili9331,PixelFormat::RGB565,
    0x22,0,0,1,0,0,0,init_ST7781R,rotate_SeeedLegacy,PixelReadFormat::RGB565,1,0};
}
inline ControllerProfile profile_SPFD5408() {
  return {"SPFD5408",240,320,0x5408,Protocol::Ili9331,PixelFormat::RGB565,
    0x22,0,0,1,0,0,0,init_SPFD5408,rotate_SeeedLegacy,PixelReadFormat::RGB565,1,0};
}
} } // namespace nice_gfx::controllers

#include "ST7789.h"

namespace nice_gfx {
template<class Controller, uint16_t Width, uint16_t Height>
class SizedPanel : public Controller {
  static PanelOptions dimensions(PanelOptions options) {
    if (!options.width) options.width = Width;
    if (!options.height) options.height = Height;
    return options;
  }
 public:
  SizedPanel(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Controller(bus, reset, dimensions(options)) {}
};
#define GFX_VENDOR_PANEL(Name, Profile) \
class Name : public Panel { \
 public: \
  static constexpr bool paddedCommands = false; \
  static constexpr uint8_t spiMode = 0; \
  Name(Transport &bus, int reset = -1, PanelOptions options = PanelOptions()) \
      : Panel(bus, controllers::Profile(), reset, options) {} \
};
GFX_VENDOR_PANEL(JD9853A_172x320, profile_JD9853A_172)
GFX_VENDOR_PANEL(JD9853_172x320, profile_JD9853_172)
GFX_VENDOR_PANEL(ST7789P_240x284, profile_ST7789P_284)
GFX_VENDOR_PANEL(ST7781R, profile_ST7781R)
GFX_VENDOR_PANEL(SPFD5408, profile_SPFD5408)
#undef GFX_VENDOR_PANEL
using ST7789V2_170x320 = SizedPanel<ST7789,170,320>;
using ST7789V2_240x280 = SizedPanel<ST7789,240,280>;
using ST7789T3_240x320 = SizedPanel<ST7789,240,320>;
} // namespace nice_gfx
