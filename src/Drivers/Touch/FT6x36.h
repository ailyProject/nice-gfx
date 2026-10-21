#pragma once
#include "I2cTouch.h"

namespace nice_gfx {

class Ft6x36 : public I2cTouch {
 public:
  Ft6x36(TouchI2cBus &bus, uint16_t w, uint16_t h, uint8_t address = 0x38)
      : I2cTouch(bus, w, h, address) {}
  bool read(RawTouch &out, uint16_t = 0) override {
    if (!startRead()) return false;
    uint8_t count;
    if (!reg8(0x02, &count, 1)) return false;
    count &= 15;
    if (count > maximumPoints()) return badPacket();
    if (!count) return false;
    uint8_t p[30];
    if (!reg8(0x03, p, size_t(count) * 6)) return false;
    RawTouch first; bool found = false;
    for (uint8_t i = 0; i < count; ++i) {
      const uint8_t *v = p + 6 * i; const uint8_t event = v[0] >> 6;
      if (event == 1) continue; // Up records can retain coordinates/count.
      if (event == 3) return badPacket();
      RawTouch candidate;
      if (!point(candidate, uint16_t((v[0] & 15) << 8 | v[1]),
                 uint16_t((v[2] & 15) << 8 | v[3]), v[4])) return false;
      if (!found) { first = candidate; found = true; }
    }
    if (found) out = first;
    return found;
  }
 protected:
  virtual uint8_t maximumPoints() const { return 2; }
};
using Ft6236 = Ft6x36;
using Ft6336 = Ft6x36;
using Ft6336U = Ft6x36;
class Ft5x06 : public Ft6x36 {
 public:
  using Ft6x36::Ft6x36;
 protected:
  uint8_t maximumPoints() const override { return 5; }
};
using Ft3168 = Ft5x06;
using Ft6146 = Ft5x06;

} // namespace nice_gfx
