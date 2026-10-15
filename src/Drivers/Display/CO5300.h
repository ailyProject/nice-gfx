#pragma once
#include "../../Display/ControllerProfile.h"

// CO5300 initialization ported from Arduino_GFX (BSD-licensed).

namespace nice_gfx { namespace controllers {
inline void init_CO5300(PanelSequence &s) {
{
  s.writecommand(0x11);
  s.delay(120);
  s.writecommand(0xFE);
  s.writedata(0x00);
  s.writecommand(0xC4);
  s.writedata(0x80);
  s.writecommand(0x3A);
  s.writedata(0x55);
  s.writecommand(0x53);
  s.writedata(0x20);
  s.writecommand(0x63);
  s.writedata(0xFF);
  s.writecommand(0x29);
  s.writecommand(0x51);
  s.writedata(0xD0);
  s.writecommand(0x58);
  s.writedata(0x00);
  s.delay(10);
  s.writecommand(0x20);
}
}
inline void rotate_CO5300(PanelSequence &s, uint8_t m) {
(void)m;
  s.rotation = 0;
  s.writecommand(0x36);
  s.writedata(s.colorOrder);
  s._width = s._init_width;
  s._height = s._init_height;
  s.colstart = 0;
  s.rowstart = 0;
}
inline ControllerProfile profile_CO5300() {
  return {"CO5300",480,480,0x5300,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,1,0,0x00,0x08,init_CO5300,rotate_CO5300,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class CO5300 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit CO5300(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_CO5300(), reset, options, 2, 2) {}
};
} // namespace nice_gfx
