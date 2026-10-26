#pragma once
#include "../../../Display/Backends/MonoPanel.h"
#include "../../../Display/ControllerProfile.h"
namespace nice_gfx {
class SH1106 : public MonoPanel {
  static MonoPanelConfig config(PanelOptions options){MonoPanelConfig c;c.controller=MonoController::SH1106;
    c.width=options.width?options.width:128;c.height=options.height?options.height:64;c.columnOffset=2;return c;}
 public:
  static constexpr uint8_t spiMode=0;
  static constexpr bool paddedCommands=false;
  static constexpr bool byteOnly=true;
  static constexpr uint32_t spiFrequency=4000000;
  SH1106(Transport&bus,int reset=-1,PanelOptions options=PanelOptions()):MonoPanel(bus,config(options),reset){}
};
}
