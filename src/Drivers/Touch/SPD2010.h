#pragma once
#include "I2cTouch.h"

namespace nice_gfx {

class Spd2010Touch : public I2cTouch {
 public:
  Spd2010Touch(TouchI2cBus &bus, uint16_t w = 412, uint16_t h = 412, uint8_t address = 0x53)
      : I2cTouch(bus, w, h, address) {}
  bool read(RawTouch &out, uint16_t = 0) override {
    uint8_t flags[4];
    if (!startRead() || !readRegister(0x20, 0, flags, sizeof(flags))) return false;
    if (flags[1] & 0x40) { if (clear()) command(4, 1); return false; }
    if (flags[1] & 0x20) { if (command(0x50, 0) && command(0x46, 0)) clear(); return false; }
    const uint16_t length = uint16_t(flags[2] | flags[3] << 8);
    if (!(flags[0] & 3)) { if ((flags[1] & 8) && (!length || (flags[0] & 8))) clear(); return false; }
    if (length < 5 || length > 64) { clear(); return badPacket(); }
    uint8_t report[64];
    if (!readRegister(0, 3, report, length)) return failAndClear();
    RawTouch candidate; bool pressed = false, valid = true;
    if ((flags[0] & 1) && report[4] <= 0x0A) {
      valid = length >= 10 && (length - 4) % 6 == 0;
      if (valid) for (uint8_t offset = 4; offset < length; offset += 6) {
        const uint8_t *v = report + offset;
        if (!v[4]) continue; // Weight zero is an explicit up record.
        RawTouch p;
        if (v[0] > 0x0A || !point(p, uint16_t((v[3] & 0xF0) << 4 | v[1]),
                                  uint16_t((v[3] & 15) << 8 | v[2]), v[4])) { valid = false; break; }
        if (!pressed) { candidate = p; pressed = true; }
      }
    } else valid = (flags[0] & 2) && report[4] == 0xF6 && length >= 7;
    if (!valid) { clear(); return badPacket(); }
    // Limit continuation draining; malformed hardware must not hang loop().
    for (uint8_t packet = 0; packet < 4; ++packet) {
      uint8_t hdp[8];
      if (!readRegister(0xFC, 2, hdp, sizeof(hdp))) return failAndClear();
      if (hdp[5] == 0x82) {
        if (!clear()) return false;
        if (pressed) out = candidate;
        return pressed;
      }
      const uint16_t next = uint16_t(hdp[2] | hdp[3] << 8);
      if (hdp[5] != 0 || !next || next > sizeof(report)) { clear(); return badPacket(); }
      if (!readRegister(0, 3, report, next)) return failAndClear();
    }
    clear(); return badPacket();
  }
 protected:
  Status initialize() override {
    Status s = bus_.reset(10, 100);
    if (s != Status::Ok) return s;
    uint8_t version[18];
    return readRegister(0x26, 0, version, sizeof(version)) ? Status::Ok : status_;
  }
 private:
  bool readRegister(uint8_t low, uint8_t high, uint8_t *rx, size_t n) {
    const uint8_t cmd[] = {low, high};
    if (!transfer(cmd, 2)) return false;
    bus_.delayUs(200);
    const bool ok = transfer(nullptr, 0, rx, n); bus_.delayUs(200); return ok;
  }
  bool command(uint8_t cmd, uint8_t value) {
    const uint8_t bytes[] = {cmd, 0, value, 0};
    const bool ok = transfer(bytes, 4); bus_.delayUs(200); return ok;
  }
  bool clear() { return command(2, 1); }
  bool failAndClear() { const Status failure = status_; clear(); status_ = failure; return false; }
};

} // namespace nice_gfx
