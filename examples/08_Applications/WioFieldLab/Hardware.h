#pragma once
#ifndef NICE_GFX_UI_HEADER
#define NICE_GFX_UI_HEADER "UI/View.h"
#endif
#ifndef NICE_GFX_FONTS_HEADER
#define NICE_GFX_FONTS_HEADER "Fonts/Builtin.h"
#endif
#include NICE_GFX_UI_HEADER
#include "Display/Presets/All.h"
// Display wiring and board startup are owned by the bundled NICE_GFX preset.
inline nice_gfx::WioTerminalDisplay &displayInstance() {
  static nice_gfx::WioTerminalDisplay instance;
  return instance;
}
static auto &gfx = displayInstance();

constexpr int kBacklightPin = 72;
constexpr int kDisplayTePin = -1;
constexpr uint8_t kRotation = 3;
constexpr uint16_t kUiWidth = 320, kUiHeight = 240;
constexpr size_t kTilePixels = size_t(kUiWidth) * 16;
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
