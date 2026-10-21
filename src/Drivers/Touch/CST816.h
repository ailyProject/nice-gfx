#pragma once
#include "I2cTouch.h"

namespace nice_gfx {

class Cst816 : public I2cTouch {
 public:
  Cst816(TouchI2cBus &bus, uint16_t w, uint16_t h, uint8_t address = 0x15)
      : I2cTouch(bus, w, h, address) {}
  bool read(RawTouch &out, uint16_t = 0) override {
    uint8_t p[5];
    if (!startRead() || !reg8(0x02, p, sizeof(p))) return false;
    if (!p[0]) return false;
    if (p[0] == 0xFF) {
      bool idle = true; for (uint8_t v : p) idle = idle && v == 0xFF;
      if (idle) return false; // CST816T's documented idle report with autosleep off.
    }
    if (p[0] != 1) return badPacket();
    return point(out, uint16_t((p[1] & 15) << 8 | p[2]), uint16_t((p[3] & 15) << 8 | p[4]));
  }
 protected:
  Status initialize() override {
    Status s = bus_.reset(30, 50);
    if (s != Status::Ok) return s;
    const uint8_t noSleep[] = {0xFE, 1};
    // Polling must remain possible after an idle period; otherwise sleeping
    // CST816x silicon can NACK I2C reads until the next touch interrupt.
    return transfer(noSleep, 2) ? Status::Ok : status_;
  }
};
using Cst816S = Cst816;
using Cst816T = Cst816;
using Cst816D = Cst816;
using Cst820 = Cst816; // Waveshare's CST820 uses the CST816x report protocol.

} // namespace nice_gfx
