#pragma once
#include "I2cTouch.h"
namespace nice_gfx {
class Gt911 : public I2cTouch {
 public:
  Gt911(TouchI2cBus &bus, uint16_t w, uint16_t h, uint8_t address = 0x5D)
      : I2cTouch(bus, w, h, address) {}
  bool read(RawTouch &out, uint16_t = 0) override {
    if (!startRead()) return false;
    uint8_t flags;
    if (!reg16(0x814E, &flags, 1)) { pressed_ = false; return false; }
    // The ready bit signals a new report; its absence is not a release.
    if (!(flags & 0x80)) { if (pressed_) out = last_; return pressed_; }
    pressed_ = false;
    const uint8_t count = flags & 15;
    uint8_t p[8];
    const bool valid = count <= 5;
    const bool received = valid && count && reg16(0x814F, p, sizeof(p));
    const Status previous = status_;
    if (!write16(0x814E, 0)) return false;
    if (!valid) return badPacket();
    if (previous != Status::Ok) { status_ = previous; return false; }
    if (!received) return false;
    if (!point(last_, uint16_t(p[1] | p[2] << 8), uint16_t(p[3] | p[4] << 8),
               uint16_t(p[5] | p[6] << 8))) return false;
    out = last_; pressed_ = true; return true;
  }
 protected:
  Status initialize() override {
    pressed_ = false;
    if (address_ != 0x5D && address_ != 0x14) return Status::InvalidArgument;
    return bus_.reset(10, 50, address_ == 0x14 ? 1 : 0);
  }
 private:
  RawTouch last_;
  bool pressed_ = false;
};

}
