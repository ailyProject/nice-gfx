#pragma once
#include "NativeTypes.h"
#include "UC81xx.h"

namespace nice_gfx { namespace native_epaper {

// An adapter to the library's existing span-oriented register interface. It
// allocates nothing and does not own the physical bus or its GPIOs.
class RegisterBus final : public Transport {
 public:
  RegisterBus(NativeEpaperIo &io,uint8_t chip):io_(io),chip_(chip){}
  Status begin() override {return Status::Ok;}
  void end() override {}
  Status command(uint8_t c,const uint8_t *p,size_t n) override {return io_.command(c,p,n,chip_);}
  Status data(const uint8_t *p,size_t n) override {return io_.data(p,n,chip_);}
  Status pixels(uint8_t,PixelView,bool) override {return Status::Unsupported;}
  Status fill(uint8_t,uint16_t,size_t) override {return Status::Unsupported;}
  Status wait() override {return Status::Ok;}
  bool busy() const override {return false;}
  bool asynchronous() const override {return false;}
  void output(int,bool) override {}
  void delayMs(uint32_t) override {}
 private:
  NativeEpaperIo &io_;uint8_t chip_;
};

inline bool ssdWide(const NativeEpaperProfile &p){return p.controller==NativeEpaperController::SSD1677;}
inline Status ssdAddress(NativeEpaperIo &io,const NativeEpaperProfile &p,uint8_t chip){
  const uint16_t right=p.storageWidth-1,bottom=p.height-1;
  const uint8_t mode=3,gate[]={uint8_t(bottom),uint8_t(bottom>>8),0};
  Status s=io.command(0x01,gate,sizeof(gate),chip);
  if(s==Status::Ok)s=io.command(0x11,&mode,1,chip);
  if(ssdWide(p)){
    const uint8_t x[]={0,0,uint8_t(right),uint8_t(right>>8)},origin[]={0,0};
    if(s==Status::Ok)s=io.command(0x44,x,sizeof(x),chip);
    if(s==Status::Ok)s=io.command(0x4e,origin,sizeof(origin),chip);
  }else{
    const uint8_t x[]={0,uint8_t(right/8)},origin=0;
    if(s==Status::Ok)s=io.command(0x44,x,sizeof(x),chip);
    if(s==Status::Ok)s=io.command(0x4e,&origin,1,chip);
  }
  const uint8_t y[]={0,0,uint8_t(bottom),uint8_t(bottom>>8)},origin[]={0,0};
  if(s==Status::Ok)s=io.command(0x45,y,sizeof(y),chip);
  if(s==Status::Ok)s=io.command(0x4f,origin,sizeof(origin),chip);
  return s;
}
inline Status ssdTrigger(NativeEpaperIo &io,uint8_t mode,uint8_t chip){
  const Status s=io.command(0x22,&mode,1,chip);
  return s==Status::Ok?io.command(0x20,nullptr,0,chip):s;
}
inline Status ssdInitialize(NativeEpaperIo &io,const NativeEpaperProfile &p,uint8_t step,uint8_t chip){
  // Solomon Systech operation flow: reset; address/scan; load the panel's
  // factory OTP mode-1 waveform using its temperature sensor. No host LUT.
  if(step==0)return io.command(0x12,nullptr,0,chip);
  if(step==1)return ssdAddress(io,p,chip);
  if(step==2){const uint8_t temperature=0x80;
    Status s=io.command(0x18,&temperature,1,chip);
    return s==Status::Ok?ssdTrigger(io,p.controller==NativeEpaperController::SSD1683?0xb9:0xb1,chip):s;}
  return Status::InvalidArgument;
}
inline Status ssdStartPlane(NativeEpaperIo &io,const NativeEpaperProfile &p,uint8_t plane,uint8_t chip){
  if(plane>1)return Status::InvalidArgument;
  const uint8_t origin[]={0,0};
  Status s=io.command(0x4e,origin,ssdWide(p)?2:1,chip);
  if(s==Status::Ok)s=io.command(0x4f,origin,2,chip);
  return s==Status::Ok?io.command(plane?0x26:0x24,nullptr,0,chip):s;
}
inline Status ssdRefresh(NativeEpaperIo &io,const NativeEpaperProfile &p,uint8_t chip){
  return ssdTrigger(io,p.controller==NativeEpaperController::SSD1683?0xcf:0xc7,chip);
}
inline Status ssdPowerOff(NativeEpaperIo &io,const NativeEpaperProfile&,uint8_t chip){return ssdTrigger(io,0x03,chip);}
inline Status ssdSleep(NativeEpaperIo &io,const NativeEpaperProfile &p,uint8_t chip){
  const uint8_t code=p.controller==NativeEpaperController::SSD1677?3:1;return io.command(0x10,&code,1,chip);
}
inline const NativeEpaperProtocol &ssdProtocol(){
  static const NativeEpaperProtocol p={3,2,5,ssdInitialize,ssdStartPlane,ssdRefresh,ssdPowerOff,ssdSleep,2};return p;
}

inline Status ucInitialize(NativeEpaperIo &io,const NativeEpaperProfile &p,uint8_t step,uint8_t chip){
  RegisterBus bus(io,chip);
  if(step==1)return uc81xx::powerOn(bus);
  if(step!=0)return Status::InvalidArgument;
  return p.controller==NativeEpaperController::UC8151D?uc81xx::configureSmall(bus,p.width,p.height):
    uc81xx::configure(bus,p.width,p.height,p.controller==NativeEpaperController::UC8179);
}
inline Status ucStartPlane(NativeEpaperIo &io,const NativeEpaperProfile&,uint8_t plane,uint8_t chip){
  return plane?Status::InvalidArgument:io.command(0x13,nullptr,0,chip);
}
inline Status ucRefresh(NativeEpaperIo &io,const NativeEpaperProfile&,uint8_t chip){return io.command(0x12,nullptr,0,chip);}
inline Status ucPowerOff(NativeEpaperIo &io,const NativeEpaperProfile&,uint8_t chip){return io.command(0x02,nullptr,0,chip);}
inline Status ucSleep(NativeEpaperIo &io,const NativeEpaperProfile&,uint8_t chip){
  const uint8_t key=0xa5;return io.command(0x07,&key,1,chip);
}
inline const NativeEpaperProtocol &ucProtocol(){
  static const NativeEpaperProtocol p={2,1,2,ucInitialize,ucStartPlane,ucRefresh,ucPowerOff,ucSleep};return p;
}

} } // namespace nice_gfx::native_epaper
