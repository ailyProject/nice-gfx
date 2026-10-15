#pragma once
#include "../../Display/ControllerProfile.h"
#include "Common/DcsRotation.h"

namespace nice_gfx { namespace controllers {
inline void init_SH8601_QSPI(PanelSequence &s) {
  s.writecommand(0x36); s.writedata(s.colorOrder);
  s.writecommand(0x3A); s.writedata(0x55);
  s.writecommand(0x44); s.writedata(0x00); s.writedata(0xC8);
  s.writecommand(0x35);
  s.writecommand(0x53); s.writedata(0x20); s.delay(25);
  s.writecommand(0x11); s.delay(120);
  s.writecommand(0x51); s.writedata(0xFF);
  s.writecommand(0x29); s.delay(20);
}

inline void rotate_SH8601_QSPI(PanelSequence &s,uint8_t rotation) { fixedRotate(s,rotation); }

inline ControllerProfile profile_SH8601_QSPI() {
  return {"SH8601_QSPI",480,480,0x8601,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0,0x01,1,0,0,0x08,init_SH8601_QSPI,rotate_SH8601_QSPI,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"
#include "Common/QspiSetup.h"
namespace nice_gfx {
class SH8601_QSPI : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit SH8601_QSPI(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_SH8601_QSPI(), reset, options, 2, 2) {}
  template<class Config> static void configureQspi(Config &c) { controllers::configureGramQspi(c,0x03,0,2,false); }
};
}
