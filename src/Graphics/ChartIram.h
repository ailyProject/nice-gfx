#pragma once
#include "../Core/BuildConfig.h"
// One private source-level switch for every translation unit. Do not override
// per-TU: inline/template COMDAT copies must agree on their section attributes.
#ifndef NICE_GFX_UI_CHART_IRAM
#define NICE_GFX_UI_CHART_IRAM 1
#endif
#define NICE_GFX_UI_CHART_IRAM_VERSION 1
#if NICE_GFX_UI_CHART_IRAM && defined(__GNUC__) && !defined(__clang__)
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM) || NICE_GFX_NATIVE_IDF
// This header is included at global scope by TileCanvas/Instruments only.
#include <sdkconfig.h>
#endif
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#include <esp_attr.h>
#define NICE_GFX_UI_CHART_IRAM_ATTR IRAM_ATTR
#define NICE_GFX_UI_CHART_IRAM_EFFECTIVE 1
#else
#define NICE_GFX_UI_CHART_IRAM_ATTR
#define NICE_GFX_UI_CHART_IRAM_EFFECTIVE 0
#endif
#else
#define NICE_GFX_UI_CHART_IRAM_ATTR
#define NICE_GFX_UI_CHART_IRAM_EFFECTIVE 0
#endif
