#pragma once
#include "../Core/Status.h"
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <new>
class NgfxGxEpd2_EPD;
namespace nice_gfx {
enum class GxEpd2ColorEncoding:uint8_t{Mono,BlackRed,BlackYellow,FourColor,SevenColor,SixColor,Gray16};
struct GxEpd2Pins{
  int clock=-1,mosi=-1,miso=-1,cs=-1,dc=-1,reset=-1,busy=-1;
  int extraCs[3]={-1,-1,-1},extraDc[3]={-1,-1,-1},extraReset[3]={-1,-1,-1},extraBusy[3]={-1,-1,-1};
};
class GxEpd2Io;
struct GxEpd2Config{
  GxEpd2Pins pins;
  uint32_t busClockHz=0;
  uint16_t resetDurationMs=10;
  bool pulldownReset=false;
  int vcomMillivolts=-1; // Preserve the device's calibrated value by default.
  size_t maxTapeBytes=SIZE_MAX<1048576UL?SIZE_MAX:size_t(1048576UL);
  uint32_t pollBudgetMicros=2000;
  uint16_t maxDataBytesPerPoll=512;
  bool preferPsram=true;
  GxEpd2Io *io=nullptr;
};
typedef NgfxGxEpd2_EPD *(*GxEpd2CreateFn)(const GxEpd2Config&);
typedef void (*GxEpd2UploadFn)(NgfxGxEpd2_EPD&,const uint8_t*,const uint8_t*,int16_t,int16_t,int16_t,int16_t);
struct GxEpd2Profile{
  const char *name;
  uint16_t width,height;
  GxEpd2ColorEncoding encoding;
  GxEpd2CreateFn create;
  GxEpd2UploadFn upload;
  bool worker;
  uint32_t fullRefreshMs;
  const uint16_t *palette;
  uint8_t paletteCount;
  uint16_t storageWidth;
  uint16_t requiredExtraPins;
  bool requiresMiso;
  GxEpd2Profile(const char*n,uint16_t w,uint16_t h,GxEpd2ColorEncoding e,GxEpd2CreateFn c,
    GxEpd2UploadFn u,bool asyncWorker=false,uint32_t ms=5000,const uint16_t*p=nullptr,uint8_t count=0,uint16_t sw=0,uint16_t extras=0,bool readback=false)
    :name(n),width(w),height(h),encoding(e),create(c),upload(u),worker(asyncWorker),fullRefreshMs(ms),
     palette(p),paletteCount(count),storageWidth(sw?sw:w),requiredExtraPins(extras),requiresMiso(readback||e==GxEpd2ColorEncoding::Gray16){}
};
inline const uint16_t *gxEpd2Palette(GxEpd2ColorEncoding e,uint8_t &count){
  static const uint16_t bw[]={0,0xffff},red[]={0,0xffff,0xf800},yellow[]={0,0xffff,0xffe0};
  static const uint16_t four[]={0,0xffff,0xffe0,0xf800};
  static const uint16_t seven[]={0,0xffff,0x07e0,0x001f,0xf800,0xffe0,0xfc00};
  static const uint16_t six[]={0,0xffff,0xffe0,0xf800,0x001f,0x07e0};
  static const uint16_t gray[]={0,0x1082,0x2104,0x3186,0x4208,0x52aa,0x632c,0x73ae,
    0x8c51,0x9cd3,0xad55,0xbdd7,0xce59,0xdefb,0xef7d,0xffff};
  switch(e){case GxEpd2ColorEncoding::Mono:count=2;return bw;
    case GxEpd2ColorEncoding::BlackRed:count=3;return red;
    case GxEpd2ColorEncoding::BlackYellow:count=3;return yellow;
    case GxEpd2ColorEncoding::FourColor:count=4;return four;
    case GxEpd2ColorEncoding::SixColor:count=6;return six;
    case GxEpd2ColorEncoding::SevenColor:count=7;return seven;
    default:count=16;return gray;}
}
inline uint8_t gxEpd2Quantize(uint16_t color,const uint16_t *palette,uint8_t count){
  const int r=((color>>11)<<3)|((color>>11)>>2);
  const int g=(((color>>5)&63)<<2)|(((color>>5)&63)>>4);
  const int b=((color&31)<<3)|((color&31)>>2);
  // Match the existing monochrome renderer and menu contrast at the exact
  // black/white midpoint. Other palettes retain their first-index tie rule.
  if(count==2&&palette[0]==0&&palette[1]==0xffff)
    return uint32_t(r)*77+uint32_t(g)*150+uint32_t(b)*29+128>=32768?1:0;
  uint32_t best=UINT32_MAX;uint8_t chosen=0;
  for(uint8_t i=0;i<count;++i){const uint16_t p=palette[i];
    const int dr=r-int(((p>>11)<<3)|((p>>11)>>2));
    const int dg=g-int((((p>>5)&63)<<2)|(((p>>5)&63)>>4));
    const int db=b-int(((p&31)<<3)|((p&31)>>2));
    const uint32_t distance=uint32_t(int32_t(dr)*dr)*77+uint32_t(int32_t(dg)*dg)*150+uint32_t(int32_t(db)*db)*29;
    if(distance<best){best=distance;chosen=i;if(!distance)break;}
  }return chosen;
}
// All transfers consume their buffers before returning. Implementations own
// their SPI bus; controller reads and BUSY polarity remain driver-specific.
class GxEpd2Io{
 public:
  virtual ~GxEpd2Io(){}
  virtual Status begin(const GxEpd2Profile&,const GxEpd2Config&){return Status::Ok;}
  virtual void end(){}
  virtual bool validPin(int p,bool input=false)const{(void)input;return p>=0;}
  virtual int physicalPin(int p)const{return p;}
  virtual Status pinMode(int,uint8_t){return Status::Unsupported;}
  virtual Status writePin(int,bool){return Status::Unsupported;}
  virtual Status readPin(int,bool&){return Status::Unsupported;}
  virtual Status spiEnable(bool){return Status::Ok;}
  virtual Status spiSettings(uint32_t,uint8_t){return Status::Ok;}
  virtual Status transfer(const uint8_t*,uint8_t*,size_t){return Status::Unsupported;}
  virtual void delayMs(uint32_t){}
  virtual void delayUs(uint32_t){}
  virtual uint32_t nowMicros()const{return 0;}
  virtual void yieldThread(){}
};
} // namespace nice_gfx
