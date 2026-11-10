#pragma once
#include "MediaShapeIram.h"
#include "ShapeOverlayCache.h"
#include "Rgb565Blend.h"
#include "../../Display/Transport.h"
#include "../../Platform/ESP32/Rgb565NativeSpan.h"
#include "../../Platform/ESP32/Rgb565P4Memory.h"
#ifndef NICE_GFX_UI_SHAPE_SOLID_LUT
// P4 360 MHz device comparison found this slower than packed colorPair.
// Keep it opt-in for experiments; exactness alone does not justify enabling it.
#define NICE_GFX_UI_SHAPE_SOLID_LUT 0
#endif
namespace nice_gfx { namespace htmlui { namespace detail {
// Composite only final opaque foreground runs. No transparent rectangle scan,
// byte conversion, saved underlay or hardware transaction occurs on a hit.
template<bool Wire=false>
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("O3")))
#elif defined(__GNUC__)
__attribute__((noinline))
#endif
NICE_GFX_UI_MEDIA_SHAPE_IRAM_ATTR inline Status paintShapeNativeSpans(const NativeFramebuffer &frame,const ShapeOverlayCache::NativeTexture &texture,
                                   Rect box,Rect clip,uint8_t alpha){
  if(!frame.pixels||!frame.width||!frame.height||frame.width>INT32_MAX||frame.height>INT32_MAX||
     frame.stride<frame.width||frame.stride>SIZE_MAX/2/frame.height||frame.bytes<((frame.height-1)*frame.stride+frame.width)*2||
     reinterpret_cast<uintptr_t>(frame.pixels)%alignof(uint16_t)||
     box.width<=0||box.height<=0||texture.width!=size_t(box.width)||texture.height!=size_t(box.height)||
     texture.count>texture.bytes/sizeof(ShapeOverlayCache::NativeSpan)||(texture.count&&!texture.spans))return Status::Unsupported;
  // Establish signed-32-bit endpoints once; every validated run is contained
  // in this box and can then clip without RV32's per-run 64-bit arithmetic.
  const int64_t boxRight=int64_t(box.x)+box.width,boxBottom=int64_t(box.y)+box.height;
  if(boxRight>INT32_MAX||boxBottom>INT32_MAX)return Status::Unsupported;
  const uintptr_t target=reinterpret_cast<uintptr_t>(frame.pixels),source=reinterpret_cast<uintptr_t>(texture.spans);
  if(target>UINTPTR_MAX-frame.bytes||source>UINTPTR_MAX-texture.bytes||
     (texture.bytes&&source<target+frame.bytes&&target<source+texture.bytes))return Status::Unsupported;
  const uintptr_t argb=reinterpret_cast<uintptr_t>(texture.argb);
  if((texture.argbBytes&&!texture.argb)||argb>UINTPTR_MAX-texture.argbBytes||
     (texture.argbBytes&&argb<target+frame.bytes&&target<argb+texture.argbBytes))return Status::Unsupported;
  if(!texture.trustedNativeSpans()){
    size_t previousY=0,previousEnd=0;
    for(size_t i=0;i<texture.count;++i){const auto &span=texture.spans[i];
      if(!span.width||span.y>=texture.height||span.x>=texture.width||span.width>texture.width-span.x||
         (i&&(span.y<previousY||(span.y==previousY&&span.x<previousEnd))))return Status::Unsupported;
      previousY=span.y;previousEnd=size_t(span.x)+span.width;
    }
  }
  if(!alpha||clip.width<=0||clip.height<=0)return Status::Ok;
  const int32_t clipLeft=std::max<int32_t>(0,std::max(box.x,clip.x)),clipTop=std::max<int32_t>(0,std::max(box.y,clip.y));
  const int64_t right64=std::min<int64_t>(frame.width,std::min(boxRight,int64_t(clip.x)+clip.width));
  const int64_t bottom64=std::min<int64_t>(frame.height,std::min(boxBottom,int64_t(clip.y)+clip.height));
  if(right64<=clipLeft||bottom64<=clipTop)return Status::Ok;
  const int32_t clipRight=int32_t(right64),clipBottom=int32_t(bottom64);
  const auto firstRow=[&](int32_t y) NICE_GFX_UI_MEDIA_SHAPE_IRAM_ATTR {size_t first=0,last=texture.count;while(first<last){const size_t middle=first+(last-first)/2;if(texture.spans[middle].y<y)first=middle+1;else last=middle;}return first;};
  const int32_t firstY=clipTop-box.y,lastY=clipBottom-box.y;
  const size_t first=firstY>0?firstRow(firstY):0,last=lastY<box.height?firstRow(lastY):texture.count;
#if NICE_GFX_UI_SHAPE_SOLID_LUT
  // A shape contains long runs of the same fill/stroke colors. Build at most
  // two small channel tables in internal task-stack memory. Reversing alpha
  // in the existing exact LUT blends a constant foreground onto any live
  // background, preserving all three original /255 rounding operations.
  rgb565_blend::SolidBackgroundLut solidLuts[2];uint16_t solidColors[2]{};size_t solidCount=0;
#endif
  for(size_t i=first;i<last;++i){const auto &span=texture.spans[i];const int32_t row=box.y+span.y,spanLeft=box.x+span.x;
    const int32_t left=std::max(clipLeft,spanLeft),right=std::min(clipRight,spanLeft+span.width);
    if(right<=left)continue;uint16_t *out=frame.pixels+size_t(row)*frame.stride+size_t(left);size_t count=size_t(right-left);
#if NICE_GFX_UI_SHAPE_SOLID_LUT
    if(alpha<255){
      size_t cached=0;while(cached<solidCount&&solidColors[cached]!=span.color)++cached;
      if(cached==solidCount&&solidCount<2&&count>=64){solidColors[cached]=span.color;solidLuts[cached].reset(span.color,uint8_t(255-alpha));++solidCount;}
      if(cached<solidCount){
        const auto &lut=solidLuts[cached];
        while(count--){const uint16_t background=Wire?rgb565_blend::fromWire(*out):*out;const uint16_t color=lut.lookupNative(background);*out++=Wire?rgb565_blend::toWire(color):color;}
        continue;
      }
    }
#endif
#if NICE_GFX_S31_PIE_NATIVE_BLEND
    // blend() consumes the entire span, including its safe CPU fallback.
    // Its return value counts PIE pixels and must never trigger a second blend.
    if(!Wire&&alpha<255){(void)platform::rgb565_native_span::blend(out,count,span.color,alpha);continue;}
#endif
    const uint16_t stored=Wire?rgb565_blend::toWire(span.color):span.color;
    if(alpha==255&&platform::rgb565_p4_memory::fill(out,count,stored))continue;
    const auto scalar=[&]() NICE_GFX_UI_MEDIA_SHAPE_IRAM_ATTR {if(alpha==255)*out=stored;else if(*out!=stored){const uint16_t blended=rgb565_blend::color(span.color,Wire?rgb565_blend::fromWire(*out):*out,alpha);*out=Wire?rgb565_blend::toWire(blended):blended;}++out;--count;};
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    if((reinterpret_cast<uintptr_t>(out)&3)&&count)scalar();
    typedef uint32_t NativeSpanPair __attribute__((__may_alias__));const uint32_t front=uint32_t(stored)|(uint32_t(stored)<<16);
    if(alpha==255){
      while(count>=16){auto *pair=reinterpret_cast<NativeSpanPair *>(out);pair[0]=front;pair[1]=front;pair[2]=front;pair[3]=front;pair[4]=front;pair[5]=front;pair[6]=front;pair[7]=front;out+=16;count-=16;}
      while(count>=2){*reinterpret_cast<NativeSpanPair *>(out)=front;out+=2;count-=2;}
    }else while(count>=2){auto *pair=reinterpret_cast<NativeSpanPair *>(out);if(*pair!=front){const uint32_t blended=rgb565_blend::colorPair(Wire?rgb565_blend::swapPairBytes(front):front,Wire?rgb565_blend::swapPairBytes(*pair):*pair,alpha);*pair=Wire?rgb565_blend::swapPairBytes(blended):blended;}out+=2;count-=2;}
#endif
    while(count)scalar();
  }
  return Status::Ok;
}
}}}
