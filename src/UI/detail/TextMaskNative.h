#pragma once
#include "TextMaskCache.h"
#include "Rgb565Blend.h"
#include "../../Display/Transport.h"
#ifndef NICE_GFX_S31_PIE_TEXT_BLEND
#define NICE_GFX_S31_PIE_TEXT_BLEND 0
#endif
#if NICE_GFX_S31_PIE_TEXT_BLEND
#include "../../Platform/ESP32/Rgb565TextSpan.h"
#endif
#ifndef NICE_GFX_UI_TEXT_MASK_NATIVE_LUT
#define NICE_GFX_UI_TEXT_MASK_NATIVE_LUT 0
#endif

namespace nice_gfx { namespace htmlui { namespace detail {

#if NICE_GFX_UI_TEXT_MASK_NATIVE_LUT
// A lazy 2 KiB stack table contains final channel values for every possible
// real background. Each row preserves /15 AA rounding before /255 node alpha;
// no background pixels are baked into the immutable mask or retained here.
class TextMaskNativeLut {
 public:
  TextMaskNativeLut(uint16_t foreground,uint8_t alpha):foreground_(foreground),alpha_(alpha) {}
  uint16_t color(uint16_t background,uint8_t coverage) {
    const uint16_t bit=uint16_t(1u<<coverage);
    if(!(ready_&bit)){build(coverage);ready_|=bit;}
    const uint8_t *table=channels_[coverage];
    return uint16_t(uint16_t(table[background>>11])<<11|
      uint16_t(table[32+((background>>5)&63)])<<5|table[96+(background&31)]);
  }
 private:
  void build(uint8_t coverage) {
    uint8_t *table=channels_[coverage];
    const auto channel=[&](unsigned foreground,unsigned background){
      const unsigned antialiased=(foreground*coverage+background*(15-coverage)+7)/15;
      return uint8_t(rgb565_blend::divide255(antialiased*alpha_+background*(255-alpha_)+127));
    };
    for(unsigned background=0;background<32;++background){
      table[background]=channel(foreground_>>11,background);
      table[96+background]=channel(foreground_&31,background);
    }
    for(unsigned background=0;background<64;++background)table[32+background]=channel((foreground_>>5)&63,background);
  }
  uint8_t channels_[16][128];
  uint16_t foreground_,ready_=0;uint8_t alpha_;
};
#endif

inline bool validNativeTextMaskRegion(const NativeFramebuffer &target,int32_t x,int32_t y,
    const TextMaskView &mask,size_t sourceX,size_t sourceY,size_t width,size_t height){
  if(!target.pixels||!mask.data||!width||!height||x<0||y<0||!target.width||!target.height||
     target.stride<target.width||!mask.width||!mask.height||mask.stride<mask.width||
     size_t(x)>target.width||width>target.width-size_t(x)||size_t(y)>target.height||height>target.height-size_t(y)||
     sourceX>mask.width||width>mask.width-sourceX||sourceY>mask.height||height>mask.height-sourceY||
     target.height>SIZE_MAX/target.stride/2||mask.height>SIZE_MAX/mask.stride)return false;
  const size_t output=target.stride*target.height*2,input=mask.stride*mask.height;
  const uintptr_t out=reinterpret_cast<uintptr_t>(target.pixels),in=reinterpret_cast<uintptr_t>(mask.data);
  return !(out%alignof(uint16_t))&&target.bytes>=output&&mask.bytes>=input&&
    out<=UINTPTR_MAX-output&&in<=UINTPTR_MAX-input&&!(out<in+input&&in<out+output);
}

// Capture stores exact four-bit glyph coverage as c*17. Keep the CPU painter's
// two separate rounding stages: AA over the current native RGB565 background,
// then node/ancestor opacity over that same background. Combining those two
// alphas first would change integer results, especially at small coverage.
// Only ink pixels read/write the destination. Padding and zero-coverage spans
// never trigger an underlay copy or a framebuffer load/store.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((optimize("O3")))
#endif
inline bool blendTextMaskIntoNative(const NativeFramebuffer &target,int32_t x,int32_t y,
    const TextMaskView &mask,size_t sourceX,size_t sourceY,size_t width,size_t height,
    uint16_t foreground,uint8_t alpha){
  if(!validNativeTextMaskRegion(target,x,y,mask,sourceX,sourceY,width,height))return false;
  if(!alpha||mask.empty)return true;
  // Complete validation precedes every destination write, so an unsupported
  // non-four-bit mask can safely fall back to the ordinary node painter.
  if(!mask.trustedCoverage4())for(size_t row=0;row<height;++row){const uint8_t *coverage=mask.data+(sourceY+row)*mask.stride+sourceX;
    for(size_t col=0;col<width;++col)if(coverage[col]%17)return false;
  }
#if NICE_GFX_UI_TEXT_MASK_NATIVE_LUT
  TextMaskNativeLut lut(foreground,alpha);
#endif
#if NICE_GFX_S31_PIE_TEXT_BLEND
  const bool textPieAllowed=width>=32&&platform::rgb565_text_span::canUseNow();
#endif
  for(size_t row=0;row<height;++row){
    const uint8_t *coverage=mask.data+(sourceY+row)*mask.stride+sourceX;
    uint16_t *pixels=target.pixels+(size_t(y)+row)*target.stride+size_t(x);
#if NICE_GFX_S31_PIE_TEXT_BLEND
    // Global region/overlap and every raw mask byte were validated above.
    // Complete rows let the vector kernel consume varying AA4 coverage; its
    // alignment prefix/tail are exact scalar. A rejected row remains intact
    // and retains the original painter below, even after earlier rows finish.
    if(textPieAllowed&&platform::rgb565_text_span::blend(pixels,coverage,width,foreground,alpha,true).painted)continue;
#endif
    size_t col=0;
    while(col<width){
      while(col<width&&!coverage[col])++col;
      const size_t first=col;while(col<width&&coverage[col])++col;
      for(size_t ink=first;ink<col;++ink){
        const uint8_t level=uint8_t(coverage[ink]/17);
        if(alpha==255&&level==15){pixels[ink]=foreground;continue;}
        const uint16_t background=pixels[ink];
#if NICE_GFX_UI_TEXT_MASK_NATIVE_LUT
        const uint16_t result=lut.color(background,level);
#else
        const uint16_t antialiased=level==15?foreground:textCoverageColor(foreground,background,level);
        const uint16_t result=alpha==255?antialiased:rgb565_blend::color(antialiased,background,alpha);
#endif
        if(result!=background)pixels[ink]=result;
      }
    }
  }
  return true;
}

}}} // namespace nice_gfx::htmlui::detail
