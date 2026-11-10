#pragma once
#include "BehaviorBackdrop.h"
#include "NativeWireCopy.h"

#ifndef NICE_GFX_UI_SYSTEM_MODAL_FUSED_DIM
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_SYSTEM_MODAL_FUSED_DIM 1
#else
#define NICE_GFX_UI_SYSTEM_MODAL_FUSED_DIM 0
#endif
#endif

// Wire UI surfaces measured faster with the lookup kernel on S31.
// Keep exact word arithmetic available as an explicit platform override.
#ifndef NICE_GFX_UI_SYSTEM_MODAL_DIM_WORD_ARITHMETIC
#define NICE_GFX_UI_SYSTEM_MODAL_DIM_WORD_ARITHMETIC 0
#endif

namespace nice_gfx { namespace htmlui { namespace detail {

// Two independent 16-bit lanes per channel; the maximum numerator is 63*255.
// No rounding bias is added: this is the legacy floor(channel*keep/255).
#if defined(__GNUC__)
__attribute__((always_inline))
#endif
inline uint32_t dimSystemModalPair(uint32_t value,uint32_t keep){
  const uint32_t red=rgb565_blend::divide255Pair(((value>>11)&0x001f001fu)*keep);
  const uint32_t green=rgb565_blend::divide255Pair(((value>>5)&0x003f003fu)*keep);
  const uint32_t blue=rgb565_blend::divide255Pair((value&0x001f001fu)*keep);
  return (red<<11)|(green<<5)|blue;
}

// Copy an immutable clean host directly to its dimmed scanout destination.
// Both complete footprints are validated before writing any pixel. This uses
// the same floor-channel table as the ordinary post-paint backdrop, rather
// than the rounded alpha blend used by other overlays.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("O3")))
#endif
inline bool copyDimSystemModalRows(uint16_t *destination,size_t destinationCapacity,size_t destinationStride,
                                   const uint16_t *source,size_t sourceCapacity,size_t sourceStride,
                                   size_t width,size_t height,PixelOrder sourceOrder,uint8_t dim) {
  size_t outputWords,inputWords;
  if((sourceOrder!=PixelOrder::Native&&sourceOrder!=PixelOrder::Wire)||
     !nativeWireCopyFootprint(destination,destinationCapacity,destinationStride,width,height,outputWords)||
     !nativeWireCopyFootprint(source,sourceCapacity,sourceStride,width,height,inputWords))return false;
  const uintptr_t output=reinterpret_cast<uintptr_t>(destination),input=reinterpret_cast<uintptr_t>(source);
  if(output<input+inputWords*2&&input<output+outputWords*2)return false;
  const BackdropFloorLut lut(dim);
  const uint32_t keep=255u-dim;
  for(size_t y=0;y<height;++y){
    auto *to=destination+y*destinationStride;const auto *from=source+y*sourceStride;size_t remaining=width;
    uint32_t previous=0,previousResult=0;bool cached=false;
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    if(!((reinterpret_cast<uintptr_t>(to)^reinterpret_cast<uintptr_t>(from))&3u)){
      if((reinterpret_cast<uintptr_t>(to)&3u)&&remaining){const uint16_t value=*from++;*to++=lut.color(sourceOrder==PixelOrder::Wire?rgb565_blend::fromWire(value):value);--remaining;}
      typedef uint32_t AliasWord __attribute__((__may_alias__));
      auto *out=reinterpret_cast<AliasWord*>(to);const auto *in=reinterpret_cast<const AliasWord*>(from);
      size_t pairs=remaining/2;
      while(pairs--){
        const uint32_t sample=*in++;
        if(!cached||sample!=previous){
#if NICE_GFX_UI_SYSTEM_MODAL_DIM_WORD_ARITHMETIC
          previousResult=dimSystemModalPair(sourceOrder==PixelOrder::Wire?rgb565_blend::swapPairBytes(sample):sample,keep);
#else
          const uint16_t first=uint16_t(sample),second=uint16_t(sample>>16);
          const uint16_t a=lut.color(sourceOrder==PixelOrder::Wire?rgb565_blend::fromWire(first):first);
          const uint16_t b=first==second?a:lut.color(sourceOrder==PixelOrder::Wire?rgb565_blend::fromWire(second):second);
          previousResult=uint32_t(a)|(uint32_t(b)<<16);
#endif
          previous=sample;cached=true;
        }
        *out++=previousResult;
      }
      to=reinterpret_cast<uint16_t*>(out);from=reinterpret_cast<const uint16_t*>(in);remaining&=1u;
    }
#endif
    while(remaining--){const uint16_t value=*from++;*to++=lut.color(sourceOrder==PixelOrder::Wire?rgb565_blend::fromWire(value):value);}
  }
  return true;
}

}}}
