#pragma once
#include <Arduino.h>

namespace benchmark_platform {
inline uint32_t micros() { return uint32_t(::micros()); }
inline uint32_t millis() { return uint32_t(::millis()); }
inline void delay(uint32_t ms) { ::delay(ms); }
inline void yield() { ::yield(); }
inline auto &console() { return Serial; }
inline void describe() {
#if defined(ESP32)
  Serial.printf("BENCH_BUILD framework=arduino sdk=%s cpu_mhz=%u task_core=%d task_priority=%u stack_bytes=8192 arduino_component=1\n",
    ESP.getSdkVersion(), getCpuFrequencyMhz(), xPortGetCoreID(), unsigned(uxTaskPriorityGet(nullptr)));
#endif
}
inline void output(int pin, bool high) { pinMode(pin, OUTPUT); digitalWrite(pin, high ? HIGH : LOW); }
inline uint32_t freeHeap() {
#if defined(ESP32) || defined(ESP8266)
  return ESP.getFreeHeap();
#elif defined(ARDUINO_PICO_MAJOR)
  return rp2040.getFreeHeap();
#else
  return UINT32_MAX;
#endif
}
inline uint32_t freePsram() {
#if defined(ESP32)
  return ESP.getPsramSize() ? ESP.getFreePsram() : UINT32_MAX;
#else
  return UINT32_MAX;
#endif
}
}
