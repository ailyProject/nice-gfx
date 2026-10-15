#pragma once
#include "../../Display/ControllerProfile.h"
#include "Common/DcsRotation.h"

namespace nice_gfx { namespace controllers {
inline void init_RM690B0_QSPI(PanelSequence &s) {
  s.writecommand(0xFE); s.writedata(0x20);
  s.writecommand(0x24); s.writedata(0x80);
  s.writecommand(0x5B); s.writedata(0x2E);
  s.writecommand(0xFE); s.writedata(0x00);
  s.writecommand(0xC2); s.writedata(0); s.delay(10);
  s.writecommand(0x35); s.writedata(0);
  s.writecommand(0x11); s.delay(120);
  s.writecommand(0x29); s.delay(10);
  s.writecommand(0x36); s.writedata(s.colorOrder);
  s.writecommand(0x3A); s.writedata(0x55);
  s.writecommand(0x80); s.writedata(0);
  s.writecommand(0x51); s.writedata(0xFF);
}

inline void rotate_RM690B0_QSPI(PanelSequence &s,uint8_t rotation) { fixedRotate(s,rotation); }

inline ControllerProfile profile_RM690B0_QSPI() {
  return {"RM690B0_QSPI",482,600,0x690B0,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,1,0,0,8,init_RM690B0_QSPI,rotate_RM690B0_QSPI,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"
#include "Common/QspiSetup.h"
namespace nice_gfx {
class RM690B0_QSPI : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit RM690B0_QSPI(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_RM690B0_QSPI(), reset, options, 2, 2) {}
  template<class Config> static void configureQspi(Config &c) { controllers::configureGramQspi(c,0x0B,0,2,false); }
};
}
