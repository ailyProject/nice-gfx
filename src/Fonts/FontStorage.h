#pragma once
// Also used by generated project fonts; independent of the graphics renderer.
#include "../Core/BuildConfig.h"
#include <stdint.h>
#if !NICE_GFX_NATIVE_IDF
#include <Arduino.h>
#endif
#if defined(ARDUINO_ARCH_RP2040)
// RP2040 Flash is directly addressable, including const font arrays.
#undef pgm_read_byte
#undef pgm_read_word
#undef pgm_read_dword
#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#define pgm_read_word(addr) (*(const uint16_t *)(addr))
#define pgm_read_dword(addr) (*(const uint32_t *)(addr))
#elif defined(__AVR__)
#include <avr/pgmspace.h>
#elif !NICE_GFX_NATIVE_IDF && (defined(ARDUINO_ARCH_ESP8266) || defined(ESP32))
#include <pgmspace.h>
#endif
#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#endif
#ifndef pgm_read_word
#define pgm_read_word(addr) (*(const uint16_t *)(addr))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(addr) (*(const uint32_t *)(addr))
#endif
#ifndef pgm_read_ptr
#if defined(__AVR__)
#define pgm_read_ptr(addr) ((void *)pgm_read_word(addr))
#else
#define pgm_read_ptr(addr) (*(const void *const *)(addr))
#endif
#endif
