#pragma once
#include "I2cTouch.h"

namespace nice_gfx {

class Axs5106L : public I2cTouch {
 public:
  Axs5106L(TouchI2cBus &bus, uint16_t w = 172, uint16_t h = 320, uint8_t address = 0x63)
      : I2cTouch(bus, w, h, address) {}
  bool read(RawTouch &out, uint16_t = 0) override {
    uint8_t p[14];
    if (!startRead() || !reg8(0x01, p, sizeof(p))) return false;
    if (!p[1]) return false;
    if (p[1] > 2) return badPacket();
    return point(out, uint16_t((p[2] & 15) << 8 | p[3]), uint16_t((p[4] & 15) << 8 | p[5]));
  }
 protected:
  Status initialize() override { return bus_.reset(200, 300); }
};

} // namespace nice_gfx
