#pragma once
#include "Touch.h"

namespace nice_gfx {
// Four-wire panels have no separate touch IC. Y+ and X- must be ADC pins.
class ResistiveTouchBus {
 public:
  virtual ~ResistiveTouchBus() {}
  virtual Status begin() = 0;
  virtual void input(uint8_t electrode) = 0;
  virtual void output(uint8_t electrode, bool high) = 0;
  virtual int analog(uint8_t electrode) = 0;
  virtual uint16_t maximum() const = 0;
};

class FourWireTouch : public TouchController {
 public:
  explicit FourWireTouch(ResistiveTouchBus &bus, uint16_t minimumPressure = 100, uint16_t jitter = 15)
      : bus_(bus), minimumPressure_(minimumPressure), jitter_(jitter) {
    calibration_.xMin = calibration_.yMin = 0;
    calibration_.xSpan = calibration_.ySpan = bus.maximum();
    calibration_.swapXY = calibration_.invertX = calibration_.invertY = false;
  }
  Status begin() override {
    ready_ = false;
    if (!bus_.maximum() || minimumPressure_ > bus_.maximum()) return status_ = Status::InvalidArgument;
    status_ = bus_.begin(); ready_ = status_ == Status::Ok;
    if (ready_) releasePins();
    return status_;
  }
  bool read(RawTouch &out, uint16_t threshold = 0) override {
    if (!ready_) { status_ = Status::NotInitialized; return false; }
    status_ = Status::Ok;
    // Electrode indices: X+, Y+, X-, Y-.
    bus_.input(1); bus_.input(3); bus_.output(0, true); bus_.output(2, false);
    uint16_t x; bool valid = axis(1, x);
    bus_.input(0); bus_.input(2); bus_.output(1, true); bus_.output(3, false);
    uint16_t y; valid = axis(2, y) && valid;
    bus_.input(1); bus_.input(2); bus_.output(0, false); bus_.output(3, true);
    const int z1 = bus_.analog(2), z2 = bus_.analog(1);
    releasePins();
    if (z1 < 0 || z2 < 0 || z1 > bus_.maximum() || z2 > bus_.maximum()) {
      status_ = Status::IoError; return false;
    }
    if (!valid || !z1 || z2 <= z1) return false;
    const uint16_t pressure = uint16_t(bus_.maximum() + z1 - z2);
    if (pressure < (threshold ? threshold : minimumPressure_)) return false;
    out.x = uint16_t(bus_.maximum() - x); out.y = uint16_t(bus_.maximum() - y);
    out.pressure = pressure; return true;
  }
 private:
  bool axis(uint8_t electrode, uint16_t &out) {
    uint16_t p[3];
    (void)bus_.analog(electrode); // Discard the conversion after GPIO changes.
    for (uint8_t i = 0; i < 3; ++i) {
      const int value = bus_.analog(electrode);
      if (value < 0 || value > bus_.maximum()) { status_ = Status::IoError; return false; }
      p[i] = uint16_t(value);
    }
    for (uint8_t i = 0; i < 3; ++i) for (uint8_t j = i + 1; j < 3; ++j)
      if (p[j] < p[i]) { const uint16_t v = p[i]; p[i] = p[j]; p[j] = v; }
    if (p[2] - p[0] > jitter_) return false;
    out = p[1]; return true;
  }
  void releasePins() { for (uint8_t i = 0; i < 4; ++i) bus_.input(i); }
  ResistiveTouchBus &bus_;
  uint16_t minimumPressure_, jitter_;
};
} // namespace nice_gfx
