#pragma once
#ifndef NICE_GFX_UI_HEADER
#define NICE_GFX_UI_HEADER <UI/View.h>
#endif
#ifndef NICE_GFX_FONTS_HEADER
#define NICE_GFX_FONTS_HEADER <Fonts/Builtin.h>
#endif
#include NICE_GFX_UI_HEADER
#include <Display/Presets/All.h>

// Change these values in the designer's hardware panel, then export again.
// Board: esp32:esp32:esp_mosaico:USBMode=default,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB; display: CO5300_480x480_QSPI; rotation: 0.
inline DisplayOptions displayOptions() {
  DisplayOptions options;
  
  return options;
}
// ESP-Mosaico's 480x480 glass requires its own CO5300 vendor program.
// https://github.com/esp-mosaico/esp-mosaico-bsp/blob/master/components/esp-mosaico-bsp/onboard/display.c
class MosaicoCO5300 : public nice_gfx::Panel {
  static void initialize(nice_gfx::PanelSequence &s) {
    s.writecommand(0x11); s.delay(600);
    s.writecommand(0xFE); s.writedata(0x20);
    s.writecommand(0x19); s.writedata(0x10);
    s.writecommand(0x1C); s.writedata(0xA0);
    s.writecommand(0xFE); s.writedata(0x00);
    s.writecommand(0xC4); s.writedata(0x80);
    s.writecommand(0x3A); s.writedata(0x55);
    s.writecommand(0x35); s.writedata(0x00);
    s.writecommand(0x53); s.writedata(0x20);
    s.writecommand(0x51); s.writedata(0xFF);
    s.writecommand(0x63); s.writedata(0xFF);
    s.writecommand(0x2A); s.writedata(0x00); s.writedata(0x00); s.writedata(0x01); s.writedata(0xDF);
    s.writecommand(0x2B); s.writedata(0x00); s.writedata(0x00); s.writedata(0x01); s.writedata(0xDF);
    s.writecommand(0x29); s.delay(600);
  }
  static nice_gfx::ControllerProfile profile() {
    auto result = nice_gfx::controllers::profile_CO5300_QSPI();
    result.initialize = initialize;
    return result;
  }
 public:
  static constexpr bool paddedCommands = nice_gfx::CO5300_QSPI::paddedCommands;
  static constexpr uint8_t spiMode = nice_gfx::CO5300_QSPI::spiMode;
  explicit MosaicoCO5300(nice_gfx::Transport &bus, int reset = -1, DisplayOptions options = DisplayOptions())
      : nice_gfx::Panel(bus, profile(), reset, options, 2, 2) {}
  template<class Config> static void configureQspi(Config &config) {
    nice_gfx::CO5300_QSPI::configureQspi(config);
  }
};
using MosaicoDisplay = nice_gfx::QspiDisplay<MosaicoCO5300>;
inline MosaicoDisplay &displayInstance() {
  static MosaicoDisplay instance(50, 44, 42, 36, 51, 35, 9, 40000000, displayOptions());
  return instance;
}
// C++11: one shared display across translation units, no duplicated driver state.
static MosaicoDisplay &gfx = displayInstance();
constexpr int kBacklightPin = -1;
// TE is retained as an input assignment; this renderer does not wait for TE.
constexpr int kDisplayTePin = 43;
constexpr uint8_t kRotation = 0;
constexpr uint16_t kUiWidth = 480, kUiHeight = 480;
constexpr size_t kTilePixels = size_t(kUiWidth) * 16;

