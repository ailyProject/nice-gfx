#pragma once
#include "Rgb565Blend.h"

namespace nice_gfx { namespace htmlui { namespace detail {

// One already-quantized native RGB565 foreground and one exact 8-bit alpha.
// The bias is prepared once per draw. Two 16-bit numerator lanes stay below
// 16193, so the existing exact /255 identity cannot carry between pixels.
class SolidForegroundBlend {
 public:
  SolidForegroundBlend(uint16_t foreground,uint8_t alpha):inverse_(255u-alpha){
    const uint32_t red=(foreground>>11)*uint32_t(alpha)+127u;
    const uint32_t green=((foreground>>5)&63u)*uint32_t(alpha)+127u;
    const uint32_t blue=(foreground&31u)*uint32_t(alpha)+127u;
    red_=red|(red<<16);green_=green|(green<<16);blue_=blue|(blue<<16);
    redBlue_=(red<<16)|blue;
  }
#if defined(__GNUC__)
  __attribute__((always_inline))
#endif
  inline uint16_t blendNative(uint16_t background)const{
    const uint32_t redBlue=rgb565_blend::divide255Pair(
      ((uint32_t(background>>11)<<16)|(background&31u))*inverse_+redBlue_);
    const uint32_t green=rgb565_blend::divide255(
      ((background>>5)&63u)*inverse_+(green_&0xffffu));
    return uint16_t(((redBlue>>16)<<11)|(green<<5)|(redBlue&31u));
  }
#if defined(__GNUC__)
  __attribute__((always_inline))
#endif
  inline uint32_t blendPair(uint32_t background)const{
    const uint32_t red=rgb565_blend::divide255Pair(((background>>11)&0x001f001fu)*inverse_+red_);
    const uint32_t green=rgb565_blend::divide255Pair(((background>>5)&0x003f003fu)*inverse_+green_);
    const uint32_t blue=rgb565_blend::divide255Pair((background&0x001f001fu)*inverse_+blue_);
    return (red<<11)|(green<<5)|blue;
  }
 private:
  uint32_t inverse_,red_,green_,blue_,redBlue_;
};

}}}
