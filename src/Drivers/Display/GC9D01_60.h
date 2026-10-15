#pragma once
#include "../../Display/ControllerProfile.h"

// 160 x 160 dual-gate data is based on Arduino_GFX. The 40/50/60 x 160
// single-gate sequences are based on displaylib_16bit_PICO.

namespace nice_gfx { namespace controllers {
inline void init_GC9D01_60(PanelSequence &s) {
{
  s.writecommand(0xFE);
  s.writecommand(0xEF);
  s.writecommand(0x80); s.writedata(0x13);
  s.writecommand(0x81); s.writedata(0x40);
  s.writecommand(0x82); s.writedata(0x0A);
  s.writecommand(0x83); s.writedata(0x0B);
  s.writecommand(0x84); s.writedata(0x20);
  s.writecommand(0x85); s.writedata(0x80);
  s.writecommand(0x86); s.writedata(0xFF);
  s.writecommand(0x87); s.writedata(0xFF);
  s.writecommand(0x89); s.writedata(0x10);
  s.writecommand(0x8A); s.writedata(0x0F);
  s.writecommand(0x8B); s.writedata(0x02);
  s.writecommand(0x8C); s.writedata(0x59);
  s.writecommand(0x8D); s.writedata(0x55);
  s.writecommand(0x8E); s.writedata(0xFF);
  s.writecommand(0x8F); s.writedata(0xFF);
  s.writecommand(0x3A); s.writedata(0x05);
  s.writecommand(0xEC); s.writedata(0x00);
  s.writecommand(0x7E); s.writedata(0x30);
  s.writecommand(0x74); do { const uint8_t gc9d01_data[] = { 0x04, 0x4E, 0x00, 0x00, 0x00, 0x00, 0x00 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0xB5); do { const uint8_t gc9d01_data[] = { 0x0F, 0x0F }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x61); do { const uint8_t gc9d01_data[] = { 0x58, 0x05, 0x6D, 0x67 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x62); do { const uint8_t gc9d01_data[] = { 0x58, 0x07, 0x6D, 0x67 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x63); do { const uint8_t gc9d01_data[] = { 0x58, 0x00, 0x6D, 0x67, 0x15 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x64); do { const uint8_t gc9d01_data[] = { 0x38, 0x0C, 0x70, 0xB4, 0x7F, 0x50 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x66); do { const uint8_t gc9d01_data[] = { 0x38, 0x08, 0x70, 0xB0, 0x7F, 0x50 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x6A); do { const uint8_t gc9d01_data[] = { 0x00, 0x00 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x6C); do { const uint8_t gc9d01_data[] = { 0x22, 0x02, 0x22, 0x02, 0x22, 0x22, 0x50 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x6E);
  do { const uint8_t gc9d01_data[] = { 0x07, 0x00, 0x08, 0x05, 0x1D, 0x1E, 0x12, 0x14, 0x0A, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x09, 0x13, 0x11, 0x1E, 0x1D, 0x04, 0x08, 0x00, 0x07 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x98); s.writedata(0x3E);
  s.writecommand(0x99); s.writedata(0x3E);
  s.writecommand(0x9B); s.writedata(0x3B);
  s.writecommand(0x93); do { const uint8_t gc9d01_data[] = { 0x33, 0x7F, 0x00 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x91); do { const uint8_t gc9d01_data[] = { 0x0E, 0x09 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x70); do { const uint8_t gc9d01_data[] = { 0x04, 0x05, 0x0D, 0x04, 0x05, 0x0D }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x71); do { const uint8_t gc9d01_data[] = { 0x04, 0x05, 0x0D }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0xC3); s.writedata(0x15);
  s.writecommand(0xC4); s.writedata(0x15);
  s.writecommand(0xC9); s.writedata(0x3E);
  s.writecommand(0xF0); do { const uint8_t gc9d01_data[] = { 0x54, 0x0D, 0x05, 0x02, 0x00, 0x32 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0xF2); do { const uint8_t gc9d01_data[] = { 0x54, 0x0D, 0x05, 0x02, 0x00, 0x32 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0xF1); do { const uint8_t gc9d01_data[] = { 0x4C, 0x94, 0x4F, 0x33, 0x34, 0xCF }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0xF3); do { const uint8_t gc9d01_data[] = { 0x4C, 0x94, 0x4F, 0x33, 0x34, 0xCF }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0xBF); s.writedata(0x00);
  s.writecommand(0xF9); s.writedata(0x40);
  s.writecommand(0x36);
  s.writedata(s.colorOrder);
  s.writecommand(0x11);
  s.delay(200);
  s.writecommand(0x29);
  s.delay(50);
  s.writecommand(0x2C);
}
}
inline void rotate_GC9D01_60(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  s.writecommand(0x36);
  switch (s.rotation) {
    case 0:
      s.writedata(s.colorOrder);
      s._width  = s._init_width;
      s._height = s._init_height;
      s.colstart = 10;
      s.rowstart = 0;
      break;
    case 1:
      s.writedata(0x20 | uint8_t(0x10 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
      s.colstart = -40;
      s.rowstart = 50;
      break;
    case 2:
      s.writedata(0x80 | uint8_t(0x40 | s.colorOrder));
      s._width  = s._init_width;
      s._height = s._init_height;
      s.colstart = 10;
      s.rowstart = 0;
      break;
    case 3:
      s.writedata(0x80 | 0x40 | 0x20 | uint8_t(0x10 | s.colorOrder));
      s._width  = s._init_height;
      s._height = s._init_width;
      s.colstart = -40;
      s.rowstart = 50;
      break;
  }
}
inline ControllerProfile profile_GC9D01_60() {
  return {"GC9D01_60",60,160,0x9D01,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0x00,0x08,0x00,init_GC9D01_60,rotate_GC9D01_60,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class GC9D01_60 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit GC9D01_60(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_GC9D01_60(), reset, options) {}
};
} // namespace nice_gfx
