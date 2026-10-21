#pragma once
#include "TouchController.h"
#include "../../Core/I2c.h"
namespace nice_gfx {
using TouchI2cBus = I2cBus; // Existing protocol-driver API.
class I2cTouch : public TouchController {
 public:
  Status begin() override {
    ready_ = false;
    if (width_ < 2 || height_ < 2 || address_ < 8 || address_ > 0x77)
      return status_ = Status::InvalidArgument;
    status_ = bus_.begin(address_);
    if (status_ == Status::Ok) status_ = initialize();
    ready_ = status_ == Status::Ok; return status_;
  }
 protected:
  I2cTouch(TouchI2cBus &bus, uint16_t width, uint16_t height, uint8_t address)
      : bus_(bus), width_(width), height_(height), address_(address) {
    calibration_.xMin = calibration_.yMin = 0;
    calibration_.xSpan = width > 1 ? width - 1 : 0;
    calibration_.ySpan = height > 1 ? height - 1 : 0;
    calibration_.swapXY = calibration_.invertX = calibration_.invertY = false;
  }
  virtual Status initialize() { return bus_.reset(10, 100); }
  bool startRead() {
    status_ = ready_ ? Status::Ok : Status::NotInitialized;
    return ready_;
  }
  bool transfer(const uint8_t *tx, size_t nt, uint8_t *rx = nullptr, size_t nr = 0,
                bool stop = true) {
    status_ = bus_.transfer(address_, tx, nt, rx, nr, stop);
    return status_ == Status::Ok;
  }
  bool reg8(uint8_t reg, uint8_t *data, size_t n) { return transfer(&reg, 1, data, n); }
  bool reg16(uint16_t reg, uint8_t *data, size_t n) {
    const uint8_t cmd[] = {uint8_t(reg >> 8), uint8_t(reg)};
    return transfer(cmd, 2, data, n);
  }
  bool write16(uint16_t reg, uint8_t value) {
    const uint8_t cmd[] = {uint8_t(reg >> 8), uint8_t(reg), value};
    return transfer(cmd, 3);
  }
  bool command16(uint16_t command) {
    const uint8_t cmd[] = {uint8_t(command >> 8), uint8_t(command)};
    return transfer(cmd, 2);
  }
  bool point(RawTouch &out, uint16_t x, uint16_t y, uint16_t strength = 1) {
    if (x >= width_ || y >= height_) return badPacket();
    out.x = x; out.y = y; out.pressure = strength ? strength : 1; return true;
  }
  bool badPacket() { status_ = Status::IoError; return false; }
  TouchI2cBus &bus_;
  uint16_t width_, height_;
  uint8_t address_;
};

}
