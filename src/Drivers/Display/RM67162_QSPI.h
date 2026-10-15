#pragma once
#include "../../Display/ControllerProfile.h"
#include "Common/DcsRotation.h"

namespace nice_gfx { namespace controllers {
inline void init_RM67162_QSPI(PanelSequence &s) {
  s.writecommand(0x11); s.delay(120);
  s.writecommand(0x36); s.writedata(s.colorOrder);
  s.writecommand(0x3A); s.writedata(0x55);
  s.writecommand(0x51); s.writedata(0xD0);
  s.writecommand(0x29); s.delay(20);
}

inline void rotate_RM67162_QSPI(PanelSequence &s,uint8_t rotation) { dcsRotate(s,rotation); }

inline ControllerProfile profile_RM67162_QSPI() {
  return {"RM67162_QSPI",240,536,0x67162,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0,0,8,init_RM67162_QSPI,rotate_RM67162_QSPI,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"
#include "Common/QspiSetup.h"
namespace nice_gfx {
class RM67162_QSPI : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit RM67162_QSPI(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_RM67162_QSPI(), reset, options, 2, 2) {}
  template<class Config> static void configureQspi(Config &c) { controllers::configureGramQspi(c,0x0B,0,2,false); }
};
}
