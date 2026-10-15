#pragma once
#include "../../Display/ControllerProfile.h"

// 160 x 160 dual-gate data is based on Arduino_GFX. The 40/50/60 x 160
// single-gate sequences are based on displaylib_16bit_PICO.

namespace nice_gfx { namespace controllers {
inline void init_GC9D01(PanelSequence &s) {
{
  s.writecommand(0xFE);
  s.writecommand(0xEF);
  for (uint8_t cmd = 0x80; cmd <= 0x8F; ++cmd) {
    s.writecommand(cmd);
    s.writedata(0xFF);
  }
  s.writecommand(0x3A);
  s.writedata(0x05);
  s.writecommand(0xEC);
  s.writedata(0x01);
  s.writecommand(0x74);
  do { const uint8_t gc9d01_data[] = { 0x02, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x98); s.writedata(0x3E);
  s.writecommand(0x99); s.writedata(0x3E);
  s.writecommand(0xB5); do { const uint8_t gc9d01_data[] = { 0x0D, 0x0D }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x60); do { const uint8_t gc9d01_data[] = { 0x38, 0x0F, 0x79, 0x67 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x61); do { const uint8_t gc9d01_data[] = { 0x38, 0x11, 0x79, 0x67 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x64); do { const uint8_t gc9d01_data[] = { 0x38, 0x17, 0x71, 0x5F, 0x79, 0x67 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x65); do { const uint8_t gc9d01_data[] = { 0x38, 0x13, 0x71, 0x5B, 0x79, 0x67 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x6A); do { const uint8_t gc9d01_data[] = { 0x00, 0x00 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x6C); do { const uint8_t gc9d01_data[] = { 0x22, 0x02, 0x22, 0x02, 0x22, 0x22, 0x50 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x6E);
  do { const uint8_t gc9d01_data[] = { 0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0x0F, 0x0F, 0x0D, 0x0D, 0x0B, 0x0B, 0x09, 0x09, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x0A, 0x0C, 0x0C, 0x0E, 0x0E, 0x10, 0x10, 0x00, 0x00, 0x02, 0x02, 0x04, 0x04 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0xBF); s.writedata(0x01);
  s.writecommand(0xF9); s.writedata(0x40);
  s.writecommand(0x9B); s.writedata(0x3B);
  s.writecommand(0x93); do { const uint8_t gc9d01_data[] = { 0x33, 0x7F, 0x00 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x7E); s.writedata(0x30);
  s.writecommand(0x70); do { const uint8_t gc9d01_data[] = { 0x0D, 0x02, 0x08, 0x0D, 0x02, 0x08 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x71); do { const uint8_t gc9d01_data[] = { 0x0D, 0x02, 0x08 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x91); do { const uint8_t gc9d01_data[] = { 0x0E, 0x09 }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0xC3); s.writedata(0x19);
  s.writecommand(0xC4); s.writedata(0x19);
  s.writecommand(0xC9); s.writedata(0x3C);
  s.writecommand(0xF0); do { const uint8_t gc9d01_data[] = { 0x53, 0x15, 0x0A, 0x04, 0x00, 0x3E }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0xF2); do { const uint8_t gc9d01_data[] = { 0x53, 0x15, 0x0A, 0x04, 0x00, 0x3A }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0xF1); do { const uint8_t gc9d01_data[] = { 0x56, 0xA8, 0x7F, 0x33, 0x34, 0x5F }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0xF3); do { const uint8_t gc9d01_data[] = { 0x52, 0xA4, 0x7F, 0x33, 0x34, 0xDF }; for (uint8_t gc9d01_i = 0; gc9d01_i < sizeof(gc9d01_data); ++gc9d01_i) s.writedata(gc9d01_data[gc9d01_i]); } while (0);
  s.writecommand(0x36);
  s.writedata(s.colorOrder);
  s.writecommand(0x11);
  s.delay(200);
  s.writecommand(0x29);
  s.delay(50);
  s.writecommand(0x2C);
}
}
inline void rotate_GC9D01(PanelSequence &s, uint8_t m) {
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
      s.colstart = 0;
      s.rowstart = 0;
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
      s.colstart = 0;
      s.rowstart = 0;
      break;
  }
}
inline ControllerProfile profile_GC9D01() {
  return {"GC9D01",160,160,0x9D01,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0x00,0x08,0x00,init_GC9D01,rotate_GC9D01,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class GC9D01 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit GC9D01(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_GC9D01(), reset, options) {}
};
} // namespace nice_gfx
