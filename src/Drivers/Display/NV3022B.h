#pragma once
#include "../../Display/ControllerProfile.h"
#include "Common/DcsRotation.h"

namespace nice_gfx { namespace controllers {
inline void init_NV3022B(PanelSequence &s) {
  s.writecommand(0x11); s.delay(100);
  s.writecommand(0x36); s.writedata(s.colorOrder);
  s.writecommand(0x3A); s.writedata(0x55);
  s.writecommand(0xFD); s.writedata(0x06); s.writedata(0x08);
  s.writecommand(0x61); s.writedata(0x07); s.writedata(0x07);
  s.writecommand(0x73); s.writedata(0x70);
  s.writecommand(0x73); s.writedata(0x00);
  s.writecommand(0x62); s.writedata(0x00); s.writedata(0x44); s.writedata(0x40);
  s.writecommand(0x65); s.writedata(0x08); s.writedata(0x10); s.writedata(0x21);
  s.writecommand(0x66); s.writedata(0x08); s.writedata(0x10); s.writedata(0x21);
  s.writecommand(0x67); s.writedata(0x21); s.writedata(0x40);
  s.writecommand(0x68); s.writedata(0x9F); s.writedata(0x30); s.writedata(0x27); s.writedata(0x21);
  s.writecommand(0xB1); s.writedata(0x0F); s.writedata(0x02); s.writedata(0x01);
  s.writecommand(0xB4); s.writedata(0x01);
  s.writecommand(0xB5); s.writedata(0x02); s.writedata(0x02); s.writedata(0x0A); s.writedata(0x14);
  s.writecommand(0xB6); s.writedata(0x44); s.writedata(0x01); s.writedata(0x9F); s.writedata(0x00); s.writedata(0x02);
  s.writecommand(0xE6); s.writedata(0x00); s.writedata(0xFF);
  s.writecommand(0xE7); s.writedata(0x01); s.writedata(0x04); s.writedata(0x03); s.writedata(0x03); s.writedata(0x00); s.writedata(0x12);
  s.writecommand(0xE8); s.writedata(0x00); s.writedata(0x70); s.writedata(0x00);
  s.writecommand(0xEC); s.writedata(0x52);
  s.writecommand(0xDF); s.writedata(0x11);
  s.writecommand(0xE0); s.writedata(0x06); s.writedata(0x05); s.writedata(0x0B); s.writedata(0x12); s.writedata(0x10); s.writedata(0x10); s.writedata(0x10); s.writedata(0x15);
  s.writecommand(0xE3); s.writedata(0x15); s.writedata(0x10); s.writedata(0x11); s.writedata(0x0E); s.writedata(0x12); s.writedata(0x0D); s.writedata(0x06); s.writedata(0x06);
  s.writecommand(0xE1); s.writedata(0x35); s.writedata(0x75);
  s.writecommand(0xE4); s.writedata(0x74); s.writedata(0x35);
  s.writecommand(0xE2); s.writedata(0x22); s.writedata(0x22); s.writedata(0x21); s.writedata(0x35); s.writedata(0x36); s.writedata(0x3F);
  s.writecommand(0xE5); s.writedata(0x3F); s.writedata(0x35); s.writedata(0x34); s.writedata(0x21); s.writedata(0x22); s.writedata(0x22);
  s.writecommand(0xF1); s.writedata(0x01); s.writedata(0x01); s.writedata(0x02);
  s.writecommand(0xF6); s.writedata(0x09); s.writedata(0x30); s.writedata(0x00); s.writedata(0x00);
  s.writecommand(0xFD); s.writedata(0xFA); s.writedata(0xFC);
  s.writecommand(0x35); s.writedata(0x00);
  s.writecommand(0x11); s.delay(200);
  s.writecommand(0x2A); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0xEF);
  s.writecommand(0x2B); s.writedata(0x00); s.writedata(0x00); s.writedata(0x01); s.writedata(0x1B);
  s.writecommand(0x29); s.delay(10);
  s.writecommand(0x2C);
}

inline void rotate_NV3022B(PanelSequence &s,uint8_t rotation) { dcsRotate(s,rotation); }

inline ControllerProfile profile_NV3022B() {
  return {"NV3022B",240,284,0x3022B,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0,0,0x08,init_NV3022B,rotate_NV3022B,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class NV3022B : public Panel {
 public:
  static constexpr bool paddedCommands=false;
  static constexpr uint8_t spiMode=0;
  explicit NV3022B(Transport &bus,int reset=-1,PanelOptions options=PanelOptions())
      : Panel(bus,controllers::profile_NV3022B(),reset,options) {}
};
}
