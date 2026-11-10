#pragma once
#include "../../Core/BuildConfig.h"
// Shared source-level switch. Every translation unit must see the same value.
// This header is included at global scope, before any UI namespace/class.
#ifndef NICE_GFX_UI_GEOMETRY_IRAM
#define NICE_GFX_UI_GEOMETRY_IRAM 1
#endif
#define NICE_GFX_UI_GEOMETRY_IRAM_VERSION 1
#if NICE_GFX_UI_GEOMETRY_IRAM && defined(__GNUC__) && !defined(__clang__)
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM) || NICE_GFX_NATIVE_IDF
#include <sdkconfig.h>
#endif
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#include <esp_attr.h>
#define NICE_GFX_UI_GEOMETRY_IRAM_ATTR IRAM_ATTR
#define NICE_GFX_UI_GEOMETRY_IRAM_EFFECTIVE 1
#else
#define NICE_GFX_UI_GEOMETRY_IRAM_ATTR
#define NICE_GFX_UI_GEOMETRY_IRAM_EFFECTIVE 0
#endif
#else
#define NICE_GFX_UI_GEOMETRY_IRAM_ATTR
#define NICE_GFX_UI_GEOMETRY_IRAM_EFFECTIVE 0
#endif
