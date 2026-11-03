#pragma once
#include "Rgb565NativeSpan.h"
#include <string.h>
#ifndef NICE_GFX_S31_PIE_NATIVE_COPY
#define NICE_GFX_S31_PIE_NATIVE_COPY 1
#endif
#if NICE_GFX_S31_PIE_NATIVE_COPY && NICE_GFX_S31_PIE_NATIVE_BLEND_AVAILABLE
#define NICE_GFX_S31_PIE_NATIVE_COPY_AVAILABLE 1
#else
#define NICE_GFX_S31_PIE_NATIVE_COPY_AVAILABLE 0
#endif

namespace nice_gfx { namespace platform { namespace rgb565_native_copy {
struct Result {bool ok=false;size_t piePixels=0;};
inline bool available(){return NICE_GFX_S31_PIE_NATIVE_COPY_AVAILABLE;}
namespace detail {
#if NICE_GFX_S31_PIE_NATIVE_COPY_AVAILABLE
inline bool &validationState(){static bool validated=false;return validated;}
#endif
#if NICE_GFX_S31_PIE_NATIVE_COPY_AVAILABLE
// Aligned data movement only: VLD/VST do not use or change CFG/SAR. As with
// the validated blend kernel, invoke only in a gated Core1 ordinary task.
// The four-vector loop moves 64 bytes per iteration without scalar packing.
static __attribute__((noinline)) IRAM_ATTR void pie(uint8_t *destination,const uint8_t *source,size_t blocks){
  size_t groups=blocks/4,tail=blocks%4;
  asm volatile(
    ".option push\n.option arch, +xespv\n"
    "mv t6, %[destination]\nmv t5, %[source]\nbeqz %[groups], 2f\n"
    "1:\n"
    "esp.vld.128.ip q0, t5, 16\nesp.vld.128.ip q1, t5, 16\n"
    "esp.vld.128.ip q2, t5, 16\nesp.vld.128.ip q3, t5, 16\n"
    "esp.vst.128.ip q0, t6, 16\nesp.vst.128.ip q1, t6, 16\n"
    "esp.vst.128.ip q2, t6, 16\nesp.vst.128.ip q3, t6, 16\n"
    "addi %[groups], %[groups], -1\nbnez %[groups], 1b\n"
    "2:\nbeqz %[tail], 4f\n3:\n"
    "esp.vld.128.ip q0, t5, 16\nesp.vst.128.ip q0, t6, 16\n"
    "addi %[tail], %[tail], -1\nbnez %[tail], 3b\n4:\n.option pop\n"
    :[groups] "+&r"(groups),[tail] "+&r"(tail)
    :[destination] "r"(destination),[source] "r"(source)
    :"t5","t6","memory");
}
#endif
}

// A separate copy self-test must pass on isolated buffers before production
// uses this new kernel. Passing the blend self-test alone is insufficient.
inline bool hardwareValidated(){
#if NICE_GFX_S31_PIE_NATIVE_COPY_AVAILABLE
  return detail::validationState();
#else
  return false;
#endif
}
inline void setHardwareValidated(bool validated){
#if NICE_GFX_S31_PIE_NATIVE_COPY_AVAILABLE
  detail::validationState()=validated;
#else
  (void)validated;
#endif
}
// Complete exact Native565 copy. Return how many pixels actually used PIE;
// zero still means a completed memcpy fallback, never an unconsumed span.
// Partial overlap, impossible byte counts and wrapping address ranges fail
// before any access. The caller supplies live readable/writable pixel spans.
inline Result copy(uint16_t *destination,const uint16_t *source,size_t pixels){
  if(!pixels)return {true,0};
  if(!destination||!source||pixels>SIZE_MAX/2)return {};
  const size_t bytes=pixels*2;
  const uintptr_t dst=reinterpret_cast<uintptr_t>(destination),src=reinterpret_cast<uintptr_t>(source);
  if(dst>UINTPTR_MAX-bytes||src>UINTPTR_MAX-bytes)return {};
  if(dst==src)return {true,0};
  if(dst<src+bytes&&src<dst+bytes)return {};
  auto *out=reinterpret_cast<uint8_t *>(destination);auto *in=reinterpret_cast<const uint8_t *>(source);
  size_t remaining=bytes,vectorBytes=0;
#if NICE_GFX_S31_PIE_NATIVE_COPY_AVAILABLE
  if(bytes>=64&&!(dst&1)&&!(src&1)&&((dst^src)&15)==0&&
     hardwareValidated()&&rgb565_native_span::state().enabled&&rgb565_native_span::canUseNow()){
    const size_t prefix=(16-(dst&15))&15;
    if(prefix){memcpy(out,in,prefix);out+=prefix;in+=prefix;remaining-=prefix;}
    vectorBytes=remaining&~size_t(15);
    if(vectorBytes){detail::pie(out,in,vectorBytes/16);out+=vectorBytes;in+=vectorBytes;remaining-=vectorBytes;}
  }
#endif
  if(remaining)memcpy(out,in,remaining);
  return {true,vectorBytes/2};
}
}}}
