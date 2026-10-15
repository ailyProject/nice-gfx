#pragma once
#include "../../Display/Panel.h"

namespace nice_gfx { namespace controllers {
// OSPTEK AM160Q480480LK reference values, CC BY 4.0, Copyright 2026 OSPTEK.
// https://github.com/osptek/1.6-amoled-480x480-qspi-ch13613
inline PanelConfig ch13613Config() {
  static const InitCommand sequence[] = {
    {0xFE,1,0,{0x00}}, {0xF0,1,0,{0x50}}, {0xB1,2,0,{0x78,0x70}},
    {0xC4,1,0,{0x80}}, {0x36,1,0,{0}}, {0x3A,1,0,{0x55}},
    {0x53,1,0,{0x20}}, {0x51,1,0,{0xFF}}, {0x63,1,0,{0xFF}},
    {0x64,1,0,{0x10}}, {0x67,1,0,{0x01}}, {0x68,1,0,{0x31}},
    {0x2A,4,0,{0,0,1,0xDF}}, {0x2B,4,0,{0,0,1,0xDF}},
    {0x11,0,120,{0}}, {0x29,0,120,{0}}
  };
  return {480,480,0x13613,sequence,sizeof(sequence)/sizeof(sequence[0]),
          {0,0,0,0},{0,0,0,0},{0,0,0,0},1};
}
} } // namespace nice_gfx::controllers

#include "../../Display/Panel.h"
#include "../../Display/QspiProtocol.h"

namespace nice_gfx {
class CH13613 : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  // CH13613 specification v1.0 p162: QSPI register/RAM reads use 03h.
  template<class Config> static void configureQspi(Config &c) {
    c.quad = true; c.mode = spiMode; c.protocol = QspiProtocol();
    c.protocol.readOpcode = 0x03;
  }
  // The module's address windows use pairs of columns and rows. Both origins
  // must be even, and the inclusive right/bottom endpoints must be odd.
  explicit CH13613(Transport &bus, int reset = -1) : Panel(bus, controllers::ch13613Config(), reset, 2, 2) {}
};
} // namespace nice_gfx
