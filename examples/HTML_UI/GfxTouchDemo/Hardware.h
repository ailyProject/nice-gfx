#pragma once
#include <NICE_GFX.h>
#ifndef NICE_GFX_UI_HEADER
#define NICE_GFX_UI_HEADER <UI/View.h>
#endif
#ifndef NICE_GFX_FONTS_HEADER
#define NICE_GFX_FONTS_HEADER <Fonts/Builtin.h>
#endif
#include NICE_GFX_UI_HEADER
#include <Display/Presets/All.h>

// Change these values in the designer's hardware panel, then export again.
// Board: esp32:esp32:XIAO_ESP32S3; display: GC9A01_240x240_SPI; rotation: 0.
inline DisplayOptions displayOptions() {
  DisplayOptions options;
  // Seeed Round Display uses MADCTL BGR (0x08); RGB swaps cyan into yellow.
  options.colorOrder = DisplayColorOrder::BGR; options.inversion = 1;
  return options;
}
inline GC9A01_240x240_SPI &displayInstance() {
  static GC9A01_240x240_SPI instance(D1, D3, -1, D8, D10, D9, 40000000, displayOptions());
  return instance;
}
// C++11: one shared display across translation units, no duplicated driver state.
static GC9A01_240x240_SPI &gfx = displayInstance();
constexpr int kBacklightPin = D6;
// TE is retained as an input assignment; this renderer does not wait for TE.
constexpr int kDisplayTePin = -1;
constexpr uint8_t kRotation = 0;
constexpr uint16_t kUiWidth = 240, kUiHeight = 240;
constexpr size_t kTilePixels = size_t(kUiWidth) * 16;
