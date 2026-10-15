#pragma once
#include "../../Display/ControllerProfile.h"
#include "Common/DcsRotation.h"

namespace nice_gfx { namespace controllers {
inline void init_PT6891(PanelSequence &s) {
  s.writecommand(0xA5);
  s.writecommand(0x58);
  s.writecommand(0xB5); s.writedata(0x87);
  s.writecommand(0x31);
  s.writecommand(0x2A); s.writedata(0x00); s.writedata(0x01); s.writedata(0x02); s.writedata(0x03); s.writedata(0x04); s.writedata(0x05); s.writedata(0x06); s.writedata(0x07); s.writedata(0x08); s.writedata(0x09); s.writedata(0x0A); s.writedata(0x0B); s.writedata(0x0D); s.writedata(0x0F); s.writedata(0x11); s.writedata(0x13); s.writedata(0x16); s.writedata(0x18); s.writedata(0x1B); s.writedata(0x1D); s.writedata(0x20); s.writedata(0x23); s.writedata(0x26); s.writedata(0x29); s.writedata(0x2C); s.writedata(0x30); s.writedata(0x33); s.writedata(0x37); s.writedata(0x3A); s.writedata(0x3E); s.writedata(0x42); s.writedata(0x46); s.writedata(0x4A); s.writedata(0x4E); s.writedata(0x53); s.writedata(0x57); s.writedata(0x5C); s.writedata(0x60); s.writedata(0x65); s.writedata(0x6A); s.writedata(0x6F); s.writedata(0x74); s.writedata(0x79); s.writedata(0x7E); s.writedata(0x83); s.writedata(0x89); s.writedata(0x8E); s.writedata(0x94); s.writedata(0x9A); s.writedata(0x9F); s.writedata(0xA5); s.writedata(0xAB); s.writedata(0xB1); s.writedata(0xB7); s.writedata(0xBE); s.writedata(0xC4); s.writedata(0xCA); s.writedata(0xD1); s.writedata(0xD8); s.writedata(0xDE); s.writedata(0xE5); s.writedata(0xEC); s.writedata(0xF3); s.writedata(0xFA);
  s.writecommand(0x2B);
  s.writecommand(0x28); s.writedata(0x00); s.writedata(0x01); s.writedata(0x02); s.writedata(0x03); s.writedata(0x04); s.writedata(0x05); s.writedata(0x06); s.writedata(0x07); s.writedata(0x08); s.writedata(0x09); s.writedata(0x0A); s.writedata(0x0B); s.writedata(0x0D); s.writedata(0x0F); s.writedata(0x11); s.writedata(0x13); s.writedata(0x16); s.writedata(0x18); s.writedata(0x1B); s.writedata(0x1D); s.writedata(0x20); s.writedata(0x23); s.writedata(0x26); s.writedata(0x29); s.writedata(0x2C); s.writedata(0x30); s.writedata(0x33); s.writedata(0x37); s.writedata(0x3A); s.writedata(0x3E); s.writedata(0x42); s.writedata(0x46); s.writedata(0x4A); s.writedata(0x4E); s.writedata(0x53); s.writedata(0x57); s.writedata(0x5C); s.writedata(0x60); s.writedata(0x65); s.writedata(0x6A); s.writedata(0x6F); s.writedata(0x74); s.writedata(0x79); s.writedata(0x7E); s.writedata(0x83); s.writedata(0x89); s.writedata(0x8E); s.writedata(0x94); s.writedata(0x9A); s.writedata(0x9F); s.writedata(0xA5); s.writedata(0xAB); s.writedata(0xB1); s.writedata(0xB7); s.writedata(0xBE); s.writedata(0xC4); s.writedata(0xCA); s.writedata(0xD1); s.writedata(0xD8); s.writedata(0xDE); s.writedata(0xE5); s.writedata(0xEC); s.writedata(0xF3); s.writedata(0xFA);
  s.writecommand(0x2B);
  s.writecommand(0x29); s.writedata(0x00); s.writedata(0x01); s.writedata(0x02); s.writedata(0x03); s.writedata(0x04); s.writedata(0x05); s.writedata(0x06); s.writedata(0x07); s.writedata(0x08); s.writedata(0x09); s.writedata(0x0A); s.writedata(0x0B); s.writedata(0x0D); s.writedata(0x0F); s.writedata(0x11); s.writedata(0x13); s.writedata(0x16); s.writedata(0x18); s.writedata(0x1B); s.writedata(0x1D); s.writedata(0x20); s.writedata(0x23); s.writedata(0x26); s.writedata(0x29); s.writedata(0x2C); s.writedata(0x30); s.writedata(0x33); s.writedata(0x37); s.writedata(0x3A); s.writedata(0x3E); s.writedata(0x42); s.writedata(0x46); s.writedata(0x4A); s.writedata(0x4E); s.writedata(0x53); s.writedata(0x57); s.writedata(0x5C); s.writedata(0x60); s.writedata(0x65); s.writedata(0x6A); s.writedata(0x6F); s.writedata(0x74); s.writedata(0x79); s.writedata(0x7E); s.writedata(0x83); s.writedata(0x89); s.writedata(0x8E); s.writedata(0x94); s.writedata(0x9A); s.writedata(0x9F); s.writedata(0xA5); s.writedata(0xAB); s.writedata(0xB1); s.writedata(0xB7); s.writedata(0xBE); s.writedata(0xC4); s.writedata(0xCA); s.writedata(0xD1); s.writedata(0xD8); s.writedata(0xDE); s.writedata(0xE5); s.writedata(0xEC); s.writedata(0xF3); s.writedata(0xFA);
  s.writecommand(0x2B);
  s.writecommand(0xE0); s.writedata(0x1B);
  s.writecommand(0xE1); s.writedata(0x00);
  s.writecommand(0xE2); s.writedata(0x00);
  s.writecommand(0xE6);
  s.writecommand(0x44);
  s.writecommand(0x4C); s.writedata(0x08);
  s.writecommand(0xD8); s.writedata(0x25); s.writedata(0x01);
  s.writecommand(0xE5); s.writedata(0x0A);
  s.writecommand(0xB4); s.writedata(0x00);
  s.writecommand(0xB3);
  s.writecommand(0x6A);
  s.writecommand(0xBC);
  s.writecommand(0xBE); s.writedata(0x08);
  s.writecommand(0xB8); s.writedata(0x2B); s.writedata(0x0E); s.writedata(0x1A);
  s.writecommand(0xC8); s.writedata(0x0A);
  s.writecommand(0xD0); s.writedata(0x00);
  s.writecommand(0x43);
  s.writecommand(0x5B);
}

inline void rotate_PT6891(PanelSequence &s,uint8_t rotation) { ptRotate(s,rotation); }

inline ControllerProfile profile_PT6891() {
  return {"PT6891",95,28,0x6891,Protocol::Dcs,PixelFormat::RGB565,
    0x83,0,0xA5,1,1,0,1,init_PT6891,rotate_PT6891,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"
#include "Common/ColorTransports.h"

namespace nice_gfx {
namespace controllers {
struct Pt6891Holder {
  Pt6891Transport ptBus;
  Pt6891Holder(Transport &bus,const PanelOptions &options)
      : ptBus(bus,options.width ? options.width : 95,options.height ? options.height : 28) {}
};
}
class PT6891 : private controllers::Pt6891Holder, public Panel {
 public:
  static constexpr bool paddedCommands=false;
  static constexpr uint8_t spiMode=0;
  explicit PT6891(Transport &bus,int reset=-1,PanelOptions options=PanelOptions())
      : Pt6891Holder(bus,options),Panel(ptBus,controllers::profile_PT6891(),reset,options) {}
};
} // namespace nice_gfx
