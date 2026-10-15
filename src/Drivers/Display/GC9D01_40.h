#pragma once
#include "../../Display/ControllerProfile.h"

// 160 x 160 dual-gate data is based on Arduino_GFX. The 40/50/60 x 160
// single-gate sequences are based on displaylib_16bit_PICO.

namespace nice_gfx { namespace controllers {
inline void init_GC9D01_40(PanelSequence &s) {
{
  s.writecommand(0xFE);
  s.writecommand(0xEF);
  for (uint8_t cmd = 0x80; cmd <= 0x8F; ++cmd) {
    s.writecommand(cmd);
    s.writedata(0xFF);
  }
  s.writecommand(0x3A); s.writedata(0x05);
  s.writecommand(0xEC); s.writedata(0x11);
  s.writecommand(0x7E); s.writedata(0x7A);
  s.writecommand(0x74); do { const uint8_t gc9d01_data[] = { 0x02, 0x0E, 0x00, 0x00, 0x28, 0x00, 0x00 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x98); s.writedata(0x3E);
  s.writecommand(0x99); s.writedata(0x3E);
  s.writecommand(0xB5); do { const uint8_t gc9d01_data[] = { 0x0E, 0x0E }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x60); do { const uint8_t gc9d01_data[] = { 0x38, 0x09, 0x6D, 0x67 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x63); do { const uint8_t gc9d01_data[] = { 0x38, 0xAD, 0x6D, 0x67, 0x05 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x64); do { const uint8_t gc9d01_data[] = { 0x38, 0x0B, 0x70, 0xAB, 0x6D, 0x67 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x66); do { const uint8_t gc9d01_data[] = { 0x38, 0x0F, 0x70, 0xAF, 0x6D, 0x67 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x6A); do { const uint8_t gc9d01_data[] = { 0x00, 0x00 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x68); do { const uint8_t gc9d01_data[] = { 0x3B, 0x08, 0x04, 0x00, 0x04, 0x64, 0x67 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x6C); do { const uint8_t gc9d01_data[] = { 0x22, 0x02, 0x22, 0x02, 0x22, 0x22, 0x50 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x6E);
  do { const uint8_t gc9d01_data[] = { 0x00, 0x00, 0x00, 0x00, 0x07, 0x01, 0x13, 0x11, 0x0B, 0x09, 0x16, 0x15, 0x1D, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x1D, 0x15, 0x16, 0x0A, 0x0C, 0x12, 0x14, 0x02, 0x08, 0x00, 0x00, 0x00, 0x00 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0xA9); s.writedata(0x1B);
  s.writecommand(0xA8); s.writedata(0x6B);
  s.writecommand(0xA8); s.writedata(0x6D);
  s.writecommand(0xA7); s.writedata(0x40);
  s.writecommand(0xAD); s.writedata(0x47);
  s.writecommand(0xAF); s.writedata(0x73);
  s.writecommand(0xAF); s.writedata(0x73);
  s.writecommand(0xAC); s.writedata(0x44);
  s.writecommand(0xA3); s.writedata(0x6C);
  s.writecommand(0xCB); s.writedata(0x00);
  s.writecommand(0xCD); s.writedata(0x22);
  s.writecommand(0xC2); s.writedata(0x10);
  s.writecommand(0xC5); s.writedata(0x00);
  s.writecommand(0xC6); s.writedata(0x0E);
  s.writecommand(0xC7); s.writedata(0x1F);
  s.writecommand(0xC8); s.writedata(0x0E);
  s.writecommand(0xBF); s.writedata(0x00);
  s.writecommand(0xF9); s.writedata(0x20);
  s.writecommand(0x9B); s.writedata(0x3B);
  s.writecommand(0x93); do { const uint8_t gc9d01_data[] = { 0x33, 0x7F, 0x00 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x70); do { const uint8_t gc9d01_data[] = { 0x0E, 0x0F, 0x03, 0x0E, 0x0F, 0x03 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x71); do { const uint8_t gc9d01_data[] = { 0x0E, 0x16, 0x03 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x91); do { const uint8_t gc9d01_data[] = { 0x0E, 0x09 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0xC3); s.writedata(0x2C);
  s.writecommand(0xC4); s.writedata(0x1A);
  s.writecommand(0xF0); do { const uint8_t gc9d01_data[] = { 0x51, 0x13, 0x0C, 0x06, 0x00, 0x2F }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0xF2); do { const uint8_t gc9d01_data[] = { 0x51, 0x13, 0x0C, 0x06, 0x00, 0x33 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0xF1); do { const uint8_t gc9d01_data[] = { 0x3C, 0x94, 0x4F, 0x33, 0x34, 0xCF }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0xF3); do { const uint8_t gc9d01_data[] = { 0x4D, 0x94, 0x4F, 0x33, 0x34, 0xCF }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x36);
  s.writedata(s.colorOrder);
  s.writecommand(0x11);
  s.delay(200);
  s.writecommand(0x29);
  s.delay(50);
  s.writecommand(0x2C);
}
}
inline void rotate_GC9D01_40(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  s.writecommand(0x36);
  switch (s.rotation) {
    case 0:
      s.writedata(s.colorOrder);
      s._width  = s._init_width;
      s._height = s._init_height;
      s.colstart = 0;
      s.rowstart = 0;
      break;
    case 1:
      s.writedata(0x20 | uint8_t(0x10 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
      s.colstart = -60;
      s.rowstart = 60;
      break;
    case 2:
      s.writedata(0x80 | uint8_t(0x40 | s.colorOrder));
      s._width  = s._init_width;
      s._height = s._init_height;
      s.colstart = 0;
      s.rowstart = 0;
      break;
    case 3:
      s.writedata(0x80 | 0x40 | 0x20 | uint8_t(0x10 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
      s.colstart = -60;
      s.rowstart = 60;
      break;
  }
}
inline ControllerProfile profile_GC9D01_40() {
  return {"GC9D01_40",40,160,0x9D01,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0x00,0x08,0x00,init_GC9D01_40,rotate_GC9D01_40,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class GC9D01_40 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit GC9D01_40(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_GC9D01_40(), reset, options) {}
};
} // namespace nice_gfx
