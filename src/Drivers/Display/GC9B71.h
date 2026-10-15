#pragma once
#include "../../Display/ControllerProfile.h"
#include "Common/DcsRotation.h"

namespace nice_gfx { namespace controllers {
inline void init_GC9B71(PanelSequence &s) {
  s.writecommand(0x36); s.writedata(s.colorOrder);
  s.writecommand(0x3A); s.writedata(0x55);
  s.writecommand(0xFE);
  s.writecommand(0xEF);
  s.writecommand(0x80); s.writedata(0x11);
  s.writecommand(0x81); s.writedata(0x70);
  s.writecommand(0x82); s.writedata(0x09);
  s.writecommand(0x83); s.writedata(0x03);
  s.writecommand(0x84); s.writedata(0x62);
  s.writecommand(0x89); s.writedata(0x18);
  s.writecommand(0x8A); s.writedata(0x40);
  s.writecommand(0x8B); s.writedata(0x0A);
  s.writecommand(0xEC); s.writedata(0x07);
  s.writecommand(0x74); s.writedata(0x01); s.writedata(0x80); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00);
  s.writecommand(0x98); s.writedata(0x3E);
  s.writecommand(0x99); s.writedata(0x3E);
  s.writecommand(0xA1); s.writedata(0x01); s.writedata(0x04);
  s.writecommand(0xA2); s.writedata(0x01); s.writedata(0x04);
  s.writecommand(0xCB); s.writedata(0x02);
  s.writecommand(0x7C); s.writedata(0xB6); s.writedata(0x24);
  s.writecommand(0xAC); s.writedata(0x74);
  s.writecommand(0xF6); s.writedata(0x80);
  s.writecommand(0xB5); s.writedata(0x09); s.writedata(0x09);
  s.writecommand(0xEB); s.writedata(0x01); s.writedata(0x81);
  s.writecommand(0x60); s.writedata(0x38); s.writedata(0x06); s.writedata(0x13); s.writedata(0x56);
  s.writecommand(0x63); s.writedata(0x38); s.writedata(0x08); s.writedata(0x13); s.writedata(0x56);
  s.writecommand(0x61); s.writedata(0x3B); s.writedata(0x1B); s.writedata(0x58); s.writedata(0x38);
  s.writecommand(0x62); s.writedata(0x3B); s.writedata(0x1B); s.writedata(0x58); s.writedata(0x38);
  s.writecommand(0x64); s.writedata(0x38); s.writedata(0x0A); s.writedata(0x73); s.writedata(0x16); s.writedata(0x13); s.writedata(0x56);
  s.writecommand(0x66); s.writedata(0x38); s.writedata(0x0B); s.writedata(0x73); s.writedata(0x17); s.writedata(0x13); s.writedata(0x56);
  s.writecommand(0x68); s.writedata(0x00); s.writedata(0x0B); s.writedata(0x22); s.writedata(0x0B); s.writedata(0x22); s.writedata(0x1C); s.writedata(0x1C);
  s.writecommand(0x69); s.writedata(0x00); s.writedata(0x0B); s.writedata(0x26); s.writedata(0x0B); s.writedata(0x26); s.writedata(0x1C); s.writedata(0x1C);
  s.writecommand(0x6A); s.writedata(0x15); s.writedata(0x00);
  s.writecommand(0x6E); s.writedata(0x08); s.writedata(0x02); s.writedata(0x1A); s.writedata(0x00); s.writedata(0x12); s.writedata(0x12); s.writedata(0x11); s.writedata(0x11); s.writedata(0x14); s.writedata(0x14); s.writedata(0x13); s.writedata(0x13); s.writedata(0x04); s.writedata(0x19); s.writedata(0x1E); s.writedata(0x1D); s.writedata(0x1D); s.writedata(0x1E); s.writedata(0x19); s.writedata(0x04); s.writedata(0x0B); s.writedata(0x0B); s.writedata(0x0C); s.writedata(0x0C); s.writedata(0x09); s.writedata(0x09); s.writedata(0x0A); s.writedata(0x0A); s.writedata(0x00); s.writedata(0x1A); s.writedata(0x01); s.writedata(0x07);
  s.writecommand(0x6C); s.writedata(0xCC); s.writedata(0x0C); s.writedata(0xCC); s.writedata(0x84); s.writedata(0xCC); s.writedata(0x04); s.writedata(0x50);
  s.writecommand(0x7D); s.writedata(0x72);
  s.writecommand(0x70); s.writedata(0x02); s.writedata(0x03); s.writedata(0x09); s.writedata(0x07); s.writedata(0x09); s.writedata(0x03); s.writedata(0x09); s.writedata(0x07); s.writedata(0x09); s.writedata(0x03);
  s.writecommand(0x90); s.writedata(0x06); s.writedata(0x06); s.writedata(0x05); s.writedata(0x06);
  s.writecommand(0x93); s.writedata(0x45); s.writedata(0xFF); s.writedata(0x00);
  s.writecommand(0xC3); s.writedata(0x15);
  s.writecommand(0xC4); s.writedata(0x36);
  s.writecommand(0xC9); s.writedata(0x3D);
  s.writecommand(0xF0); s.writedata(0x47); s.writedata(0x07); s.writedata(0x0A); s.writedata(0x0A); s.writedata(0x00); s.writedata(0x29);
  s.writecommand(0xF2); s.writedata(0x47); s.writedata(0x07); s.writedata(0x0A); s.writedata(0x0A); s.writedata(0x00); s.writedata(0x29);
  s.writecommand(0xF1); s.writedata(0x42); s.writedata(0x91); s.writedata(0x10); s.writedata(0x2D); s.writedata(0x2F); s.writedata(0x6F);
  s.writecommand(0xF3); s.writedata(0x42); s.writedata(0x91); s.writedata(0x10); s.writedata(0x2D); s.writedata(0x2F); s.writedata(0x6F);
  s.writecommand(0xF9); s.writedata(0x30);
  s.writecommand(0xBE); s.writedata(0x11);
  s.writecommand(0xFB); s.writedata(0x00); s.writedata(0x00);
  s.writecommand(0x11); s.delay(1000);
  s.writecommand(0x29); s.delay(20);
}

inline void rotate_GC9B71(PanelSequence &s,uint8_t rotation) { dcsRotate(s,rotation); }

inline ControllerProfile profile_GC9B71() {
  return {"GC9B71",320,386,0x9B71,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0,0,0x08,init_GC9B71,rotate_GC9B71,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class GC9B71 : public Panel {
 public:
  static constexpr bool paddedCommands=false;
  static constexpr uint8_t spiMode=0;
  explicit GC9B71(Transport &bus,int reset=-1,PanelOptions options=PanelOptions())
      : Panel(bus,controllers::profile_GC9B71(),reset,options,2,2) {}
};
}

#include "../../Display/Panel.h"
#include "Common/QspiSetup.h"
namespace nice_gfx {
class GC9B71_QSPI : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit GC9B71_QSPI(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_GC9B71(), reset, options, 2, 2) {}
  template<class Config> static void configureQspi(Config &c) { controllers::configureGramQspi(c,0x03,0,2,false); }
};
}
