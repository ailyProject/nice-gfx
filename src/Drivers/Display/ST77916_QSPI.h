#pragma once
#include "../../Display/ControllerProfile.h"
#include "Common/DcsRotation.h"

namespace nice_gfx { namespace controllers {
inline void init_ST77916_QSPI(PanelSequence &s) {
  s.writecommand(0x36); s.writedata(s.colorOrder);
  s.writecommand(0x3A); s.writedata(0x55);
  s.writecommand(0xF0); s.writedata(0x08);
  s.writecommand(0xF2); s.writedata(0x08);
  s.writecommand(0x9B); s.writedata(0x51);
  s.writecommand(0x86); s.writedata(0x53);
  s.writecommand(0xF2); s.writedata(0x80);
  s.writecommand(0xF0); s.writedata(0x00);
  s.writecommand(0xF0); s.writedata(0x01);
  s.writecommand(0xF1); s.writedata(0x01);
  s.writecommand(0xB0); s.writedata(0x54);
  s.writecommand(0xB1); s.writedata(0x3F);
  s.writecommand(0xB2); s.writedata(0x2A);
  s.writecommand(0xB4); s.writedata(0x46);
  s.writecommand(0xB5); s.writedata(0x34);
  s.writecommand(0xB6); s.writedata(0xD5);
  s.writecommand(0xB7); s.writedata(0x30);
  s.writecommand(0xB8); s.writedata(0x04);
  s.writecommand(0xBA); s.writedata(0x00);
  s.writecommand(0xBB); s.writedata(0x08);
  s.writecommand(0xBC); s.writedata(0x08);
  s.writecommand(0xBD); s.writedata(0x00);
  s.writecommand(0xC0); s.writedata(0x80);
  s.writecommand(0xC1); s.writedata(0x10);
  s.writecommand(0xC2); s.writedata(0x37);
  s.writecommand(0xC3); s.writedata(0x80);
  s.writecommand(0xC4); s.writedata(0x10);
  s.writecommand(0xC5); s.writedata(0x37);
  s.writecommand(0xC6); s.writedata(0xA9);
  s.writecommand(0xC7); s.writedata(0x41);
  s.writecommand(0xC8); s.writedata(0x51);
  s.writecommand(0xC9); s.writedata(0xA9);
  s.writecommand(0xCA); s.writedata(0x41);
  s.writecommand(0xCB); s.writedata(0x51);
  s.writecommand(0xD0); s.writedata(0x91);
  s.writecommand(0xD1); s.writedata(0x68);
  s.writecommand(0xD2); s.writedata(0x69);
  s.writecommand(0xF5); s.writedata(0x00); s.writedata(0xA5);
  s.writecommand(0xDD); s.writedata(0x35);
  s.writecommand(0xDE); s.writedata(0x35);
  s.writecommand(0xF1); s.writedata(0x10);
  s.writecommand(0xF0); s.writedata(0x00);
  s.writecommand(0xF0); s.writedata(0x02);
  s.writecommand(0xE0); s.writedata(0x70); s.writedata(0x09); s.writedata(0x12); s.writedata(0x0C); s.writedata(0x0B); s.writedata(0x27); s.writedata(0x38); s.writedata(0x54); s.writedata(0x4E); s.writedata(0x19); s.writedata(0x15); s.writedata(0x15); s.writedata(0x2C); s.writedata(0x2F);
  s.writecommand(0xE1); s.writedata(0x70); s.writedata(0x08); s.writedata(0x11); s.writedata(0x0C); s.writedata(0x0B); s.writedata(0x27); s.writedata(0x38); s.writedata(0x43); s.writedata(0x4C); s.writedata(0x18); s.writedata(0x14); s.writedata(0x14); s.writedata(0x2B); s.writedata(0x2D);
  s.writecommand(0xF0); s.writedata(0x10);
  s.writecommand(0xF3); s.writedata(0x10);
  s.writecommand(0xE0); s.writedata(0x0A);
  s.writecommand(0xE1); s.writedata(0x00);
  s.writecommand(0xE2); s.writedata(0x0B);
  s.writecommand(0xE3); s.writedata(0x00);
  s.writecommand(0xE4); s.writedata(0xE0);
  s.writecommand(0xE5); s.writedata(0x06);
  s.writecommand(0xE6); s.writedata(0x21);
  s.writecommand(0xE7); s.writedata(0x00);
  s.writecommand(0xE8); s.writedata(0x05);
  s.writecommand(0xE9); s.writedata(0x82);
  s.writecommand(0xEA); s.writedata(0xDF);
  s.writecommand(0xEB); s.writedata(0x89);
  s.writecommand(0xEC); s.writedata(0x20);
  s.writecommand(0xED); s.writedata(0x14);
  s.writecommand(0xEE); s.writedata(0xFF);
  s.writecommand(0xEF); s.writedata(0x00);
  s.writecommand(0xF8); s.writedata(0xFF);
  s.writecommand(0xF9); s.writedata(0x00);
  s.writecommand(0xFA); s.writedata(0x00);
  s.writecommand(0xFB); s.writedata(0x30);
  s.writecommand(0xFC); s.writedata(0x00);
  s.writecommand(0xFD); s.writedata(0x00);
  s.writecommand(0xFE); s.writedata(0x00);
  s.writecommand(0xFF); s.writedata(0x00);
  s.writecommand(0x60); s.writedata(0x42);
  s.writecommand(0x61); s.writedata(0xE0);
  s.writecommand(0x62); s.writedata(0x40);
  s.writecommand(0x63); s.writedata(0x40);
  s.writecommand(0x64); s.writedata(0x02);
  s.writecommand(0x65); s.writedata(0x00);
  s.writecommand(0x66); s.writedata(0x40);
  s.writecommand(0x67); s.writedata(0x03);
  s.writecommand(0x68); s.writedata(0x00);
  s.writecommand(0x69); s.writedata(0x00);
  s.writecommand(0x6A); s.writedata(0x00);
  s.writecommand(0x6B); s.writedata(0x00);
  s.writecommand(0x70); s.writedata(0x42);
  s.writecommand(0x71); s.writedata(0xE0);
  s.writecommand(0x72); s.writedata(0x40);
  s.writecommand(0x73); s.writedata(0x40);
  s.writecommand(0x74); s.writedata(0x02);
  s.writecommand(0x75); s.writedata(0x00);
  s.writecommand(0x76); s.writedata(0x40);
  s.writecommand(0x77); s.writedata(0x03);
  s.writecommand(0x78); s.writedata(0x00);
  s.writecommand(0x79); s.writedata(0x00);
  s.writecommand(0x7A); s.writedata(0x00);
  s.writecommand(0x7B); s.writedata(0x00);
  s.writecommand(0x80); s.writedata(0x38);
  s.writecommand(0x81); s.writedata(0x00);
  s.writecommand(0x82); s.writedata(0x04);
  s.writecommand(0x83); s.writedata(0x02);
  s.writecommand(0x84); s.writedata(0xDC);
  s.writecommand(0x85); s.writedata(0x00);
  s.writecommand(0x86); s.writedata(0x00);
  s.writecommand(0x87); s.writedata(0x00);
  s.writecommand(0x88); s.writedata(0x38);
  s.writecommand(0x89); s.writedata(0x00);
  s.writecommand(0x8A); s.writedata(0x06);
  s.writecommand(0x8B); s.writedata(0x02);
  s.writecommand(0x8C); s.writedata(0xDE);
  s.writecommand(0x8D); s.writedata(0x00);
  s.writecommand(0x8E); s.writedata(0x00);
  s.writecommand(0x8F); s.writedata(0x00);
  s.writecommand(0x90); s.writedata(0x38);
  s.writecommand(0x91); s.writedata(0x00);
  s.writecommand(0x92); s.writedata(0x08);
  s.writecommand(0x93); s.writedata(0x02);
  s.writecommand(0x94); s.writedata(0xE0);
  s.writecommand(0x95); s.writedata(0x00);
  s.writecommand(0x96); s.writedata(0x00);
  s.writecommand(0x97); s.writedata(0x00);
  s.writecommand(0x98); s.writedata(0x38);
  s.writecommand(0x99); s.writedata(0x00);
  s.writecommand(0x9A); s.writedata(0x0A);
  s.writecommand(0x9B); s.writedata(0x02);
  s.writecommand(0x9C); s.writedata(0xE2);
  s.writecommand(0x9D); s.writedata(0x00);
  s.writecommand(0x9E); s.writedata(0x00);
  s.writecommand(0x9F); s.writedata(0x00);
  s.writecommand(0xA0); s.writedata(0x38);
  s.writecommand(0xA1); s.writedata(0x00);
  s.writecommand(0xA2); s.writedata(0x03);
  s.writecommand(0xA3); s.writedata(0x02);
  s.writecommand(0xA4); s.writedata(0xDB);
  s.writecommand(0xA5); s.writedata(0x00);
  s.writecommand(0xA6); s.writedata(0x00);
  s.writecommand(0xA7); s.writedata(0x00);
  s.writecommand(0xA8); s.writedata(0x38);
  s.writecommand(0xA9); s.writedata(0x00);
  s.writecommand(0xAA); s.writedata(0x05);
  s.writecommand(0xAB); s.writedata(0x02);
  s.writecommand(0xAC); s.writedata(0xDD);
  s.writecommand(0xAD); s.writedata(0x00);
  s.writecommand(0xAE); s.writedata(0x00);
  s.writecommand(0xAF); s.writedata(0x00);
  s.writecommand(0xB0); s.writedata(0x38);
  s.writecommand(0xB1); s.writedata(0x00);
  s.writecommand(0xB2); s.writedata(0x07);
  s.writecommand(0xB3); s.writedata(0x02);
  s.writecommand(0xB4); s.writedata(0xDF);
  s.writecommand(0xB5); s.writedata(0x00);
  s.writecommand(0xB6); s.writedata(0x00);
  s.writecommand(0xB7); s.writedata(0x00);
  s.writecommand(0xB8); s.writedata(0x38);
  s.writecommand(0xB9); s.writedata(0x00);
  s.writecommand(0xBA); s.writedata(0x09);
  s.writecommand(0xBB); s.writedata(0x02);
  s.writecommand(0xBC); s.writedata(0xE1);
  s.writecommand(0xBD); s.writedata(0x00);
  s.writecommand(0xBE); s.writedata(0x00);
  s.writecommand(0xBF); s.writedata(0x00);
  s.writecommand(0xC0); s.writedata(0x22);
  s.writecommand(0xC1); s.writedata(0xAA);
  s.writecommand(0xC2); s.writedata(0x65);
  s.writecommand(0xC3); s.writedata(0x74);
  s.writecommand(0xC4); s.writedata(0x47);
  s.writecommand(0xC5); s.writedata(0x56);
  s.writecommand(0xC6); s.writedata(0x00);
  s.writecommand(0xC7); s.writedata(0x88);
  s.writecommand(0xC8); s.writedata(0x99);
  s.writecommand(0xC9); s.writedata(0x33);
  s.writecommand(0xD0); s.writedata(0x11);
  s.writecommand(0xD1); s.writedata(0xAA);
  s.writecommand(0xD2); s.writedata(0x65);
  s.writecommand(0xD3); s.writedata(0x74);
  s.writecommand(0xD4); s.writedata(0x47);
  s.writecommand(0xD5); s.writedata(0x56);
  s.writecommand(0xD6); s.writedata(0x00);
  s.writecommand(0xD7); s.writedata(0x88);
  s.writecommand(0xD8); s.writedata(0x99);
  s.writecommand(0xD9); s.writedata(0x33);
  s.writecommand(0xF3); s.writedata(0x01);
  s.writecommand(0xF0); s.writedata(0x00);
  s.writecommand(0xF0); s.writedata(0x01);
  s.writecommand(0xF1); s.writedata(0x01);
  s.writecommand(0xA0); s.writedata(0x0B);
  s.writecommand(0xA3); s.writedata(0x2A);
  s.writecommand(0xA5); s.writedata(0xC3); s.delay(1);
  s.writecommand(0xA3); s.writedata(0x2B);
  s.writecommand(0xA5); s.writedata(0xC3); s.delay(1);
  s.writecommand(0xA3); s.writedata(0x2C);
  s.writecommand(0xA5); s.writedata(0xC3); s.delay(1);
  s.writecommand(0xA3); s.writedata(0x2D);
  s.writecommand(0xA5); s.writedata(0xC3); s.delay(1);
  s.writecommand(0xA3); s.writedata(0x2E);
  s.writecommand(0xA5); s.writedata(0xC3); s.delay(1);
  s.writecommand(0xA3); s.writedata(0x2F);
  s.writecommand(0xA5); s.writedata(0xC3); s.delay(1);
  s.writecommand(0xA3); s.writedata(0x30);
  s.writecommand(0xA5); s.writedata(0xC3); s.delay(1);
  s.writecommand(0xA3); s.writedata(0x31);
  s.writecommand(0xA5); s.writedata(0xC3); s.delay(1);
  s.writecommand(0xA3); s.writedata(0x32);
  s.writecommand(0xA5); s.writedata(0xC3); s.delay(1);
  s.writecommand(0xA3); s.writedata(0x33);
  s.writecommand(0xA5); s.writedata(0xC3); s.delay(1);
  s.writecommand(0xA0); s.writedata(0x09);
  s.writecommand(0xF1); s.writedata(0x10);
  s.writecommand(0xF0); s.writedata(0x00);
  s.writecommand(0x2A); s.writedata(0x00); s.writedata(0x00); s.writedata(0x01); s.writedata(0x67);
  s.writecommand(0x2B); s.writedata(0x01); s.writedata(0x68); s.writedata(0x01); s.writedata(0x68);
  s.writecommand(0x4D); s.writedata(0x00);
  s.writecommand(0x4E); s.writedata(0x00);
  s.writecommand(0x4F); s.writedata(0x00);
  s.writecommand(0x4C); s.writedata(0x01); s.delay(10);
  s.writecommand(0x4C); s.writedata(0x00);
  s.writecommand(0x2A); s.writedata(0x00); s.writedata(0x00); s.writedata(0x01); s.writedata(0x67);
  s.writecommand(0x2B); s.writedata(0x00); s.writedata(0x00); s.writedata(0x01); s.writedata(0x67);
  s.writecommand(0x21); s.writedata(0x00);
  s.writecommand(0x11); s.writedata(0x00); s.delay(120);
  s.writecommand(0x29); s.delay(20);
}

inline void rotate_ST77916_QSPI(PanelSequence &s,uint8_t rotation) { dcsRotate(s,rotation); }

inline ControllerProfile profile_ST77916_QSPI() {
  return {"ST77916_QSPI",360,360,0x77916,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0,0,0x08,init_ST77916_QSPI,rotate_ST77916_QSPI,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"
#include "Common/QspiSetup.h"
namespace nice_gfx {
class ST77916_QSPI : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit ST77916_QSPI(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ST77916_QSPI(), reset, options, 1, 1) {}
  template<class Config> static void configureQspi(Config &c) { controllers::configureGramQspi(c,0x0B,0,1,false); }
};
}
