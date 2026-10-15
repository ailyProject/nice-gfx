#pragma once
#include "../../Display/ControllerProfile.h"

// ILI9806 initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_ILI9806(PanelSequence &s) {
{
  s.writecommand(0xFF);
  s.writedata(0xFF);
  s.writedata(0x98);
  s.writedata(0x06);
  s.writecommand(0xBA);
  s.writedata(0x60);
  s.writecommand(0xBC);
  s.writedata(0x01);
  s.writedata(0x10);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x01);
  s.writedata(0x01);
  s.writedata(0x0B);
  s.writedata(0x11);
  s.writedata(0x32);
  s.writedata(0x10);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x01);
  s.writedata(0x01);
  s.writedata(0x01);
  s.writedata(0x01);
  s.writedata(0x50);
  s.writedata(0x52);
  s.writedata(0x01);
  s.writedata(0x00);
  s.writedata(0x40);
  s.writecommand(0xBD);
  s.writedata(0x01);
  s.writedata(0x23);
  s.writedata(0x45);
  s.writedata(0x67);
  s.writedata(0x01);
  s.writedata(0x23);
  s.writedata(0x45);
  s.writedata(0x67);
  s.writecommand(0xBE);
  s.writedata(0x00);
  s.writedata(0x21);
  s.writedata(0xAB);
  s.writedata(0x60);
  s.writedata(0x22);
  s.writedata(0x22);
  s.writedata(0x22);
  s.writedata(0x22);
  s.writedata(0x22);
  s.writecommand(0xC7);
  s.writedata(0x30);
  s.writecommand(0xED);
  s.writedata(0x7F);
  s.writedata(0x0F);
  s.writedata(0x00);
  s.writecommand(0xC0);
  s.writedata(0x03);
  s.writedata(0x0B);
  s.writedata(0x0C);
  s.writecommand(0xFD);
  s.writedata(0x0A);
  s.writedata(0x00);
  s.writecommand(0xFC);
  s.writedata(0x08);
  s.writecommand(0xDF);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x20);
  s.writecommand(0xF3);
  s.writedata(0x74);
  s.writecommand(0xB4);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writedata(0x00);
  s.writecommand(0xB5);
  s.writedata(0x08);
  s.writedata(0x15);
  s.writecommand(0xF7);
  s.writedata(0x81);
  s.writecommand(0xB1);
  s.writedata(0x00);
  s.writedata(0x13);
  s.writedata(0x13);
  s.writecommand(0xF2);
  s.writedata(0x80);
  s.writedata(0x04);
  s.writedata(0x40);
  s.writedata(0x28);
  s.writecommand(0xC1);
  s.writedata(0x17);
  s.writedata(0x71);
  s.writedata(0x71);
  s.writecommand(0xE0);
  s.writedata(0x00);
  s.writedata(0x13);
  s.writedata(0x1A);
  s.writedata(0x0C);
  s.writedata(0x0E);
  s.writedata(0x0B);
  s.writedata(0x07);
  s.writedata(0x05);
  s.writedata(0x05);
  s.writedata(0x0A);
  s.writedata(0x0F);
  s.writedata(0x0F);
  s.writedata(0x0E);
  s.writedata(0x1C);
  s.writedata(0x16);
  s.writedata(0x00);
  s.writecommand(0xE1);
  s.writedata(0x00);
  s.writedata(0x13);
  s.writedata(0x1A);
  s.writedata(0x0C);
  s.writedata(0x0E);
  s.writedata(0x0B);
  s.writedata(0x07);
  s.writedata(0x05);
  s.writedata(0x05);
  s.writedata(0x0A);
  s.writedata(0x0F);
  s.writedata(0x0F);
  s.writedata(0x0E);
  s.writedata(0x1C);
  s.writedata(0x16);
  s.writedata(0x00);
  s.writecommand(0x3A);
  s.writedata(0x55);
  s.writecommand(0x11);
  s.delay(120);
  s.writecommand(0x29);
  s.delay(25);
  s.writecommand(0x2C);
  s.writecommand(0x20);
}
}
inline void rotate_ILI9806(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  s.writecommand(0x36);
  switch (s.rotation) {
    case 0:
      s.writedata(0x00 | s.colorOrder);
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 1:
      s.writedata(0x60 | s.colorOrder);
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 2:
      s.writedata(0xC0 | s.colorOrder);
      s._width = s._init_width;
      s._height = s._init_height;
      s.colstart = 0; s.rowstart = 0;
      break;
    case 3:
      s.writedata(0xA0 | s.colorOrder);
      s._width = s._init_height;
      s._height = s._init_width;
      s.colstart = 0; s.rowstart = 0;
      break;
  }
}
inline ControllerProfile profile_ILI9806() {
  return {"ILI9806",480,854,0x9806,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0,0x00,0x08,init_ILI9806,rotate_ILI9806,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class ILI9806 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit ILI9806(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ILI9806(), reset, options) {}
};
} // namespace nice_gfx
