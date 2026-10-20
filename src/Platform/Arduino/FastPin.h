#pragma once
#include <Arduino.h>
#if defined(ARDUINO_ARCH_ESP32)
#include <driver/gpio.h>
#include <soc/gpio_reg.h>
#elif defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
#include <hardware/structs/sio.h>
#elif defined(ARDUINO_ARCH_AVR)
#include <avr/interrupt.h>
#endif

namespace nice_gfx {
// Cache register addresses once, so runtime pin selection has no pin lookup in
// software SPI and parallel pixel loops. Atomic set/clear preserves other pins.
struct FastPin {
  int number = -1;
  volatile uint32_t *set = nullptr, *clear = nullptr;
  uint32_t mask = 0;
  bool combined = false;
#if defined(ARDUINO_ARCH_AVR)
  volatile uint8_t *port8 = nullptr;
  uint8_t mask8 = 0;
#endif
  static int gpioNumber(int pin) {
#if defined(BOARD_HAS_PIN_REMAP) && !defined(BOARD_USES_HW_GPIO_NUMBERS)
    if (pin < 0) return pin;
    if (pin >= NUM_DIGITAL_PINS) return -1;
    return digitalPinToGPIONumber(pin);
#else
    return pin;
#endif
  }
  static bool valid(int pin) {
#if defined(ARDUINO_ARCH_ESP32)
    return GPIO_IS_VALID_OUTPUT_GPIO(gpioNumber(pin));
#elif defined(ARDUINO_ARCH_STM32)
    return pin >= 0 && digitalPinToPinName(pin) != NC;
#elif defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
    return pin >= 0 && pin < NUM_BANK0_GPIOS;
#elif defined(ARDUINO_ARCH_MBED)
    return pin >= 0 && pin < PINS_COUNT && digitalPinToPinName(pin) != NC;
#elif defined(ARDUINO_ARCH_SAMD) && defined(PINS_COUNT)
    // NUM_DIGITAL_PINS counts header pins on Seeed SAMD (Wio: 23),
    // while valid onboard LCD/SD/sensor routes extend to PINS_COUNT (93).
    return pin >= 0 && pin < PINS_COUNT;
#elif defined(ESP8266)
    return pin >= 0 && pin <= 16 && (pin < 6 || pin > 11);
#elif defined(NUM_DIGITAL_PINS)
    return pin >= 0 && pin < NUM_DIGITAL_PINS;
#else
    return pin >= 0;
#endif
  }
  static bool validInput(int pin) {
#if defined(ARDUINO_ARCH_ESP32)
    return GPIO_IS_VALID_GPIO(gpioNumber(pin));
#else
    return valid(pin);
#endif
  }
  void attach(int pin, bool high = false) {
    number = pin; set = clear = nullptr; mask = 0; combined = false;
#if defined(ARDUINO_ARCH_AVR)
    port8 = nullptr; mask8 = 0;
#endif
    if (pin < 0) return;
    pinMode(pin, OUTPUT);
#if defined(ARDUINO_ARCH_ESP32)
    const int gpio = gpioNumber(pin);
    if (gpio < 32) {
      set = reinterpret_cast<volatile uint32_t *>(GPIO_OUT_W1TS_REG);
      clear = reinterpret_cast<volatile uint32_t *>(GPIO_OUT_W1TC_REG);
    }
#if defined(GPIO_OUT1_W1TS_REG)
    else {
      set = reinterpret_cast<volatile uint32_t *>(GPIO_OUT1_W1TS_REG);
      clear = reinterpret_cast<volatile uint32_t *>(GPIO_OUT1_W1TC_REG);
    }
#endif
    mask = uint32_t(1) << (gpio & 31);
#elif defined(ARDUINO_ARCH_STM32)
    auto port = digitalPinToPort(pin);
    set = clear = &port->BSRR; mask = digitalPinToBitMask(pin); combined = true;
#elif defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
    if (pin < 32) {
      set = &sio_hw->gpio_set; clear = &sio_hw->gpio_clr;
    }
#if NUM_BANK0_GPIOS > 32
    else {
      set = &sio_hw->gpio_hi_set; clear = &sio_hw->gpio_hi_clr;
    }
#endif
    mask = uint32_t(1) << (pin & 31);
#elif defined(ESP8266)
    if (pin < 16) { set = &GPOS; clear = &GPOC; mask = uint32_t(1) << pin; }
#elif defined(ARDUINO_ARCH_AVR)
    const uint8_t port = digitalPinToPort(pin);
    if (port != NOT_A_PIN) { port8 = portOutputRegister(port); mask8 = digitalPinToBitMask(pin); }
#endif
    write(high);
  }
  inline void write(bool high) const {
#if defined(ARDUINO_ARCH_AVR)
    if (port8) {
      const uint8_t saved = SREG;
      cli();
      if (high) *port8 |= mask8;
      else *port8 &= uint8_t(~mask8);
      SREG = saved;
      return;
    }
#endif
    if (set) {
      if (combined) *set = high ? mask : mask << 16;
      else *(high ? set : clear) = mask;
    } else if (number >= 0) digitalWrite(number, high ? HIGH : LOW);
  }
};
}
