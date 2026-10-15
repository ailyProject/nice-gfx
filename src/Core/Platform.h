#pragma once
#include "BuildConfig.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#if NICE_GFX_NATIVE_IDF
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#else
#include <Arduino.h>
#endif

namespace nice_gfx { namespace platform {

inline uint32_t micros() {
#if NICE_GFX_NATIVE_IDF
    return uint32_t(esp_timer_get_time());
#else
    return uint32_t(::micros());
#endif
}
inline uint32_t millis() {
#if NICE_GFX_NATIVE_IDF
    return uint32_t(uint64_t(esp_timer_get_time()) / 1000);
#else
    return uint32_t(::millis());
#endif
}
inline void delayMs(uint32_t ms) {
#if NICE_GFX_NATIVE_IDF
    if (ms) {
        const TickType_t ticks = pdMS_TO_TICKS(ms);
        vTaskDelay(ticks ? ticks : 1);
    }
#else
    ::delay(ms);
#endif
}
inline long random(long upper) {
#if NICE_GFX_NATIVE_IDF
    return upper > 0 ? long(uint32_t(::rand()) % uint32_t(upper)) : 0;
#else
    return ::random(upper);
#endif
}
inline bool psramFound() {
#if NICE_GFX_NATIVE_IDF
    return heap_caps_get_total_size(MALLOC_CAP_SPIRAM) != 0;
#elif defined(ESP32) && (defined(CONFIG_SPIRAM_SUPPORT) || defined(CONFIG_SPIRAM))
    return ::psramFound();
#else
    return false;
#endif
}
inline void *callocPsram(size_t bytes) {
#if NICE_GFX_NATIVE_IDF
    return heap_caps_calloc(bytes, 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#elif defined(ESP32) && (defined(CONFIG_SPIRAM_SUPPORT) || defined(CONFIG_SPIRAM))
    return ::ps_calloc(bytes, 1);
#else
    return nullptr;
#endif
}

} } // namespace nice_gfx::platform
