#pragma once
#include "Rgb565P4Blend.h"
#include <atomic>
#if NICE_GFX_P4_PIE_BLEND_AVAILABLE
#include <esp_cpu.h>
#include <esp_memory_utils.h>
#include <riscv/csr_pie.h>
#endif

namespace nice_gfx { namespace platform { namespace rgb565_p4_blend {
// Experimental/default-OFF. Validated only after this binary's scratch test.
// Caller owns stable Native565 RAM source + exclusive writable RAM destination,
// proves full eight-texel non-key runs, and retains all display/DMA fences.
// Rejection leaves source and destination untouched; no scalar fallback here.
struct SafeStats {
  uint32_t chunks=0,rejectedContext=0,rejectedArguments=0,rejectedUnverified=0;
  uint32_t maximumMaskedCycles=0;
  uint32_t maximumEnvelopeCycles=0;
  uint64_t pixels=0,maskedCycles=0;
};
inline std::atomic<bool> &verifiedState(){static std::atomic<bool> state{false};return state;}
inline bool verified(){return verifiedState().load(std::memory_order_acquire);}

namespace detail {
#if NICE_GFX_P4_PIE_BLEND_AVAILABLE
static inline bool validTask(){
  if(xPortInIsrContext()||!xPortCanYield()||xPortGetCoreID()!=1)return false;
  return xTaskGetCurrentTaskHandle()!=nullptr&&xTaskGetCoreID(nullptr)==1;
}
static inline bool ramSpan(const void *p,size_t bytes){
  if(!p||!bytes)return false;const uintptr_t at=reinterpret_cast<uintptr_t>(p);
  if(at>UINTPTR_MAX-(bytes-1))return false;const auto *last=reinterpret_cast<const void*>(at+bytes-1);
  return esp_ptr_byte_accessible(p)&&esp_ptr_byte_accessible(last)&&
    ((esp_ptr_internal(p)&&esp_ptr_internal(last))||(esp_ptr_external_ram(p)&&esp_ptr_external_ram(last)));
}
static inline bool validArguments(uint16_t *dst,const uint16_t *src,size_t blocks,const Plan &plan){
  if(blocks>SIZE_MAX/16)return false;const size_t bytes=blocks*16;
  const uintptr_t d=reinterpret_cast<uintptr_t>(dst),s=reinterpret_cast<uintptr_t>(src),p=reinterpret_cast<uintptr_t>(&plan);
  if((d|s|p)&15)return false;
  if(!ramSpan(dst,bytes)||!ramSpan(src,bytes)||!ramSpan(&plan,sizeof(plan)))return false;
  if(d<s+bytes&&s<d+bytes)return false;
  return !(d<p+sizeof(plan)&&p<d+bytes);
}
struct alignas(16) ContextImage {uint32_t q[32];uint32_t sar;};
static __attribute__((always_inline)) inline void readContext(ContextImage &image){
  asm volatile(".option push\n.option arch, +xesppie\n"
    "mv s9, %[p]\n"
    "esp.vst.128.ip q0,s9,16\nesp.vst.128.ip q1,s9,16\n"
    "esp.vst.128.ip q2,s9,16\nesp.vst.128.ip q3,s9,16\n"
    "esp.vst.128.ip q4,s9,16\nesp.vst.128.ip q5,s9,16\n"
    "esp.vst.128.ip q6,s9,16\nesp.vst.128.ip q7,s9,16\n"
    "esp.movx.r.sar t3\nsw t3,0(s9)\n.option pop\n"
    ::[p]"r"(&image):"s9","t3","memory");
}
static __attribute__((always_inline)) inline void writeContext(const ContextImage &image){
  asm volatile(".option push\n.option arch, +xesppie\n"
    "mv s9, %[p]\n"
    "esp.vld.128.ip q0,s9,16\nesp.vld.128.ip q1,s9,16\n"
    "esp.vld.128.ip q2,s9,16\nesp.vld.128.ip q3,s9,16\n"
    "esp.vld.128.ip q4,s9,16\nesp.vld.128.ip q5,s9,16\n"
    "esp.vld.128.ip q6,s9,16\nesp.vld.128.ip q7,s9,16\n"
    "lw t3,0(s9)\nesp.movx.w.sar t3\n.option pop\n"
    ::[p]"r"(&image):"s9","t3","memory");
}
// No Plan construction, allocation, printing or scheduler operation is inside
// the mask. At most8 blocks/64 pixels. The installed IDF5.5.5 lazy PIE context
// omits q3 and swaps SAR/FFT restoration; do not depend on that path here.
static __attribute__((noinline,optimize("O3"))) IRAM_ATTR void maskedChunk(uint16_t *dst,const uint16_t *src,
    size_t blocks,const Plan &plan,SafeStats *stats,const ContextImage *seed=nullptr,ContextImage *after=nullptr){
  ContextImage ambient;uint32_t previousPie;
  const uint32_t envelopeStarted=esp_cpu_get_cycle_count();
  const auto previousMask=portSET_INTERRUPT_MASK_FROM_ISR();
  const uint32_t maskedStarted=esp_cpu_get_cycle_count();
  asm volatile("csrr %0,0x7f2\ncsrwi 0x7f2,1":"=r"(previousPie)::"memory");
  // Enabling locally avoids first-use lazy-coprocessor traps inside the mask.
  // No task switch may interleave this bounded borrow. The original state and
  // all touched QR/SAR values are restored before restoring the original mask.
  if(seed){readContext(ambient);writeContext(*seed);}
  rawBlocks(dst,src,blocks,plan);
  if(seed&&after){readContext(*after);writeContext(ambient);}
  asm volatile("csrw 0x7f2,%0"::"r"(previousPie):"memory");
  const uint32_t elapsed=esp_cpu_get_cycle_count()-maskedStarted;
  portCLEAR_INTERRUPT_MASK_FROM_ISR(previousMask);
  const uint32_t envelope=esp_cpu_get_cycle_count()-envelopeStarted;
  if(stats){++stats->chunks;stats->pixels+=blocks*8;stats->maskedCycles+=elapsed;
    if(elapsed>stats->maximumMaskedCycles)stats->maximumMaskedCycles=elapsed;
    if(envelope>stats->maximumEnvelopeCycles)stats->maximumEnvelopeCycles=envelope;}
}
static inline bool checkedBlocks(uint16_t *dst,const uint16_t *src,size_t blocks,const Plan &plan,SafeStats *stats){
  if(!blocks)return true;
  if(!validTask()){if(stats)++stats->rejectedContext;return false;}
  if(!validArguments(dst,src,blocks,plan)){if(stats)++stats->rejectedArguments;return false;}
  while(blocks){const size_t count=blocks>8?8:blocks;maskedChunk(dst,src,count,plan,stats);dst+=count*8;src+=count*8;blocks-=count;}
  return true;
}
#endif
} // detail

inline bool tryBlocks(uint16_t *dst,const uint16_t *src,size_t blocks,const Plan &plan,SafeStats *stats=nullptr){
  if(!blocks)return true;
#if NICE_GFX_P4_PIE_BLEND_AVAILABLE
  if(!verified()){if(stats)++stats->rejectedUnverified;return false;}
  return detail::checkedBlocks(dst,src,blocks,plan,stats);
#else
  (void)dst;(void)src;(void)plan;if(stats)++stats->rejectedUnverified;return false;
#endif
}
}}} // nice_gfx::platform::rgb565_p4_blend
