#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <esp_heap_caps.h>
namespace nice_gfx { namespace platform {
inline void *allocateUiTile(size_t pixels) {
  return heap_caps_calloc(pixels, sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
}
inline void *allocateRetained(size_t bytes) {
  void *memory = nullptr;
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  if (bytes >= 1024) memory = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#endif
  return memory ? memory : ::malloc(bytes);
}
inline void freeUiMemory(void *memory) { heap_caps_free(memory); }
constexpr bool retainedMemoryPreferred() {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  return true;
#else
  return false;
#endif
}
} }
