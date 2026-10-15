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
// Board: esp32:esp32:esp32s3:FlashMode=qio,FlashSize=16M,PartitionScheme=custom,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc; display: CH13613_480x480_QSPI; rotation: 0.
inline DisplayOptions displayOptions() {
  DisplayOptions options;
  
  return options;
}
inline nice_gfx::CH13613_480x480_QSPI &displayInstance() {
  static nice_gfx::CH13613_480x480_QSPI instance(45, 7, 17, 8, 16, 15, 18, 40000000);
  return instance;
}
// C++11: one shared display across translation units, no duplicated driver state.
static nice_gfx::CH13613_480x480_QSPI &gfx = displayInstance();
constexpr int kBacklightPin = -1;
// TE is retained as an input assignment; this renderer does not wait for TE.
constexpr int kDisplayTePin = -1;
constexpr uint8_t kRotation = 0;
constexpr uint16_t kUiWidth = 480, kUiHeight = 480;
constexpr size_t kTilePixels = size_t(kUiWidth) * 16;
