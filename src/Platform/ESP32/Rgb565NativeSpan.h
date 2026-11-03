#pragma once
#include "../../Core/BuildConfig.h"
#include <stddef.h>
#include <stdint.h>
#ifndef NICE_GFX_S31_PIE_NATIVE_BLEND
#define NICE_GFX_S31_PIE_NATIVE_BLEND 0
#endif
#if NICE_GFX_S31_PIE_NATIVE_BLEND && (defined(ARDUINO_ARCH_ESP32) || NICE_GFX_NATIVE_IDF)
#include <sdkconfig.h>
#endif
#if NICE_GFX_S31_PIE_NATIVE_BLEND && defined(CONFIG_IDF_TARGET_ESP32S31) && \
    (defined(ARDUINO_ARCH_ESP32) || NICE_GFX_NATIVE_IDF) && defined(__GNUC__)
#define NICE_GFX_S31_PIE_NATIVE_BLEND_AVAILABLE 1
#include <esp_attr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#else
#define NICE_GFX_S31_PIE_NATIVE_BLEND_AVAILABLE 0
#endif

namespace nice_gfx { namespace platform { namespace rgb565_native_span {
// Single UI-owner task only. Compile the same macro value into every TU.
// Candidate builds start disabled: a successful explicit hardware self-test
// may enable the kernel. A trap/reboot restores the safe scalar path.
struct Stats {bool enabled=false;uint64_t blocks=0,gateRejects=0;};
inline bool available(){return NICE_GFX_S31_PIE_NATIVE_BLEND_AVAILABLE;}
#if NICE_GFX_S31_PIE_NATIVE_BLEND
inline Stats &state(){static Stats value;return value;}
inline Stats stats(){return state();}
inline void resetStats(){auto &s=state();s.blocks=s.gateRejects=0;}
inline void setEnabled(bool enabled){state().enabled=enabled&&available();}
#else
inline Stats stats(){return Stats{};}
inline void resetStats(){}
inline void setEnabled(bool){}
#endif
inline bool canUseNow(){
#if NICE_GFX_S31_PIE_NATIVE_BLEND_AVAILABLE
  return xPortGetCoreID()==1&&!xPortInIsrContext()&&xPortCanYield();
#else
  return false;
#endif
}

namespace detail {
inline uint32_t divide255(uint32_t n){return (n+1u+(n>>8))>>8;}
inline uint16_t color(uint16_t foreground,uint16_t background,uint8_t alpha){
  const uint32_t inverse=255u-alpha;
  return uint16_t((divide255((foreground>>11)*alpha+(background>>11)*inverse+127u)<<11)|
    (divide255(((foreground>>5)&63u)*alpha+((background>>5)&63u)*inverse+127u)<<5)|
    divide255((foreground&31u)*alpha+(background&31u)*inverse+127u));
}
inline uint32_t divide255Pair(uint32_t n){return ((n+0x00010001u+((n>>8)&0x00ff00ffu))>>8)&0x00ff00ffu;}
inline uint32_t colorPair(uint32_t foreground,uint32_t background,uint8_t alpha){
  const uint32_t inverse=255u-alpha;
  return (divide255Pair(((foreground>>11)&0x001f001fu)*alpha+((background>>11)&0x001f001fu)*inverse+0x007f007fu)<<11)|
    (divide255Pair(((foreground>>5)&0x003f003fu)*alpha+((background>>5)&0x003f003fu)*inverse+0x007f007fu)<<5)|
    divide255Pair((foreground&0x001f001fu)*alpha+(background&0x001f001fu)*inverse+0x007f007fu);
}
inline void scalar(uint16_t *pixels,size_t count,uint16_t foreground,uint8_t alpha){
  if(!alpha)return;
  if((reinterpret_cast<uintptr_t>(pixels)&3)&&count){*pixels=color(foreground,*pixels,alpha);++pixels;--count;}
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  typedef uint32_t Pair __attribute__((__may_alias__));
  const uint32_t front=uint32_t(foreground)|(uint32_t(foreground)<<16);
  while(count>=2){auto *pair=reinterpret_cast<Pair *>(pixels);
    if(alpha==255)*pair=front;else if(*pair!=front)*pair=colorPair(front,*pair,alpha);
    pixels+=2;count-=2;
  }
#endif
  while(count){if(alpha==255)*pixels=foreground;else if(*pixels!=foreground)*pixels=color(foreground,*pixels,alpha);++pixels;--count;}
}

#if NICE_GFX_S31_PIE_NATIVE_BLEND_AVAILABLE
// All eight U16 numerators stay <= 63*255+127=16192; even the divide255
// correction stays <=16256. No multiply truncation or signed-add saturation
// occurs. SAR=0 makes VMUL exact. Set CFG to the official esp-nn floor/misaligned
// load configuration and restore it afterwards. Only RIGHT VSLD shifts remain:
// packing high RGB bits with a signed saturating left shift produces 0x7fff
// on S31 (observed by the hardware self-test). Unsigned multiply by 2048/32
// packs red/green exactly, including red values >=16.
// Special-register MOVX only accepts a subset of GPRs: use t3/t4 explicitly.
static __attribute__((noinline)) IRAM_ATTR void pie(uint16_t *pixels,size_t blocks,uint16_t foreground,uint8_t alpha){
  // Pack pairs explicitly so GCC initializes only aligned 32-bit stack words.
  // VLDBC reads the individual little-endian halfwords from this table.
  alignas(16) const uint32_t parameters[]={uint32_t(255u-alpha)|(uint32_t(uint16_t(-8))<<16),
    uint32_t(uint16_t(-11))|(31u<<16),2048u|(uint32_t(uint16_t(-5))<<16),63u|(32u<<16),31u};
  const uint32_t red=(foreground>>11)*alpha+127u;
  const uint32_t green=((foreground>>5)&63u)*alpha+127u;
  const uint32_t blue=(foreground&31u)*alpha+127u;
  asm volatile(
    ".option push\n.option arch, +xespv\n"
    "esp.movx.r.sar t3\nesp.movx.r.cfg a5\nli t4, 2\nesp.movx.w.cfg t4\n"
    "li t4, 0\nesp.movx.w.sar t4\nli t4, 1\n"
    "mv t6, %[pixels]\nmv t5, %[parameters]\n"
    "mv a2, %[red]\nmv a3, %[green]\nmv a4, %[blue]\n"
    "esp.vldbc.16.ip q1, t5, 2\n"
    "esp.vldbc.16.ip q2, t5, 2\n"
    "1:\n"
    "addi t5, %[parameters], 4\n"
    "esp.vld.128.ip q0, t6, 0\n"
    "esp.vldbc.16.ip q7, t5, 2\n"
    "esp.vldbc.16.ip q3, t5, 2\n"
    "esp.vsld.16 q5, q0, q7\n"
    "esp.andq q5, q5, q3\n"
    "esp.vmul.u16 q5, q5, q1\n"
    "esp.vsadds.s16 q5, q5, a2\n"
    "esp.vsld.16 q6, q5, q2\n"
    "esp.vsadds.s16 q5, q5, t4\n"
    "esp.vadd.u16 q5, q5, q6\n"
    "esp.vsld.16 q5, q5, q2\n"
    "esp.vldbc.16.ip q7, t5, 2\n"
    "esp.vmul.u16 q4, q5, q7\n"
    "esp.vldbc.16.ip q7, t5, 2\n"
    "esp.vldbc.16.ip q3, t5, 2\n"
    "esp.vsld.16 q5, q0, q7\n"
    "esp.andq q5, q5, q3\n"
    "esp.vmul.u16 q5, q5, q1\n"
    "esp.vsadds.s16 q5, q5, a3\n"
    "esp.vsld.16 q6, q5, q2\n"
    "esp.vsadds.s16 q5, q5, t4\n"
    "esp.vadd.u16 q5, q5, q6\n"
    "esp.vsld.16 q5, q5, q2\n"
    "esp.vldbc.16.ip q7, t5, 2\n"
    "esp.vmul.u16 q5, q5, q7\n"
    "esp.orq q4, q4, q5\n"
    "esp.vldbc.16.ip q3, t5, 2\n"
    "esp.andq q5, q0, q3\n"
    "esp.vmul.u16 q5, q5, q1\n"
    "esp.vsadds.s16 q5, q5, a4\n"
    "esp.vsld.16 q6, q5, q2\n"
    "esp.vsadds.s16 q5, q5, t4\n"
    "esp.vadd.u16 q5, q5, q6\n"
    "esp.vsld.16 q5, q5, q2\n"
    "esp.orq q4, q4, q5\n"
    "esp.vst.128.ip q4, t6, 16\n"
    "addi %[blocks], %[blocks], -1\nbnez %[blocks], 1b\n"
    "esp.movx.w.sar t3\nesp.movx.w.cfg a5\n.option pop\n"
    :[blocks] "+&r"(blocks)
    :[pixels] "r"(pixels),[parameters] "r"(parameters),[red] "r"(red),[green] "r"(green),[blue] "r"(blue)
    :"a2","a3","a4","a5","t3","t4","t5","t6","memory");
}
#endif
} // namespace detail

// Complete synchronous span. The caller owns a valid writable RGB565 span.
// Return how many pixels actually used PIE (zero means exact scalar fallback).
// Prefix/tail never read or write outside this span. Opaque spans may retain
// their caller's separate unrolled fill instead of using this alpha kernel.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((optimize("O3")))
#endif
inline size_t blend(uint16_t *pixels,size_t count,uint16_t foreground,uint8_t alpha){
  if(!pixels||!count||!alpha)return 0;
  size_t vectorPixels=0;
#if NICE_GFX_S31_PIE_NATIVE_BLEND
  auto &s=state();
#if NICE_GFX_S31_PIE_NATIVE_BLEND_AVAILABLE
  if(s.enabled&&alpha<255&&count>=32&&canUseNow()&&!(reinterpret_cast<uintptr_t>(pixels)&1)){
    const size_t prefix=((16-(reinterpret_cast<uintptr_t>(pixels)&15))&15)/2;
    detail::scalar(pixels,prefix,foreground,alpha);pixels+=prefix;count-=prefix;
    vectorPixels=count&~size_t(7);
    if(vectorPixels){detail::pie(pixels,vectorPixels/8,foreground,alpha);s.blocks+=vectorPixels/8;pixels+=vectorPixels;count-=vectorPixels;}
  }else ++s.gateRejects;
#else
  ++s.gateRejects;
#endif
#endif
  detail::scalar(pixels,count,foreground,alpha);return vectorPixels;
}
}}} // namespace nice_gfx::platform::rgb565_native_span
