#pragma once
#include <NICE_GFX.h>

// easyVoice 1306 dev Arduino pin numbers. Override these with build flags or
// edit this file when using a different wiring. A module without CS uses -1.
#ifndef CI130X_TFT_CS
#define CI130X_TFT_CS 17
#endif
#ifndef CI130X_TFT_DC
#define CI130X_TFT_DC 16
#endif
#ifndef CI130X_TFT_RST
#define CI130X_TFT_RST 5
#endif
#ifndef CI130X_TFT_SCK
#define CI130X_TFT_SCK 22
#endif
#ifndef CI130X_TFT_MOSI
#define CI130X_TFT_MOSI 23
#endif
#ifndef CI130X_TFT_BL
#define CI130X_TFT_BL 2
#endif
#ifndef CI130X_TFT_SPI_HZ
#define CI130X_TFT_SPI_HZ 4000000UL
#endif

#if defined(NICE_GFX_UI_MONOCHROME_ONLY) && NICE_GFX_UI_MONOCHROME_ONLY
#error "CI130X_ST7789 is a colour example; disable NICE_GFX_UI_MONOCHROME_ONLY."
#endif

// ArduinoSpi retains the core SPI object's ownership and transaction lock.
// Eligible CI13XX MODE0 writes use NICE_GFX's default masked-GPIO bulk path;
// a global -DNICE_GFX_CI13XX_FAST_SPI=0 selects the core bulk writer instead.
// Both are software SPI, with a configured request capped at 4 MHz.
// No MISO connection is used; successful writes cannot confirm panel reception.
using ExampleDisplay = ST7789_240x240_SPI;

