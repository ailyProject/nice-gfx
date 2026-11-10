#pragma once
#include "NativeMono.h"

namespace nice_gfx { namespace native_epaper {

// Product flowcharts require an explicit border and booster soft-start stage.
// Keep the documented reset values; no panel voltage or host LUT is invented.
inline Status ssdProductInitialize(NativeEpaperIo &io,const NativeEpaperProfile &p,
                                   uint8_t step,uint8_t chip) {
  if(step!=1)return ssdInitialize(io,p,step,chip);
  Status s=ssdAddress(io,p,chip);
  // GDEY042T81 section 14.2 specifies LUT1; the other product command tables
  // specify C0h (high impedance) as the power-on border state.
  const uint8_t border=p.controller==NativeEpaperController::SSD1683?0x01:0xc0;
  return s==Status::Ok?io.command(0x3c,&border,1,chip):s;
}
inline Status ssdProductRefresh(NativeEpaperIo &io,const NativeEpaperProfile &p,
                               uint8_t chip) {
  // SSD1680/1681/1683 product command tables: R0Ch POR, four bytes.
  const uint8_t softStart[]={0x8b,0x9c,0x96,0x0f};
  const Status s=io.command(0x0c,softStart,sizeof(softStart),chip);
  return s==Status::Ok?ssdRefresh(io,p,chip):s;
}
inline const NativeEpaperProtocol &ssdProductProtocol() {
  static const NativeEpaperProtocol p={3,2,5,ssdProductInitialize,ssdStartPlane,
    ssdProductRefresh,ssdPowerOff,ssdSleep,2};
  return p;
}

// GDEQ0426T82 section 8.1: initialize both complete controller RAM banks
// before defining the smaller panel window. Each automatic fill asserts BUSY.
inline Status gdeq0426Initialize(NativeEpaperIo &io,const NativeEpaperProfile &p,
                                 uint8_t step,uint8_t chip) {
  if(step==0)return io.command(0x12,nullptr,0,chip);
  if(step==1||step==2){
    const uint8_t fill=0xf7;
    return io.command(step==1?0x46:0x47,&fill,1,chip);
  }
  if(step==3)return ssdProductInitialize(io,p,1,chip);
  if(step==4)return ssdInitialize(io,p,2,chip);
  return Status::InvalidArgument;
}
inline Status gdeq0426Refresh(NativeEpaperIo &io,const NativeEpaperProfile &p,
                              uint8_t chip) {
  // SSD1677 Rev 1.0, p24: documented level-1 inrush-current setting.
  // This is a five-byte command, unlike the SSD168x soft-start command.
  const uint8_t softStart[]={0xae,0xc7,0xc3,0xc0,0x40};
  const Status s=io.command(0x0c,softStart,sizeof(softStart),chip);
  return s==Status::Ok?ssdRefresh(io,p,chip):s;
}
inline const NativeEpaperProtocol &gdeq0426Protocol() {
  static const NativeEpaperProtocol p={5,2,23,gdeq0426Initialize,ssdStartPlane,
    gdeq0426Refresh,ssdPowerOff,ssdSleep,2};
  return p;
}

// GDEM133T91 section 10.2 explicitly uses the factory OTP waveform. Its
// product sequence does not replace the SSD1677 analog/soft-start settings.
// GDEH116T91 section 4.1 makes soft-start configuration optional.
inline const NativeEpaperProtocol &gdem133Protocol() {
  static const NativeEpaperProtocol p={3,2,5,ssdProductInitialize,ssdStartPlane,
    ssdRefresh,ssdPowerOff,ssdSleep,2};
  return p;
}

// The P-suffix 3.97-inch panel is a distinct product, not a substitute for
// undocumented GDEM0397T81 glass. Its p24 reference specifies split-gate
// scanning, level-2 booster start and the combined temperature/LUT update.
inline Status gdem0397pInitialize(NativeEpaperIo &io,const NativeEpaperProfile &p,
                                  uint8_t step,uint8_t chip) {
  if(step==0)return io.command(0x12,nullptr,0,chip);
  if(step==1){
    const uint8_t softStart[]={0xae,0xc7,0xc3,0xc0,0x80};
    Status s=io.command(0x0c,softStart,sizeof(softStart),chip);
    if(s==Status::Ok)s=ssdAddress(io,p,chip);
    const uint16_t bottom=p.height-1;
    const uint8_t gate[]={uint8_t(bottom),uint8_t(bottom>>8),0x02},border=0x01;
    if(s==Status::Ok)s=io.command(0x01,gate,sizeof(gate),chip);
    return s==Status::Ok?io.command(0x3c,&border,1,chip):s;
  }
  if(step==2){const uint8_t temperature=0x80;return io.command(0x18,&temperature,1,chip);}
  return Status::InvalidArgument;
}
inline Status gdem0397pRefresh(NativeEpaperIo &io,const NativeEpaperProfile&,uint8_t chip) {
  return ssdTrigger(io,0xf7,chip);
}
inline const NativeEpaperProtocol &gdem0397pProtocol() {
  static const NativeEpaperProtocol p={3,2,1,gdem0397pInitialize,ssdStartPlane,
    gdem0397pRefresh,ssdPowerOff,ssdSleep,2};
  return p;
}

} } // namespace nice_gfx::native_epaper
