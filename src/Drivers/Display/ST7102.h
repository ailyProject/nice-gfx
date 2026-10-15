#pragma once
#include "../../Display/ControllerProfile.h"
#include "Common/DcsRotation.h"

namespace nice_gfx { namespace controllers {
inline void init_ST7102(PanelSequence &s) {
  s.writecommand(0x36); s.writedata(s.colorOrder);
  s.writecommand(0x3A); s.writedata(0x55);
  s.writecommand(0x99); s.writedata(0x71); s.writedata(0x02); s.writedata(0xA2);
  s.writecommand(0x99); s.writedata(0x71); s.writedata(0x02); s.writedata(0xA3);
  s.writecommand(0x99); s.writedata(0x71); s.writedata(0x02); s.writedata(0xA4);
  s.writecommand(0xA4); s.writedata(0x31);
  s.writecommand(0xB0); s.writedata(0x22); s.writedata(0x61); s.writedata(0x1E); s.writedata(0x61); s.writedata(0x2F); s.writedata(0x39); s.writedata(0x39);
  s.writecommand(0xB7); s.writedata(0x46); s.writedata(0x46);
  s.writecommand(0xBF); s.writedata(0x50); s.writedata(0x50);
  s.writecommand(0xD7); s.writedata(0x00); s.writedata(0x10); s.writedata(0x8C); s.writedata(0x08); s.writedata(0xF0); s.writedata(0xF0);
  s.writecommand(0xA3); s.writedata(0x40); s.writedata(0x03); s.writedata(0x8C); s.writedata(0x40); s.writedata(0x45); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x1E); s.writedata(0x01); s.writedata(0x00); s.writedata(0x12); s.writedata(0x00); s.writedata(0x45); s.writedata(0x05); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x1E); s.writedata(0x01); s.writedata(0x00); s.writedata(0x12); s.writedata(0x20); s.writedata(0x52); s.writedata(0x00); s.writedata(0x05); s.writedata(0x00); s.writedata(0x00); s.writedata(0xFF);
  s.writecommand(0xA6); s.writedata(0x08); s.writedata(0x00); s.writedata(0x24); s.writedata(0x55); s.writedata(0x35); s.writedata(0x00); s.writedata(0x76); s.writedata(0x40); s.writedata(0x4E); s.writedata(0x4E); s.writedata(0x00); s.writedata(0x24); s.writedata(0x55); s.writedata(0x00); s.writedata(0x00); s.writedata(0x40); s.writedata(0x40); s.writedata(0x4E); s.writedata(0x4E); s.writedata(0x02); s.writedata(0xAC); s.writedata(0x51); s.writedata(0x00); s.writedata(0xCC); s.writedata(0x40); s.writedata(0x40); s.writedata(0x4E); s.writedata(0x4E); s.writedata(0x00); s.writedata(0xAC); s.writedata(0x11); s.writedata(0x00); s.writedata(0x00); s.writedata(0x40); s.writedata(0x40); s.writedata(0x4E); s.writedata(0x4E); s.writedata(0x00); s.writedata(0x00); s.writedata(0x06); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00);
  s.writecommand(0xA7); s.writedata(0x19); s.writedata(0x19); s.writedata(0x00); s.writedata(0x64); s.writedata(0x40); s.writedata(0x07); s.writedata(0x16); s.writedata(0x40); s.writedata(0x00); s.writedata(0x44); s.writedata(0x43); s.writedata(0x4E); s.writedata(0x4E); s.writedata(0x00); s.writedata(0x64); s.writedata(0x40); s.writedata(0x25); s.writedata(0x34); s.writedata(0x00); s.writedata(0x00); s.writedata(0x42); s.writedata(0x41); s.writedata(0x4E); s.writedata(0x4E); s.writedata(0x00); s.writedata(0x64); s.writedata(0x40); s.writedata(0x4B); s.writedata(0x5A); s.writedata(0x00); s.writedata(0x00); s.writedata(0x42); s.writedata(0x41); s.writedata(0x4E); s.writedata(0x4E); s.writedata(0x00); s.writedata(0x24); s.writedata(0x40); s.writedata(0x69); s.writedata(0x78); s.writedata(0x00); s.writedata(0x00); s.writedata(0x40); s.writedata(0x40); s.writedata(0x4E); s.writedata(0x4E); s.writedata(0x00); s.writedata(0x44);
  s.writecommand(0xAC); s.writedata(0x00); s.writedata(0x1C); s.writedata(0x04); s.writedata(0x1A); s.writedata(0x19); s.writedata(0x1B); s.writedata(0x1B); s.writedata(0x18); s.writedata(0x06); s.writedata(0x13); s.writedata(0x19); s.writedata(0x11); s.writedata(0x1B); s.writedata(0x08); s.writedata(0x18); s.writedata(0x0A); s.writedata(0x01); s.writedata(0x1C); s.writedata(0x04); s.writedata(0x1A); s.writedata(0x19); s.writedata(0x1B); s.writedata(0x1B); s.writedata(0x18); s.writedata(0x06); s.writedata(0x12); s.writedata(0x19); s.writedata(0x10); s.writedata(0x1B); s.writedata(0x09); s.writedata(0x18); s.writedata(0x0B); s.writedata(0xBF); s.writedata(0xAA); s.writedata(0xBF); s.writedata(0xAA); s.writedata(0x00);
  s.writecommand(0xAD); s.writedata(0xCC); s.writedata(0x40); s.writedata(0x46); s.writedata(0x11); s.writedata(0x04); s.writedata(0x6F); s.writedata(0x6F);
  s.writecommand(0xE8); s.writedata(0x30); s.writedata(0x07); s.writedata(0x05); s.writedata(0x6A); s.writedata(0x6A); s.writedata(0x9C); s.writedata(0x00); s.writedata(0xE2); s.writedata(0x04); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0xEF);
  s.writecommand(0x75); s.writedata(0x03); s.writedata(0x04);
  s.writecommand(0xE7); s.writedata(0x8B); s.writedata(0x3C); s.writedata(0x00); s.writedata(0x0C); s.writedata(0xF0); s.writedata(0x5D); s.writedata(0x00); s.writedata(0x5D); s.writedata(0x00); s.writedata(0x5D); s.writedata(0x00); s.writedata(0x5D); s.writedata(0x00); s.writedata(0xFF); s.writedata(0x00); s.writedata(0x08); s.writedata(0x7B); s.writedata(0x00); s.writedata(0x00); s.writedata(0xC8); s.writedata(0x6A); s.writedata(0x5A); s.writedata(0x08); s.writedata(0x1A); s.writedata(0x3C); s.writedata(0x00); s.writedata(0xA1); s.writedata(0x01); s.writedata(0x8C); s.writedata(0x01); s.writedata(0x7F); s.writedata(0xF0); s.writedata(0x22);
  s.writecommand(0xE9); s.writedata(0x3C); s.writedata(0x7F); s.writedata(0x08); s.writedata(0x10); s.writedata(0x1A); s.writedata(0x7A); s.writedata(0x22); s.writedata(0x1A); s.writedata(0x33);
  s.writecommand(0xC8); s.writedata(0x00); s.writedata(0x00); s.writedata(0x15); s.writedata(0x26); s.writedata(0x44); s.writedata(0x00); s.writedata(0x78); s.writedata(0x03); s.writedata(0xBE); s.writedata(0x06); s.writedata(0x11); s.writedata(0x1C); s.writedata(0x09); s.writedata(0x8A); s.writedata(0x03); s.writedata(0x21); s.writedata(0xD4); s.writedata(0x01); s.writedata(0x11); s.writedata(0x0F); s.writedata(0x22); s.writedata(0x4A); s.writedata(0x0F); s.writedata(0x8F); s.writedata(0x0A); s.writedata(0x32); s.writedata(0xF0); s.writedata(0x0A); s.writedata(0x41); s.writedata(0x0D); s.writedata(0xF3); s.writedata(0x80); s.writedata(0x0D); s.writedata(0xAE); s.writedata(0xC5); s.writedata(0x03); s.writedata(0xC4);
  s.writecommand(0xC9); s.writedata(0x00); s.writedata(0x00); s.writedata(0x15); s.writedata(0x26); s.writedata(0x44); s.writedata(0x00); s.writedata(0x78); s.writedata(0x03); s.writedata(0xBE); s.writedata(0x06); s.writedata(0x11); s.writedata(0x1C); s.writedata(0x09); s.writedata(0x8A); s.writedata(0x03); s.writedata(0x21); s.writedata(0xD4); s.writedata(0x01); s.writedata(0x11); s.writedata(0x0F); s.writedata(0x22); s.writedata(0x4A); s.writedata(0x0F); s.writedata(0x8F); s.writedata(0x0A); s.writedata(0x32); s.writedata(0xF0); s.writedata(0x0A); s.writedata(0x41); s.writedata(0x0D); s.writedata(0xF3); s.writedata(0x80); s.writedata(0x0D); s.writedata(0xAE); s.writedata(0xC5); s.writedata(0x03); s.writedata(0xC4);
  s.writecommand(0x99); s.writedata(0x71); s.writedata(0x02); s.writedata(0x00);
  s.writecommand(0x11); s.delay(120);
  s.writecommand(0x29); s.delay(20);
  s.writecommand(0x35); s.writedata(0x00);
  s.writecommand(0x36); s.writedata(0x00);
}

