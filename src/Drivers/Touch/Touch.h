#pragma once
#include "../../Display/Transport.h"

namespace nice_gfx {

struct RawTouch {
  uint16_t x = 0, y = 0, pressure = 0;
};

// A transaction belongs to one controller. Shared display buses must complete
// outstanding DMA before select(); implementations can do so in select().
class TouchBus {
 public:
  virtual ~TouchBus() {}
  virtual Status begin() = 0;
  virtual Status select() = 0;
  virtual uint8_t transfer(uint8_t value) = 0;
  virtual void deselect() = 0;
  virtual bool pressed() const { return true; } // Optional active-low PENIRQ.
 protected:
  TouchBus() {}
 private:
  TouchBus(const TouchBus &) = delete;
  TouchBus &operator=(const TouchBus &) = delete;
};

struct TouchCalibration {
  uint16_t xMin = 300, xSpan = 3600, yMin = 300, ySpan = 3600;
  bool swapXY = true, invertX = true, invertY = false;
  uint8_t rotation = 0; // Display rotation at calibration time (0..7).

  bool valid() const {
    return xSpan && ySpan && rotation < 8 &&
           uint32_t(xMin) + xSpan <= UINT16_MAX &&
           uint32_t(yMin) + ySpan <= UINT16_MAX;
  }
  Status decodeWords(const uint16_t *data, uint8_t referenceRotation = 0) {
    if (!data || !data[1] || !data[3] || (data[4] & ~7U) || referenceRotation > 7)
      return Status::InvalidArgument;
    TouchCalibration c;
    c.xMin = data[0]; c.xSpan = data[1]; c.yMin = data[2]; c.ySpan = data[3];
    c.swapXY = (data[4] & 1) != 0;
    c.invertX = (data[4] & 2) != 0; c.invertY = (data[4] & 4) != 0;
    c.rotation = referenceRotation;
    if (!c.valid()) return Status::InvalidArgument;
    *this = c;
    return Status::Ok;
  }
  void encodeWords(uint16_t *data) const {
    if (!data) return;
    data[0] = xMin; data[1] = xSpan; data[2] = yMin; data[3] = ySpan;
    data[4] = uint16_t(uint16_t(swapXY) | (uint16_t(invertX) << 1) | (uint16_t(invertY) << 2));
  }
  // Four averaged raw positions at top-left, bottom-left, top-right,
  // bottom-right. Callers own the calibration UI and sampling timeout.
  Status fromCorners(const RawTouch *corners, uint8_t referenceRotation = 0) {
    if (!corners || referenceRotation > 7) return Status::InvalidArgument;
    TouchCalibration c;
    c.rotation = referenceRotation;
    const int32_t dx = int32_t(corners[0].x) - corners[1].x;
    const int32_t dy = int32_t(corners[0].y) - corners[1].y;
    c.swapXY = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy);
    uint32_t left = 0, right = 0, top = 0, bottom = 0;
    for (uint8_t i = 0; i < 4; ++i) {
      const uint16_t x = c.swapXY ? corners[i].y : corners[i].x;
      const uint16_t y = c.swapXY ? corners[i].x : corners[i].y;
      (i < 2 ? left : right) += x;
      (i & 1 ? bottom : top) += y;
    }
    left /= 2; right /= 2; top /= 2; bottom /= 2;
    if (left == right || top == bottom) return Status::InvalidArgument;
    c.invertX = left > right; c.invertY = top > bottom;
    c.xMin = uint16_t(c.invertX ? right : left);
    c.xSpan = uint16_t(c.invertX ? left - right : right - left);
    c.yMin = uint16_t(c.invertY ? bottom : top);
    c.ySpan = uint16_t(c.invertY ? top - bottom : bottom - top);
    *this = c;
    return Status::Ok;
  }
  bool convert(uint16_t &x, uint16_t &y, uint16_t width, uint16_t height,
               uint8_t displayRotation = 0) const {
    if (!valid() || !width || !height || displayRotation > 7) return false;
    // Controllers disagree on the meaning of mirror rotations 4..7. Those
    // modes use calibration taken in that exact mode, never a guessed mirror.
    if ((rotation >= 4 || displayRotation >= 4) && rotation != displayRotation) return false;
    const int32_t rawX = swapXY ? y : x, rawY = swapXY ? x : y;
    if (rawX < xMin || rawY < yMin || rawX > int32_t(xMin) + xSpan ||
        rawY > int32_t(yMin) + ySpan) return false;
    const uint8_t delta = (displayRotation + 4 - rotation) & 3;
    const uint16_t w = delta & 1 ? height : width, h = delta & 1 ? width : height;
    uint16_t px = uint16_t(uint64_t(rawX - xMin) * (w - 1) / xSpan);
    uint16_t py = uint16_t(uint64_t(rawY - yMin) * (h - 1) / ySpan);
    if (invertX) px = uint16_t(w - 1 - px);
    if (invertY) py = uint16_t(h - 1 - py);
    switch (delta) {
      case 0: x = px; y = py; break;
      case 1: x = py; y = uint16_t(w - 1 - px); break;
      case 2: x = uint16_t(w - 1 - px); y = uint16_t(h - 1 - py); break;
      default: x = uint16_t(h - 1 - py); y = px; break;
    }
    return true;
  }
};

// Common first-contact API for resistive and capacitive controllers. Drivers
// leave caller coordinates unchanged on release or failure.
class TouchController {
 public:
  virtual ~TouchController() {}
  virtual Status begin() = 0;
  virtual bool read(RawTouch &out, uint16_t threshold = 0) = 0;
  Status status() const { return status_; }
  bool ready() const { return ready_; }
  const TouchCalibration &calibration() const { return calibration_; }
  Status setCalibration(const TouchCalibration &c) {
    if (!c.valid()) return status_ = Status::InvalidArgument;
    calibration_ = c; return status_ = Status::Ok;
  }
  Status setCalibration(const uint16_t *data, uint8_t referenceRotation = 0) {
    return status_ = calibration_.decodeWords(data, referenceRotation);
  }
  virtual bool getTouchRaw(uint16_t *x, uint16_t *y) {
    if (!x || !y) { status_ = Status::InvalidArgument; return false; }
    RawTouch point;
    if (!read(point)) return false;
    *x = point.x; *y = point.y; return true;
  }
  virtual uint16_t getTouchRawZ() { RawTouch p; return read(p) ? p.pressure : 0; }
  bool getTouch(uint16_t *x, uint16_t *y, uint16_t width, uint16_t height,
                uint8_t rotation = 0, uint16_t threshold = 0) {
    if (!x || !y || !width || !height || rotation > 7) {
      status_ = Status::InvalidArgument; return false;
    }
    RawTouch point;
    if (!read(point, threshold) || !calibration_.convert(point.x, point.y, width, height, rotation))
      return false;
    *x = point.x; *y = point.y; return true;
  }
 protected:
  TouchController() {}
  TouchCalibration calibration_;
  Status status_ = Status::NotInitialized;
  bool ready_ = false;
 private:
  TouchController(const TouchController &) = delete;
  TouchController &operator=(const TouchController &) = delete;
};

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
