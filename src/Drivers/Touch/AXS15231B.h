#pragma once
#include "I2cTouch.h"

namespace nice_gfx {

class Axs15231bTouch : public I2cTouch {
 public:
  Axs15231bTouch(TouchI2cBus &bus, uint16_t w, uint16_t h, uint8_t address = 0x3B)
      : I2cTouch(bus, w, h, address) {}
  bool read(RawTouch &out, uint16_t = 0) override {
    const uint8_t cmd[] = {0xB5, 0xAB, 0xA5, 0x5A, 0, 0, 0, 8, 0, 0, 0};
    uint8_t p[8];
    if (!startRead() || !transfer(cmd, sizeof(cmd))) return false;
    bus_.delayUs(100);
    if (!transfer(nullptr, 0, p, sizeof(p))) return false;
    if (!p[1] || p[2] >> 6 == 1) return false;
    if (p[1] != 1 || p[2] >> 6 == 3) return badPacket();
    return point(out, uint16_t((p[2] & 15) << 8 | p[3]), uint16_t((p[4] & 15) << 8 | p[5]));
  }
};

} // namespace nice_gfx
