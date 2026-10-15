#pragma once
#include "../../Display/ControllerProfile.h"
#include "Common/DcsRotation.h"

namespace nice_gfx { namespace controllers {
inline void init_AXS15231B_QSPI(PanelSequence &s) {
  s.writecommand(0x11); s.delay(100);
  s.writecommand(0x36); s.writedata(s.colorOrder);
  s.writecommand(0x3A); s.writedata(0x55);
  s.writecommand(0xBB); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x5A); s.writedata(0xA5);
  s.writecommand(0xA0); s.writedata(0x00); s.writedata(0x10); s.writedata(0x00); s.writedata(0x02); s.writedata(0x00); s.writedata(0x00); s.writedata(0x64); s.writedata(0x3F); s.writedata(0x20); s.writedata(0x05); s.writedata(0x3F); s.writedata(0x3F); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00);
  s.writecommand(0xA2); s.writedata(0x30); s.writedata(0x04); s.writedata(0x0A); s.writedata(0x3C); s.writedata(0xEC); s.writedata(0x54); s.writedata(0xC4); s.writedata(0x30); s.writedata(0xAC); s.writedata(0x28); s.writedata(0x7F); s.writedata(0x7F); s.writedata(0x7F); s.writedata(0x20); s.writedata(0xF8); s.writedata(0x10); s.writedata(0x02); s.writedata(0xFF); s.writedata(0xFF); s.writedata(0xF0); s.writedata(0x90); s.writedata(0x01); s.writedata(0x32); s.writedata(0xA0); s.writedata(0x91); s.writedata(0xC0); s.writedata(0x20); s.writedata(0x7F); s.writedata(0xFF); s.writedata(0x00); s.writedata(0x54);
  s.writecommand(0xD0); s.writedata(0x30); s.writedata(0xAC); s.writedata(0x21); s.writedata(0x24); s.writedata(0x08); s.writedata(0x09); s.writedata(0x10); s.writedata(0x01); s.writedata(0xAA); s.writedata(0x14); s.writedata(0xC2); s.writedata(0x00); s.writedata(0x22); s.writedata(0x22); s.writedata(0xAA); s.writedata(0x03); s.writedata(0x10); s.writedata(0x12); s.writedata(0x40); s.writedata(0x14); s.writedata(0x1E); s.writedata(0x51); s.writedata(0x15); s.writedata(0x00); s.writedata(0x40); s.writedata(0x10); s.writedata(0x00); s.writedata(0x03); s.writedata(0x3D); s.writedata(0x12);
  s.writecommand(0xA3); s.writedata(0xA0); s.writedata(0x06); s.writedata(0xAA); s.writedata(0x08); s.writedata(0x08); s.writedata(0x02); s.writedata(0x0A); s.writedata(0x04); s.writedata(0x04); s.writedata(0x04); s.writedata(0x04); s.writedata(0x04); s.writedata(0x04); s.writedata(0x04); s.writedata(0x04); s.writedata(0x04); s.writedata(0x04); s.writedata(0x04); s.writedata(0x04); s.writedata(0x00); s.writedata(0x55); s.writedata(0x55);
  s.writecommand(0xC1); s.writedata(0x33); s.writedata(0x04); s.writedata(0x02); s.writedata(0x02); s.writedata(0x71); s.writedata(0x05); s.writedata(0x24); s.writedata(0x55); s.writedata(0x02); s.writedata(0x00); s.writedata(0x41); s.writedata(0x00); s.writedata(0x53); s.writedata(0xFF); s.writedata(0xFF); s.writedata(0xFF); s.writedata(0x4F); s.writedata(0x52); s.writedata(0x00); s.writedata(0x4F); s.writedata(0x52); s.writedata(0x00); s.writedata(0x45); s.writedata(0x3B); s.writedata(0x0B); s.writedata(0x02); s.writedata(0x0D); s.writedata(0x00); s.writedata(0xFF); s.writedata(0x40);
  s.writecommand(0xC3); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x50); s.writedata(0x03); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x01); s.writedata(0x80); s.writedata(0x01);
  s.writecommand(0xC4); s.writedata(0x00); s.writedata(0x24); s.writedata(0x33); s.writedata(0x90); s.writedata(0x50); s.writedata(0xEA); s.writedata(0x64); s.writedata(0x32); s.writedata(0xC8); s.writedata(0x64); s.writedata(0xC8); s.writedata(0x32); s.writedata(0x90); s.writedata(0x90); s.writedata(0x11); s.writedata(0x06); s.writedata(0xDC); s.writedata(0xFA); s.writedata(0x04); s.writedata(0x03); s.writedata(0x80); s.writedata(0xFE); s.writedata(0x10); s.writedata(0x10); s.writedata(0x00); s.writedata(0x0A); s.writedata(0x0A); s.writedata(0x44); s.writedata(0x50);
  s.writecommand(0xC5); s.writedata(0x18); s.writedata(0x00); s.writedata(0x00); s.writedata(0x03); s.writedata(0xFE); s.writedata(0x78); s.writedata(0x33); s.writedata(0x20); s.writedata(0x30); s.writedata(0x10); s.writedata(0x88); s.writedata(0xDE); s.writedata(0x0D); s.writedata(0x08); s.writedata(0x0F); s.writedata(0x0F); s.writedata(0x01); s.writedata(0x78); s.writedata(0x33); s.writedata(0x20); s.writedata(0x10); s.writedata(0x10); s.writedata(0x80);
  s.writecommand(0xC6); s.writedata(0x05); s.writedata(0x0A); s.writedata(0x05); s.writedata(0x0A); s.writedata(0x00); s.writedata(0xE0); s.writedata(0x2E); s.writedata(0x0B); s.writedata(0x12); s.writedata(0x22); s.writedata(0x12); s.writedata(0x22); s.writedata(0x01); s.writedata(0x00); s.writedata(0x00); s.writedata(0x3F); s.writedata(0x6A); s.writedata(0x18); s.writedata(0xC8); s.writedata(0x22);
  s.writecommand(0xC7); s.writedata(0x50); s.writedata(0x32); s.writedata(0x28); s.writedata(0x00); s.writedata(0xA2); s.writedata(0x80); s.writedata(0x8F); s.writedata(0x00); s.writedata(0x80); s.writedata(0xFF); s.writedata(0x07); s.writedata(0x11); s.writedata(0x9F); s.writedata(0x6F); s.writedata(0xFF); s.writedata(0x26); s.writedata(0x0C); s.writedata(0x0D); s.writedata(0x0E); s.writedata(0x0F);
  s.writecommand(0xC9); s.writedata(0x33); s.writedata(0x44); s.writedata(0x44); s.writedata(0x01);
  s.writecommand(0xCF); s.writedata(0x34); s.writedata(0x1E); s.writedata(0x88); s.writedata(0x58); s.writedata(0x13); s.writedata(0x18); s.writedata(0x56); s.writedata(0x18); s.writedata(0x1E); s.writedata(0x68); s.writedata(0xF7); s.writedata(0x00); s.writedata(0x65); s.writedata(0x0C); s.writedata(0x22); s.writedata(0xC4); s.writedata(0x0C); s.writedata(0x77); s.writedata(0x22); s.writedata(0x44); s.writedata(0xAA); s.writedata(0x55); s.writedata(0x04); s.writedata(0x04); s.writedata(0x12); s.writedata(0xA0); s.writedata(0x08);
  s.writecommand(0xD5); s.writedata(0x3E); s.writedata(0x3E); s.writedata(0x88); s.writedata(0x00); s.writedata(0x44); s.writedata(0x04); s.writedata(0x78); s.writedata(0x33); s.writedata(0x20); s.writedata(0x78); s.writedata(0x33); s.writedata(0x20); s.writedata(0x04); s.writedata(0x28); s.writedata(0xD3); s.writedata(0x47); s.writedata(0x03); s.writedata(0x03); s.writedata(0x03); s.writedata(0x03); s.writedata(0x86); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x30); s.writedata(0x52); s.writedata(0x3F); s.writedata(0x40); s.writedata(0x40); s.writedata(0x96);
  s.writecommand(0xD6); s.writedata(0x10); s.writedata(0x32); s.writedata(0x54); s.writedata(0x76); s.writedata(0x98); s.writedata(0xBA); s.writedata(0xDC); s.writedata(0xFE); s.writedata(0x95); s.writedata(0x00); s.writedata(0x01); s.writedata(0x83); s.writedata(0x75); s.writedata(0x36); s.writedata(0x20); s.writedata(0x75); s.writedata(0x36); s.writedata(0x20); s.writedata(0x3F); s.writedata(0x03); s.writedata(0x03); s.writedata(0x03); s.writedata(0x10); s.writedata(0x10); s.writedata(0x00); s.writedata(0x04); s.writedata(0x51); s.writedata(0x20); s.writedata(0x01); s.writedata(0x00);
  s.writecommand(0xD7); s.writedata(0x0A); s.writedata(0x08); s.writedata(0x0E); s.writedata(0x0C); s.writedata(0x1E); s.writedata(0x18); s.writedata(0x19); s.writedata(0x1F); s.writedata(0x00); s.writedata(0x1F); s.writedata(0x1A); s.writedata(0x1F); s.writedata(0x3E); s.writedata(0x3E); s.writedata(0x04); s.writedata(0x00); s.writedata(0x1F); s.writedata(0x1F); s.writedata(0x1F);
  s.writecommand(0xD8); s.writedata(0x0B); s.writedata(0x09); s.writedata(0x0F); s.writedata(0x0D); s.writedata(0x1E); s.writedata(0x18); s.writedata(0x19); s.writedata(0x1F); s.writedata(0x01); s.writedata(0x1F); s.writedata(0x1A); s.writedata(0x1F);
  s.writecommand(0xD9); s.writedata(0x00); s.writedata(0x0D); s.writedata(0x0F); s.writedata(0x09); s.writedata(0x0B); s.writedata(0x1F); s.writedata(0x18); s.writedata(0x19); s.writedata(0x1F); s.writedata(0x01); s.writedata(0x1E); s.writedata(0x1A); s.writedata(0x1F);
  s.writecommand(0xDD); s.writedata(0x0C); s.writedata(0x0E); s.writedata(0x08); s.writedata(0x0A); s.writedata(0x1F); s.writedata(0x18); s.writedata(0x19); s.writedata(0x1F); s.writedata(0x00); s.writedata(0x1E); s.writedata(0x1A); s.writedata(0x1F);
  s.writecommand(0xDF); s.writedata(0x44); s.writedata(0x73); s.writedata(0x4B); s.writedata(0x69); s.writedata(0x00); s.writedata(0x0A); s.writedata(0x02); s.writedata(0x90);
  s.writecommand(0xE0); s.writedata(0x19); s.writedata(0x20); s.writedata(0x0A); s.writedata(0x13); s.writedata(0x0E); s.writedata(0x09); s.writedata(0x12); s.writedata(0x28); s.writedata(0xD4); s.writedata(0x24); s.writedata(0x0C); s.writedata(0x35); s.writedata(0x13); s.writedata(0x31); s.writedata(0x36); s.writedata(0x2F); s.writedata(0x03);
  s.writecommand(0xE1); s.writedata(0x38); s.writedata(0x20); s.writedata(0x09); s.writedata(0x12); s.writedata(0x0E); s.writedata(0x08); s.writedata(0x12); s.writedata(0x28); s.writedata(0xC5); s.writedata(0x24); s.writedata(0x0C); s.writedata(0x34); s.writedata(0x12); s.writedata(0x31); s.writedata(0x36); s.writedata(0x2F); s.writedata(0x27);
  s.writecommand(0xE2); s.writedata(0x19); s.writedata(0x20); s.writedata(0x0A); s.writedata(0x11); s.writedata(0x09); s.writedata(0x06); s.writedata(0x11); s.writedata(0x25); s.writedata(0xD4); s.writedata(0x22); s.writedata(0x0B); s.writedata(0x33); s.writedata(0x12); s.writedata(0x2D); s.writedata(0x32); s.writedata(0x2F); s.writedata(0x03);
  s.writecommand(0xE3); s.writedata(0x38); s.writedata(0x20); s.writedata(0x0A); s.writedata(0x11); s.writedata(0x09); s.writedata(0x06); s.writedata(0x11); s.writedata(0x25); s.writedata(0xC4); s.writedata(0x21); s.writedata(0x0A); s.writedata(0x32); s.writedata(0x11); s.writedata(0x2C); s.writedata(0x32); s.writedata(0x2F); s.writedata(0x27);
  s.writecommand(0xE4); s.writedata(0x19); s.writedata(0x20); s.writedata(0x0D); s.writedata(0x14); s.writedata(0x0D); s.writedata(0x08); s.writedata(0x12); s.writedata(0x2A); s.writedata(0xD4); s.writedata(0x26); s.writedata(0x0E); s.writedata(0x35); s.writedata(0x13); s.writedata(0x34); s.writedata(0x39); s.writedata(0x2F); s.writedata(0x03);
  s.writecommand(0xE5); s.writedata(0x38); s.writedata(0x20); s.writedata(0x0D); s.writedata(0x13); s.writedata(0x0D); s.writedata(0x07); s.writedata(0x12); s.writedata(0x29); s.writedata(0xC4); s.writedata(0x25); s.writedata(0x0D); s.writedata(0x35); s.writedata(0x12); s.writedata(0x33); s.writedata(0x39); s.writedata(0x2F); s.writedata(0x27);
  s.writecommand(0xBB); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00);
  s.writecommand(0x13);
  s.writecommand(0x11); s.delay(200);
  s.writecommand(0x29); s.delay(200);
  s.writecommand(0x2C); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00); s.writedata(0x00);
  s.writecommand(0x22); s.delay(200);
}

