#pragma once
#include <stddef.h>
#include <stdint.h>

namespace nice_gfx { namespace detail {

// Constant 16-bit words need no source-row readback. The stored color can be
// native or wire RGB565; repeating its two halves preserves either byte order.
// GNU may_alias permits word stores into the borrowed uint16_t pixel array.
#if defined(__GNUC__) || defined(__clang__)
typedef uint32_t FillWord32 __attribute__((__may_alias__));
#endif
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((optimize("O3")))
#endif
inline bool fillWordRect16(uint16_t *pixels,size_t capacity,size_t width,size_t height,size_t stride,uint16_t color) {
  if(!pixels||!width||!height||stride<width||width>capacity||
     height-1>(capacity-width)/stride||capacity>SIZE_MAX/sizeof(uint16_t)||
     reinterpret_cast<uintptr_t>(pixels)%alignof(uint16_t)||
     reinterpret_cast<uintptr_t>(pixels)>UINTPTR_MAX-capacity*sizeof(uint16_t))return false;
#if defined(__GNUC__) || defined(__clang__)
  if(width==stride){width*=height;height=1;stride=width;}
  const uint32_t pair=uint32_t(color)|(uint32_t(color)<<16);
  for(size_t y=0;y<height;++y){
    uint16_t *row=pixels+y*stride;size_t remaining=width;
    if(reinterpret_cast<uintptr_t>(row)&3u){*row++=color;--remaining;}
    auto *words=reinterpret_cast<FillWord32*>(row);
    while(remaining>=16){
      words[0]=pair;words[1]=pair;words[2]=pair;words[3]=pair;
      words[4]=pair;words[5]=pair;words[6]=pair;words[7]=pair;
      words+=8;remaining-=16;
    }
    while(remaining>=2){*words++=pair;remaining-=2;}
    if(remaining)*reinterpret_cast<uint16_t*>(words)=color;
  }
  return true;
#else
  (void)color;return false;
#endif
}

}} // namespace nice_gfx::detail
