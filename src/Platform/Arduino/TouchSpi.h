#pragma once
#include "../../Drivers/Touch/Touch.h"

#if defined(ARDUINO)
#include <Arduino.h>
#include <SPI.h>
#include "FastPin.h"

namespace nice_gfx {
namespace detail {
inline bool validTouchInput(int pin) {
  return FastPin::validInput(pin);
}
}

// Borrows an already initialized SPI master. Its clock/data pins belong to the
// SPI master instance; touch CS and optional PENIRQ are constructor arguments.
// When display and touch share the master, supply a beforeSelect callback to
// drain pending pixels before changing SPI settings. All access is single-task.
class TouchSpi : public TouchBus {
 public:
  using BeforeSelect = Status (*)(void *context);
  TouchSpi(SPIClass &spi, int cs, int irq = -1, uint32_t frequency = 2500000,
           BeforeSelect beforeSelect = nullptr, void *context = nullptr)
      : spi_(spi), cs_(cs), irq_(irq), frequency_(frequency), before_(beforeSelect), context_(context) {}
  ~TouchSpi() override { deselect(); }
  Status begin() override {
    if (!FastPin::valid(cs_) || irq_ < -1 ||
        (irq_ >= 0 && !detail::validTouchInput(irq_)) ||
        (irq_ >= 0 && FastPin::gpioNumber(cs_) == FastPin::gpioNumber(irq_)) ||
        !frequency_ || frequency_ > 2500000)
      return Status::InvalidArgument;
    digitalWrite(cs_, HIGH); pinMode(cs_, OUTPUT);
    if (irq_ >= 0) pinMode(irq_, INPUT_PULLUP);
    ready_ = true; return Status::Ok;
  }
  Status select() override {
    if (!ready_) return Status::NotInitialized;
    if (selected_) return Status::Busy;
    if (before_) {
      Status s = before_(context_);
      if (s != Status::Ok) return s;
    }
    spi_.beginTransaction(SPISettings(frequency_, MSBFIRST, SPI_MODE0));
    digitalWrite(cs_, LOW); selected_ = true;
    return Status::Ok;
  }
  uint8_t transfer(uint8_t value) override { return spi_.transfer(value); }
  void deselect() override {
    if (!selected_) return;
    digitalWrite(cs_, HIGH); spi_.endTransaction(); selected_ = false;
  }
  bool pressed() const override { return irq_ < 0 || digitalRead(irq_) == LOW; }
 private:
  SPIClass &spi_;
  int cs_, irq_;
  uint32_t frequency_;
  BeforeSelect before_;
  void *context_;
  bool ready_ = false, selected_ = false;
};

struct TouchPins {
  int clock, mosi, miso, cs, irq;
  TouchPins(int clk, int out, int in, int select, int interrupt = -1)
      : clock(clk), mosi(out), miso(in), cs(select), irq(interrupt) {}
};

// Portable independent four-wire touch bus. Hardware SPI is preferred when a
// spare master is available. A 1 us half period respects XPT2046 timing on fast
// processors without relying on digitalWrite's implementation overhead.
class TouchSoftwareSpi : public TouchBus {
 public:
  explicit TouchSoftwareSpi(TouchPins pins) : pins_(pins) {}
  ~TouchSoftwareSpi() override { deselect(); }
  Status begin() override {
    const int p[] = {pins_.clock, pins_.mosi, pins_.miso, pins_.cs, pins_.irq};
    for (uint8_t i = 0; i < 5; ++i) {
      if (p[i] < (i == 4 ? -1 : 0)) return Status::InvalidArgument;
      if (p[i] >= 0 && !(i == 2 || i == 4 ? detail::validTouchInput(p[i]) : FastPin::valid(p[i])))
        return Status::InvalidArgument;
      for (uint8_t j = 0; j < i; ++j)
        if (p[i] >= 0 && FastPin::gpioNumber(p[i]) == FastPin::gpioNumber(p[j])) return Status::InvalidArgument;
    }
    digitalWrite(pins_.cs, HIGH); pinMode(pins_.cs, OUTPUT);
    digitalWrite(pins_.clock, LOW); pinMode(pins_.clock, OUTPUT);
    digitalWrite(pins_.mosi, LOW); pinMode(pins_.mosi, OUTPUT);
    pinMode(pins_.miso, INPUT);
    if (pins_.irq >= 0) pinMode(pins_.irq, INPUT_PULLUP);
    ready_ = true; return Status::Ok;
  }
  Status select() override {
    if (!ready_) return Status::NotInitialized;
    if (selected_) return Status::Busy;
    digitalWrite(pins_.cs, LOW); selected_ = true;
    return Status::Ok;
  }
  uint8_t transfer(uint8_t value) override {
    uint8_t result = 0;
    for (uint8_t mask = 0x80; mask; mask >>= 1) {
      digitalWrite(pins_.mosi, value & mask ? HIGH : LOW);
      delayMicroseconds(1); digitalWrite(pins_.clock, HIGH);
      result = uint8_t((result << 1) | (digitalRead(pins_.miso) != LOW));
      delayMicroseconds(1); digitalWrite(pins_.clock, LOW);
    }
    return result;
  }
  void deselect() override {
    if (selected_) digitalWrite(pins_.cs, HIGH);
    selected_ = false;
  }
  bool pressed() const override { return pins_.irq < 0 || digitalRead(pins_.irq) == LOW; }
 private:
  TouchPins pins_;
  bool ready_ = false, selected_ = false;
};

} // namespace nice_gfx
#endif
