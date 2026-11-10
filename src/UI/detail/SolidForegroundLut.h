#pragma once
#include "Rgb565Blend.h"
#include <stdint.h>
#include <stddef.h>

namespace nice_gfx { namespace htmlui { namespace detail {

#if defined(__GNUC__)
#define NICE_GFX_FG_LUT_INLINE inline __attribute__((always_inline))
#else
#define NICE_GFX_FG_LUT_INLINE inline
#endif

// Exactly one constant foreground and alpha, with an arbitrary real RGB565
// destination. No destination is sampled during construction. Each component
// is derived from the canonical renderer, rather than an approximate blend.
class SolidForegroundLut {
 public:
  bool matches(uint16_t foreground,uint8_t alpha)const{return ready_&&foreground_==foreground&&alpha_==alpha;}
  void reset(uint16_t foreground,uint8_t alpha){
    for(unsigned i=0;i<32;++i){
      red_[i]=uint16_t(rgb565_blend::color(foreground,uint16_t(i<<11),alpha)&0xf800u);
      blue_[i]=uint16_t(rgb565_blend::color(foreground,uint16_t(i),alpha)&0x001fu);
    }
    for(unsigned i=0;i<64;++i)green_[i]=uint16_t(rgb565_blend::color(foreground,uint16_t(i<<5),alpha)&0x07e0u);
    foreground_=foreground;alpha_=alpha;ready_=true;
  }
  NICE_GFX_FG_LUT_INLINE uint16_t lookupNative(uint16_t destination)const{
    return uint16_t(red_[destination>>11]|green_[(destination>>5)&63u]|blue_[destination&31u]);
  }
  NICE_GFX_FG_LUT_INLINE uint32_t lookupPair(uint32_t destination)const{
    return uint32_t(lookupNative(uint16_t(destination)))|(uint32_t(lookupNative(uint16_t(destination>>16)))<<16);
  }
  static bool enoughCoverage(const uint32_t *words,size_t count,uint32_t minimum,uint64_t &probed){
    if(!minimum)return true;uint32_t covered=0;
    for(size_t i=0;i<count;++i){
      uint32_t bits=words[i];
      bits-=((bits>>1)&0x55555555u);
      bits=(bits&0x33333333u)+((bits>>2)&0x33333333u);
      bits=(bits+(bits>>4))&0x0f0f0f0fu;
      covered+=(bits*0x01010101u)>>24;
      if(covered>=minimum){probed+=i+1;return true;}
    }
    probed+=count;return false;
  }
 private:
  uint16_t red_[32]{},green_[64]{},blue_[32]{};
  uint16_t foreground_=0;uint8_t alpha_=0;bool ready_=false;
};
struct SolidForegroundLutStats{
  uint64_t draws=0,builds=0,reuses=0,smallBypasses=0,pixels=0,probeWords=0,buildMicros=0;
};

#undef NICE_GFX_FG_LUT_INLINE

}}}
