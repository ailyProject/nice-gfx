#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "Rle565.h"

namespace nice_gfx { namespace htmlui { namespace rgb565_blend {

#if defined(__GNUC__)
#define NICE_GFX_BLEND_INLINE inline __attribute__((always_inline))
#else
#define NICE_GFX_BLEND_INLINE inline
#endif

// The tile and its saved underlay both contain big-endian RGB565 bytes.
// Reading a word through its bytes lets the compiler select the native swap
// instruction without depending on platform-specific endian definitions.
NICE_GFX_BLEND_INLINE uint16_t fromWire(uint16_t stored) {
  const uint8_t *bytes=reinterpret_cast<const uint8_t *>(&stored);
  return uint16_t(uint16_t(bytes[0])<<8|bytes[1]);
}
NICE_GFX_BLEND_INLINE uint16_t toWire(uint16_t color) {
  uint16_t stored;
  uint8_t *bytes=reinterpret_cast<uint8_t *>(&stored);
  bytes[0]=uint8_t(color>>8);bytes[1]=uint8_t(color);
  return stored;
}

// For the 5/6-bit channels the rounded numerator is at most 63*255+127.
// This identity is exactly /255 over that range and uses only 32-bit adds
// and shifts, including on MCUs without divide or high-half multiply.
NICE_GFX_BLEND_INLINE uint32_t divide255(uint32_t numerator) {
  return (numerator+1u+(numerator>>8))>>8;
}
// Match the renderer's original independent 5/6/5-channel rounding exactly.
NICE_GFX_BLEND_INLINE uint16_t color(uint16_t foreground,uint16_t background,uint8_t alpha) {
  const uint32_t inverse=255u-alpha;
  const uint32_t r=divide255((foreground>>11)*alpha+(background>>11)*inverse+127u);
  const uint32_t g=divide255(((foreground>>5)&63u)*alpha+((background>>5)&63u)*inverse+127u);
  const uint32_t b=divide255((foreground&31u)*alpha+(background&31u)*inverse+127u);
  return uint16_t((r<<11)|(g<<5)|b);
}

// Exact channel-separable blend against one real constant background. Tables
// cover every RGB565 foreground, including colors produced by glyph overlap.
// Construction costs 128 small channel samples; each lookup uses three loads.
class SolidBackgroundLut {
 public:
  SolidBackgroundLut(uint16_t background,uint8_t alpha) {
    const uint32_t inverse=255u-alpha;
    uint32_t r=(background>>11)*inverse+127u;
    uint32_t g=((background>>5)&63u)*inverse+127u;
    uint32_t b=(background&31u)*inverse+127u;
    for(unsigned i=0;i<32;++i,r+=alpha,b+=alpha){red_[i]=uint16_t(divide255(r)<<11);blue_[i]=uint16_t(divide255(b));}
    for(unsigned i=0;i<64;++i,g+=alpha)green_[i]=uint16_t(divide255(g)<<5);
  }
  NICE_GFX_BLEND_INLINE uint16_t lookupNative(uint16_t foreground) const {
    return uint16_t(red_[foreground>>11]|green_[(foreground>>5)&63u]|blue_[foreground&31u]);
  }
 private:
  uint16_t red_[32],green_[64],blue_[32];
};

// Two independent 16-bit lanes fit each weighted 5/6-bit numerator. No sum
// reaches the next lane, so integer multiply/add handles two pixels exactly.
// This keeps the original per-channel /255 rounding, rather than quantizing
// alpha to a cheaper 5-bit blend. Inputs/output are native RGB565 word pairs.
NICE_GFX_BLEND_INLINE uint32_t divide255Pair(uint32_t numerator) {
  return ((numerator+0x00010001u+((numerator>>8)&0x00ff00ffu))>>8)&0x00ff00ffu;
}
NICE_GFX_BLEND_INLINE uint32_t colorPair(uint32_t foreground,uint32_t background,uint8_t alpha) {
  const uint32_t inverse=255u-alpha;
  const uint32_t r=divide255Pair(((foreground>>11)&0x001f001fu)*alpha+
    ((background>>11)&0x001f001fu)*inverse+0x007f007fu);
  const uint32_t g=divide255Pair(((foreground>>5)&0x003f003fu)*alpha+
    ((background>>5)&0x003f003fu)*inverse+0x007f007fu);
  const uint32_t b=divide255Pair((foreground&0x001f001fu)*alpha+
    (background&0x001f001fu)*inverse+0x007f007fu);
  return (r<<11)|(g<<5)|b;
}
NICE_GFX_BLEND_INLINE uint32_t swapPairBytes(uint32_t pair) {
  return ((pair&0x00ff00ffu)<<8)|((pair>>8)&0x00ff00ffu);
}

// Immutable Wire565 page caches -> native RGB scanout, without a wire tile,
// a second byte-swap pass, or an underlay. Reuse exact repeated color pairs
// across flat UI spans; alpha and channel rounding remain bit-identical.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((optimize("O3")))
#endif
inline void nativeRowsFromWire(uint16_t *output,size_t outputStride,
                              const uint16_t *foreground,size_t foregroundStride,
                              const uint16_t *background,size_t backgroundStride,
                              size_t width,size_t height,uint8_t alpha,uint16_t solid=0) {
  if(!width||!height)return;
  uint32_t oldFront=0,oldBack=0,oldResult=0;bool previous=false;
  const uint32_t solidPair=uint32_t(solid)|(uint32_t(solid)<<16);
  for(size_t row=0;row<height;++row){
    size_t x=0;
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    typedef uint32_t BlendWord __attribute__((__may_alias__));
    if(!((reinterpret_cast<uintptr_t>(output)|reinterpret_cast<uintptr_t>(foreground)|reinterpret_cast<uintptr_t>(background))&3)){
      auto *out=reinterpret_cast<BlendWord *>(output);
      const auto *front=reinterpret_cast<const BlendWord *>(foreground);
      const auto *back=reinterpret_cast<const BlendWord *>(background);
      for(;x+1<width;x+=2,++out,++front){
        const uint32_t f=*front,b=back?*back:swapPairBytes(solidPair);
        const bool repeated=previous&&oldFront==f&&oldBack==b;
        uint32_t value;
        if(alpha==255||f==b)value=swapPairBytes(f);
        else if(!alpha)value=swapPairBytes(b);
        else if(repeated)value=oldResult;
        else value=colorPair(swapPairBytes(f),swapPairBytes(b),alpha);
        oldFront=f;oldBack=b;oldResult=value;previous=true;
        // Eight identical word pairs cover sixteen solid pixels. The exact
        // comparison authorizes an unrolled fill, while text/gradients retain
        // the ordinary two-pixel path. Never probe beyond the clipped row.
        // Probe a long flat run only after two equal pairs. Photographs
        // otherwise paid eight speculative PSRAM reads for every two pixels.
        if(repeated&&x+15<width&&
           !((front[1]^f)|(front[2]^f)|(front[3]^f)|(front[4]^f)|(front[5]^f)|(front[6]^f)|(front[7]^f))&&
           (!back||!((back[1]^b)|(back[2]^b)|(back[3]^b)|(back[4]^b)|(back[5]^b)|(back[6]^b)|(back[7]^b)))){
          out[0]=value;out[1]=value;out[2]=value;out[3]=value;out[4]=value;out[5]=value;out[6]=value;out[7]=value;
          x+=14;out+=7;front+=7;if(back)back+=8;
        }else{*out=value;if(back)++back;}
      }
    }
#endif
    for(;x<width;++x){const uint16_t f=fromWire(foreground[x]),b=background?fromWire(background[x]):solid;output[x]=alpha==255?f:!alpha?b:f==b?f:color(f,b,alpha);}
    if(row+1<height){output+=outputStride;foreground+=foregroundStride;if(background)background+=backgroundStride;}
  }
}

// Merge two run streams directly into native scanout. Flat panels and empty
// backgrounds read a few compressed words instead of two full PSRAM pages.
// Each intersecting run is blended once; glyph/gradient pixels remain exact.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((optimize("O3")))
#endif
inline bool nativeRunSpan(uint16_t *output,size_t count,rle565::Reader &front,
                          rle565::Reader *back,size_t sourceOffset,uint8_t alpha,uint16_t solid) {
  if(!front.seek(sourceOffset)||(back&&!back->seek(sourceOffset)))return false;
  while(count){
    if(!front.ready()||(back&&!back->ready()))return false;
    size_t run=count<front.remaining?count:front.remaining;if(back&&run>back->remaining)run=back->remaining;
    const uint16_t f=fromWire(front.color),b=back?fromWire(back->color):solid;
    const uint16_t value=alpha==255||f==b?f:!alpha?b:color(f,b,alpha);
    size_t written=run;
#if defined(__GNUC__)
    typedef uint32_t FillWord __attribute__((__may_alias__));
    if((reinterpret_cast<uintptr_t>(output)&3)&&written){*output++=value;--written;}
    auto *out=reinterpret_cast<FillWord *>(output);const uint32_t pair=uint32_t(value)|(uint32_t(value)<<16);
    while(written>=16){out[0]=pair;out[1]=pair;out[2]=pair;out[3]=pair;out[4]=pair;out[5]=pair;out[6]=pair;out[7]=pair;out+=8;written-=16;}
    while(written>=2){*out++=pair;written-=2;}output=reinterpret_cast<uint16_t *>(out);
#endif
    while(written--)*output++=value;
    front.remaining-=run;front.position+=run;if(back){back->remaining-=run;back->position+=run;}count-=run;
  }
  return true;
}

// Blend two immutable cached page rows directly into a separate tile. This
// avoids painting/copying both layers plus an underlay for a cached page fade.
inline void wireRowsFrom(uint16_t *output,size_t outputStride,
                         const uint16_t *foreground,size_t foregroundStride,
                         const uint16_t *background,size_t backgroundStride,
                         size_t width,size_t height,uint8_t alpha) {
  if(!width||!height)return;
  for(size_t row=0;row<height;++row){
    if(!alpha)memcpy(output,background,width*sizeof(uint16_t));
    else if(alpha==255)memcpy(output,foreground,width*sizeof(uint16_t));
    else {
      size_t x=0;
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      if(!((reinterpret_cast<uintptr_t>(output)|reinterpret_cast<uintptr_t>(foreground)|reinterpret_cast<uintptr_t>(background))&3)){
        typedef uint32_t BlendWord __attribute__((__may_alias__));
        auto *destination=reinterpret_cast<BlendWord *>(output);
        const auto *front=reinterpret_cast<const BlendWord *>(foreground);
        const auto *back=reinterpret_cast<const BlendWord *>(background);
        for(;x+1<width;x+=2,++destination,++front,++back){
          const uint32_t f=*front,b=*back;
          *destination=f==b?f:swapPairBytes(colorPair(swapPairBytes(f),swapPairBytes(b),alpha));
        }
      }
#endif
      for(;x<width;++x)output[x]=toWire(color(fromWire(foreground[x]),fromWire(background[x]),alpha));
    }
    if(row+1<height){output+=outputStride;foreground+=foregroundStride;background+=backgroundStride;}
  }
}

// A single cached page fades against the transition's constant background.
// Read the page once; avoid materializing a full background/underlay. The
// output may exactly alias foreground with the same stride (fresh tiles).
inline void wireRowsSolid(uint16_t *output,size_t outputStride,
                          const uint16_t *foreground,size_t foregroundStride,
                          size_t width,size_t height,uint16_t background,uint8_t alpha) {
  if(!width||!height)return;
  const uint16_t wireBackground=toWire(background);
  for(size_t row=0;row<height;++row){
    if(alpha==255)memcpy(output,foreground,width*sizeof(uint16_t));
    else if(!alpha){for(size_t x=0;x<width;++x)output[x]=wireBackground;}
    else {
      const uint16_t first=foreground[0];bool solid=false;
      if(width>=16&&foreground[1]==first&&foreground[3]==first&&
         foreground[width/2]==first&&foreground[width-1]==first){
        uint16_t different=0;for(size_t x=1;x<width;++x)different|=uint16_t(foreground[x]^first);
        if(!different){
          const uint16_t blended=toWire(color(fromWire(first),background,alpha));
          for(size_t x=0;x<width;++x)output[x]=blended;
          solid=true;
        }
      }
      if(!solid){
      size_t x=0;
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      if(!((reinterpret_cast<uintptr_t>(output)|reinterpret_cast<uintptr_t>(foreground))&3)){
        typedef uint32_t BlendWord __attribute__((__may_alias__));
        auto *destination=reinterpret_cast<BlendWord *>(output);
        const auto *front=reinterpret_cast<const BlendWord *>(foreground);
        const uint32_t back=uint32_t(background)|(uint32_t(background)<<16);
        const uint32_t wireBack=swapPairBytes(back);
        for(;x+1<width;x+=2,++destination,++front){
          const uint32_t f=*front;
          *destination=f==wireBack?f:swapPairBytes(colorPair(swapPairBytes(f),back,alpha));
        }
      }
#endif
      for(;x<width;++x){const uint16_t f=foreground[x];output[x]=f==wireBackground?f:toWire(color(fromWire(f),background,alpha));}
      }
    }
    if(row+1<height){output+=outputStride;foreground+=foregroundStride;}
  }
}

// The caller has already clipped the rectangle and owns writable foreground
// rows. Underlay rows are either disjoint or exactly the same rectangle.
// Strides are in pixels; row padding is never read or modified.
template<bool Wire> inline void orderedRows(uint16_t *foreground,size_t foregroundStride,
                     const uint16_t *background,size_t backgroundStride,
                     size_t width,size_t height,uint8_t alpha) {
  if(!width||!height||alpha==255||
     (foreground==background&&foregroundStride==backgroundStride))return;
  for(size_t row=0;row<height;++row) {
    if(!alpha)memcpy(foreground,background,width*sizeof(uint16_t));
    else {
      const uint16_t firstForeground=foreground[0],firstBackground=background[0];
      bool completed=false;
      // Solid rounded panels commonly have long constant interior rows. Cheap
      // samples reject gradients/images; only the exact full-row comparison
      // below authorizes reusing one blended color, without hashes or guesses.
      if(width>=16&&foreground[1]==firstForeground&&background[1]==firstBackground&&
         foreground[3]==firstForeground&&background[3]==firstBackground&&
         foreground[width/2]==firstForeground&&background[width/2]==firstBackground&&
         foreground[width-1]==firstForeground&&background[width-1]==firstBackground) {
        uint32_t different=0;
        for(size_t x=1;x<width;++x)
          different|=uint32_t(foreground[x]^firstForeground)|uint32_t(background[x]^firstBackground);
        if(!different) {
          const uint16_t blended=Wire?toWire(color(fromWire(firstForeground),fromWire(firstBackground),alpha)):
            color(firstForeground,firstBackground,alpha);
          for(size_t x=0;x<width;++x)foreground[x]=blended;
          completed=true;
        }
      }
      // Transparent text/shape rectangles also contain untouched rows. They
      // already equal their underlay and remain bit-identical at every alpha.
      if(!completed&&firstForeground==firstBackground&&
         !memcmp(foreground,background,width*sizeof(uint16_t)))completed=true;
      if(!completed){
        size_t x=0;
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        // RGB scanout uses PSRAM: one aligned word store costs substantially
        // less than two halfword stores. Other alignments retain the scalar
        // path, and an odd final pixel never reads beyond its visible row.
        if(!((reinterpret_cast<uintptr_t>(foreground)|reinterpret_cast<uintptr_t>(background))&3)){
          typedef uint32_t BlendWord __attribute__((__may_alias__));
          auto *output=reinterpret_cast<BlendWord *>(foreground);
          const auto *input=reinterpret_cast<const BlendWord *>(background);
          for(;x+1<width;x+=2,++output,++input){
            const uint32_t f=*output,b=*input;
            if(f!=b)*output=Wire?swapPairBytes(colorPair(swapPairBytes(f),swapPairBytes(b),alpha)):colorPair(f,b,alpha);
          }
        }
#endif
        for(;x<width;++x)foreground[x]=Wire?toWire(color(fromWire(foreground[x]),fromWire(background[x]),alpha)):
          color(foreground[x],background[x],alpha);
      }
    }
    // Do not form pointers beyond the final visible row of padded buffers.
    if(row+1<height){foreground+=foregroundStride;background+=backgroundStride;}
  }
}

inline void wireRows(uint16_t *foreground,size_t foregroundStride,const uint16_t *background,size_t backgroundStride,
                     size_t width,size_t height,uint8_t alpha) {
  orderedRows<true>(foreground,foregroundStride,background,backgroundStride,width,height,alpha);
}
inline void nativeRows(uint16_t *foreground,size_t foregroundStride,const uint16_t *background,size_t backgroundStride,
                       size_t width,size_t height,uint8_t alpha) {
  orderedRows<false>(foreground,foregroundStride,background,backgroundStride,width,height,alpha);
}

#undef NICE_GFX_BLEND_INLINE
}}} // namespace nice_gfx::htmlui::rgb565_blend
