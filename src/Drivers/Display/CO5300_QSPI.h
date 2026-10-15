#pragma once
#include "../../Display/ControllerProfile.h"
#include "Common/DcsRotation.h"

namespace nice_gfx { namespace controllers {
inline void init_CO5300_QSPI(PanelSequence &s) {
  s.writecommand(0x36); s.writedata(s.colorOrder);
  s.writecommand(0x3A); s.writedata(0x55);
  s.writecommand(0xFE); s.writedata(0x00);
  s.writecommand(0xC4); s.writedata(0x80);
  s.writecommand(0x35); s.delay(10);
  s.writecommand(0x53); s.writedata(0x20); s.delay(10);
  s.writecommand(0x51); s.writedata(0xFF); s.delay(10);
  s.writecommand(0x63); s.writedata(0xFF); s.delay(10);
  s.writecommand(0x2A); s.writedata(0x00); s.writedata(0x06); s.writedata(0x01); s.writedata(0xDD);
  s.writecommand(0x2B); s.writedata(0x00); s.writedata(0x00); s.writedata(0x01); s.writedata(0xD1);
  s.writecommand(0x11); s.delay(60);
  s.writecommand(0x29);
}

inline void rotate_CO5300_QSPI(PanelSequence &s,uint8_t rotation) { fixedRotate(s,rotation); }

inline ControllerProfile profile_CO5300_QSPI() {
  return {"CO5300_QSPI",480,480,0x5300,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,1,0,0,0x08,init_CO5300_QSPI,rotate_CO5300_QSPI,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"
#include "Common/QspiSetup.h"
namespace nice_gfx {
class CO5300_QSPI : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit CO5300_QSPI(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_CO5300_QSPI(), reset, options, 2, 2) {}
  template<class Config> static void configureQspi(Config &c) { controllers::configureGramQspi(c,0x03,0,2,false); }
};
}
