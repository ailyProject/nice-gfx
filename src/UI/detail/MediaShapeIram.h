#pragma once
#include "../../Core/BuildConfig.h"
#ifndef NICE_GFX_UI_MEDIA_SHAPE_IRAM
#define NICE_GFX_UI_MEDIA_SHAPE_IRAM 1
#endif
#if NICE_GFX_UI_MEDIA_SHAPE_IRAM && (defined(ARDUINO_ARCH_ESP32)||defined(ESP_PLATFORM))
#include <sdkconfig.h>
#endif
#if NICE_GFX_UI_MEDIA_SHAPE_IRAM && defined(CONFIG_IDF_TARGET_ESP32P4) && defined(__GNUC__) && !defined(__clang__)
#include <esp_attr.h>
#define NICE_GFX_UI_MEDIA_SHAPE_IRAM_ATTR IRAM_ATTR
#define NICE_GFX_UI_MEDIA_SHAPE_IRAM_EFFECTIVE 1
#else
#define NICE_GFX_UI_MEDIA_SHAPE_IRAM_ATTR
#define NICE_GFX_UI_MEDIA_SHAPE_IRAM_EFFECTIVE 0
#endif
