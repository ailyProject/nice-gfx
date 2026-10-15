#pragma once
#include "Touch.h"

namespace nice_gfx {

// The application initializes its I2C peripheral/pins once. Protocol drivers
// neither change its clock nor probe read-only devices with empty writes.
class TouchI2cBus {
 public:
  virtual ~TouchI2cBus() {}
  virtual Status begin(uint8_t address) = 0;
  virtual Status transfer(uint8_t address, const uint8_t *tx, size_t txSize,
                          uint8_t *rx, size_t rxSize, bool stop = true) = 0;
  virtual Status reset(uint16_t, uint16_t, int = -1) { return Status::Ok; }
  virtual void delayMs(uint16_t) {}
  virtual void delayUs(uint16_t) {}
};

class I2cTouch : public TouchController {
 public:
  Status begin() override {
    ready_ = false;
    if (width_ < 2 || height_ < 2 || address_ < 8 || address_ > 0x77)
      return status_ = Status::InvalidArgument;
    status_ = bus_.begin(address_);
    if (status_ == Status::Ok) status_ = initialize();
    ready_ = status_ == Status::Ok; return status_;
  }
 protected:
  I2cTouch(TouchI2cBus &bus, uint16_t width, uint16_t height, uint8_t address)
      : bus_(bus), width_(width), height_(height), address_(address) {
    calibration_.xMin = calibration_.yMin = 0;
    calibration_.xSpan = width > 1 ? width - 1 : 0;
    calibration_.ySpan = height > 1 ? height - 1 : 0;
    calibration_.swapXY = calibration_.invertX = calibration_.invertY = false;
  }
  virtual Status initialize() { return bus_.reset(10, 100); }
  bool startRead() {
    status_ = ready_ ? Status::Ok : Status::NotInitialized;
    return ready_;
  }
  bool transfer(const uint8_t *tx, size_t nt, uint8_t *rx = nullptr, size_t nr = 0,
                bool stop = true) {
    status_ = bus_.transfer(address_, tx, nt, rx, nr, stop);
    return status_ == Status::Ok;
  }
  bool reg8(uint8_t reg, uint8_t *data, size_t n) { return transfer(&reg, 1, data, n); }
  bool reg16(uint16_t reg, uint8_t *data, size_t n) {
    const uint8_t cmd[] = {uint8_t(reg >> 8), uint8_t(reg)};
    return transfer(cmd, 2, data, n);
  }
  bool write16(uint16_t reg, uint8_t value) {
    const uint8_t cmd[] = {uint8_t(reg >> 8), uint8_t(reg), value};
    return transfer(cmd, 3);
  }
  bool command16(uint16_t command) {
    const uint8_t cmd[] = {uint8_t(command >> 8), uint8_t(command)};
    return transfer(cmd, 2);
  }
  bool point(RawTouch &out, uint16_t x, uint16_t y, uint16_t strength = 1) {
    if (x >= width_ || y >= height_) return badPacket();
    out.x = x; out.y = y; out.pressure = strength ? strength : 1; return true;
  }
  bool badPacket() { status_ = Status::IoError; return false; }
  TouchI2cBus &bus_;
  uint16_t width_, height_;
  uint8_t address_;
};

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

// GT1151 uses a checksummed report, not GT911's ready-bit/held-point protocol.
// Protocol reference: espressif/esp-bsp, esp_lcd_touch_gt1151.
class Gt1151 : public I2cTouch {
 public:
  Gt1151(TouchI2cBus &bus, uint16_t w, uint16_t h, uint8_t address = 0x14)
      : I2cTouch(bus, w, h, address) {}
  bool read(RawTouch &out, uint16_t = 0) override {
    if (!startRead()) return false;
    uint8_t flags;
    if (!reg16(0x814E, &flags, 1)) return false;
    const uint8_t count = flags & 15;
    const bool valid = count <= 10;
    // Status + eight bytes per contact + key byte + additive checksum.
    uint8_t report[83];
    const size_t bytes = 1 + size_t(count) * 8 + 2;
    const bool received = valid && count && reg16(0x814E, report, bytes);
    const Status previous = status_;
    // Clear even malformed reports, so the controller can publish the next one.
    if (!write16(0x814E, 0)) return false;
    if (!valid) return badPacket();
    if (previous != Status::Ok) { status_ = previous; return false; }
    if (!received) return false;
    uint8_t checksum = 0;
    for (size_t i = 0; i < bytes; ++i) checksum = uint8_t(checksum + report[i]);
    if (checksum || (report[0] & 15) != count) return badPacket();
    RawTouch first;
    for (uint8_t i = 0; i < count; ++i) {
      const uint8_t *p = report + 1 + size_t(i) * 8;
      RawTouch candidate;
      if (!point(candidate, uint16_t(p[1] | p[2] << 8), uint16_t(p[3] | p[4] << 8),
                 uint16_t(p[5] | p[6] << 8))) return false;
      if (!i) first = candidate;
    }
    out = first;
    return true;
  }
 protected:
  Status initialize() override {
    if (address_ != 0x14) return Status::InvalidArgument;
    return bus_.reset(10, 10);
  }
};

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
