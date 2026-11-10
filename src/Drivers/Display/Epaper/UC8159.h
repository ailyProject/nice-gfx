#pragma once
#include "NativeTypes.h"

namespace nice_gfx { namespace native_epaper {

// Original implementation of the two exact panel specifications' factory-flash
// flow diagrams. See docs/epaper-uc-sources.md. No host waveform is included.
// The backend owns hardware reset, recovery time and active-low BUSY waits.
inline bool uc8159Valid(const NativeEpaperProfile &p) {
  return p.controller==NativeEpaperController::UC8159 &&
    ((p.width==640 && p.height==384) || (p.width==600 && p.height==448)) &&
    p.storageWidth==p.width && p.encoding==NativeEpaperEncoding::Mono &&
    p.chipCount==1;
}

inline Status uc8159Initialize(NativeEpaperIo &io,const NativeEpaperProfile &p,
                              uint8_t step,uint8_t chip) {
  if(!uc8159Valid(p) || chip || step>1)return Status::InvalidArgument;
  if(step==0) {
    // Each glass specifies its own phase-C timing; these are not chip defaults.
    const uint8_t booster[]={0xc7,0xcc,p.width==640?uint8_t(0x28):uint8_t(0x15)};
    const uint8_t power[]={0x37,0x00,0x08,0x08};
    Status s=io.command(0x06,booster,sizeof(booster),chip);
    if(s==Status::Ok)s=io.command(0x01,power,sizeof(power),chip);
    return s==Status::Ok?io.command(0x04,nullptr,0,chip):s;
  }

  // The exact product flow diagrams supply these settings. Their TRES byte
  // order conflicts with the preliminary controller's generic table; use the
  // product's literal values, which encode its actual in-range dimensions.
  const uint8_t panel[]={0xcf,0x00};
  const uint8_t pll=0x3a;
  const uint8_t resolution[]={uint8_t(p.width>>8),uint8_t(p.width),
                              uint8_t(p.height>>8),uint8_t(p.height)};
  const uint8_t vcom=0x1e,interval=0x17,flash=0x03;
  Status s=io.command(0x00,panel,sizeof(panel),chip);
  if(s==Status::Ok)s=io.command(0x30,&pll,1,chip);
  if(s==Status::Ok)s=io.command(0x61,resolution,sizeof(resolution),chip);
  if(s==Status::Ok)s=io.command(0x82,&vcom,1,chip);
  if(s==Status::Ok)s=io.command(0x50,&interval,1,chip);
  return s==Status::Ok?io.command(0xe5,&flash,1,chip):s;
}

inline Status uc8159StartPlane(NativeEpaperIo &io,const NativeEpaperProfile &p,
                              uint8_t plane,uint8_t chip) {
  if(!uc8159Valid(p) || chip || plane)return Status::InvalidArgument;
  return io.command(0x10,nullptr,0,chip);
}

inline Status uc8159Refresh(NativeEpaperIo &io,const NativeEpaperProfile &p,uint8_t chip) {
  return uc8159Valid(p) && !chip?io.command(0x12,nullptr,0,chip):Status::InvalidArgument;
}

inline Status uc8159PowerOff(NativeEpaperIo &io,const NativeEpaperProfile &p,uint8_t chip) {
  return uc8159Valid(p) && !chip?io.command(0x02,nullptr,0,chip):Status::InvalidArgument;
}

inline Status uc8159Sleep(NativeEpaperIo &io,const NativeEpaperProfile &p,uint8_t chip) {
  if(!uc8159Valid(p) || chip)return Status::InvalidArgument;
  // This sleeps the controller. External flash deep sleep requires a separate
  // MFCSB path; no unverified DAM/flash command is sent on the display CS.
  const uint8_t check=0xa5;
  return io.command(0x07,&check,1,chip);
}

// Source: packed MSB-first 0=black / 1=white. Destination: four bytes per source
// byte, two 3-bit pixels in bits 6:4 and 2:0 of each byte; DDX=1 selects black=0
// and white=3. Source/destination spans must not overlap.
inline void uc8159EncodeMono(const uint8_t *source,uint8_t *destination,size_t sourceBytes) {
  static const uint8_t pairs[]={0x00,0x03,0x30,0x33};
  for(size_t i=0;i<sourceBytes;++i) {
    const uint8_t pixels=source[i];
    *destination++=pairs[pixels>>6];
    *destination++=pairs[(pixels>>4)&3];
    *destination++=pairs[(pixels>>2)&3];
    *destination++=pairs[pixels&3];
  }
}

inline const NativeEpaperProtocol &uc8159Protocol() {
  // Wait after power-on before writing the remaining configuration. Ordinary
  // idle retains power for the next frame. Explicit sleep powers off first.
  static const NativeEpaperProtocol p={2,1,1,uc8159Initialize,uc8159StartPlane,
    uc8159Refresh,uc8159PowerOff,uc8159Sleep,0,uc8159EncodeMono,4};
  return p;
}

} } // namespace nice_gfx::native_epaper
