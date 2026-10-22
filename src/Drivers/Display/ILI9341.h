#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_ILI9341(PanelSequence &s, uint8_t scan) {
{
  s.writecommand(0xEF);
  s.writedata(0x03);
  s.writedata(0x80);
  s.writedata(0x02);
  s.writecommand(0xCF);
  s.writedata(0x00);
  s.writedata(0XC1);
  s.writedata(0X30);
  s.writecommand(0xED);
  s.writedata(0x64);
  s.writedata(0x03);
  s.writedata(0X12);
  s.writedata(0X81);
  s.writecommand(0xE8);
  s.writedata(0x85);
  s.writedata(0x00);
  s.writedata(0x78);
  s.writecommand(0xCB);
  s.writedata(0x39);
  s.writedata(0x2C);
  s.writedata(0x00);
  s.writedata(0x34);
  s.writedata(0x02);
  s.writecommand(0xF7);
  s.writedata(0x20);
  s.writecommand(0xEA);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writecommand(0xC0);
  s.writedata(0x23);
  s.writecommand(0xC1);
  s.writedata(0x10);
  s.writecommand(0xC5);
  s.writedata(0x3e);
  s.writedata(0x28);
  s.writecommand(0xC7);
  s.writedata(0x86);
  s.writecommand(0x36);
  s.writedata(uint8_t(scan | s.colorOrder));
  s.writecommand(0x3A);
  s.writedata(0x55);
  s.writecommand(0xB1);
  s.writedata(0x00);
  s.writedata(0x13);
  s.writecommand(0xB6);
  s.writedata(0x08);
  s.writedata(0x82);
  s.writedata(0x27);
  s.writecommand(0xF2);
  s.writedata(0x00);
  s.writecommand(0x26);
  s.writedata(0x01);
  s.writecommand(0xE0);
  s.writedata(0x0F);
  s.writedata(0x31);
  s.writedata(0x2B);
  s.writedata(0x0C);
  s.writedata(0x0E);
  s.writedata(0x08);
  s.writedata(0x4E);
  s.writedata(0xF1);
  s.writedata(0x37);
  s.writedata(0x07);
  s.writedata(0x10);
  s.writedata(0x03);
  s.writedata(0x0E);
  s.writedata(0x09);
  s.writedata(0x00);
  s.writecommand(0xE1);
  s.writedata(0x00);
  s.writedata(0x0E);
  s.writedata(0x14);
  s.writedata(0x03);
  s.writedata(0x11);
  s.writedata(0x07);
  s.writedata(0x31);
  s.writedata(0xC1);
  s.writedata(0x48);
  s.writedata(0x08);
  s.writedata(0x0F);
  s.writedata(0x0C);
  s.writedata(0x31);
  s.writedata(0x36);
  s.writedata(0x0F);
  s.writecommand(0x11);
  s.endPanelWrite();
  s.delay(120);
  s.beginPanelWrite();
  s.writecommand(0x29);
}
}
inline void init_ILI9341(PanelSequence &s) { init_ILI9341(s, 0x40); }
inline void rotate_ILI9341(PanelSequence &s, uint8_t m, const uint8_t (&scan)[8]) {
  s.rotation = m % 8;
  s.writecommand(0x36);
  s.writedata(uint8_t(scan[s.rotation] | s.colorOrder));
  s._width  = s.rotation & 1 ? s._init_height : s._init_width;
  s._height = s.rotation & 1 ? s._init_width : s._init_height;
}
inline void rotate_ILI9341(PanelSequence &s, uint8_t m) {
  static const uint8_t scan[] = {0x40,0x20,0x80,0xE0,0xC0,0x60,0x00,0xA0};
  rotate_ILI9341(s, m, scan);
}
inline ControllerProfile profile_ILI9341() {
  return {"ILI9341",240,320,0x9341,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,8,0x08,0x00,0x08,init_ILI9341,rotate_ILI9341,PixelReadFormat::RGB888,1,0xD9};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class ILI9341 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit ILI9341(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ILI9341(), reset, options) {}
};
} // namespace nice_gfx
