#pragma once
#include "../../../Display/Backends/MonoPanel.h"
#include "../../../Display/ControllerProfile.h"
namespace nice_gfx {
class SSD1306 : public MonoPanel {
  static MonoPanelConfig config(PanelOptions options){MonoPanelConfig c;c.width=options.width?options.width:128;c.height=options.height?options.height:64;return c;}
 public:
  static constexpr uint8_t spiMode=0;
  static constexpr bool paddedCommands=false;
  static constexpr bool byteOnly=true;
  static constexpr uint32_t spiFrequency=8000000;
  SSD1306(Transport&bus,int reset=-1,PanelOptions options=PanelOptions()):MonoPanel(bus,config(options),reset){}
};
class SSD1306_32 : public SSD1306 {
  static PanelOptions options32(PanelOptions options){if(!options.height)options.height=32;return options;}
 public:SSD1306_32(Transport&bus,int reset=-1,PanelOptions options=PanelOptions()):SSD1306(bus,reset,options32(options)){}
};
}
