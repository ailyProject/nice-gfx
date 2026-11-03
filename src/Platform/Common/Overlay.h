#pragma once
#include "../../Display/Transport.h"
#include <limits.h>
namespace nice_gfx { namespace platform {
enum class OverlayFormat : uint8_t { Rgb565, Alpha8, Argb8888 };
// Immutable, packed native-endian pixels. Stride is measured in pixels for
// every format. Alpha8 uses a fixed RGB565 color; RGB565 may enable an exact
// RGB565 color key. Inputs remain borrowed only until the blocking call ends.
struct OverlayView {
  const uint8_t *data=nullptr;size_t width=0,height=0,stride=0,bytes=0;
  OverlayFormat format=OverlayFormat::Rgb565;uint8_t alpha=255;
  uint16_t color=0,key=0;bool colorKey=false;
};
inline size_t overlayPixelBytes(OverlayFormat format){
  return format==OverlayFormat::Rgb565?2:format==OverlayFormat::Alpha8?1:format==OverlayFormat::Argb8888?4:0;
}
inline uint32_t overlayRgb565Rgb888(uint16_t color){return (uint32_t(color>>11)<<19)|(uint32_t((color>>5)&63)<<10)|(uint32_t(color&31)<<3);}
inline uint32_t overlayRgb565KeyHigh(uint16_t color){return overlayRgb565Rgb888(color)|0x070307u;}
inline uint16_t overlayAlphaFactor(uint8_t alpha){return uint16_t(unsigned(alpha)+1u);}
// Pure preflight, shared by hardware and host tests. PPA block fields are
// 14-bit. Require complete, cache-line-aligned allocations so row-range cache
// maintenance cannot invalidate unrelated allocator storage or dirty padding.
inline bool validOverlayRegion(const NativeFramebuffer &target,int32_t x,int32_t y,const OverlayView &source,
    size_t sourceX,size_t sourceY,size_t width,size_t height,size_t line){
  const size_t bpp=overlayPixelBytes(source.format);
  if(!line||(line&(line-1))||!target.pixels||!source.data||!bpp||!width||!height||x<0||y<0||
     !target.width||!target.height||target.stride<target.width||target.stride>=0x4000||target.height>=0x4000||
     !source.width||!source.height||source.stride<source.width||source.stride>=0x4000||source.height>=0x4000||
     sourceX>source.width||width>source.width-sourceX||sourceY>source.height||height>source.height-sourceY||
     size_t(x)>target.width||width>target.width-size_t(x)||size_t(y)>target.height||height>target.height-size_t(y)||
     target.height>SIZE_MAX/target.stride/2||source.height>SIZE_MAX/source.stride/bpp||
     (source.colorKey&&source.format!=OverlayFormat::Rgb565))return false;
  const size_t output=target.stride*target.height*2,input=source.stride*source.height*bpp;
  const uintptr_t out=reinterpret_cast<uintptr_t>(target.pixels),in=reinterpret_cast<uintptr_t>(source.data);
  return output<=UINT32_MAX&&input<=UINT32_MAX&&target.bytes>=output&&source.bytes>=input&&
    !(out%line)&&!(in%line)&&!(output%line)&&!(input%line)&&
    out<=UINTPTR_MAX-output&&in<=UINTPTR_MAX-input&&!(out<in+input&&in<out+output);
}
}} // namespace nice_gfx::platform
