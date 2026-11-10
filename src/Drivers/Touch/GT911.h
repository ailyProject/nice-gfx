#pragma once
#include "I2cTouch.h"
namespace nice_gfx {
class Gt911 : public I2cTouch {
 public:
  static constexpr uint8_t MaxContacts = 5;
  struct Contact { uint16_t x=0,y=0,pressure=0; uint8_t id=0; };
  struct Frame {
    static constexpr bool ExplicitRelease = true;
    Contact contacts[MaxContacts]; uint8_t count=0; bool ready=false;
  };
  Gt911(TouchI2cBus &bus, uint16_t w, uint16_t h, uint8_t address = 0x5D)
      : I2cTouch(bus, w, h, address) {}
  bool read(RawTouch &out, uint16_t = 0) override {
    Frame frame;if(!readFrame(frame))return false;
    const auto &first=frame.contacts[0];
    out.x=first.x;out.y=first.y;out.pressure=first.pressure?first.pressure:1;
    return true;
  }
  // Read/ACK one complete report. No new ready bit retains the prior contacts
  // with ready=false; a ready zero-contact report explicitly releases them.
  // The bool remains pressed/released, so existing scalar callers stay valid.
  bool readFrame(Frame &out) {
    if (!startRead()) return false;
    uint8_t flags;
    if (!reg16(0x814E, &flags, 1)) { last_={};return false; }
    // The ready bit signals a new report; its absence is not a release.
    if (!(flags & 0x80)) { out=last_;out.ready=false;return out.count!=0; }
    last_={};
    const uint8_t count = flags & 15;
    uint8_t points[MaxContacts*8];
    const bool valid = count <= MaxContacts;
    const bool received = valid && count && reg16(0x814F, points, size_t(count)*8);
    const Status previous = status_;
    if (!write16(0x814E, 0)) return false;
    if (!valid) return badPacket();
    if (previous != Status::Ok) { status_ = previous; return false; }
    if(count&&!received)return false;
    Frame frame;frame.count=count;frame.ready=true;uint16_t seenIds=0;
    for(uint8_t i=0;i<count;++i){
      const uint8_t *p=points+size_t(i)*8;RawTouch candidate;
      if(!point(candidate,uint16_t(p[1]|p[2]<<8),uint16_t(p[3]|p[4]<<8),uint16_t(p[5]|p[6]<<8)))return false;
      const uint8_t id=p[0]&15;if(seenIds&uint16_t(1u<<id))return badPacket();seenIds|=uint16_t(1u<<id);
      frame.contacts[i]={candidate.x,candidate.y,uint16_t(p[5]|p[6]<<8),id};
    }
    last_=frame;out=frame;return count!=0;
  }
  bool getTouchFrame(Frame &out,uint16_t width,uint16_t height,uint8_t rotation=0){
    if(!width||!height||rotation>7){status_=Status::InvalidArgument;return false;}
    Frame frame;const bool pressed=readFrame(frame);
    if(status_!=Status::Ok)return false;
    for(uint8_t i=0;i<frame.count;++i)if(!calibration_.convert(frame.contacts[i].x,frame.contacts[i].y,width,height,rotation)){
      status_=Status::InvalidArgument;return false;
    }
    out=frame;return pressed;
  }
 protected:
  Status initialize() override {
    last_={};
    if (address_ != 0x5D && address_ != 0x14) return Status::InvalidArgument;
    return bus_.reset(10, 50, address_ == 0x14 ? 1 : 0);
  }
 private:
  Frame last_;
};

}
