#pragma once
#include "../Display.h"
#include "../../Drivers/Display/CH13613.h"
#include "../../Drivers/Display/ILI9341.h"
namespace nice_gfx {
#if !NICE_GFX_NATIVE_IDF
using ILI9341_240x320_ARDUINO_SPI=ArduinoSpiDisplay<ILI9341>;
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
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S3)
using OSPTEK_CH13613_480x480_ESP32S3=nice_gfx::OSPTEK_CH13613_480x480_ESP32S3;
#endif
