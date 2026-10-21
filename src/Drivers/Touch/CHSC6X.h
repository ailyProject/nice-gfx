#pragma once
#include "I2cTouch.h"

namespace nice_gfx {

class Chsc6x : public I2cTouch {
 public:
  Chsc6x(TouchI2cBus &bus, uint16_t w = 240, uint16_t h = 240, uint8_t address = 0x2E)
      : I2cTouch(bus, w, h, address) {}
  bool read(RawTouch &out, uint16_t = 0) override {
    uint8_t p[5];
    if (!startRead() || !transfer(nullptr, 0, p, sizeof(p))) return false;
    if (!p[0]) return false;
    if (p[0] != 1) return badPacket();
    // Bytes 1/3 are not high coordinate bytes on the XIAO Round Display.
    return point(out, p[2], p[4]);
  }
};

} // namespace nice_gfx
