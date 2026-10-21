#pragma once
#include "I2cTouch.h"

namespace nice_gfx {

// GT1151 uses a checksummed report, not GT911's ready-bit/held-point protocol.
// Protocol reference: espressif/esp-bsp, esp_lcd_touch_gt1151.
class Gt1151 : public I2cTouch {
 public:
  static constexpr uint8_t MaxContacts = 10;
  struct Contact { uint16_t x=0,y=0,pressure=0; uint8_t id=0; };
  struct Frame { Contact contacts[MaxContacts]; uint8_t count=0; };
  Gt1151(TouchI2cBus &bus, uint16_t w, uint16_t h, uint8_t address = 0x14)
      : I2cTouch(bus, w, h, address) {}
  bool read(RawTouch &out, uint16_t = 0) override {
    Frame frame;
    if (!readFrame(frame)) return false;
    const auto &first=frame.contacts[0];
    out.x=first.x;out.y=first.y;out.pressure=first.pressure?first.pressure:1;
    return true;
  }
  // Acquire/acknowledge one report. Coordinates and pressure are raw; contact
  // IDs are the controller's low-nibble tracking IDs, not array positions.
  // On release/failure leave the whole caller frame unchanged. False + Ok is
  // release, false + non-Ok is a fault; a fresh local Frame{} is convenient.
  bool readFrame(Frame &out) {
    if (!startRead()) return false;
    uint8_t flags;
    if (!reg16(0x814E, &flags, 1)) return false;
    const uint8_t count = flags & 15;
    const bool valid = count <= MaxContacts;
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
    Frame frame;frame.count=count;uint16_t seenIds=0;
    for (uint8_t i = 0; i < count; ++i) {
      const uint8_t *p = report + 1 + size_t(i) * 8;
      RawTouch candidate;
      if (!point(candidate, uint16_t(p[1] | p[2] << 8), uint16_t(p[3] | p[4] << 8),
                 uint16_t(p[5] | p[6] << 8))) return false;
      const uint8_t id=p[0]&15;
      if (seenIds & uint16_t(1u<<id)) return badPacket();
      seenIds |= uint16_t(1u<<id);
      Contact &contact=frame.contacts[i];
      contact.x=candidate.x;contact.y=candidate.y;
      contact.pressure=uint16_t(p[5]|p[6]<<8);contact.id=id;
    }
    out=frame;
    return true;
  }
  // Same report acquisition, with the existing calibration/rotation rules
  // applied atomically to every contact. Do not call getTouch() as well in the
  // same sample: that would consume and ACK a second hardware report.
  bool getTouchFrame(Frame &out,uint16_t width,uint16_t height,uint8_t rotation=0) {
    if (!width||!height||rotation>7) { status_=Status::InvalidArgument;return false; }
    Frame frame;
    if (!readFrame(frame)) return false;
    for(uint8_t i=0;i<frame.count;++i)
      if(!calibration_.convert(frame.contacts[i].x,frame.contacts[i].y,width,height,rotation)) {
        status_=Status::InvalidArgument;return false;
      }
    out=frame;return true;
  }
 protected:
  Status initialize() override {
    if (address_ != 0x14) return Status::InvalidArgument;
    return bus_.reset(10, 10);
  }
};

} // namespace nice_gfx
