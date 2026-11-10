#pragma once
#include "../../Core/BuildConfig.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#ifndef NICE_GFX_P4_PIE_MEMORY
#define NICE_GFX_P4_PIE_MEMORY 1
#endif
#if NICE_GFX_P4_PIE_MEMORY && defined(CONFIG_IDF_TARGET_ESP32P4) && (defined(ARDUINO_ARCH_ESP32)||NICE_GFX_NATIVE_IDF) && defined(__GNUC__) && !defined(__clang__)
#include <esp_attr.h>
#include <esp_memory_utils.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#define NICE_GFX_P4_PIE_MEMORY_AVAILABLE 1
#else
#define NICE_GFX_P4_PIE_MEMORY_AVAILABLE 0
#endif
namespace nice_gfx { namespace platform { namespace rgb565_p4_memory {
inline bool available(){return NICE_GFX_P4_PIE_MEMORY_AVAILABLE;}
#if NICE_GFX_P4_PIE_MEMORY_AVAILABLE
namespace detail {
struct State{bool tested=false,enabled=false,testing=false;};
inline State &state(){static State value;return value;}
inline bool task(){return xPortGetCoreID()==1&&!xPortInIsrContext()&&xPortCanYield();}
static __attribute__((noinline)) IRAM_ATTR void copyBlocks(void *destination,const void *source,size_t blocks){
  asm volatile(".option push\n.option arch, +xesppie\n"
    "mv t6, %[dst]\nmv t5, %[src]\n1:\n"
    "esp.vld.128.ip q0, t5, 16\nesp.vst.128.ip q0, t6, 16\n"
    "addi %[blocks], %[blocks], -1\nbnez %[blocks], 1b\n.option pop\n"
    :[blocks] "+&r"(blocks):[dst] "r"(destination),[src] "r"(source):"t5","t6","memory");
}
static __attribute__((noinline)) IRAM_ATTR void fillBlocks(void *destination,const void *pattern,size_t blocks){
  asm volatile(".option push\n.option arch, +xesppie\n"
    "mv t6, %[dst]\nmv t5, %[pattern]\nesp.vld.128.ip q0, t5, 0\n1:\n"
    "esp.vst.128.ip q0, t6, 16\naddi %[blocks], %[blocks], -1\nbnez %[blocks], 1b\n.option pop\n"
    :[blocks] "+&r"(blocks):[dst] "r"(destination),[pattern] "r"(pattern):"t5","t6","memory");
}
inline bool validate(){
  auto &s=state();if(s.tested)return s.enabled;if(s.testing||!task())return false;s.testing=true;
  alignas(16) uint32_t source[32],destination[32];for(size_t i=0;i<32;++i){source[i]=0x3e8179a5u^uint32_t(i*1777u);destination[i]=0;}
  copyBlocks(destination,source,8);bool ok=memcmp(source,destination,sizeof(source))==0;
  fillBlocks(destination,source,8);for(size_t i=0;i<32;++i)if(destination[i]!=source[i%4])ok=false;
  s.enabled=ok;s.tested=true;s.testing=false;return ok;
}
}
#endif
inline bool hardwareValidated(){
#if NICE_GFX_P4_PIE_MEMORY_AVAILABLE
  return detail::state().tested&&detail::state().enabled;
#else
  return false;
#endif
}
// Each successful operation consumes the complete span synchronously. Pure
// VLD/VST leave CFG/SAR untouched. Prefix/tail and unsupported contexts use C++.
inline bool copy(uint16_t *destination,const uint8_t *source,size_t pixels){
#if NICE_GFX_P4_PIE_MEMORY_AVAILABLE
  if(pixels<32||!destination||!source||pixels>SIZE_MAX/2||!detail::task())return false;
  const uintptr_t d=reinterpret_cast<uintptr_t>(destination),s=reinterpret_cast<uintptr_t>(source);const size_t bytes=pixels*2;
  if((d&1)||((d^s)&15)||d>UINTPTR_MAX-bytes||s>UINTPTR_MAX-bytes||(s<d+bytes&&d<s+bytes)||
     (!esp_ptr_internal(source)&&!esp_ptr_external_ram(source))||!detail::validate())return false;
  auto *out=reinterpret_cast<uint8_t*>(destination);const size_t prefix=(16-(d&15))&15;size_t remaining=bytes;
  if(prefix){memcpy(out,source,prefix);out+=prefix;source+=prefix;remaining-=prefix;}
  const size_t vectorBytes=remaining&~size_t(15);if(vectorBytes)detail::copyBlocks(out,source,vectorBytes/16);
  if(remaining>vectorBytes)memcpy(out+vectorBytes,source+vectorBytes,remaining-vectorBytes);return true;
#else
  (void)destination;(void)source;(void)pixels;return false;
#endif
}
inline bool fill(uint16_t *destination,size_t pixels,uint16_t color){
#if NICE_GFX_P4_PIE_MEMORY_AVAILABLE
  if(pixels<32||pixels>SIZE_MAX/2||!destination||(reinterpret_cast<uintptr_t>(destination)&1)||reinterpret_cast<uintptr_t>(destination)>UINTPTR_MAX-pixels*2||!detail::task()||!detail::validate())return false;
  while(reinterpret_cast<uintptr_t>(destination)&15){*destination++=color;--pixels;}
  const uint32_t pair=uint32_t(color)|(uint32_t(color)<<16);alignas(16) const uint32_t pattern[4]={pair,pair,pair,pair};
  const size_t vectorPixels=pixels&~size_t(7);if(vectorPixels)detail::fillBlocks(destination,pattern,vectorPixels/8);
  destination+=vectorPixels;pixels-=vectorPixels;while(pixels--)*destination++=color;return true;
#else
  (void)destination;(void)pixels;(void)color;return false;
#endif
}
}}}
