#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "../../Drivers/Touch/TouchI2c.h"

namespace nice_gfx {
// Call Wire.begin(...) in the sketch, using the selected core's pin API.
class TouchWire : public TouchI2cBus {
 public:
  explicit TouchWire(TwoWire &wire, int reset = -1, int irq = -1)
      : wire_(wire), reset_(reset), irq_(irq) {}
  Status begin(uint8_t address) override {
    if (address < 8 || address > 0x77 || reset_ < -1 || irq_ < -1 ||
        (reset_ >= 0 && reset_ == irq_)) return Status::InvalidArgument;
    if (irq_ >= 0) pinMode(irq_, INPUT_PULLUP);
    return Status::Ok;
  }
  Status transfer(uint8_t address, const uint8_t *tx, size_t nt,
                  uint8_t *rx, size_t nr, bool stop = true) override {
    if (address < 8 || address > 0x77 || (nt && !tx) || (nr && !rx) || nr > 255)
      return Status::InvalidArgument;
    if (nt) {
      wire_.beginTransmission(address);
      const size_t written = wire_.write(tx, nt);
      const uint8_t error = wire_.endTransmission(nr && written == nt ? stop : true);
      if (written != nt || error) return Status::IoError;
    }
    if (nr) {
      const size_t received = wire_.requestFrom(address, uint8_t(nr));
      if (received != nr) { while (wire_.available()) wire_.read(); return Status::IoError; }
      for (size_t i = 0; i < nr; ++i) {
        const int value = wire_.read();
        if (value < 0) return Status::IoError;
        rx[i] = uint8_t(value);
      }
    }
    return Status::Ok;
  }
  Status reset(uint16_t activeMs, uint16_t settleMs, int select = -1) override {
    if (reset_ < 0) return Status::Ok; // Board reset/expander managed by sketch.
    pinMode(reset_, OUTPUT); digitalWrite(reset_, LOW);
    if (select >= 0 && irq_ >= 0) { pinMode(irq_, OUTPUT); digitalWrite(irq_, select ? HIGH : LOW); }
    delay(activeMs); digitalWrite(reset_, HIGH);
    if (select >= 0 && irq_ >= 0) { delay(6); digitalWrite(irq_, LOW); delay(50); pinMode(irq_, INPUT_PULLUP); }
    delay(settleMs); return Status::Ok;
  }
  void delayMs(uint16_t ms) override { delay(ms); }
  void delayUs(uint16_t us) override { delayMicroseconds(us); }
 private:
  TwoWire &wire_;
  int reset_, irq_;
};
} // namespace nice_gfx
