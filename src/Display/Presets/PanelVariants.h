#pragma once
#include "../../Drivers/Display/ILI9341.h"
#include "../../Drivers/Display/ILI9486_Parallel.h"
#include "../../Drivers/Display/ST7796.h"
#include "../../Drivers/Display/CO5300_QSPI.h"

namespace nice_gfx { namespace controllers {

// Transposed scan wiring retains the logical dimensions and rotation numbers.
inline void init_ILI9341_Transposed(PanelSequence &s) { init_ILI9341(s, 0xA0); }
inline void rotate_ILI9341_Transposed(PanelSequence &s, uint8_t m) {
  static const uint8_t scan[] = {0xA0,0x00,0x60,0xC0,0xE0,0x80,0x20,0x40};
  rotate_ILI9341(s, m, scan);
}
inline ControllerProfile profile_ILI9341_Transposed() {
  ControllerProfile p = profile_ILI9341();
  p.name = "ILI9341_Transposed";
  p.initialize = init_ILI9341_Transposed;
  p.rotate = rotate_ILI9341_Transposed;
  return p;
}

// SPI-to-parallel bridges pad each command to 16 bits; pixel data stays RGB565.
inline ControllerProfile profile_ILI9486_PaddedSPI() {
  ControllerProfile p = profile_ILI9486_Parallel();
  p.name = "ILI9486_PaddedSPI";
  p.readFormat = PixelReadFormat::RGB888;
  return p;
}
inline ControllerProfile profile_ST7796_PaddedSPI() {
  ControllerProfile p = profile_ST7796();
  p.name = "ST7796_PaddedSPI";
  return p;
}

// 480x480 glass sequence, sourced from the editor's existing board export:
// https://github.com/esp-mosaico/esp-mosaico-bsp/blob/master/components/esp-mosaico-bsp/onboard/display.c
inline void init_CO5300_Sequence2(PanelSequence &s) {
  s.writecommand(0x11);s.delay(600);
  s.writecommand(0xFE);s.writedata(0x20);
  s.writecommand(0x19);s.writedata(0x10);
  s.writecommand(0x1C);s.writedata(0xA0);
  s.writecommand(0xFE);s.writedata(0x00);
  s.writecommand(0xC4);s.writedata(0x80);
  s.writecommand(0x3A);s.writedata(0x55);
  s.writecommand(0x35);s.writedata(0x00);
  s.writecommand(0x53);s.writedata(0x20);
  s.writecommand(0x51);s.writedata(0xFF);
  s.writecommand(0x63);s.writedata(0xFF);
  s.writecommand(0x2A);s.writedata(0);s.writedata(0);s.writedata(1);s.writedata(0xDF);
  s.writecommand(0x2B);s.writedata(0);s.writedata(0);s.writedata(1);s.writedata(0xDF);
  s.writecommand(0x29);s.delay(600);
}
inline ControllerProfile profile_CO5300_Sequence2_QSPI() {
  ControllerProfile p=profile_CO5300_QSPI();p.name="CO5300_Sequence2_QSPI";
  p.initialize=init_CO5300_Sequence2;return p;
}

} } // namespace nice_gfx::controllers

namespace nice_gfx {

class CO5300_Sequence2_QSPI : public Panel {
 public:
  static constexpr bool paddedCommands=CO5300_QSPI::paddedCommands;
  static constexpr uint8_t spiMode=CO5300_QSPI::spiMode;
  explicit CO5300_Sequence2_QSPI(Transport &bus,int reset=-1,PanelOptions options=PanelOptions())
    :Panel(bus,controllers::profile_CO5300_Sequence2_QSPI(),reset,options,2,2) {}
  template<class Config>static void configureQspi(Config &config){CO5300_QSPI::configureQspi(config);}
};

class ILI9341_Transposed : public Panel {
 public:
  static constexpr bool paddedCommands = false;
  static constexpr uint8_t spiMode = 0;
  explicit ILI9341_Transposed(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ILI9341_Transposed(), reset, options) {}
};

class ILI9486_PaddedSPI : public Panel {
 public:
  static constexpr bool paddedCommands = true;
  static constexpr uint8_t spiMode = 0;
  explicit ILI9486_PaddedSPI(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ILI9486_PaddedSPI(), reset, options) {}
};

class ST7796_PaddedSPI : public Panel {
 public:
  static constexpr bool paddedCommands = true;
  static constexpr uint8_t spiMode = 0;
  explicit ST7796_PaddedSPI(Transport &bus, int reset = -1, PanelOptions options = PanelOptions())
      : Panel(bus, controllers::profile_ST7796_PaddedSPI(), reset, options) {}
};

} // namespace nice_gfx
