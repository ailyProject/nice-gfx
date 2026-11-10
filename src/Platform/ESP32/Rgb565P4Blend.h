#pragma once
#include "../../Core/BuildConfig.h"
#include <stddef.h>
#include <stdint.h>
#ifndef NICE_GFX_P4_PIE_BLEND
#define NICE_GFX_P4_PIE_BLEND 0
#endif
#define NICE_GFX_P4_PIE_BLEND_STATS_VERSION 1
#define NICE_GFX_P4_PIE_BLEND_KERNEL_VERSION 3
#if NICE_GFX_P4_PIE_BLEND && defined(CONFIG_IDF_TARGET_ESP32P4) && (defined(ARDUINO_ARCH_ESP32)||NICE_GFX_NATIVE_IDF) && defined(__GNUC__) && !defined(__clang__)
#include <esp_attr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#define NICE_GFX_P4_PIE_BLEND_AVAILABLE 1
#else
#define NICE_GFX_P4_PIE_BLEND_AVAILABLE 0
#endif
namespace nice_gfx { namespace platform { namespace rgb565_p4_blend {
// Every multiply/add numerator is <=63*255+128; no U16 operation saturates.
// SAR=0 makes multiplication exact under every CFG rounding mode. CFG,
// SAR_BYTES, FFT_BIT_WIDTH and accumulators are never touched.
struct alignas(16) Plan {
  uint16_t lane[9][8];
  explicit Plan(uint8_t alpha) {
    for(unsigned i=0;i<8;++i){
      lane[0][i]=alpha;lane[1][i]=uint16_t(255-alpha);
      lane[2][i]=uint16_t(-11);lane[3][i]=uint16_t(-8);
      lane[4][i]=2048;lane[5][i]=uint16_t(-5);
      lane[6][i]=63;lane[7][i]=5;lane[8][i]=31;
    }
  }
};
inline uint16_t reference(uint16_t f,uint16_t b,uint8_t alpha){
  const uint32_t a=alpha,c=255-a;
  return uint16_t((((uint32_t(f>>11)*a+uint32_t(b>>11)*c+127)/255)<<11)|
    ((((uint32_t((f>>5)&63)*a+uint32_t((b>>5)&63)*c+127)/255)<<5))|
    ((uint32_t(f&31)*a+uint32_t(b&31)*c+127)/255));
}
inline bool taskContext(){
#if NICE_GFX_P4_PIE_BLEND_AVAILABLE
  return xPortGetCoreID()==1&&!xPortInIsrContext()&&xPortCanYield();
#else
  return false;
#endif
}
#if NICE_GFX_P4_PIE_BLEND_AVAILABLE
// Full eight-pixel blocks only. The caller proves aligned, disjoint, owned
// buffers and excludes color-key texels before any destination access.
// Preserve all QR registers and SAR at the C++ boundary.
// Unsafe primitive: use the bounded mask/PIE-state adapter, not a direct UI
// call. The installed IDF5.5.5 lazy context does not correctly preserve q3/SAR.
static __attribute__((noinline,optimize("O3"))) IRAM_ATTR void rawBlocks(uint16_t *destination,const uint16_t *source,size_t blocks,const Plan &plan){
  if(!blocks)return;
  alignas(16) uint32_t saved[32];
  asm volatile(".option push\n.option arch, +xesppie\n"
    "mv t6, %[dst]\n"
    "mv t5, %[src]\n"
    "mv s9, %[saved]\n"
    "esp.vst.128.ip q0, s9, 16\n"
    "esp.vst.128.ip q1, s9, 16\n"
    "esp.vst.128.ip q2, s9, 16\n"
    "esp.vst.128.ip q3, s9, 16\n"
    "esp.vst.128.ip q4, s9, 16\n"
    "esp.vst.128.ip q5, s9, 16\n"
    "esp.vst.128.ip q6, s9, 16\n"
    "esp.vst.128.ip q7, s9, 16\n"
    "esp.movx.r.sar s8\n"
    "li t3, 0\nesp.movx.w.sar t3\n"
    "li t3, 128\n"
    "mv t4, %[plan]\n"
    "esp.vld.128.ip q5, t4, 16\n"
    "esp.vld.128.ip q6, t4, 16\n"
    "1:\n"
    "esp.vld.128.ip q0, t5, 16\n"
    "esp.vld.128.ip q1, t6, 0\n"
    "esp.zero.q q2\n"
    "addi t4, %[plan], 32\n"
    "esp.vld.128.ip q7, t4, 0\n"
    "esp.vsld.16 q3, q0, q7\n"
    "esp.vsld.16 q4, q1, q7\n"
    // Explicit RGB565 extraction is correct for logical or signed right shift.
    // The original unmasked red extraction failed on the actual rev3.2 board.
    "addi t4, %[plan], 128\n"
    "esp.vld.128.ip q7, t4, 0\n"
    "esp.andq q3, q3, q7\n"
    "esp.andq q4, q4, q7\n"
    "esp.vmul.u16 q3, q3, q5\n"
    "esp.vmul.u16 q4, q4, q6\n"
    "esp.vadd.u16 q3, q3, q4\n"
    "esp.vsadds.u16 q3, q3, t3\n"
    "addi t4, %[plan], 48\n"
    "esp.vld.128.ip q7, t4, 0\n"
    "esp.vsld.16 q4, q3, q7\n"
    "esp.vadd.u16 q3, q3, q4\n"
    "esp.vsld.16 q3, q3, q7\n"
    "addi t4, %[plan], 64\n"
    "esp.vld.128.ip q7, t4, 0\n"
    // A signed saturating VSLD pack cannot represent red16<<11. U16 multiply
    // with SAR0 represents every red0..31 exactly, including bit15.
    "esp.vmul.u16 q3, q3, q7\n"
    "esp.orq q2, q2, q3\n"
    "addi t4, %[plan], 80\n"
    "esp.vld.128.ip q7, t4, 0\n"
    "esp.vsld.16 q3, q0, q7\n"
    "esp.vsld.16 q4, q1, q7\n"
    "addi t4, %[plan], 96\n"
    "esp.vld.128.ip q7, t4, 0\n"
    "esp.andq q3, q3, q7\n"
    "esp.andq q4, q4, q7\n"
    "esp.vmul.u16 q3, q3, q5\n"
    "esp.vmul.u16 q4, q4, q6\n"
    "esp.vadd.u16 q3, q3, q4\n"
    "esp.vsadds.u16 q3, q3, t3\n"
    "addi t4, %[plan], 48\n"
    "esp.vld.128.ip q7, t4, 0\n"
    "esp.vsld.16 q4, q3, q7\n"
    "esp.vadd.u16 q3, q3, q4\n"
    "esp.vsld.16 q3, q3, q7\n"
    "addi t4, %[plan], 112\n"
    "esp.vld.128.ip q7, t4, 0\n"
    "esp.vsld.16 q3, q3, q7\n"
    "esp.orq q2, q2, q3\n"
    "addi t4, %[plan], 128\n"
    "esp.vld.128.ip q7, t4, 0\n"
    "esp.andq q3, q0, q7\n"
    "esp.andq q4, q1, q7\n"
    "esp.vmul.u16 q3, q3, q5\n"
    "esp.vmul.u16 q4, q4, q6\n"
    "esp.vadd.u16 q3, q3, q4\n"
    "esp.vsadds.u16 q3, q3, t3\n"
    "addi t4, %[plan], 48\n"
    "esp.vld.128.ip q7, t4, 0\n"
    "esp.vsld.16 q4, q3, q7\n"
    "esp.vadd.u16 q3, q3, q4\n"
    "esp.vsld.16 q3, q3, q7\n"
    "esp.orq q2, q2, q3\n"
    "esp.vst.128.ip q2, t6, 16\n"
    "addi %[blocks], %[blocks], -1\n"
    "bnez %[blocks], 1b\n"
    "esp.movx.w.sar s8\n"
    "mv s9, %[saved]\n"
    "esp.vld.128.ip q0, s9, 16\n"
    "esp.vld.128.ip q1, s9, 16\n"
    "esp.vld.128.ip q2, s9, 16\n"
    "esp.vld.128.ip q3, s9, 16\n"
    "esp.vld.128.ip q4, s9, 16\n"
    "esp.vld.128.ip q5, s9, 16\n"
    "esp.vld.128.ip q6, s9, 16\n"
    "esp.vld.128.ip q7, s9, 16\n"
    ".option pop\n"
    :[blocks] "+&r"(blocks)
    :[dst] "r"(destination),[src] "r"(source),[plan] "r"(&plan),[saved] "r"(saved)
    :"s8","s9","t3","t4","t5","t6","memory");
}
#endif
}}}

