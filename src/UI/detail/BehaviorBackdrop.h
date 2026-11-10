#pragma once
#include "../Compiled.h"
#include "Rgb565Blend.h"
#include "../../Display/Transport.h"

// Arduino's size-oriented build can outline the tiny span/color lambdas in
// this pixel loop. Keep speed optimization local to this integer-only helper;
// the rest of the UI retains the application's chosen optimization settings.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC push_options
#pragma GCC optimize ("O3")
#endif

namespace nice_gfx { namespace htmlui { namespace detail {

// Preserve the dialog painter's floor(channel * (255-dim) / 255). This is
// deliberately distinct from the rounded RGB565 alpha-blend operation.
struct BackdropFloorLut {
  uint16_t red[32],green[64],blue[32];
  explicit BackdropFloorLut(uint8_t dim) {
    const unsigned keep=255u-dim;
    for(unsigned i=0;i<32;++i){red[i]=uint16_t((i*keep/255u)<<11);blue[i]=uint16_t(i*keep/255u);}
    for(unsigned i=0;i<64;++i)green[i]=uint16_t((i*keep/255u)<<5);
  }
  uint16_t color(uint16_t value)const{return uint16_t(red[value>>11]|green[(value>>5)&63]|blue[value&31]);}
};
static_assert(sizeof(BackdropFloorLut)==256,"Dialog dim lookup has a bounded 256-byte stack cost");
struct BackdropWork {uint64_t pixels=0,spans=0;};

#ifndef NICE_GFX_UI_BACKDROP_WORDS
#if defined(CONFIG_IDF_TARGET_ESP32S31) && defined(__GNUC__)
#define NICE_GFX_UI_BACKDROP_WORDS 1
#else
#define NICE_GFX_UI_BACKDROP_WORDS 0
#endif
#endif

// RGB scanout is PSRAM-backed on S31. Keep both halves of aligned words in
// registers and issue one store per pair. This helper deliberately has no
// callback in its pixel loop: -Os may outline callbacks even inside an O3
// caller. GNU's alias type permits word access to the existing uint16_t store.
#if NICE_GFX_UI_BACKDROP_WORDS && defined(__GNUC__)
typedef uint32_t BackdropAliasWord __attribute__((__may_alias__));
template<bool Wire>
__attribute__((always_inline)) inline void dimBackdropWords(uint16_t *row,size_t count,const BackdropFloorLut &lut) {
  if(count&&(reinterpret_cast<uintptr_t>(row)&3u)){
    const uint16_t sample=*row,native=Wire?rgb565_blend::fromWire(sample):sample,result=lut.color(native);
    *row++=Wire?rgb565_blend::toWire(result):result;--count;
  }
  auto *words=reinterpret_cast<BackdropAliasWord*>(row);
  uint32_t previous=0,previousResult=0;bool cached=false;
  size_t pairs=count/2;
  while(pairs--){
    const uint32_t sample=*words;
    if(!cached||sample!=previous){
      const uint16_t first=uint16_t(sample),second=uint16_t(sample>>16);
      const uint16_t a=lut.color(Wire?rgb565_blend::fromWire(first):first);
      const uint16_t b=first==second?a:lut.color(Wire?rgb565_blend::fromWire(second):second);
      const uint16_t storedA=Wire?rgb565_blend::toWire(a):a,storedB=Wire?rgb565_blend::toWire(b):b;
      previous=sample;previousResult=uint32_t(storedA)|(uint32_t(storedB)<<16);cached=true;
    }
    *words++=previousResult;
  }
  if(count&1u){
    row=reinterpret_cast<uint16_t*>(words);const uint16_t sample=*row;
    const uint16_t result=lut.color(Wire?rgb565_blend::fromWire(sample):sample);
    *row=Wire?rgb565_blend::toWire(result):result;
  }
}
#endif

// Visit uncovered row spans, instead of testing the modal and every toast for
// every pixel. getToast(index,rect) exposes the frozen behavior snapshot and
// does not allocate a second rectangle collection. Overlapping toasts form a
// union; every surviving backdrop pixel is dimmed exactly once.
template<class GetToast>
inline bool dimBehaviorBackdrop(Rect area,Rect panel,uint16_t *pixels,size_t stride,
                                PixelOrder order,uint8_t dim,size_t toastCount,GetToast getToast,BackdropWork *work=nullptr) {
  if(area.width<=0||area.height<=0||!dim)return true;
  if(!pixels||stride<size_t(area.width)||size_t(area.height)>SIZE_MAX/stride)return false;
  const BackdropFloorLut lut(dim);
  const int64_t left=area.x,right=left+area.width,top=area.y;
  const int64_t panelRight=int64_t(panel.x)+panel.width,panelBottom=int64_t(panel.y)+panel.height;
#if !NICE_GFX_UI_BACKDROP_WORDS || !defined(__GNUC__)
  uint16_t previous=0,previousResult=0;bool cached=false;
#endif
  const auto paint=[&](int64_t y,int64_t begin,int64_t end){
    while(begin<end){
      int64_t coveredEnd=begin,clearEnd=end;
      for(size_t i=0;i<toastCount;++i){Rect toast;if(!getToast(i,toast)||toast.width<=0||toast.height<=0||y<toast.y||y>=int64_t(toast.y)+toast.height)continue;
        const int64_t finish=int64_t(toast.x)+toast.width;
        if(toast.x<=begin){if(finish>coveredEnd)coveredEnd=finish;}
        else if(toast.x<clearEnd)clearEnd=toast.x;
      }
      if(coveredEnd>begin){begin=coveredEnd<end?coveredEnd:end;continue;}
      if(work){work->pixels+=uint64_t(clearEnd-begin);++work->spans;}
      auto *row=pixels+size_t(y-top)*stride+size_t(begin-left);
      size_t remaining=size_t(clearEnd-begin);
#if NICE_GFX_UI_BACKDROP_WORDS && defined(__GNUC__)
      if(order==PixelOrder::Native)dimBackdropWords<false>(row,remaining,lut);
      else dimBackdropWords<true>(row,remaining,lut);
#else
      while(remaining--){
        const uint16_t sample=*row;
        if(!cached||sample!=previous){const uint16_t native=order==PixelOrder::Wire?rgb565_blend::fromWire(sample):sample;
          const uint16_t result=lut.color(native);previous=sample;previousResult=order==PixelOrder::Wire?rgb565_blend::toWire(result):result;cached=true;}
        *row++=previousResult;
      }
#endif
      begin=clearEnd;
    }
  };
  for(int64_t y=top;y<top+area.height;++y){
    if(panel.width<=0||panel.height<=0||y<panel.y||y>=panelBottom){paint(y,left,right);continue;}
    const int64_t before=panel.x<right?panel.x:right,after=panelRight>left?panelRight:left;
    if(before>left)paint(y,left,before);
    if(after<right)paint(y,after,right);
  }
  return true;
}

}}} // namespace nice_gfx::htmlui::detail

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC pop_options
#endif
