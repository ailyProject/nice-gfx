#pragma once
#include "TouchTypes.h"

namespace nice_gfx {

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

} // namespace nice_gfx
