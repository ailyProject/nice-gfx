#pragma once
#include "../../Display/ControllerProfile.h"

namespace nice_gfx { namespace controllers {
inline void init_ILI9163(PanelSequence &s) {
{
 const uint8_t ILI9163_cmds[] =
 {
 17,
 0x01, 0 + 0x80, 120,
 0x11, 0 + 0x80, 5,
 0x3A, 1, 0x05,
 0x26, 1, 0x04,
 0xF2, 1, 0x01,
 0xE0, 15, 0x3F, 0x25, 0x1C, 0x1E, 0x20, 0x12, 0x2A, 0x90,
           0x24, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00,
 0xE1, 15, 0x20, 0x20, 0x20, 0x20, 0x05, 0x00, 0x15,0xA7,
           0x3D, 0x18, 0x25, 0x2A, 0x2B, 0x2B, 0x3A,
 0xB1, 2, 0x08, 0x08,
 0xB4, 1, 0x07,
 0xC0, 2, 0x0A, 0x02,
 0xC1, 1, 0x02,
 0xC5, 2, 0x50, 0x5B,
 0xC7, 1, 0x40,
 0x2A, 4, 0x00, 0x00, 0x00, 0x7F,
 0x2B, 4 + 0x80, 0x00, 0x00, 0x00, 0x9F, 250,
 0x36, 1, 0xC8,
 0x29, 0,
 };
 s.commandList(ILI9163_cmds);
      s.colstart = 0;
      s.rowstart = 0;
}
}
inline void rotate_ILI9163(PanelSequence &s, uint8_t m) {
s.rotation = m % 4;
  s.writecommand(0x36);
  switch (s.rotation) {
    case 0:
      s.writedata(0x40 | uint8_t(0x80 | s.colorOrder));
      s._width = s._init_width;
      s._height = s._init_height;
        s.colstart = 0;
        s.rowstart = 0;
      break;
    case 1:
      s.writedata(0x20 | uint8_t(0x80 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
        s.colstart = 0;
        s.rowstart = 0;
      break;
    case 2:
      s.writedata(s.colorOrder);
      s._width = s._init_width;
      s._height = s._init_height;
        s.colstart = 0;
        s.rowstart = 160 - s._init_height;
      break;
    case 3:
      s.writedata(0x40 | uint8_t(0x20 | s.colorOrder));
      s._width = s._init_height;
      s._height = s._init_width;
        s.colstart = 160 - s._init_height;
        s.rowstart = 0;
      break;
  }
}
inline ControllerProfile profile_ILI9163() {
  return {"ILI9163",128,160,0x9163,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0x08,0x00,0x08,init_ILI9163,rotate_ILI9163,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class ILI9163 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit ILI9163(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ILI9163(), reset, options) {}
};
} // namespace nice_gfx
