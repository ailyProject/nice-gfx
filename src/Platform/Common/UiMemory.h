#pragma once
#include "../../Core/BuildConfig.h"
#include <stddef.h>
#include <stdlib.h>
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include "../ESP32/UiMemory.h"
#else
namespace nice_gfx { namespace platform {
inline void *allocateUiTile(size_t) { return nullptr; }
inline void *allocateUiMetadata(size_t bytes,size_t) { return ::malloc(bytes); }
inline void *allocateRetained(size_t bytes) { return ::malloc(bytes); }
inline void freeUiMemory(void *memory) { ::free(memory); }
constexpr bool retainedMemoryPreferred() { return false; }
} }
#endif
