#pragma once
#include "../../Display/ControllerProfile.h"
#include "Common/DcsRotation.h"

namespace nice_gfx { namespace controllers {
inline void init_JD9853(PanelSequence &s) {
  s.writecommand(0xDF); s.writedata(0x98); s.writedata(0x53);
  s.writecommand(0xDE); s.writedata(0x00);
  s.writecommand(0xB2); s.writedata(0x25);
  s.writecommand(0xB7); s.writedata(0x00); s.writedata(0x21); s.writedata(0x00); s.writedata(0x49);
  s.writecommand(0xBB); s.writedata(0x1F); s.writedata(0x9A); s.writedata(0x55); s.writedata(0x73); s.writedata(0x63); s.writedata(0xF0);
  s.writecommand(0xC0); s.writedata(0x22); s.writedata(0x22);
  s.writecommand(0xC1); s.writedata(0x12);
  s.writecommand(0xC3); s.writedata(0x7D); s.writedata(0x07); s.writedata(0x14); s.writedata(0x06); s.writedata(0xC8); s.writedata(0x6A); s.writedata(0x6C); s.writedata(0x77);
  s.writecommand(0xC4); s.writedata(0x00); s.writedata(0x00); s.writedata(0xA0); s.writedata(0x6F); s.writedata(0x1E); s.writedata(0x1A); s.writedata(0x16); s.writedata(0x79); s.writedata(0x1E); s.writedata(0x1A); s.writedata(0x16); s.writedata(0x82);
  s.writecommand(0xC8); s.writedata(0x3F); s.writedata(0x2C); s.writedata(0x26); s.writedata(0x20); s.writedata(0x25); s.writedata(0x26); s.writedata(0x21); s.writedata(0x21); s.writedata(0x1F); s.writedata(0x1F); s.writedata(0x1F); s.writedata(0x13); s.writedata(0x11); s.writedata(0x0B); s.writedata(0x04); s.writedata(0x00); s.writedata(0x3F); s.writedata(0x2C); s.writedata(0x26); s.writedata(0x20); s.writedata(0x25); s.writedata(0x27); s.writedata(0x21); s.writedata(0x21); s.writedata(0x1F); s.writedata(0x1F); s.writedata(0x1F); s.writedata(0x13); s.writedata(0x11); s.writedata(0x0B); s.writedata(0x04); s.writedata(0x00);
  s.writecommand(0xD0); s.writedata(0x04); s.writedata(0x06); s.writedata(0x62); s.writedata(0x0F); s.writedata(0x00);
  s.writecommand(0xD7); s.writedata(0x00); s.writedata(0x30);
  s.writecommand(0xE6); s.writedata(0x14);
  s.writecommand(0xDE); s.writedata(0x01);
  s.writecommand(0xB7); s.writedata(0x03); s.writedata(0x13); s.writedata(0xEF); s.writedata(0x35); s.writedata(0x35);
  s.writecommand(0xC1); s.writedata(0x14); s.writedata(0x15); s.writedata(0xC0);
  s.writecommand(0xC2); s.writedata(0x06); s.writedata(0x3A); s.writedata(0xC7);
  s.writecommand(0xC4); s.writedata(0x72); s.writedata(0x12);
  s.writecommand(0xBE); s.writedata(0x00);
  s.writecommand(0xDE); s.writedata(0x00);
  s.writecommand(0x35); s.writedata(0x00);
  s.writecommand(0x36); s.writedata(0x00);
  s.writecommand(0x3A); s.writedata(0x05);
  s.writecommand(0x2A); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0xEF);
  s.writecommand(0x2B); s.writedata(0x00); s.writedata(0x00); s.writedata(0x01); s.writedata(0x1B);
  s.writecommand(0x11); s.delay(120);
  s.writecommand(0x29); s.delay(50);
}

inline void rotate_JD9853(PanelSequence &s,uint8_t rotation) { dcsRotate(s,rotation); }

inline ControllerProfile profile_JD9853() {
  return {"JD9853",240,284,0x9853,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0x2E,0x01,4,0,0,0x08,init_JD9853,rotate_JD9853,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"

namespace nice_gfx {
class JD9853 : public Panel {
 public:
  static constexpr bool paddedCommands=false;
  static constexpr uint8_t spiMode=0;
  explicit JD9853(Transport &bus,int reset=-1,PanelOptions options=PanelOptions())
      : Panel(bus,controllers::profile_JD9853(),reset,options) {}
};
}
