#pragma once
#include "Rgb565Blend.h"

namespace nice_gfx { namespace htmlui { namespace detail {

// Composite a proven solid interior directly. Border/corner/AA strips remain
// on the ordinary ordered painter; children keep their independent opacity.
// Validate the complete destination before the first write.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("O3")))
#endif
inline bool blendContainerNativeInterior(uint16_t *pixels,size_t capacity,size_t stride,
                                         size_t width,size_t height,uint16_t color,uint8_t alpha) {
  if(!width||!height)return true;
  if(!pixels||stride<width||height-1>(SIZE_MAX-width)/stride)return false;
  const size_t words=(height-1)*stride+width;
  const uintptr_t address=reinterpret_cast<uintptr_t>(pixels);
  if(words>capacity||words>SIZE_MAX/sizeof(uint16_t)||address%alignof(uint16_t)||
     address>UINTPTR_MAX-words*sizeof(uint16_t))return false;
  if(!alpha)return true;
#if defined(__GNUC__)
  typedef uint32_t ContainerAliasWord __attribute__((__may_alias__));
  const uint32_t foreground=uint32_t(color)|(uint32_t(color)<<16);
  uint32_t previous=0,previousResult=0;bool cached=false;
#endif
  for(size_t y=0;y<height;++y){
    auto *row=pixels+y*stride;size_t remaining=width;
#if defined(__GNUC__)
    if((reinterpret_cast<uintptr_t>(row)&3u)&&remaining){
      *row=alpha==255?color:rgb565_blend::color(color,*row,alpha);++row;--remaining;
    }
    auto *pairs=reinterpret_cast<ContainerAliasWord*>(row);
    size_t count=remaining/2;
    if(alpha==255){while(count--)*pairs++=foreground;}
    else while(count--){
      const uint32_t background=*pairs;
      if(!cached||background!=previous){
        previous=background;previousResult=background==foreground?foreground:rgb565_blend::colorPair(foreground,background,alpha);cached=true;
      }
      *pairs++=previousResult;
    }
    row=reinterpret_cast<uint16_t*>(pairs);remaining&=1u;
#endif
    while(remaining--){*row=alpha==255?color:rgb565_blend::color(color,*row,alpha);++row;}
  }
  return true;
}

}}} // namespace nice_gfx::htmlui::detail
