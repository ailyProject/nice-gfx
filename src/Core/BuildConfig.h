#pragma once

// ESP-IDF projects select the native path without defining Arduino board flags.
// Arduino-as-an-IDF-component retains its existing public APIs and transports.
#ifndef NICE_GFX_NATIVE_IDF
#if defined(ESP_PLATFORM) && !defined(ARDUINO) && !defined(ARDUINO_ARCH_ESP32)
#define NICE_GFX_NATIVE_IDF 1
#else
#define NICE_GFX_NATIVE_IDF 0
#endif
#endif
