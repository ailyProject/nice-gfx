#pragma once
#include "Rgb565Blend.h"
#include "../../Display/Transport.h"
#include <stddef.h>
#include <stdint.h>

namespace nice_gfx { namespace htmlui { namespace detail {

inline bool nativeWireCopyFootprint(const void *pixels,size_t capacityWords,size_t stride,
                                    size_t width,size_t height,size_t &words){
  if(!pixels||!width||!height||stride<width||height-1>(SIZE_MAX-width)/stride)return false;
  words=(height-1)*stride+width;const uintptr_t address=reinterpret_cast<uintptr_t>(pixels);
  return words<=capacityWords&&words<=SIZE_MAX/sizeof(uint16_t)&&!(address%alignof(uint16_t))&&
    address<=UINTPTR_MAX-words*sizeof(uint16_t);
}

// Convert host-order RGB565 to its retained big-endian byte representation.
// There are no allocations. Validate every row and both complete footprints,
// including conservative alias bounds, before the first destination write.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("O3")))
#endif
inline bool copyNativeRgb565ToWire(uint16_t *destination,size_t destinationCapacityWords,size_t destinationStride,
                                  const uint16_t *source,size_t sourceCapacityWords,size_t sourceStride,
                                  size_t width,size_t height){
  size_t outputWords,inputWords;
  if(!nativeWireCopyFootprint(destination,destinationCapacityWords,destinationStride,width,height,outputWords)||
     !nativeWireCopyFootprint(source,sourceCapacityWords,sourceStride,width,height,inputWords))return false;
  const uintptr_t output=reinterpret_cast<uintptr_t>(destination),input=reinterpret_cast<uintptr_t>(source);
  if(output<input+inputWords*sizeof(uint16_t)&&input<output+outputWords*sizeof(uint16_t))return false;
  for(size_t y=0;y<height;++y){
    auto *to=destination+y*destinationStride;const auto *from=source+y*sourceStride;size_t remaining=width;
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    // A scalar prefix can align both addresses only when their phases agree.
    // Independent odd/even strides can change that condition on each row.
    if(!((reinterpret_cast<uintptr_t>(to)^reinterpret_cast<uintptr_t>(from))&3u)){
      if((reinterpret_cast<uintptr_t>(to)&3u)&&remaining){*to++=rgb565_blend::toWire(*from++);--remaining;}
      typedef uint32_t AliasWord __attribute__((__may_alias__));
      auto *out=reinterpret_cast<AliasWord*>(to);const auto *in=reinterpret_cast<const AliasWord*>(from);
      size_t pairs=remaining/2;
      while(pairs--){const uint32_t native=*in++;*out++=rgb565_blend::swapPairBytes(native);}
      to=reinterpret_cast<uint16_t*>(out);from=reinterpret_cast<const uint16_t*>(in);remaining&=1u;
    }
#endif
    while(remaining--)*to++=rgb565_blend::toWire(*from++);
  }
  return true;
}

// Both descriptors describe complete owned pictures, not ROI-offset pointers.
// The blocking copier preserves adjacent output cache lines. Only Unsupported
// permits the original CPU conversion; a possibly partial DMA error returns.
template<class Copier>
inline Status copyNativeRgb565ToWire(Copier &copier,const NativeFramebuffer &destination,
                                    const NativeFramebuffer &source,const DirtyRect &area,bool accelerated){
  size_t outputWords,inputWords;
  if(!nativeWireCopyFootprint(destination.pixels,destination.bytes/2,destination.stride,destination.width,destination.height,outputWords)||
     !nativeWireCopyFootprint(source.pixels,source.bytes/2,source.stride,source.width,source.height,inputWords)||
     area.x<0||area.y<0||area.width<=0||area.height<=0||
     uint64_t(area.x)+uint64_t(area.width)>destination.width||uint64_t(area.y)+uint64_t(area.height)>destination.height||
     uint64_t(area.x)+uint64_t(area.width)>source.width||uint64_t(area.y)+uint64_t(area.height)>source.height)return Status::Unsupported;
  if(accelerated&&area.pixels()>=16384u){
    const PixelView input(source.pixels,source.width,source.height,source.stride,PixelOrder::Native);
    const Status copied=copier.copyIntoWire(destination,area.x,area.y,input,source.bytes,
      size_t(area.x),size_t(area.y),size_t(area.width),size_t(area.height));
    if(copied!=Status::Unsupported)return copied;
  }
  const size_t outputOffset=size_t(area.y)*destination.stride+size_t(area.x);
  const size_t inputOffset=size_t(area.y)*source.stride+size_t(area.x);
  return copyNativeRgb565ToWire(destination.pixels+outputOffset,destination.bytes/2-outputOffset,destination.stride,
    source.pixels+inputOffset,source.bytes/2-inputOffset,source.stride,size_t(area.width),size_t(area.height))?Status::Ok:Status::Unsupported;
}
}}}