inline void rotate_AXS15231B_QSPI(PanelSequence &s,uint8_t rotation) { fixedRotate(s,rotation); }

inline ControllerProfile profile_AXS15231B_QSPI() {
  return {"AXS15231B_QSPI",360,640,0x15231B,Protocol::Dcs,PixelFormat::RGB565,
    0x2C,0,0x01,1,0,0,0x08,init_AXS15231B_QSPI,rotate_AXS15231B_QSPI,PixelReadFormat::RGB888,1,0};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"
#include "../../Display/QspiProtocol.h"
#include "Common/QspiSetup.h"
#include "Common/ColorTransports.h"
namespace nice_gfx { namespace controllers {
struct Axs15231bHolder {
  Axs15231bTransport axsBus;
  Axs15231bHolder(Transport &bus,const PanelOptions &options)
      : axsBus(bus,options.width ? options.width : 360,options.height ? options.height : 640) {}
};
} }
namespace nice_gfx {
class AXS15231B_QSPI : private controllers::Axs15231bHolder, public Panel {
 public:
  static constexpr bool paddedCommands=false;
  static constexpr uint8_t spiMode=3;
  explicit AXS15231B_QSPI(Transport &bus,int reset=-1,PanelOptions options=PanelOptions())
      : Axs15231bHolder(bus,options),Panel(axsBus,controllers::profile_AXS15231B_QSPI(),reset,options,4,1) {}
  template<class Config> static void configureQspi(Config &c) { controllers::configureGramQspi(c,0x0B,3); }
};
}
