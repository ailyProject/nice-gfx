#pragma once
#include "All.h"
#include "../../Platform/Arduino/TouchSpi.h"

// One owning display object for the bundled examples. Change the ordinary
// constructor arguments below to match your wiring. Platform checks select
// sample connections; no display pin or controller selection macros are used.
namespace nice_gfx {

#if defined(SEEED_WIO_TERMINAL)
using ExampleDisplay = WioTerminalDisplay;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
class ExampleDisplay : public CH13613_480x480_QSPI {
 public: ExampleDisplay() : CH13613_480x480_QSPI(45, 7, 17, 8, 16, 15, 18) {}
};
#elif defined(CONFIG_IDF_TARGET_ESP32)
class ExampleDisplay : public ILI9341_240x320_SPI {
 public: ExampleDisplay() : ILI9341_240x320_SPI(5, 2, 4, 18, 23, 19) {}
};
#elif defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
// C2/C3/C6/H2 and S2 do not all expose GPIO23. Low GPIOs also avoid the C3
// module's dedicated flash pins; change them for your board routing.
class ExampleDisplay : public ILI9341_240x320_SPI {
 public: ExampleDisplay() : ILI9341_240x320_SPI(7, 2, 3, 4, 6, 5) {}
};
#elif defined(ESP8266)
class ExampleDisplay : public ILI9341_240x320_SPI {
 public: ExampleDisplay() : ILI9341_240x320_SPI(15, 0, 2, 14, 13, 12) {}
};
#elif defined(ARDUINO_ARCH_RP2040)
class ExampleDisplay : public ILI9341_240x320_SPI {
 public: ExampleDisplay() : ILI9341_240x320_SPI(17, 20, 21, 18, 19, 16) {}
};
#else
// Other Arduino cores: variant SPI pins, with CS=10/DC=9/RST=8.
class ExampleDisplay : public ILI9341_240x320_SPI {
 public: ExampleDisplay() : ILI9341_240x320_SPI(10, 9, 8, SCK, MOSI, MISO) {}
};
#endif

// Most sample modules tie backlight to power; Wio has a dedicated control pin.
// Set a GPIO here for other modules with an independent backlight input.
inline int exampleBacklightPin() {
#if defined(SEEED_WIO_TERMINAL)
  return LCD_BACKLIGHT;
#else
  return -1;
#endif
}

inline Status exampleTouchWait(void *context) {
  NICE_GFX &display = *static_cast<NICE_GFX *>(context);
  return display.wait();
}

// The bundled touch sketches contain one display. This helper owns their one
// optional touch controller and binds shared-master waiting to that display.
inline Status exampleAttachTouch(NICE_GFX &display) {
#if defined(SEEED_WIO_TERMINAL)
  // The production Wio Terminal has no fitted touchscreen controller.
  (void)display;
  return Status::Unsupported;
#else
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  // Optional external XPT2046; this is not a CH13613 built-in touch controller.
  static TouchSoftwareSpi bus(TouchPins(10, 11, 12, 13));
#elif defined(CONFIG_IDF_TARGET_ESP32)
  static TouchSoftwareSpi bus(TouchPins(25, 26, 27, 32));
#elif defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
  static TouchSoftwareSpi bus(TouchPins(8, 9, 10, 1));
#elif defined(ESP8266)
  static TouchSpi bus(SPI, 4, -1, 2500000, exampleTouchWait, &display);
#elif defined(ARDUINO_ARCH_RP2040)
  static TouchSoftwareSpi bus(TouchPins(10, 11, 12, 13));
#else
  static TouchSpi bus(SPI, 7, -1, 2500000, exampleTouchWait, &display);
#endif
  static Xpt2046 touch(bus);
  Status s = touch.begin();
  if (s == Status::Ok) display.attachTouch(touch);
  return s;
#endif
}

} // namespace nice_gfx

using nice_gfx::ExampleDisplay;
using nice_gfx::exampleAttachTouch;
using nice_gfx::exampleBacklightPin;
