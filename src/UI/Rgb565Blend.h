#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace nice_gfx { namespace htmlui { namespace rgb565_blend {

// The tile and its saved underlay both contain big-endian RGB565 bytes.
// Reading a word through its bytes lets the compiler select the native swap
// instruction without depending on platform-specific endian definitions.
inline uint16_t fromWire(uint16_t stored) {
  const uint8_t *bytes=reinterpret_cast<const uint8_t *>(&stored);
  return uint16_t(uint16_t(bytes[0])<<8|bytes[1]);
}
inline uint16_t toWire(uint16_t color) {
  uint16_t stored;
  uint8_t *bytes=reinterpret_cast<uint8_t *>(&stored);
  bytes[0]=uint8_t(color>>8);bytes[1]=uint8_t(color);
  return stored;
}

// For the 5/6-bit channels the rounded numerator is at most 63*255+127.
// This identity is exactly /255 over that range and uses only 32-bit adds
// and shifts, including on MCUs without divide or high-half multiply.
inline uint32_t divide255(uint32_t numerator) {
  return (numerator+1u+(numerator>>8))>>8;
}
// Match the renderer's original independent 5/6/5-channel rounding exactly.
inline uint16_t color(uint16_t foreground,uint16_t background,uint8_t alpha) {
  const uint32_t inverse=255u-alpha;
  const uint32_t r=divide255((foreground>>11)*alpha+(background>>11)*inverse+127u);
  const uint32_t g=divide255(((foreground>>5)&63u)*alpha+((background>>5)&63u)*inverse+127u);
  const uint32_t b=divide255((foreground&31u)*alpha+(background&31u)*inverse+127u);
  return uint16_t((r<<11)|(g<<5)|b);
}

// Two independent 16-bit lanes fit each weighted 5/6-bit numerator. No sum
// reaches the next lane, so integer multiply/add handles two pixels exactly.
// This keeps the original per-channel /255 rounding, rather than quantizing
// alpha to a cheaper 5-bit blend. Inputs/output are native RGB565 word pairs.
inline uint32_t divide255Pair(uint32_t numerator) {
  return ((numerator+0x00010001u+((numerator>>8)&0x00ff00ffu))>>8)&0x00ff00ffu;
}
inline uint32_t colorPair(uint32_t foreground,uint32_t background,uint8_t alpha) {
  const uint32_t inverse=255u-alpha;
  const uint32_t r=divide255Pair(((foreground>>11)&0x001f001fu)*alpha+
    ((background>>11)&0x001f001fu)*inverse+0x007f007fu);
  const uint32_t g=divide255Pair(((foreground>>5)&0x003f003fu)*alpha+
    ((background>>5)&0x003f003fu)*inverse+0x007f007fu);
  const uint32_t b=divide255Pair((foreground&0x001f001fu)*alpha+
    (background&0x001f001fu)*inverse+0x007f007fu);
  return (r<<11)|(g<<5)|b;
}
inline uint32_t swapPairBytes(uint32_t pair) {
  return ((pair&0x00ff00ffu)<<8)|((pair>>8)&0x00ff00ffu);
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

// The caller has already clipped the rectangle and owns writable foreground
// rows. Underlay rows are either disjoint or exactly the same rectangle.
// Strides are in pixels; row padding is never read or modified.
inline void wireRows(uint16_t *foreground,size_t foregroundStride,
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
          const uint16_t blended=toWire(color(fromWire(firstForeground),fromWire(firstBackground),alpha));
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
            if(f!=b)*output=swapPairBytes(colorPair(swapPairBytes(f),swapPairBytes(b),alpha));
          }
        }
#endif
        for(;x<width;++x)foreground[x]=toWire(color(fromWire(foreground[x]),fromWire(background[x]),alpha));
      }
    }
    // Do not form pointers beyond the final visible row of padded buffers.
    if(row+1<height){foreground+=foregroundStride;background+=backgroundStride;}
  }
}

}}} // namespace nice_gfx::htmlui::rgb565_blend