inline void rotate_ST7102(PanelSequence &s,uint8_t rotation) { fixedRotate(s,rotation); }

inline ControllerProfile profile_ST7102() {
  return {"ST7102",480,800,0x7102,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,1,0,0,0x08,init_ST7102,rotate_ST7102,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class ST7102 : public Panel {
 public:
  static constexpr bool paddedCommands=false;
  static constexpr uint8_t spiMode=0;
  explicit ST7102(Transport &bus,int reset=-1,PanelOptions options=PanelOptions())
      : Panel(bus,controllers::profile_ST7102(),reset,options) {}
};
}

#include "../../Display/Panel.h"
#include "../../Display/QspiProtocol.h"
#include "Common/QspiSetup.h"
#include "Common/ColorTransports.h"
namespace nice_gfx { namespace controllers {
struct St7102Holder {
  Axs15231bTransport stBus;
  St7102Holder(Transport &bus,const PanelOptions &options)
      : stBus(bus,options.width ? options.width : 480,options.height ? options.height : 800,true,1) {}
};
} }
namespace nice_gfx {
class ST7102_QSPI : private controllers::St7102Holder, public Panel {
 public:
  static constexpr bool paddedCommands=false;
  static constexpr uint8_t spiMode=0;
  explicit ST7102_QSPI(Transport &bus,int reset=-1,PanelOptions options=PanelOptions())
      : St7102Holder(bus,options),Panel(stBus,profile(),reset,options) {}
  template<class Config> static void configureQspi(Config &c) {
    controllers::configureGramQspi(c,0x0B,0); c.protocol.pixelContinuation=false;
  }
 private:
  static ControllerProfile profile() {
    ControllerProfile p=controllers::profile_ST7102(); p.ramRead=0; return p;
  }
};
}
