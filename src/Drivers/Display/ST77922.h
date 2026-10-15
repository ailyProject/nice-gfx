#pragma once
#include "../../Display/ControllerProfile.h"
#include "Common/DcsRotation.h"

namespace nice_gfx { namespace controllers {
inline void init_ST77922(PanelSequence &s) {
  s.writecommand(0x36); s.writedata(s.colorOrder);
  s.writecommand(0x3A); s.writedata(0x55);
  s.writecommand(0x28);
  s.writecommand(0x10);
  s.writecommand(0x2A); s.writedata(0x00); s.writedata(0x00); s.writedata(0x02); s.writedata(0x13);
  s.writecommand(0x2B); s.writedata(0x00); s.writedata(0x00); s.writedata(0x01); s.writedata(0x2B);
  s.writecommand(0xD0); s.writedata(0x80);
  s.writecommand(0xF1);
  s.writecommand(0x60); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00);
  s.writecommand(0x65); s.writedata(0x00);
  s.writecommand(0x66); s.writedata(0x00); s.writedata(0x3F);
  s.writecommand(0xBE); s.writedata(0x1E); s.writedata(0x01); s.writedata(0x06);
  s.writecommand(0x70); s.writedata(0x02); s.writedata(0x7D); s.writedata(0x12); s.writedata(0x14); s.writedata(0x30); s.writedata(0x00); s.writedata(0x07); s.writedata(0x52); s.writedata(0x01); s.writedata(0x00); s.writedata(0x00); s.writedata(0x1A);
  s.writecommand(0x71); s.writedata(0xD0);
  s.writecommand(0x7B); s.writedata(0x00); s.writedata(0x08); s.writedata(0x08);
  s.writecommand(0x80); s.writedata(0x55); s.writedata(0x62); s.writedata(0x2F); s.writedata(0x17); s.writedata(0xF0); s.writedata(0x52); s.writedata(0x70); s.writedata(0xD2); s.writedata(0x52); s.writedata(0x62); s.writedata(0xEA);
  s.writecommand(0x81); s.writedata(0x26); s.writedata(0x52); s.writedata(0x72); s.writedata(0x27);
  s.writecommand(0x84); s.writedata(0x92); s.writedata(0x25);
  s.writecommand(0x86); s.writedata(0xC6); s.writedata(0x04); s.writedata(0xB1); s.writedata(0x02); s.writedata(0x58); s.writedata(0x12); s.writedata(0x58); s.writedata(0x10); s.writedata(0x13); s.writedata(0x01); s.writedata(0xAA); s.writedata(0x00); s.writedata(0xAA); s.writedata(0xAA);
  s.writecommand(0x87); s.writedata(0x10); s.writedata(0x10); s.writedata(0x58); s.writedata(0x00); s.writedata(0x02); s.writedata(0x3A);
  s.writecommand(0x88); s.writedata(0x00); s.writedata(0x00); s.writedata(0x2C); s.writedata(0x10); s.writedata(0x04); s.writedata(0x00); s.writedata(0x01); s.writedata(0x01); s.writedata(0x01); s.writedata(0x01); s.writedata(0x01); s.writedata(0x01); s.writedata(0x01); s.writedata(0x00); s.writedata(0x06);
  s.writecommand(0x89); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00);
  s.writecommand(0x8A); s.writedata(0x13); s.writedata(0x00); s.writedata(0x2C); s.writedata(0x00); s.writedata(0x00); s.writedata(0x2C); s.writedata(0x10); s.writedata(0x10); s.writedata(0x00); s.writedata(0x3E); s.writedata(0x19);
  s.writecommand(0x8B); s.writedata(0x15); s.writedata(0xB1); s.writedata(0xB1); s.writedata(0x44); s.writedata(0x96); s.writedata(0x2C); s.writedata(0x10); s.writedata(0x97); s.writedata(0x8E);
  s.writecommand(0x8C); s.writedata(0x1D); s.writedata(0xB1); s.writedata(0xB1); s.writedata(0x44); s.writedata(0x96); s.writedata(0x2C); s.writedata(0x10); s.writedata(0x50); s.writedata(0x0F); s.writedata(0x01); s.writedata(0xC5); s.writedata(0x12); s.writedata(0x09);
  s.writecommand(0x8D); s.writedata(0x0C);
  s.writecommand(0x8E); s.writedata(0x33); s.writedata(0x01); s.writedata(0x0C); s.writedata(0x13); s.writedata(0x01); s.writedata(0x01);
  s.writecommand(0x90); s.writedata(0x00); s.writedata(0x44); s.writedata(0x33); s.writedata(0x36); s.writedata(0x00); s.writedata(0x79); s.writedata(0x40); s.writedata(0xB6); s.writedata(0xB6);
  s.writecommand(0x91); s.writedata(0x00); s.writedata(0x44); s.writedata(0x33); s.writedata(0x37); s.writedata(0x00); s.writedata(0x78); s.writedata(0x40); s.writedata(0xB6); s.writedata(0xB6);
  s.writecommand(0x92); s.writedata(0x02); s.writedata(0x44); s.writedata(0x55); s.writedata(0x82); s.writedata(0x86); s.writedata(0x2F); s.writedata(0x00); s.writedata(0x04); s.writedata(0x73); s.writedata(0xB6);
  s.writecommand(0x93); s.writedata(0x0C); s.writedata(0x00); s.writedata(0x11); s.writedata(0x81); s.writedata(0x87); s.writedata(0x3F); s.writedata(0x00); s.writedata(0x00); s.writedata(0x73); s.writedata(0x73);
  s.writecommand(0x94); s.writedata(0x08); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00);
  s.writecommand(0x95); s.writedata(0x1A); s.writedata(0x1A); s.writedata(0x00); s.writedata(0x00); s.writedata(0xFF);
  s.writecommand(0x96); s.writedata(0x44); s.writedata(0x35); s.writedata(0x07); s.writedata(0x16); s.writedata(0x20); s.writedata(0x21); s.writedata(0x07); s.writedata(0x06); s.writedata(0xB6); s.writedata(0xB6); s.writedata(0x00); s.writedata(0x40);
  s.writecommand(0x97); s.writedata(0x44); s.writedata(0x35); s.writedata(0x25); s.writedata(0x34); s.writedata(0x22); s.writedata(0x23); s.writedata(0x05); s.writedata(0x04); s.writedata(0xB6); s.writedata(0xB6); s.writedata(0x00); s.writedata(0x40);
  s.writecommand(0xBA); s.writedata(0x55); s.writedata(0xB6); s.writedata(0xB6); s.writedata(0xB6); s.writedata(0xB6);
  s.writecommand(0x9A); s.writedata(0x40); s.writedata(0x0C); s.writedata(0x56); s.writedata(0x01); s.writedata(0x08); s.writedata(0xB6); s.writedata(0xB6);
  s.writecommand(0x9B); s.writedata(0x00); s.writedata(0x00); s.writedata(0x06); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00);
  s.writecommand(0x9C); s.writedata(0x00); s.writedata(0x12); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x12); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x12); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00);
  s.writecommand(0x9D); s.writedata(0x80); s.writedata(0x15); s.writedata(0x00); s.writedata(0x07); s.writedata(0x01); s.writedata(0x80); s.writedata(0x73); s.writedata(0x73);
  s.writecommand(0x9E); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x80); s.writedata(0x1E); s.writedata(0x01);
  s.writecommand(0x9F); s.writedata(0xA0); s.writedata(0x09); s.writedata(0x00); s.writedata(0x57);
  s.writecommand(0xB3); s.writedata(0x00); s.writedata(0x30); s.writedata(0x0F); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00);
  s.writecommand(0xB4); s.writedata(0x10); s.writedata(0x1C); s.writedata(0x19); s.writedata(0x14); s.writedata(0x18); s.writedata(0x01); s.writedata(0x1D); s.writedata(0x03); s.writedata(0x12); s.writedata(0x0A); s.writedata(0x11); s.writedata(0x08);
  s.writecommand(0xB5); s.writedata(0x1C); s.writedata(0x1C); s.writedata(0x19); s.writedata(0x14); s.writedata(0x18); s.writedata(0x00); s.writedata(0x1D); s.writedata(0x02); s.writedata(0x10); s.writedata(0x0B); s.writedata(0x13); s.writedata(0x09);
  s.writecommand(0xB6); s.writedata(0xFF); s.writedata(0xFF); s.writedata(0x00); s.writedata(0x0F); s.writedata(0xFE); s.writedata(0x0F); s.writedata(0xFE);
  s.writecommand(0xB7); s.writedata(0x00); s.writedata(0x09); s.writedata(0x10); s.writedata(0x0B); s.writedata(0x0A); s.writedata(0x06); s.writedata(0x38); s.writedata(0x04); s.writedata(0x04); s.writedata(0x4F); s.writedata(0x09); s.writedata(0x15); s.writedata(0x15); s.writedata(0x30); s.writedata(0x37); s.writedata(0x0F);
  s.writecommand(0xB8); s.writedata(0x00); s.writedata(0x09); s.writedata(0x0F); s.writedata(0x0A); s.writedata(0x09); s.writedata(0x05); s.writedata(0x37); s.writedata(0x03); s.writedata(0x03); s.writedata(0x4F); s.writedata(0x09); s.writedata(0x15); s.writedata(0x15); s.writedata(0x31); s.writedata(0x36); s.writedata(0x0F);
  s.writecommand(0xB9); s.writedata(0x23); s.writedata(0x23);
  s.writecommand(0xBF); s.writedata(0x0F); s.writedata(0x13); s.writedata(0x13); s.writedata(0x09); s.writedata(0x09); s.writedata(0x09);
  s.writecommand(0xF2);
  s.writecommand(0x73); s.writedata(0x04); s.writedata(0xBA); s.writedata(0x1A); s.writedata(0x58); s.writedata(0x5B);
  s.writecommand(0x77); s.writedata(0x6B); s.writedata(0x5B); s.writedata(0xFB); s.writedata(0xC3); s.writedata(0xC5);
  s.writecommand(0x7A); s.writedata(0x15); s.writedata(0x27);
  s.writecommand(0x7B); s.writedata(0x04); s.writedata(0x57);
  s.writecommand(0x7E); s.writedata(0x01); s.writedata(0x0E);
  s.writecommand(0xBF); s.writedata(0x36);
  s.writecommand(0xE3); s.writedata(0x43); s.writedata(0x43);
  s.writecommand(0xF0);
  s.writecommand(0x21); s.writedata(0x00);
  s.writecommand(0x11); s.writedata(0x00); s.delay(120);
  s.writecommand(0x35); s.writedata(0x00); s.delay(20);
  s.writecommand(0x29); s.delay(20);
}

inline void rotate_ST77922(PanelSequence &s,uint8_t rotation) { fixedRotate(s,rotation); }

inline ControllerProfile profile_ST77922() {
  return {"ST77922",532,300,0x77922,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,1,0,0,0x08,init_ST77922,rotate_ST77922,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class ST77922 : public Panel {
 public:
  static constexpr bool paddedCommands=false;
  static constexpr uint8_t spiMode=0;
  explicit ST77922(Transport &bus,int reset=-1,PanelOptions options=PanelOptions())
      : Panel(bus,controllers::profile_ST77922(),reset,options,4,1) {}
};
}

#include "../../Display/Panel.h"
#include "Common/QspiSetup.h"
namespace nice_gfx {
class ST77922_QSPI : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit ST77922_QSPI(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ST77922(), reset, options, 4, 1) {}
  template<class Config> static void configureQspi(Config &c) { controllers::configureGramQspi(c,0x0B,0,4,false); }
};
}
