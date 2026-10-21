#pragma once
#include "I2cTouch.h"

namespace nice_gfx {

class Cst3530 : public I2cTouch {
 public:
  Cst3530(TouchI2cBus &bus, uint16_t w, uint16_t h, uint8_t address = 0x58)
      : I2cTouch(bus, w, h, address) {}
  bool read(RawTouch &out, uint16_t = 0) override {
    uint8_t p[32]; const uint8_t cmd[] = {0xD0, 0x07, 0, 0};
    if (!startRead()) return false;
    const bool received = transfer(cmd, 4, p, sizeof(p), false);
    const Status previous = status_;
    const uint8_t ack[] = {0xD0, 0, 2, 0xAB};
    if (!transfer(ack, 4)) return false;
    if (!received) { status_ = previous; return false; }
    if (p[2] != 0xFF) return false;
    const uint8_t count = p[3] & 15, keys = p[3] >> 4;
    if (count > 5 || keys > 4 || uint16_t(keys + count) * 5 + 4 > sizeof(p)) return badPacket();
    if (!count) return false;
    uint16_t sum = 0x55;
    for (uint8_t i = 4; i < 4 + (keys + count) * 5; ++i) sum = uint16_t(sum + p[i]);
    if (sum != uint16_t(p[0] | p[1] << 8)) return badPacket();
    for (uint8_t i = 0; i < count; ++i) {
      const uint8_t *v = p + (keys + i) * 5 + 4;
      if (!(v[4] >> 4)) continue;
      return point(out, uint16_t(v[0] | (v[3] & 15) << 8), uint16_t(v[1] | (v[3] & 0xF0) << 4), v[2]);
    }
    return false;
  }
 protected:
  Status initialize() override { return bus_.reset(10, 100, 0); }
};

} // namespace nice_gfx
