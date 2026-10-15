#pragma once
#include <Arduino.h>
#include "../../Drivers/Touch/TouchResistive.h"

namespace nice_gfx {
class TouchAnalog : public ResistiveTouchBus {
 public:
  TouchAnalog(int xp, int yp, int xm, int ym, uint16_t adcMaximum = 1023)
      : pins_{xp, yp, xm, ym}, maximum_(adcMaximum) {}
  Status begin() override {
    if (!maximum_) return Status::InvalidArgument;
    for (uint8_t i = 0; i < 4; ++i) {
      if (pins_[i] < 0) return Status::InvalidArgument;
      for (uint8_t j = 0; j < i; ++j) if (pins_[i] == pins_[j]) return Status::InvalidArgument;
    }
    return Status::Ok;
  }
  void input(uint8_t e) override { pinMode(pins_[e], INPUT); digitalWrite(pins_[e], LOW); }
  void output(uint8_t e, bool high) override { pinMode(pins_[e], OUTPUT); digitalWrite(pins_[e], high ? HIGH : LOW); }
  int analog(uint8_t e) override { return analogRead(pins_[e]); }
  uint16_t maximum() const override { return maximum_; }
 private:
  int pins_[4];
  uint16_t maximum_;
};
} // namespace nice_gfx
