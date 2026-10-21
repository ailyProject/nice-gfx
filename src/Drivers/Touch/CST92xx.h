#pragma once
#include "I2cTouch.h"

namespace nice_gfx {

class Cst9217 : public I2cTouch {
 public:
  Cst9217(TouchI2cBus &bus, uint16_t w, uint16_t h, uint8_t address = 0x5A)
      : I2cTouch(bus, w, h, address) {}
  bool read(RawTouch &out, uint16_t = 0) override {
    uint8_t p[15];
    if (!startRead()) return false;
    const bool received = reg16(0xD000, p, sizeof(p));
    const Status previous = status_;
    if (ack_ && !write16(0xD000, 0xAB)) return false;
    if (!received) { status_ = previous; return false; }
    if (p[0] == 0 && p[6] != 0xAB) {
      for (uint8_t v : p) if (v) return badPacket();
      return false;
    }
    if (p[0] == 0xAB || p[6] != 0xAB || (p[5] & 0x7F) > 2) return badPacket();
    for (uint8_t i = 0; i < (p[5] & 0x7F); ++i) {
      const uint8_t *v = p + (i ? 7 : 0);
      if (((v[0] & 15) >> 1) != 3) continue;
      return point(out, uint16_t(v[1] << 4 | v[3] >> 4), uint16_t(v[2] << 4 | (v[3] & 15)));
    }
    return false;
  }
 protected:
  Status initialize() override {
    Status s = bus_.reset(10, 100);
    if (s != Status::Ok) return s;
    // Reference CST92xx normal mode, with immediate report acknowledgement.
    ack_ = true;
    return command16(0xD109) ? Status::Ok : status_;
  }
  bool ack_ = true;
};
class Cst9220 : public Cst9217 {
 public:
  using Cst9217::Cst9217;
 protected:
  Status initialize() override {
    Status s = bus_.reset(10, 100);
    if (s != Status::Ok) return s;
    if (!command16(0xD101)) return status_;
    bus_.delayMs(10);
    uint8_t id[4];
    if (!reg16(0xD204, id, sizeof(id))) { const Status failure = status_; command16(0xD109); return failure; }
    if (uint16_t(id[2] | id[3] << 8) != 0x9220) { command16(0xD109); return Status::Unsupported; }
    ack_ = uint16_t(id[0] | id[1] << 8) != 0x542F;
    if (!ack_ && !command16(0xD109)) return status_;
    bus_.delayMs(10); return Status::Ok;
  }
};

} // namespace nice_gfx
