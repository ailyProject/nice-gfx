#pragma once
// Optional exact AA4/native RGB565 span kernel. Compile and runtime gates
// default OFF; enable only after its separate hardware self-test passes.
#include "Rgb565NativeSpan.h"
#include <stddef.h>
#include <stdint.h>
#ifndef NICE_GFX_S31_PIE_TEXT_BLEND
#define NICE_GFX_S31_PIE_TEXT_BLEND 0
#endif
#if NICE_GFX_S31_PIE_TEXT_BLEND && NICE_GFX_S31_PIE_NATIVE_BLEND_AVAILABLE
#define NICE_GFX_S31_PIE_TEXT_BLEND_AVAILABLE 1
#else
#define NICE_GFX_S31_PIE_TEXT_BLEND_AVAILABLE 0
#endif

namespace nice_gfx { namespace platform { namespace rgb565_text_span {
struct Stats {bool enabled=false;uint64_t blocks=0,gateRejects=0;};
inline Stats &state(){static Stats value;return value;}
inline Stats stats(){return state();}
inline bool available(){return NICE_GFX_S31_PIE_TEXT_BLEND_AVAILABLE;}
inline void setEnabled(bool enable){state().enabled=enable&&available();}
inline bool canUseNow(){
#if NICE_GFX_S31_PIE_TEXT_BLEND_AVAILABLE
  return state().enabled&&rgb565_native_span::stats().enabled&&rgb565_native_span::canUseNow();
#else
  return false;
#endif
}
struct Result {bool painted=false;size_t piePixels=0;};
namespace detail {
// Sequential folding is essential. The parallel expression
// (q+(q>>4)+(q>>8))>>4 is WRONG for n=510,750,765 in this domain.
inline uint32_t divide15(uint32_t n){uint32_t q=n+1;q+=q>>4;q+=q>>8;return q>>4;}
inline uint32_t divide255(uint32_t n){return (n+1+(n>>8))>>8;}
inline uint32_t channel(uint32_t foreground,uint32_t background,uint8_t coverage,uint8_t alpha){
  const uint32_t aa=divide15(foreground*coverage+background*(15-coverage)+7);
  return divide255(aa*alpha+background*(255-alpha)+127);
}
inline uint16_t color(uint16_t foreground,uint16_t background,uint8_t coverage,uint8_t alpha){
  return uint16_t(channel(foreground>>11,background>>11,coverage,alpha)<<11|
    channel((foreground>>5)&63,(background>>5)&63,coverage,alpha)<<5|
    channel(foreground&31,background&31,coverage,alpha));
}
inline void scalar(uint16_t *pixels,const uint8_t *mask,size_t count,uint16_t foreground,uint8_t alpha){
  if(!alpha)return;
  for(size_t i=0;i<count;++i){if(!mask[i])continue;const uint16_t before=pixels[i];const uint8_t coverage=mask[i]>>4;
    const uint16_t after=alpha==255&&coverage==15?foreground:color(foreground,before,coverage,alpha);if(after!=before)pixels[i]=after;}
}

#if NICE_GFX_S31_PIE_TEXT_BLEND_AVAILABLE
#define XJ_TEXT_BC7(OFFSET) "addi t5,%[parameters]," #OFFSET "\nesp.vldbc.16.ip q7,t5,0\n"
#define XJ_TEXT_EXTRACT5(SHIFT,MASK) XJ_TEXT_BC7(SHIFT) "esp.vsld.16 q5,q0,q7\n" XJ_TEXT_BC7(MASK) "esp.andq q5,q5,q7\n"
#define XJ_TEXT_EXTRACT6(SHIFT,MASK) XJ_TEXT_BC7(SHIFT) "esp.vsld.16 q6,q0,q7\n" XJ_TEXT_BC7(MASK) "esp.andq q6,q6,q7\n"
#define XJ_TEXT_BLUE5 XJ_TEXT_BC7(20) "esp.andq q5,q0,q7\n"
#define XJ_TEXT_BLUE6 XJ_TEXT_BC7(20) "esp.andq q6,q0,q7\n"
// q0 original RGB565; q1 coverage 0..15; q2 15-coverage; q3 shift(-4);
// q4 packed output; q5 current numerator; q6/q7 scratch/constants.
// AA numerator <=952, sequential correction <=1015; opacity numerator
// <=16192, correction <=16256. No signed saturation or U16 multiply wraps.
#define XJ_TEXT_CHANNEL(EXTRACT5,EXTRACT6,FOREGROUND,PACK) \
  EXTRACT5 "esp.vmul.u16 q5,q5,q2\n" XJ_TEXT_BC7(FOREGROUND) \
  "esp.vmul.u16 q6,q1,q7\nesp.vadd.u16 q5,q5,q6\nesp.vsadds.s16 q5,q5,a2\n" \
  "esp.vsld.16 q6,q5,q3\nesp.vadd.u16 q5,q5,q6\n" \
  "esp.vsld.16 q6,q5,q3\nesp.vsld.16 q6,q6,q3\nesp.vadd.u16 q5,q5,q6\nesp.vsld.16 q5,q5,q3\n" \
  "bnez %[opaque],1f\n" EXTRACT6 XJ_TEXT_BC7(26) "esp.vmul.u16 q6,q6,q7\n" \
  XJ_TEXT_BC7(24) "esp.vmul.u16 q5,q5,q7\nesp.vadd.u16 q5,q5,q6\nesp.vsadds.s16 q5,q5,a3\n" \
  XJ_TEXT_BC7(28) "esp.vsld.16 q6,q5,q7\nesp.vsadds.s16 q5,q5,a4\nesp.vadd.u16 q5,q5,q6\nesp.vsld.16 q5,q5,q7\n1:\n" PACK

static __attribute__((noinline)) IRAM_ATTR void pie(uint16_t *pixels,const uint8_t *mask,size_t blocks,uint16_t foreground,uint8_t alpha){
  alignas(16) const uint32_t parameters[]={uint32_t(uint16_t(-4))|(15u<<16),uint32_t(uint16_t(-11))|(31u<<16),
    uint32_t(foreground>>11)|(2048u<<16),uint32_t(uint16_t(-5))|(63u<<16),
    uint32_t((foreground>>5)&63)|(32u<<16),31u|(uint32_t(foreground&31)<<16),
    uint32_t(alpha)|(uint32_t(255-alpha)<<16),uint32_t(uint16_t(-8))};
  const uint32_t opaque=alpha==255;
  asm volatile(
    ".option push\n.option arch,+xespv\nesp.movx.r.sar t3\nesp.movx.r.cfg a5\n"
    "li t4,2\nesp.movx.w.cfg t4\nli t4,0\nesp.movx.w.sar t4\n"
    "mv t6,%[pixels]\nmv a1,%[mask]\nmv t5,%[parameters]\nesp.vldbc.16.ip q3,t5,0\n"
    "li a2,8\nli a3,127\nli a4,1\n9:\n"
    // This official esp-dl pair reads exactly eight A8 bytes.
    // q7 receives unused upper input lanes; no 16-byte source overread.
    "esp.vld.l.64.ip q5,a1,8\nesp.vext.u8 q1,q7,q5\nesp.vsld.16 q1,q1,q3\n"
    XJ_TEXT_BC7(2) "esp.xorq q2,q1,q7\nesp.vld.128.ip q0,t6,0\n"
    XJ_TEXT_CHANNEL(XJ_TEXT_EXTRACT5(4,6),XJ_TEXT_EXTRACT6(4,6),8,
      XJ_TEXT_BC7(10) "esp.vmul.u16 q5,q5,q7\n")
    "esp.orq q4,q5,q5\n"
    XJ_TEXT_CHANNEL(XJ_TEXT_EXTRACT5(12,14),XJ_TEXT_EXTRACT6(12,14),16,
      XJ_TEXT_BC7(18) "esp.vmul.u16 q5,q5,q7\n")
    "esp.orq q4,q4,q5\n"
    XJ_TEXT_CHANNEL(XJ_TEXT_BLUE5,XJ_TEXT_BLUE6,22,"")
    "esp.orq q4,q4,q5\nesp.vst.128.ip q4,t6,16\naddi %[blocks],%[blocks],-1\nbnez %[blocks],9b\n"
    "esp.movx.w.sar t3\nesp.movx.w.cfg a5\n.option pop\n"
    :[blocks] "+&r"(blocks)
    :[pixels] "r"(pixels),[mask] "r"(mask),[parameters] "r"(parameters),[opaque] "r"(opaque)
    :"a1","a2","a3","a4","a5","t3","t4","t5","t6","memory");
}
#undef XJ_TEXT_CHANNEL
#undef XJ_TEXT_BLUE6
#undef XJ_TEXT_BLUE5
#undef XJ_TEXT_EXTRACT6
#undef XJ_TEXT_EXTRACT5
#undef XJ_TEXT_BC7
#endif
} // namespace detail

// Complete synchronous borrowed span, never an asynchronous lease. A true
// trustedCoverage4 flag requires a valid immutable TextMaskCache certificate.
// Raw masks validate every byte before any write; overlap is unsupported.
// Default experiment gate is OFF even when the existing shape PIE gate is ON.
inline Result blend(uint16_t *pixels,const uint8_t *mask,size_t count,uint16_t foreground,uint8_t alpha,bool trustedCoverage4=false){
  if(!count)return {true,0};if(!pixels||!mask||(reinterpret_cast<uintptr_t>(pixels)&1)||count>SIZE_MAX/2)return {};
  const uintptr_t out=reinterpret_cast<uintptr_t>(pixels),in=reinterpret_cast<uintptr_t>(mask);
  if(count*2>UINTPTR_MAX-out||count>UINTPTR_MAX-in||(out<in+count&&in<out+count*2))return {};
  if(!trustedCoverage4)for(size_t i=0;i<count;++i)if(mask[i]%17)return {};
  if(!alpha)return {true,0};size_t vectorPixels=0;
#if NICE_GFX_S31_PIE_TEXT_BLEND_AVAILABLE
  namespace owner=nice_gfx::platform::rgb565_native_span;
  auto &s=state();
  if(s.enabled&&owner::stats().enabled&&owner::canUseNow()&&count>=32){
    const size_t prefix=((16-(reinterpret_cast<uintptr_t>(pixels)&15))&15)/2;
    detail::scalar(pixels,mask,prefix,foreground,alpha);pixels+=prefix;mask+=prefix;count-=prefix;
    vectorPixels=count&~size_t(7);
    if(vectorPixels){detail::pie(pixels,mask,vectorPixels/8,foreground,alpha);s.blocks+=vectorPixels/8;pixels+=vectorPixels;mask+=vectorPixels;count-=vectorPixels;}
  }else ++s.gateRejects;
#endif
  detail::scalar(pixels,mask,count,foreground,alpha);return {true,vectorPixels};
}
}}} // namespace nice_gfx::platform::rgb565_text_span
