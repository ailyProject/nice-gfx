#pragma once
#ifndef NICE_GFX_UI_HEADER
#define NICE_GFX_UI_HEADER "UI/View.h"
#endif
#ifndef NICE_GFX_FONTS_HEADER
#define NICE_GFX_FONTS_HEADER "Fonts/Builtin.h"
#endif
#include NICE_GFX_UI_HEADER
#include "Display/Presets/All.h"
#if !defined(SEEED_WIO_TERMINAL)
#error "Select Seeed Wio Terminal (Seeeduino:samd:seeed_wio_terminal)"
#endif
// LCD_SPI is SPI3 / SERCOM7. SPI is the external header, SPI2 is microSD.

// Change these values in the designer's hardware panel, then export again.
// Board: Seeeduino:samd:seeed_wio_terminal; display: ILI9341_240x320_SPI; rotation: 3.
inline DisplayOptions displayOptions() {
  DisplayOptions options;
  
  return options;
}
inline nice_gfx::ILI9341_240x320_ARDUINO_SPI &displayInstance() {
  static nice_gfx::ILI9341_240x320_ARDUINO_SPI instance(LCD_SPI, 69, 70, 71, 68, 67, 66, 40000000, displayOptions());
  return instance;
}
// C++11: one shared display across translation units, no duplicated driver state.
static nice_gfx::ILI9341_240x320_ARDUINO_SPI &gfx = displayInstance();
constexpr int kBacklightPin = 72;
// TE is retained as an input assignment; this renderer does not wait for TE.
constexpr int kDisplayTePin = -1;
constexpr uint8_t kRotation = 3;
constexpr uint16_t kUiWidth = 320, kUiHeight = 240;
constexpr size_t kTilePixels = size_t(kUiWidth) * 8;
struct UiButtonPin { uint32_t id; int pin; bool activeLow; uint8_t mode; uint16_t debounce, longPress, repeatDelay, repeatInterval; };
inline const UiButtonPin *uiButtonPins() {
  static const UiButtonPin pins[] = {
    {1128467232u, 31, true, INPUT_PULLUP, 30, 800, 450, 120},
    {1035581717u, 34, true, INPUT_PULLUP, 30, 800, 450, 120},
    {306900080u, 32, true, INPUT_PULLUP, 30, 800, 450, 120},
    {2028154341u, 33, true, INPUT_PULLUP, 30, 800, 450, 120},
    {1893396047u, 35, true, INPUT_PULLUP, 30, 800, 450, 120},
    {3826002220u, 28, true, INPUT_PULLUP, 30, 800, 450, 120},
    {3876335077u, 29, true, INPUT_PULLUP, 30, 800, 450, 120},
    {3859557458u, 30, true, INPUT_PULLUP, 30, 800, 450, 120}
  }; return pins;
}
constexpr size_t kUiButtonCount = 8;
