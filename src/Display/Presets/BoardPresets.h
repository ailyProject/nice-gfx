#pragma once
#include "../Display.h"
#include "../../Drivers/Display/CH13613.h"
#include "../../Drivers/Display/ILI9341.h"
namespace nice_gfx {
#if !NICE_GFX_NATIVE_IDF
using ILI9341_240x320_ARDUINO_SPI=ArduinoSpiDisplay<ILI9341>;
#if defined(SEEED_WIO_TERMINAL)
// Seeed ArduinoCore-samd/variants/wio_terminal: dedicated SERCOM7, not SPI.
// setRotation(3) gives the normal 320x240 view with the buttons at the top.
class WioTerminalDisplay : public ILI9341_240x320_ARDUINO_SPI {
 public:
  explicit WioTerminalDisplay(uint32_t hz=40000000, PanelOptions options=PanelOptions())
      : ILI9341_240x320_ARDUINO_SPI(LCD_SPI,LCD_SS_PIN,LCD_DC,LCD_RESET,
          LCD_SCK_PIN,LCD_MOSI_PIN,LCD_MISO_PIN,hz,options) {}
};
#endif
#endif
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
using CH13613_480x480_QSPI=QspiDisplay<CH13613>;
// Exact OSPTEK 1.6-inch module wiring used by the bundled S3 example.
class OSPTEK_CH13613_480x480_ESP32S3 : public CH13613_480x480_QSPI {
 public:
  OSPTEK_CH13613_480x480_ESP32S3() : CH13613_480x480_QSPI(45,7,17,8,16,15,18) {}
};
#endif
}
#if !NICE_GFX_NATIVE_IDF
using ILI9341_240x320_ARDUINO_SPI=nice_gfx::ILI9341_240x320_ARDUINO_SPI;
#if defined(SEEED_WIO_TERMINAL)
using WioTerminalDisplay=nice_gfx::WioTerminalDisplay;
#endif
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S3)
using OSPTEK_CH13613_480x480_ESP32S3=nice_gfx::OSPTEK_CH13613_480x480_ESP32S3;
#endif
