#pragma once
#include "../../Display/ControllerProfile.h"
#include "Common/DcsRotation.h"

namespace nice_gfx { namespace controllers {
inline void init_NV3041A_QSPI(PanelSequence &s) {
  s.writecommand(0x11); s.delay(100);
  s.writecommand(0x36); s.writedata(s.colorOrder);
  s.writecommand(0x3A); s.writedata(0x55);
  s.writecommand(0x38);
  s.writecommand(0xFF); s.writedata(0xA5);
  s.writecommand(0xE7); s.writedata(0x10);
  s.writecommand(0x35); s.writedata(0x00);
  s.writecommand(0x36); s.writedata(0x00);
  s.writecommand(0x3A); s.writedata(0x01);
  s.writecommand(0x40); s.writedata(0x01);
  s.writecommand(0x41); s.writedata(0x01);
  s.writecommand(0x44); s.writedata(0x15);
  s.writecommand(0x45); s.writedata(0x15);
  s.writecommand(0x7D); s.writedata(0x02);
  s.writecommand(0xC1); s.writedata(0xBB);
  s.writecommand(0xC2); s.writedata(0x05);
  s.writecommand(0xC3); s.writedata(0x10);
  s.writecommand(0xC6); s.writedata(0x3E);
  s.writecommand(0xC7); s.writedata(0x25);
  s.writecommand(0xC8); s.writedata(0x11);
  s.writecommand(0x7A); s.writedata(0x5F);
  s.writecommand(0x6F); s.writedata(0x44);
  s.writecommand(0x78); s.writedata(0x70);
  s.writecommand(0xC9); s.writedata(0x00);
  s.writecommand(0x67); s.writedata(0x21);
  s.writecommand(0x51); s.writedata(0x0A);
  s.writecommand(0x52); s.writedata(0x76);
  s.writecommand(0x53); s.writedata(0x0A);
  s.writecommand(0x54); s.writedata(0x76);
  s.writecommand(0x46); s.writedata(0x0A);
  s.writecommand(0x47); s.writedata(0x2A);
  s.writecommand(0x48); s.writedata(0x0A);
  s.writecommand(0x49); s.writedata(0x1A);
  s.writecommand(0x56); s.writedata(0x43);
  s.writecommand(0x57); s.writedata(0x42);
  s.writecommand(0x58); s.writedata(0x3C);
  s.writecommand(0x59); s.writedata(0x64);
  s.writecommand(0x5A); s.writedata(0x41);
  s.writecommand(0x5B); s.writedata(0x3C);
  s.writecommand(0x5C); s.writedata(0x02);
  s.writecommand(0x5D); s.writedata(0x3C);
  s.writecommand(0x5E); s.writedata(0x1F);
  s.writecommand(0x60); s.writedata(0x80);
  s.writecommand(0x61); s.writedata(0x3F);
  s.writecommand(0x62); s.writedata(0x21);
  s.writecommand(0x63); s.writedata(0x07);
  s.writecommand(0x64); s.writedata(0xE0);
  s.writecommand(0x65); s.writedata(0x02);
  s.writecommand(0xCA); s.writedata(0x20);
  s.writecommand(0xCB); s.writedata(0x52);
  s.writecommand(0xCC); s.writedata(0x10);
  s.writecommand(0xCD); s.writedata(0x42);
  s.writecommand(0xD0); s.writedata(0x20);
  s.writecommand(0xD1); s.writedata(0x52);
  s.writecommand(0xD2); s.writedata(0x10);
  s.writecommand(0xD3); s.writedata(0x42);
  s.writecommand(0xD4); s.writedata(0x0A);
  s.writecommand(0xD5); s.writedata(0x32);
  s.writecommand(0xF8); s.writedata(0x03);
  s.writecommand(0xF9); s.writedata(0x20);
  s.writecommand(0x80); s.writedata(0x00);
  s.writecommand(0xA0); s.writedata(0x00);
  s.writecommand(0x81); s.writedata(0x07);
  s.writecommand(0xA1); s.writedata(0x06);
  s.writecommand(0x82); s.writedata(0x02);
  s.writecommand(0xA2); s.writedata(0x01);
  s.writecommand(0x86); s.writedata(0x11);
  s.writecommand(0xA6); s.writedata(0x10);
  s.writecommand(0x87); s.writedata(0x27);
  s.writecommand(0xA7); s.writedata(0x27);
  s.writecommand(0x83); s.writedata(0x37);
  s.writecommand(0xA3); s.writedata(0x37);
  s.writecommand(0x84); s.writedata(0x35);
  s.writecommand(0xA4); s.writedata(0x35);
  s.writecommand(0x85); s.writedata(0x3F);
  s.writecommand(0xA5); s.writedata(0x3F);
  s.writecommand(0x88); s.writedata(0x0B);
  s.writecommand(0xA8); s.writedata(0x0B);
  s.writecommand(0x89); s.writedata(0x14);
  s.writecommand(0xA9); s.writedata(0x14);
  s.writecommand(0x8A); s.writedata(0x1A);
  s.writecommand(0xAA); s.writedata(0x1A);
  s.writecommand(0x8B); s.writedata(0x0A);
  s.writecommand(0xAB); s.writedata(0x0A);
  s.writecommand(0x8C); s.writedata(0x14);
  s.writecommand(0xAC); s.writedata(0x08);
  s.writecommand(0x8D); s.writedata(0x17);
  s.writecommand(0xAD); s.writedata(0x07);
  s.writecommand(0x8E); s.writedata(0x16);
  s.writecommand(0xAE); s.writedata(0x06);
  s.writecommand(0x8F); s.writedata(0x1B);
  s.writecommand(0xAF); s.writedata(0x07);
  s.writecommand(0x90); s.writedata(0x04);
  s.writecommand(0xB0); s.writedata(0x04);
  s.writecommand(0x91); s.writedata(0x0A);
  s.writecommand(0xB1); s.writedata(0x0A);
  s.writecommand(0x92); s.writedata(0x16);
  s.writecommand(0xB2); s.writedata(0x15);
  s.writecommand(0xFF); s.writedata(0x00);
  s.writecommand(0x11); s.writedata(0x00); s.delay(700);
  s.writecommand(0x29); s.writedata(0x00); s.delay(100);
}

inline void rotate_NV3041A_QSPI(PanelSequence &s,uint8_t rotation) { fixedRotate(s,rotation); }

inline ControllerProfile profile_NV3041A_QSPI() {
  return {"NV3041A_QSPI",480,272,0x3041A,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0,0x01,1,0,0,0x08,init_NV3041A_QSPI,rotate_NV3041A_QSPI,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"
#include "../../Display/QspiProtocol.h"
#include "Common/QspiSetup.h"
#include "Common/ColorTransports.h"
namespace nice_gfx { namespace controllers {
struct Nv3041aHolder {
  Axs15231bTransport nvBus;
  Nv3041aHolder(Transport &bus,const PanelOptions &options)
      : nvBus(bus,options.width ? options.width : 480,options.height ? options.height : 272,true,1) {}
};
} }
namespace nice_gfx {
class NV3041A_QSPI : private controllers::Nv3041aHolder, public Panel {
 public:
  static constexpr bool paddedCommands=false;
  static constexpr uint8_t spiMode=0;
  explicit NV3041A_QSPI(Transport &bus,int reset=-1,PanelOptions options=PanelOptions())
      : Nv3041aHolder(bus,options),Panel(nvBus,controllers::profile_NV3041A_QSPI(),reset,options) {}
  template<class Config> static void configureQspi(Config &c) {
    controllers::configureGramQspi(c,0x03,0); c.protocol.pixelContinuation=false;
  }
};
}
