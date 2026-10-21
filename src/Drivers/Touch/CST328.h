#pragma once
#include "I2cTouch.h"

namespace nice_gfx {

class Cst328 : public I2cTouch {
 public:
  Cst328(TouchI2cBus &bus, uint16_t w, uint16_t h, uint8_t address = 0x1A)
      : I2cTouch(bus, w, h, address) {}
  bool read(RawTouch &out, uint16_t = 0) override {
    if (!startRead()) return false;
    uint8_t count;
    if (!reg16(0xD005, &count, 1)) return false;
    count &= 15;
    uint8_t p[27];
    const bool valid = count <= 5;
    const bool received = valid && count && reg16(0xD000, p, sizeof(p));
    const Status previous = status_;
    if (!write16(0xD005, 0)) return false;
    if (!valid) return badPacket();
    if (previous != Status::Ok) { status_ = previous; return false; }
    if (!received) return false;
    // Point 0 occupies bytes 0..4; count/marker occupy bytes 5/6.
    if ((p[0] & 15) != 6) return false;
    return point(out, uint16_t(p[1] << 4 | p[3] >> 4), uint16_t(p[2] << 4 | (p[3] & 15)), p[4]);
  }
};

} // namespace nice_gfx
