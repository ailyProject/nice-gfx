#pragma once
#include "Rgb565Blend.h"
#include <stddef.h>
#include <stdint.h>

namespace nice_gfx { namespace htmlui { namespace detail {

// No allocations or retained metadata. Validate the full two-dimensional
// footprint before reading/writing; unaligned word phases stay scalar.
inline bool containerTemplateFootprint(const void *pixels,size_t capacity,size_t stride,
                                      size_t width,size_t height,size_t &words){
  if(!pixels||!width||!height||stride<width||height-1>(SIZE_MAX-width)/stride)return false;
  words=(height-1)*stride+width;const uintptr_t address=reinterpret_cast<uintptr_t>(pixels);
  return words<=capacity&&words<=SIZE_MAX/sizeof(uint16_t)&&!(address%alignof(uint16_t))&&
    address<=UINTPTR_MAX-words*sizeof(uint16_t);
}
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("O3")))
#endif
inline bool uniformContainerTemplateNative(const uint16_t *pixels,size_t capacity,size_t stride,
                                          size_t width,size_t height,uint16_t color){
  size_t words;if(!containerTemplateFootprint(pixels,capacity,stride,width,height,words))return false;
#if defined(__GNUC__)
  typedef uint32_t AliasWord __attribute__((__may_alias__));
  const uint32_t repeated=uint32_t(color)|(uint32_t(color)<<16);
#endif
  for(size_t y=0;y<height;++y){const auto *row=pixels+y*stride;size_t remaining=width;
#if defined(__GNUC__)
    if((reinterpret_cast<uintptr_t>(row)&3u)&&remaining){if(*row++!=color)return false;--remaining;}
    const auto *pairs=reinterpret_cast<const AliasWord*>(row);size_t count=remaining/2;
    while(count--)if(*pairs++!=repeated)return false;
    row=reinterpret_cast<const uint16_t*>(pairs);remaining&=1u;
#endif
    while(remaining--)if(*row++!=color)return false;
  }return true;
}

#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("O3")))
#endif
inline bool compositeContainerTemplateNative(uint16_t *destination,size_t destinationCapacity,size_t destinationStride,
                                            const uint16_t *source,size_t sourceCapacity,size_t sourceStride,
                                            size_t width,size_t height,uint16_t background,uint8_t alpha){
  size_t outputWords,inputWords;
  if(!containerTemplateFootprint(destination,destinationCapacity,destinationStride,width,height,outputWords)||
     !containerTemplateFootprint(source,sourceCapacity,sourceStride,width,height,inputWords))return false;
  const uintptr_t output=reinterpret_cast<uintptr_t>(destination),input=reinterpret_cast<uintptr_t>(source);
  if(output<input+inputWords*2&&input<output+outputWords*2)return false;
  const rgb565_blend::SolidBackgroundLut lut(background,alpha);
#if defined(__GNUC__)
  typedef uint32_t AliasWord __attribute__((__may_alias__));
  uint32_t previousSource=0,previousResult=0;bool cached=false;
#endif
  for(size_t y=0;y<height;++y){auto *to=destination+y*destinationStride;const auto *from=source+y*sourceStride;size_t remaining=width;
#if defined(__GNUC__)
    // Both pointers must reach a word boundary after the same scalar prefix.
    if(!((reinterpret_cast<uintptr_t>(to)^reinterpret_cast<uintptr_t>(from))&3u)){
      if((reinterpret_cast<uintptr_t>(to)&3u)&&remaining){*to++=lut.lookupNative(*from++);--remaining;}
      auto *out=reinterpret_cast<AliasWord*>(to);const auto *in=reinterpret_cast<const AliasWord*>(from);size_t count=remaining/2;
      while(count--){const uint32_t word=*in++;
        if(!cached||word!=previousSource){previousSource=word;previousResult=uint32_t(lut.lookupNative(uint16_t(word)))|
          (uint32_t(lut.lookupNative(uint16_t(word>>16)))<<16);cached=true;}
        *out++=previousResult;
      }
      to=reinterpret_cast<uint16_t*>(out);from=reinterpret_cast<const uint16_t*>(in);remaining&=1u;
    }
#endif
    while(remaining--)*to++=lut.lookupNative(*from++);
  }return true;
}
}}}
