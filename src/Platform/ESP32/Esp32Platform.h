#pragma once
#include "../../Core/BuildConfig.h"

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include <driver/gpio.h>

#if NICE_GFX_NATIVE_IDF
#include <esp_rom_sys.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/gpio_reg.h>
#else
#include "../Arduino/FastPin.h"
#endif

namespace nice_gfx {

#if NICE_GFX_NATIVE_IDF
// Native IDF pins are physical GPIO numbers. Cache the same atomic register
// writes as the Arduino FastPin path for optional SPI-bridge WR strobes.
struct Esp32Pin {
  int number = -1;
  volatile uint32_t *set = nullptr, *clear = nullptr;
  uint32_t mask = 0;
  static int gpioNumber(int pin) { return pin; }
  void attach(int pin, bool high = false) {
    number = pin; set = clear = nullptr; mask = 0;
    if (!GPIO_IS_VALID_OUTPUT_GPIO(pin)) return;
    gpio_set_level(gpio_num_t(pin), high ? 1 : 0);
    gpio_config_t config = {};
    config.pin_bit_mask = uint64_t(1) << pin;
    config.mode = GPIO_MODE_OUTPUT;
    gpio_config(&config);
    if (pin < 32) {
      set = reinterpret_cast<volatile uint32_t *>(GPIO_OUT_W1TS_REG);
      clear = reinterpret_cast<volatile uint32_t *>(GPIO_OUT_W1TC_REG);
    }
#if defined(GPIO_OUT1_W1TS_REG)
    else if (pin < 64) {
      set = reinterpret_cast<volatile uint32_t *>(GPIO_OUT1_W1TS_REG);
      clear = reinterpret_cast<volatile uint32_t *>(GPIO_OUT1_W1TC_REG);
    }
#endif
    mask = uint32_t(1) << (pin & 31);
  }
  inline void write(bool high) const {
    if (set) *(high ? set : clear) = mask;
    else if (GPIO_IS_VALID_OUTPUT_GPIO(number)) gpio_set_level(gpio_num_t(number), high ? 1 : 0);
  }
};
#else
using Esp32Pin = FastPin;
#endif

namespace esp32_platform {
inline void write(int pin, bool high) {
#if NICE_GFX_NATIVE_IDF
  gpio_set_level(gpio_num_t(pin), high ? 1 : 0);
#else
  digitalWrite(pin, high ? HIGH : LOW);
#endif
}
inline void output(int pin, bool high) {
#if NICE_GFX_NATIVE_IDF
  if (!GPIO_IS_VALID_OUTPUT_GPIO(pin)) return;
  gpio_set_level(gpio_num_t(pin), high ? 1 : 0);
  gpio_config_t config = {};
  config.pin_bit_mask = uint64_t(1) << pin;
  config.mode = GPIO_MODE_OUTPUT;
  gpio_config(&config);
#else
  pinMode(pin, OUTPUT);
  digitalWrite(pin, high ? HIGH : LOW);
#endif
}
inline void delayMicros(uint32_t us) {
#if NICE_GFX_NATIVE_IDF
  esp_rom_delay_us(us);
#else
  delayMicroseconds(us);
#endif
}
inline void delayMillis(uint32_t ms) {
#if NICE_GFX_NATIVE_IDF
  if (ms) {
    const TickType_t ticks = pdMS_TO_TICKS(ms);
    vTaskDelay(ticks ? ticks : 1);
  }
#else
  delay(ms);
#endif
}
} // namespace esp32_platform
} // namespace nice_gfx
#endif
