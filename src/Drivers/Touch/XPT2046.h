#pragma once
#include "TouchBus.h"
#include "TouchController.h"

namespace nice_gfx {

// Bounded acquisition: discard the first axis conversion, median-filter five
// samples and reject an unstable middle three. No delays or unbounded debounce.
class Xpt2046 : public TouchController {
 public:
  explicit Xpt2046(TouchBus &bus, uint16_t threshold = 600, uint16_t jitter = 20)
      : bus_(bus), threshold_(threshold), jitter_(jitter) {}
  Xpt2046(const Xpt2046 &) = delete;
  Xpt2046 &operator=(const Xpt2046 &) = delete;
  Status begin() { status_ = bus_.begin(); ready_ = status_ == Status::Ok; return status_; }
  // Returns true only for a stable pressed sample; the output is unchanged on
  // release, jitter or failure. status() distinguishes IO errors from release.
  bool read(RawTouch &out, uint16_t threshold = 0) {
    if (!select()) return false;
    const uint16_t minimum = threshold ? threshold : threshold_;
    const uint16_t before = pressure();
    if (before < minimum) { bus_.deselect(); return false; }
    uint16_t xs[5], ys[5];
    sample(0xD0);
    for (uint8_t i = 0; i < 5; ++i) xs[i] = sample(0xD0);
    sample(0x90);
    for (uint8_t i = 0; i < 5; ++i) ys[i] = sample(0x90);
    const uint16_t after = pressure();
    bus_.deselect();
    sort(xs); sort(ys);
    if (after < minimum || xs[3] - xs[1] > jitter_ || ys[3] - ys[1] > jitter_) return false;
    out.x = xs[2]; out.y = ys[2]; out.pressure = after;
    return true;
  }
  bool getTouchRaw(uint16_t *x, uint16_t *y) {
    if (!x || !y) { status_ = Status::InvalidArgument; return false; }
    if (!select(false)) return false;
    sample(0xD0); const uint16_t px = sample(0xD0);
    sample(0x90); const uint16_t py = sample(0x90);
    bus_.deselect(); *x = px; *y = py; return true;
  }
  uint16_t getTouchRawZ() {
    if (!select(false)) return 0;
    const uint16_t z = pressure(); bus_.deselect(); return z;
  }
 private:
  bool select(bool checkIrq = true) {
    if (!ready_) { status_ = Status::NotInitialized; return false; }
    status_ = Status::Ok;
    if (checkIrq && !bus_.pressed()) return false;
    status_ = bus_.select(); return status_ == Status::Ok;
  }
  uint16_t sample(uint8_t command) {
    bus_.transfer(command);
    const uint16_t high = bus_.transfer(0);
    const uint16_t low = bus_.transfer(0);
    return uint16_t(((high << 8) | low) >> 3) & 0xFFF;
  }
  uint16_t pressure() {
    const uint16_t z1 = sample(0xB0), z2 = sample(0xC0);
    const uint16_t z = uint16_t(4095 + z1 - z2);
    return z == 4095 ? 0 : z;
  }
  static void sort(uint16_t (&values)[5]) {
    for (uint8_t i = 1; i < 5; ++i) {
      const uint16_t v = values[i]; uint8_t j = i;
      while (j && values[j-1] > v) { values[j] = values[j-1]; --j; }
      values[j] = v;
    }
  }
  TouchBus &bus_;
  uint16_t threshold_, jitter_;
};

} // namespace nice_gfx
