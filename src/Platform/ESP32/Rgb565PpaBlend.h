#ifndef NICE_GFX_RGB565_PPA_BLEND_H
#define NICE_GFX_RGB565_PPA_BLEND_H
#include "../../Core/BuildConfig.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../../Display/Transport.h"
#include "../Common/Overlay.h"
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include <soc/soc_caps.h>
#endif
#if (defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32P4)) && defined(SOC_PPA_SUPPORTED) && SOC_PPA_SUPPORTED && !(defined(CONFIG_IDF_TARGET_ESP32P4) && NICE_GFX_P4_DISABLE_PPA)
#define NICE_GFX_S31_PPA_BLEND_AVAILABLE 1
#ifndef NICE_GFX_S31_PPA_BLEND_BURST
// Match the driver's default burst. Override with 64 for a measured comparison
// when the application shares its memory bus with other active peripherals.
#define NICE_GFX_S31_PPA_BLEND_BURST 128
#endif
#include <driver/ppa.h>
#include <esp_cache.h>
#include <esp_heap_caps.h>
#include <esp_memory_utils.h>
#include <esp_idf_version.h>
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#include <esp_private/esp_cache_private.h>
#endif
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6,1,0)
#include <esp_private/dma2d.h>
#endif
#include <esp_timer.h>
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#include <atomic>
#include <new>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#endif
#ifndef NICE_GFX_S31_OVERLAY_DRIVER_CACHE_SYNC
// IDF 6.1 ppa_do_blend synchronizes the BG/FG extended row windows and
// invalidates the matching OUT window itself. BG == OUT is supported. Avoid
// repeating those same cache operations for every small UI overlay.
// P4 retains a post-DMA invalidation; its driver invalidates before DMA.
#define NICE_GFX_S31_OVERLAY_DRIVER_CACHE_SYNC (defined(CONFIG_IDF_TARGET_ESP32S31) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6,1,0))
#endif
#ifndef NICE_GFX_P4_PPA_DRIVER_INPUT_CACHE_SYNC
// Verified 5.5.5 BLEND writes back complete, L2-aligned BG/FG row windows;
// FILL writes back its complete output rows before invalidating them. With
// BG == OUT those windows match our post-DMA invalidation exactly. Keep the
// explicit compatibility path for SDKs whose implementation was not audited.
#define NICE_GFX_P4_PPA_DRIVER_INPUT_CACHE_SYNC (defined(CONFIG_IDF_TARGET_ESP32P4) && ESP_IDF_VERSION == ESP_IDF_VERSION_VAL(5,5,5))
#endif
#else
#define NICE_GFX_S31_PPA_BLEND_AVAILABLE 0
#endif

namespace nice_gfx { namespace platform { namespace rgb565_ppa {
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
inline size_t cacheLineSize(const void *address){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  size_t line=0;
  return esp_cache_get_alignment(esp_ptr_external_ram(address)?MALLOC_CAP_SPIRAM:MALLOC_CAP_INTERNAL,&line)==ESP_OK?line:0;
#else
  return esp_cache_get_line_size_by_addr(address);
#endif
}
#endif
// Wire565 cache-writeback attempts only. Native copies, including an older
// async job drained by a Wire call, never contribute driver time or hits.
struct WireCopyStats {
  uint64_t attempts=0,completed=0,rejected=0,errors=0,driverCalls=0,driverMicros=0,pixels=0;
  uint64_t rejectedFootprint=0,rejectedContext=0,rejectedSourceCache=0;
  uint64_t rejectedMemory=0,rejectedDmaAlignment=0,rejectedPlatform=0;
};
struct Stats {
  WireCopyStats wireCopy;
  uint64_t completed=0,driverMicros=0,conversionMicros=0;
  uint64_t copyCompleted=0,copyDriverMicros=0;
  uint64_t overlayPreflightMicros=0,overlayDriverMicros=0,overlaySyncMicros=0;
  uint64_t overlayPreflightCalls=0,overlaySyncCalls=0,overlaySyncedBytes=0;
  uint32_t rejected=0,failures=0;
  int lastError=0;
  size_t scratchBytes=0;
};
// Optional S31 backend. Single owner task only.
// Inputs remain immutable through a blocking call or its explicit async join.
// Scratch output is independent, aligned native RGB565. A Wire565 caller can
// request word-pair conversion after success; native display paths skip it. Reuse the
// original background on every fade step, not an already blended frame.
class Context {
 public:
  Context()=default;
  Context(const Context&)=delete;Context&operator=(const Context&)=delete;
  ~Context(){close();}
  static constexpr bool available(){return NICE_GFX_S31_PPA_BLEND_AVAILABLE;}
  const Stats &stats()const{return stats_;}
  void resetWireCopyStats(){stats_.wireCopy=WireCopyStats{};}
  void resetOverlayStats(){
    stats_.overlayPreflightMicros=stats_.overlayDriverMicros=stats_.overlaySyncMicros=0;
    stats_.overlayPreflightCalls=stats_.overlaySyncCalls=stats_.overlaySyncedBytes=0;
  }
  bool blend(uint16_t *foreground,size_t foregroundStride,const uint16_t *background,
             size_t backgroundStride,size_t width,size_t height,uint8_t alpha){
    if(!processNative(foreground,foregroundStride,background,backgroundStride,width,height,alpha))return false;
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
    convertToWire(foreground,foregroundStride,width,height);return true;
#else
    return false;
#endif
  }
  // Sources stay immutable. The returned contiguous Wire565 surface remains
  // borrowed until the next blend/close; do not call again while a display
  // transfer still reads it. Hardware channel rounding may differ by one 5/6
  // bit channel step from the CPU renderer; alpha and timing are unchanged.
  const uint16_t *blendSourcesToWire(const uint16_t *foreground,size_t foregroundStride,
             const uint16_t *background,size_t backgroundStride,size_t width,size_t height,uint8_t alpha){
    return borrowSources(foreground,foregroundStride,background,backgroundStride,width,height,alpha,true);
  }
  // Same borrowing/immutability contract, with native RGB565 output. A display
  // transport accepting native pixels can consume it without a CPU byte-order pass.
  const uint16_t *blendSourcesToNative(const uint16_t *foreground,size_t foregroundStride,
             const uint16_t *background,size_t backgroundStride,size_t width,size_t height,uint8_t alpha){
    return borrowSources(foreground,foregroundStride,background,backgroundStride,width,height,alpha,false);
  }
  // Blend a shared viewport of two complete, aligned Wire565 pictures. Keep
  // the picture bases aligned even when sourceX is not a cache-line boundary;
  // PPA block offsets select the region without exposing an unaligned DMA base.
  // Output is packed native RGB565 with the same lifetime as other borrowed blends.
  const uint16_t *blendSourceRegionsToNative(const uint16_t *foreground,size_t foregroundStride,
             const uint16_t *background,size_t backgroundStride,size_t pictureHeight,
             size_t sourceX,size_t sourceY,size_t width,size_t height,uint8_t alpha){
    if(!processRegionNative(foreground,foregroundStride,background,backgroundStride,
        pictureHeight,sourceX,sourceY,width,height,alpha))return nullptr;
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
    stats_.lastError=ESP_OK;++stats_.completed;return scratch_;
#else
    return nullptr;
#endif
  }
  // A mapped native display target avoids a scratch surface and the subsequent
  // full-frame copy. Inputs remain immutable; target is exclusively writable
  // until this blocking call returns, then it may be submitted to the display.
  // Only complete packed target rows are accepted: invalidating DMA output
  // must never discard dirty padding owned by another producer.
  // Only complete packed target rows are accepted: invalidating DMA output
  // must never discard dirty padding owned by another producer.
  bool blendSourcesIntoNative(uint16_t *target,size_t targetStride,size_t targetBytes,
             const uint16_t *foreground,size_t foregroundStride,const uint16_t *background,
             size_t backgroundStride,size_t width,size_t height,uint8_t alpha){
    return targetBlend(target,targetStride,targetBytes,foreground,foregroundStride,
      background,backgroundStride,width,height,alpha,false,0);
  }
  bool blendSolidIntoNative(uint16_t *target,size_t targetStride,size_t targetBytes,
             const uint16_t *page,size_t pageStride,size_t width,size_t height,
             uint16_t background,uint8_t alpha){
    return targetBlend(target,targetStride,targetBytes,page,pageStride,nullptr,0,
      width,height,alpha,true,background);
  }
  Status blendSolidNativeIntoNative(uint16_t *target,size_t targetStride,size_t targetBytes,
             const uint16_t *page,size_t pageStride,size_t width,size_t height,uint16_t background,uint8_t alpha){
    const uint32_t rejected=stats_.rejected;
    if(targetBlend(target,targetStride,targetBytes,page,pageStride,nullptr,0,width,height,alpha,true,background,PixelOrder::Native))return Status::Ok;
    return stats_.rejected!=rejected?Status::Unsupported:Status::IoError;
  }
  bool canCopyIntoNative(const NativeFramebuffer &target,int32_t x,int32_t y,PixelView source,size_t sourceBytes,
             size_t sourceX,size_t sourceY,size_t width,size_t height)const{
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
    // DMA2D picture/block dimensions are 14-bit, independently of PPA's
    // public 32-bit configuration fields. Reject before descriptor truncation.
    if(!target.pixels||!target.width||!target.height||!target.stride||!source.valid()||uint8_t(source.order)>uint8_t(PixelOrder::Wire)||!width||!height||x<0||y<0||target.width>=0x4000||target.height>=0x4000||
       target.stride<target.width||target.stride>=0x4000||source.stride>=0x4000||source.height>=0x4000||
       sourceX>source.width||width>source.width-sourceX||sourceY>source.height||height>source.height-sourceY||
       size_t(x)>target.width||width>target.width-size_t(x)||size_t(y)>target.height||height>target.height-size_t(y)||
       target.height>SIZE_MAX/target.stride/2||source.height>SIZE_MAX/source.stride/2)return false;
    const size_t targetBytes=target.stride*target.height*2,inputBytes=source.stride*source.height*2;
    const uintptr_t out=reinterpret_cast<uintptr_t>(target.pixels),in=reinterpret_cast<uintptr_t>(source.data);
    const size_t line=cacheLineSize(target.pixels);
    if(!line||(line&(line-1))||out%line||targetBytes%line||target.bytes<targetBytes||targetBytes>UINT32_MAX||
       sourceBytes<inputBytes||(in&1)||out>UINTPTR_MAX-targetBytes||in>UINTPTR_MAX-inputBytes||
       (out<in+inputBytes&&in<out+targetBytes))return false;
    return true;
#else
    (void)target;(void)x;(void)y;(void)source;(void)sourceBytes;(void)sourceX;(void)sourceY;(void)width;(void)height;return false;
#endif
  }
  // BEGIN SINGLE_ASYNC_COPY_API
  // The owner retains both surfaces through finish. Busy keeps ownership;
  // a finish IoError has drained DMA and reports a cache synchronization error.
  bool canBeginCopyIntoNative(const NativeFramebuffer &target,int32_t x,int32_t y,PixelView source,size_t sourceBytes,
             size_t sourceX,size_t sourceY,size_t width,size_t height)const{
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE && defined(CONFIG_IDF_TARGET_ESP32S31)
    return !xPortInIsrContext()&&xPortCanYield()&&(!asyncCopy_||!asyncCopy_->pending)&&
      canCopyIntoNative(target,x,y,source,sourceBytes,sourceX,sourceY,width,height);
#else
    (void)target;(void)x;(void)y;(void)source;(void)sourceBytes;(void)sourceX;(void)sourceY;(void)width;(void)height;
    return false;
#endif
  }
  Status beginCopyIntoNative(const NativeFramebuffer &target,int32_t x,int32_t y,PixelView source,size_t sourceBytes,
             size_t sourceX,size_t sourceY,size_t width,size_t height){
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE && defined(CONFIG_IDF_TARGET_ESP32S31)
    if(asyncCopy_&&asyncCopy_->pending)return Status::Busy;
    if(!canBeginCopyIntoNative(target,x,y,source,sourceBytes,sourceX,sourceY,width,height))return Status::Unsupported;
    if(!asyncCopy_){
      void *memory=heap_caps_calloc(1,sizeof(AsyncCopyState),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
      if(!memory){stats_.lastError=ESP_ERR_NO_MEM;++stats_.rejected;return Status::Unsupported;}
      asyncCopy_=new(memory) AsyncCopyState;
      asyncCopy_->semaphore=xSemaphoreCreateBinaryStatic(&asyncCopy_->semaphoreStorage);
      if(!asyncCopy_->semaphore){
        asyncCopy_->~AsyncCopyState();heap_caps_free(asyncCopy_);asyncCopy_=nullptr;
        stats_.lastError=ESP_ERR_NO_MEM;++stats_.rejected;return Status::Unsupported;
      }
    }
    if(!copyClient_){
      ppa_client_config_t config={};config.oper_type=PPA_OPERATION_SRM;config.max_pending_trans_num=1;
      config.data_burst_length=NICE_GFX_S31_PPA_BLEND_BURST==128?PPA_DATA_BURST_LENGTH_128:PPA_DATA_BURST_LENGTH_64;
      const auto error=ppa_register_client(&config,&copyClient_);
      if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return error==ESP_ERR_NOT_SUPPORTED?Status::Unsupported:Status::IoError;}
      asyncCopy_->callbackRegistered=false;
    }
    if(!asyncCopy_->callbackRegistered){
      ppa_event_callbacks_t callbacks={};callbacks.on_trans_done=asyncCopyDone;
      const auto error=ppa_client_register_event_callbacks(copyClient_,&callbacks);
      if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return Status::IoError;}
      asyncCopy_->callbackRegistered=true;
    }
    const size_t line=cacheLineSize(target.pixels),bytes=target.stride*target.height*2;
    const size_t begin=(size_t(y)*target.stride*2)&~(line-1);
    const size_t end=((size_t(y)+height)*target.stride*2+line-1)&~(line-1);
    if(end>bytes)return Status::Unsupported;
    auto *rows=reinterpret_cast<uint8_t*>(target.pixels)+begin;
    const auto synchronized=esp_cache_msync(rows,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    if(synchronized!=ESP_OK){stats_.lastError=synchronized;++stats_.failures;return Status::IoError;}
    ppa_srm_oper_config_t operation={};
    operation.in.buffer=source.data;operation.in.pic_w=source.stride;operation.in.pic_h=source.height;
    operation.in.block_w=width;operation.in.block_h=height;operation.in.block_offset_x=sourceX;operation.in.block_offset_y=sourceY;
    operation.in.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
    operation.out.buffer=target.pixels;operation.out.buffer_size=bytes;operation.out.pic_w=target.stride;operation.out.pic_h=target.height;
    operation.out.block_offset_x=x;operation.out.block_offset_y=y;operation.out.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
    operation.scale_x=operation.scale_y=1.0f;operation.byte_swap=source.order==PixelOrder::Wire;
    operation.mode=PPA_TRANS_MODE_NON_BLOCKING;operation.user_data=asyncCopy_;
    while(xSemaphoreTake(asyncCopy_->semaphore,0)==pdTRUE){}
    asyncCopy_->completed.store(false,std::memory_order_relaxed);
    asyncCopy_->rows=rows;asyncCopy_->rowBytes=end-begin;
    asyncCopy_->startedMicros=uint32_t(esp_timer_get_time());asyncCopy_->pending=true;
    const auto error=ppa_do_scale_rotate_mirror(copyClient_,&operation);
    if(error!=ESP_OK){
      // IDF94639 rejects before queueing or rolls this submission back. A
      // client can still be recycling an older blocking transaction, so an
      // unregister failure is not evidence that this new callback will arrive.
      if(asyncCopy_->completed.load(std::memory_order_acquire)){
        const Status drained=finishCopyIntoNative();
        if(drained==Status::Busy)return drained;
      }else asyncCopy_->pending=false;
      stats_.lastError=error;++stats_.failures;return Status::IoError;
    }
    stats_.lastError=ESP_OK;return Status::Ok;
#else
    (void)target;(void)x;(void)y;(void)source;(void)sourceBytes;(void)sourceX;(void)sourceY;(void)width;(void)height;
    return Status::Unsupported;
#endif
  }
  Status finishCopyIntoNative(){
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE && defined(CONFIG_IDF_TARGET_ESP32S31)
    if(!asyncCopy_||!asyncCopy_->pending)return Status::Ok;
    if(xPortInIsrContext()||!xPortCanYield())return Status::Busy;
    // The existing SDK blocking contract retains both targets and sources
    // until an accepted transaction has actually completed.
    for(;;){
      while(xSemaphoreTake(asyncCopy_->semaphore,portMAX_DELAY)!=pdTRUE){}
      if(asyncCopy_->completed.load(std::memory_order_acquire))break;
    }
    stats_.copyDriverMicros+=uint32_t(asyncCopy_->completedMicros.load(std::memory_order_relaxed)-asyncCopy_->startedMicros);
    asyncCopy_->pending=false;
    const auto synchronized=esp_cache_msync(asyncCopy_->rows,asyncCopy_->rowBytes,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    if(synchronized!=ESP_OK){stats_.lastError=synchronized;++stats_.failures;return Status::IoError;}
    ++stats_.copyCompleted;stats_.lastError=ESP_OK;
#endif
    return Status::Ok;
  }
  // END SINGLE_ASYNC_COPY_API
  Status copyIntoNative(const NativeFramebuffer &target,int32_t x,int32_t y,PixelView source,size_t sourceBytes,
             size_t sourceX,size_t sourceY,size_t width,size_t height){
    return copyIntoRgb565(target,x,y,source,sourceBytes,sourceX,sourceY,width,height,PixelOrder::Native);
  }
  // The descriptor retains the complete writable picture base and capacity.
  // SRM RGB565 scale-one copying swaps each input word when orders differ;
  // the same exact operation therefore writes Native565 or Wire565 bytes.
  // Unsupported has made no output writes. A driver/cache error must propagate.
  Status copyIntoWire(const NativeFramebuffer &target,int32_t x,int32_t y,PixelView source,size_t sourceBytes,
             size_t sourceX,size_t sourceY,size_t width,size_t height){
    ++stats_.wireCopy.attempts;
    const Status result=copyIntoRgb565(target,x,y,source,sourceBytes,sourceX,sourceY,width,height,PixelOrder::Wire);
    if(result==Status::Ok){++stats_.wireCopy.completed;stats_.wireCopy.pixels+=uint64_t(width)*height;}
    else if(result==Status::Unsupported)++stats_.wireCopy.rejected;
    else ++stats_.wireCopy.errors;
    return result;
  }
  Status fillIntoNative(const NativeFramebuffer &target,const DirtyRect &r,uint16_t color){
    const Status completed=finishCopyIntoNative();if(completed!=Status::Ok)return completed;
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
    if(!target.pixels||!target.width||!target.height||target.stride<target.width||target.stride>=0x4000||target.height>=0x4000||
       target.height>SIZE_MAX/target.stride/2||target.bytes<target.stride*target.height*2||r.x<0||r.y<0||r.width<=0||r.height<=0||int64_t(r.x)+r.width>target.width||int64_t(r.y)+r.height>target.height)return Status::Unsupported;
    const size_t bytes=target.stride*target.height*2;const uintptr_t base=reinterpret_cast<uintptr_t>(target.pixels);
    if(base>UINTPTR_MAX-target.bytes)return Status::Unsupported;
    const auto *last=reinterpret_cast<const void*>(base+bytes-1);
    if(!((esp_ptr_internal(target.pixels)&&esp_ptr_internal(last))||(esp_ptr_external_ram(target.pixels)&&esp_ptr_external_ram(last))))return Status::Unsupported;
    const size_t line=cacheLineSize(target.pixels);if(!line||(line&(line-1))||bytes%line||base%line)return Status::Unsupported;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6,1,0)
    if(!dma2d_check_transaction_alignment_constraint(target.pixels,target.stride,r.width,size_t(r.x),16))return Status::Unsupported;
#elif defined(CONFIG_IDF_TARGET_ESP32S31)
    if((target.stride*2|size_t(r.x)*2|size_t(r.width)*2)&15u)return Status::Unsupported;
#endif
    if(!fillClient_){ppa_client_config_t config={};config.oper_type=PPA_OPERATION_FILL;config.max_pending_trans_num=1;config.data_burst_length=PPA_DATA_BURST_LENGTH_128;
      const auto error=ppa_register_client(&config,&fillClient_);if(error!=ESP_OK){stats_.lastError=error;
        if(error==ESP_ERR_NOT_SUPPORTED){++stats_.rejected;return Status::Unsupported;}
        ++stats_.failures;return Status::IoError;}}
    const size_t begin=(size_t(r.y)*target.stride*2)&~(line-1),end=((size_t(r.y+r.height)*target.stride*2)+line-1)&~(line-1);
    if(end>bytes)return Status::Unsupported;auto *rows=reinterpret_cast<uint8_t*>(target.pixels)+begin;
    // Preserve adjacent dirty CPU pixels when output cache lines/rows overlap.
#if !NICE_GFX_S31_OVERLAY_DRIVER_CACHE_SYNC && !NICE_GFX_P4_PPA_DRIVER_INPUT_CACHE_SYNC
    const auto beforeSync=esp_cache_msync(rows,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    if(beforeSync!=ESP_OK){stats_.lastError=beforeSync;++stats_.failures;return Status::IoError;}
#endif
    ppa_fill_oper_config_t op={};op.out.buffer=target.pixels;op.out.buffer_size=bytes;op.out.pic_w=target.stride;op.out.pic_h=target.height;op.out.block_offset_x=r.x;op.out.block_offset_y=r.y;op.out.fill_cm=PPA_FILL_COLOR_MODE_RGB565;
    op.fill_block_w=r.width;op.fill_block_h=r.height;op.fill_argb_color.a=255;op.fill_argb_color.r=uint8_t((color>>11)<<3);op.fill_argb_color.g=uint8_t(((color>>5)&63)<<2);op.fill_argb_color.b=uint8_t((color&31)<<3);op.mode=PPA_TRANS_MODE_BLOCKING;
    const auto error=ppa_do_fill(fillClient_,&op);
    if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return Status::IoError;}
    const auto afterSync=esp_cache_msync(rows,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    if(afterSync!=ESP_OK){stats_.lastError=afterSync;++stats_.failures;return Status::IoError;}
    stats_.lastError=ESP_OK;return Status::Ok;
#else
    (void)target;(void)r;(void)color;return Status::Unsupported;
#endif
  }
  // A8 supplies no RGB, and FIX replaces every sampled alpha. The bounded
  // target picture may therefore serve as the dummy A8 input: its byte values
  // cannot affect the result, even while BG/OUT share the RGB565 picture.
  Status blendColorIntoNative(const NativeFramebuffer &target,const DirtyRect &r,uint16_t color,uint8_t alpha){
    const Status completed=finishCopyIntoNative();if(completed!=Status::Ok)return completed;
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE && defined(CONFIG_IDF_TARGET_ESP32P4) && !CONFIG_SPIRAM_ECC_ENABLE
    if(!target.pixels||!target.width||!target.height||target.stride<target.width||target.stride>=0x2000||target.height>=0x4000||
       target.height>SIZE_MAX/target.stride/2||target.bytes<target.stride*target.height*2||r.x<0||r.y<0||r.width<=0||r.height<=0||
       int64_t(r.x)+r.width>target.width||int64_t(r.y)+r.height>target.height)return Status::Unsupported;
    const size_t bytes=target.stride*target.height*2,line=cacheLineSize(target.pixels);const uintptr_t base=reinterpret_cast<uintptr_t>(target.pixels);
    if(!line||bytes%line||base%line||base>UINTPTR_MAX-bytes)return Status::Unsupported;
    const auto *last=reinterpret_cast<const void*>(base+bytes-1);
    if(!((esp_ptr_internal(target.pixels)&&esp_ptr_internal(last))||(esp_ptr_external_ram(target.pixels)&&esp_ptr_external_ram(last))))return Status::Unsupported;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6,1,0)
    if(!dma2d_check_transaction_alignment_constraint(target.pixels,target.stride,r.width,size_t(r.x),16)||
       !dma2d_check_transaction_alignment_constraint(target.pixels,target.stride*2,r.width,size_t(r.x)*2,8))return Status::Unsupported;
#endif
    if(!alpha)return Status::Ok;
    if(!client_){ppa_client_config_t cfg={};cfg.oper_type=PPA_OPERATION_BLEND;cfg.max_pending_trans_num=1;cfg.data_burst_length=PPA_DATA_BURST_LENGTH_128;
      const auto error=ppa_register_client(&cfg,&client_);if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return Status::Unsupported;}}
    const size_t begin=(size_t(r.y)*target.stride*2)&~(line-1),end=((size_t(r.y+r.height)*target.stride*2)+line-1)&~(line-1);
    auto *region=reinterpret_cast<uint8_t*>(target.pixels)+begin;
#if !NICE_GFX_P4_PPA_DRIVER_INPUT_CACHE_SYNC
    if(overlaySynchronize(region,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_C2M)!=ESP_OK)return Status::IoError;
#endif
    ppa_blend_oper_config_t op={};
    op.in_bg.buffer=target.pixels;op.in_bg.pic_w=target.stride;op.in_bg.pic_h=target.height;
    op.in_bg.block_offset_x=r.x;op.in_bg.block_offset_y=r.y;op.in_bg.block_w=r.width;op.in_bg.block_h=r.height;op.in_bg.blend_cm=PPA_BLEND_COLOR_MODE_RGB565;
    op.bg_alpha_update_mode=PPA_ALPHA_FIX_VALUE;op.bg_alpha_fix_val=255;
    op.in_fg.buffer=target.pixels;op.in_fg.pic_w=target.stride*2;op.in_fg.pic_h=target.height;
    op.in_fg.block_offset_x=size_t(r.x)*2;op.in_fg.block_offset_y=r.y;op.in_fg.block_w=r.width;op.in_fg.block_h=r.height;op.in_fg.blend_cm=PPA_BLEND_COLOR_MODE_A8;
    op.fg_alpha_update_mode=PPA_ALPHA_FIX_VALUE;op.fg_alpha_fix_val=alpha;
    const uint32_t rgb=overlayRgb565Rgb888(color);op.fg_fix_rgb_val.r=uint8_t(rgb>>16);op.fg_fix_rgb_val.g=uint8_t(rgb>>8);op.fg_fix_rgb_val.b=uint8_t(rgb);
    op.out.buffer=target.pixels;op.out.buffer_size=bytes;op.out.pic_w=target.stride;op.out.pic_h=target.height;op.out.block_offset_x=r.x;op.out.block_offset_y=r.y;op.out.blend_cm=PPA_BLEND_COLOR_MODE_RGB565;op.mode=PPA_TRANS_MODE_BLOCKING;
    const int64_t started=esp_timer_get_time();auto error=ppa_do_blend(client_,&op);const uint64_t elapsed=uint64_t(esp_timer_get_time()-started);
    stats_.driverMicros+=elapsed;stats_.overlayDriverMicros+=elapsed;
    if(error==ESP_OK)error=overlaySynchronize(region,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return Status::IoError;}
    stats_.lastError=ESP_OK;++stats_.completed;return Status::Ok;
#else
    (void)target;(void)r;(void)color;(void)alpha;return Status::Unsupported;
#endif
  }
  bool canBlendOverlayIntoNative(const NativeFramebuffer &target,int32_t x,int32_t y,const OverlayView &source,
             size_t sourceX,size_t sourceY,size_t width,size_t height)const{
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
    struct Timer {Stats &stats;int64_t started;
      ~Timer(){stats.overlayPreflightMicros+=uint64_t(esp_timer_get_time()-started);}} timer{stats_,esp_timer_get_time()};
    ++stats_.overlayPreflightCalls;
    if(!target.pixels||!validOverlayRegion(target,x,y,source,sourceX,sourceY,width,height,cacheLineSize(target.pixels)))return false;
    const auto ram=[](const void *first,size_t bytes){const auto *last=static_cast<const uint8_t *>(first)+bytes-1;
      return (esp_ptr_external_ram(first)&&esp_ptr_external_ram(last))||(esp_ptr_internal(first)&&esp_ptr_internal(last));};
    if(!ram(target.pixels,target.stride*target.height*2)||!ram(source.data,source.stride*source.height*overlayPixelBytes(source.format)))return false;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6,1,0)
    return dma2d_check_transaction_alignment_constraint(target.pixels,target.stride,width,size_t(x),16)&&
      dma2d_check_transaction_alignment_constraint(source.data,source.stride,width,sourceX,uint32_t(overlayPixelBytes(source.format)*8));
#else
    // P4 without PSRAM ECC accepts arbitrary RGB/alpha pixel/block offsets. Its
    // aligned picture bases/rows stay validated above; do not inherit the
    // older S31/MSPI 16-byte ROI restriction for these native RAM pictures.
#if defined(CONFIG_IDF_TARGET_ESP32P4) && !CONFIG_SPIRAM_ECC_ENABLE
    return true;
#endif
    // Older SDKs do not expose the runtime MSPI/ECC alignment query. Keep a
    // conservative complete 16-byte window instead of risking DMA rejection.
    return !((target.stride*2|size_t(x)*2|width*2|source.stride*overlayPixelBytes(source.format)|
      sourceX*overlayPixelBytes(source.format)|width*overlayPixelBytes(source.format))&15);
#endif
#else
    (void)target;(void)x;(void)y;(void)source;(void)sourceX;(void)sourceY;(void)width;(void)height;return false;
#endif
  }
  // S31 BLEND explicitly permits BG and OUT to share a picture. Keep their
  // blocks identical and preserve foreground storage until the blocking DMA
  // completes. Unsupported preflight never touches any destination pixel.
  Status blendOverlayIntoNative(const NativeFramebuffer &target,int32_t x,int32_t y,const OverlayView &source,
             size_t sourceX,size_t sourceY,size_t width,size_t height){
    const Status completed=finishCopyIntoNative();if(completed!=Status::Ok)return completed;
    if(!canBlendOverlayIntoNative(target,x,y,source,sourceX,sourceY,width,height))return Status::Unsupported;
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
    if(!source.alpha)return Status::Ok;
    if(!client_){ppa_client_config_t config={};config.oper_type=PPA_OPERATION_BLEND;config.max_pending_trans_num=1;
      config.data_burst_length=NICE_GFX_S31_PPA_BLEND_BURST==128?PPA_DATA_BURST_LENGTH_128:PPA_DATA_BURST_LENGTH_64;
      const auto error=ppa_register_client(&config,&client_);if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return Status::Unsupported;}}
    const size_t outBytes=target.stride*target.height*2;
#if !NICE_GFX_S31_OVERLAY_DRIVER_CACHE_SYNC
    const size_t line=cacheLineSize(target.pixels);
    const size_t begin=(size_t(y)*target.stride*2)&~(line-1),end=((size_t(y)+height)*target.stride*2+line-1)&~(line-1);
    auto *region=reinterpret_cast<uint8_t *>(target.pixels)+begin;
    // Preserve the same whole rows the driver invalidates, then invalidate
    // again after DMA to discard speculative CPU reads during the transfer.
#if !NICE_GFX_P4_PPA_DRIVER_INPUT_CACHE_SYNC
    auto error=overlaySynchronize(region,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    if(error==ESP_OK)error=overlaySynchronize(const_cast<uint8_t *>(source.data),source.stride*source.height*overlayPixelBytes(source.format),ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return Status::IoError;}
#else
    esp_err_t error=ESP_OK;
#endif
#else
    esp_err_t error=ESP_OK;
#endif
    ppa_blend_oper_config_t op={};
    op.in_bg.buffer=target.pixels;op.in_bg.pic_w=target.stride;op.in_bg.pic_h=target.height;
    op.in_bg.block_w=width;op.in_bg.block_h=height;op.in_bg.block_offset_x=x;op.in_bg.block_offset_y=y;op.in_bg.blend_cm=PPA_BLEND_COLOR_MODE_RGB565;
    op.bg_alpha_update_mode=PPA_ALPHA_FIX_VALUE;op.bg_alpha_fix_val=255;
    op.in_fg.buffer=source.data;op.in_fg.pic_w=source.stride;op.in_fg.pic_h=source.height;
    op.in_fg.block_w=width;op.in_fg.block_h=height;op.in_fg.block_offset_x=sourceX;op.in_fg.block_offset_y=sourceY;
    op.in_fg.blend_cm=source.format==OverlayFormat::Rgb565?PPA_BLEND_COLOR_MODE_RGB565:
      source.format==OverlayFormat::Alpha8?PPA_BLEND_COLOR_MODE_A8:PPA_BLEND_COLOR_MODE_ARGB8888;
    if(source.format==OverlayFormat::Rgb565){op.fg_alpha_update_mode=PPA_ALPHA_FIX_VALUE;op.fg_alpha_fix_val=source.alpha;}
    else if(source.alpha==255)op.fg_alpha_update_mode=PPA_ALPHA_NO_CHANGE;
    else {op.fg_alpha_update_mode=PPA_ALPHA_SCALE;
      // The engine multiplies by an 8-bit factor then shifts by eight. Use
      // alpha+1 so an opaque (255) texel becomes exactly the requested alpha.
      op.fg_alpha_scale_ratio=float(overlayAlphaFactor(source.alpha))/256.0f;}
    const auto rgb=[](uint32_t color){color_pixel_rgb888_data_t value={};value.r=uint8_t(color>>16);value.g=uint8_t(color>>8);value.b=uint8_t(color);return value;};
    if(source.format==OverlayFormat::Alpha8)op.fg_fix_rgb_val=rgb(overlayRgb565Rgb888(source.color));
    if(source.colorKey){op.fg_ck_en=true;op.fg_ck_rgb_low_thres=rgb(overlayRgb565Rgb888(source.key));
      // A 5/6-bit component may expand by zero fill or bit replication. Its
      // complete RGB888 bucket matches exactly one original RGB565 component.
      op.fg_ck_rgb_high_thres=rgb(overlayRgb565KeyHigh(source.key));}
    op.out.buffer=target.pixels;op.out.buffer_size=outBytes;op.out.pic_w=target.stride;op.out.pic_h=target.height;
    op.out.block_offset_x=x;op.out.block_offset_y=y;op.out.blend_cm=PPA_BLEND_COLOR_MODE_RGB565;op.mode=PPA_TRANS_MODE_BLOCKING;
    const int64_t started=esp_timer_get_time();error=ppa_do_blend(client_,&op);
    const uint64_t elapsed=uint64_t(esp_timer_get_time()-started);stats_.driverMicros+=elapsed;stats_.overlayDriverMicros+=elapsed;
#if !NICE_GFX_S31_OVERLAY_DRIVER_CACHE_SYNC
    if(error==ESP_OK)error=overlaySynchronize(region,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
#endif
    if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return Status::IoError;}
    stats_.lastError=ESP_OK;++stats_.completed;return Status::Ok;
#else
    return Status::Unsupported;
#endif
  }
  bool close(){
    const Status drained=finishCopyIntoNative();
    if(drained==Status::Busy)return false;
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
    // Blocking operations have completed before return. If the SDK reports an
    // unfinished transaction, retain its memory rather than free DMA storage.
    if(fillClient_&&ppa_unregister_client(fillClient_)!=ESP_OK)return false;
    fillClient_=nullptr;
    if(copyClient_&&ppa_unregister_client(copyClient_)!=ESP_OK)return false;
    copyClient_=nullptr;
    if(client_&&ppa_unregister_client(client_)!=ESP_OK)return false;
    client_=nullptr;heap_caps_free(scratch_);scratch_=nullptr;capacity_=0;stats_.scratchBytes=0;
#if defined(CONFIG_IDF_TARGET_ESP32S31)
    if(asyncCopy_){
      vSemaphoreDelete(asyncCopy_->semaphore);asyncCopy_->~AsyncCopyState();heap_caps_free(asyncCopy_);asyncCopy_=nullptr;
    }
#endif
#endif
    return drained==Status::Ok;
  }
 private:
  Status copyIntoRgb565(const NativeFramebuffer &target,int32_t x,int32_t y,PixelView source,size_t sourceBytes,
             size_t sourceX,size_t sourceY,size_t width,size_t height,PixelOrder outputOrder){
    const Status completed=finishCopyIntoNative();if(completed!=Status::Ok)return completed;
#if !NICE_GFX_S31_PPA_BLEND_AVAILABLE || !defined(CONFIG_IDF_TARGET_ESP32S31)
    if(outputOrder==PixelOrder::Wire){++stats_.wireCopy.rejectedPlatform;return Status::Unsupported;}
#endif
    if(!canCopyIntoNative(target,x,y,source,sourceBytes,sourceX,sourceY,width,height)){
      if(outputOrder==PixelOrder::Wire)++stats_.wireCopy.rejectedFootprint;
      return Status::Unsupported;
    }
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
    if(outputOrder==PixelOrder::Wire){
#if defined(CONFIG_IDF_TARGET_ESP32S31)
      // Wire writeback is optional. Admit only an ordinary blocking task and
      // complete owned RAM pictures before any driver access or cache change.
      if(xPortInIsrContext()||!xPortCanYield()){++stats_.wireCopy.rejectedContext;return Status::Unsupported;}
      const size_t inputBytes=source.stride*source.height*2;
      const size_t inputLine=cacheLineSize(source.data);
      if(!inputLine||(inputLine&(inputLine-1))||reinterpret_cast<uintptr_t>(source.data)%inputLine||inputBytes%inputLine){++stats_.wireCopy.rejectedSourceCache;return Status::Unsupported;}
      const auto *inputEnd=source.data+inputBytes-1;
      const auto *outputEnd=reinterpret_cast<const uint8_t*>(target.pixels)+target.stride*target.height*2-1;
      if(!((esp_ptr_internal(source.data)&&esp_ptr_internal(inputEnd))||(esp_ptr_external_ram(source.data)&&esp_ptr_external_ram(inputEnd)))||
         !((esp_ptr_internal(target.pixels)&&esp_ptr_internal(outputEnd))||(esp_ptr_external_ram(target.pixels)&&esp_ptr_external_ram(outputEnd)))){++stats_.wireCopy.rejectedMemory;return Status::Unsupported;}
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6,1,0)
      if(!dma2d_check_transaction_alignment_constraint(source.data,source.stride,width,sourceX,16)||
         !dma2d_check_transaction_alignment_constraint(target.pixels,target.stride,width,size_t(x),16)){++stats_.wireCopy.rejectedDmaAlignment;return Status::Unsupported;}
#else
      if((reinterpret_cast<uintptr_t>(source.data)|reinterpret_cast<uintptr_t>(target.pixels)|source.stride*2|target.stride*2|sourceX*2|size_t(x)*2|width*2)&15u){++stats_.wireCopy.rejectedDmaAlignment;return Status::Unsupported;}
#endif
#else
      return Status::Unsupported;
#endif
    }
    if(!copyClient_){ppa_client_config_t config={};config.oper_type=PPA_OPERATION_SRM;config.max_pending_trans_num=1;config.data_burst_length=NICE_GFX_S31_PPA_BLEND_BURST==128?PPA_DATA_BURST_LENGTH_128:PPA_DATA_BURST_LENGTH_64;
      const auto error=ppa_register_client(&config,&copyClient_);if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return Status::IoError;}}
    const size_t line=cacheLineSize(target.pixels),bytes=target.stride*target.height*2;
    const size_t begin=(size_t(y)*target.stride*2)&~(line-1),end=((size_t(y)+height)*target.stride*2+line-1)&~(line-1);
    auto *region=reinterpret_cast<uint8_t *>(target.pixels)+begin;
    // CPU outside regions and overlays may share these SDK-invalidated rows.
    if(esp_cache_msync(region,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_C2M)!=ESP_OK){++stats_.failures;return Status::IoError;}
    ppa_srm_oper_config_t op={};op.in.buffer=source.data;op.in.pic_w=source.stride;op.in.pic_h=source.height;
    op.in.block_w=width;op.in.block_h=height;op.in.block_offset_x=sourceX;op.in.block_offset_y=sourceY;op.in.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
    op.out.buffer=target.pixels;op.out.buffer_size=bytes;op.out.pic_w=target.stride;op.out.pic_h=target.height;
    op.out.block_offset_x=x;op.out.block_offset_y=y;op.out.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
    op.scale_x=op.scale_y=1.0f;op.byte_swap=source.order!=outputOrder;op.mode=PPA_TRANS_MODE_BLOCKING;
    if(outputOrder==PixelOrder::Wire)++stats_.wireCopy.driverCalls;
    const int64_t started=esp_timer_get_time();const auto error=ppa_do_scale_rotate_mirror(copyClient_,&op);
    const uint64_t elapsed=uint64_t(esp_timer_get_time()-started);stats_.copyDriverMicros+=elapsed;
    if(outputOrder==PixelOrder::Wire)stats_.wireCopy.driverMicros+=elapsed;
    if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return Status::IoError;}
    const auto sync=esp_cache_msync(region,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    if(sync!=ESP_OK){stats_.lastError=sync;++stats_.failures;return Status::IoError;}
    ++stats_.copyCompleted;stats_.lastError=ESP_OK;return Status::Ok;
#else
    return Status::Unsupported;
#endif
  }
  bool targetBlend(uint16_t *target,size_t targetStride,size_t targetBytes,
             const uint16_t *foreground,size_t foregroundStride,const uint16_t *background,
             size_t backgroundStride,size_t width,size_t height,uint8_t alpha,bool solid,uint16_t solidColor,PixelOrder sourceOrder=PixelOrder::Wire){
    if(finishCopyIntoNative()!=Status::Ok)return false;
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
    if(!target||!foreground||(!solid&&!background)||!width||!height||width>=0x4000||height>=0x4000||
       targetStride!=width||targetStride>=0x4000||foregroundStride<width||foregroundStride>=0x4000||
       (!solid&&(backgroundStride<width||backgroundStride>=0x4000))||
       (reinterpret_cast<uintptr_t>(target)&63)||(reinterpret_cast<uintptr_t>(foreground)&63)||
       (!solid&&(reinterpret_cast<uintptr_t>(background)&63))||
       targetStride>SIZE_MAX/height/2||targetStride*height*2>targetBytes||
       targetStride*height*2>UINT32_MAX||(height-1)>(SIZE_MAX/2-width)/foregroundStride||
       (!solid&&(height-1)>(SIZE_MAX/2-width)/backgroundStride)||
       (targetStride*height*2)%64||(foregroundStride*2)%64||(!solid&&(backgroundStride*2)%64)){
      ++stats_.rejected;return false;
    }
    const size_t outBytes=targetStride*height*2;
    const uintptr_t out=reinterpret_cast<uintptr_t>(target),front=reinterpret_cast<uintptr_t>(foreground);
    const size_t frontBytes=((height-1)*foregroundStride+width)*2;
    if(out>UINTPTR_MAX-outBytes||front>UINTPTR_MAX-frontBytes||
       (out<front+frontBytes&&front<out+outBytes)){++stats_.rejected;return false;}
    if(!solid){const uintptr_t back=reinterpret_cast<uintptr_t>(background);const size_t backBytes=((height-1)*backgroundStride+width)*2;
      if(back>UINTPTR_MAX-backBytes||(out<back+backBytes&&back<out+outBytes)){++stats_.rejected;return false;}}
    if(!client_){ppa_client_config_t config={};config.oper_type=PPA_OPERATION_BLEND;
      config.max_pending_trans_num=1;config.data_burst_length=NICE_GFX_S31_PPA_BLEND_BURST==128?PPA_DATA_BURST_LENGTH_128:PPA_DATA_BURST_LENGTH_64;
      const auto error=ppa_register_client(&config,&client_);
      if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return false;}}
    ppa_blend_oper_config_t operation={};
    operation.in_bg.buffer=solid?foreground:background;
    operation.in_bg.pic_w=solid?foregroundStride:backgroundStride;operation.in_bg.pic_h=height;
    operation.in_bg.block_w=width;operation.in_bg.block_h=height;operation.in_bg.blend_cm=PPA_BLEND_COLOR_MODE_RGB565;
    operation.bg_byte_swap=sourceOrder==PixelOrder::Wire;operation.bg_alpha_update_mode=PPA_ALPHA_FIX_VALUE;operation.bg_alpha_fix_val=255;
    operation.in_fg.buffer=foreground;operation.in_fg.pic_w=foregroundStride;operation.in_fg.pic_h=height;
    operation.in_fg.block_w=width;operation.in_fg.block_h=height;
    operation.in_fg.blend_cm=solid?PPA_BLEND_COLOR_MODE_A8:PPA_BLEND_COLOR_MODE_RGB565;
    operation.fg_byte_swap=!solid;operation.fg_alpha_update_mode=PPA_ALPHA_FIX_VALUE;
    operation.fg_alpha_fix_val=solid?255-alpha:alpha;
    if(solid){
      // A8 supplies alpha only. FIX replaces every source alpha, so the same
      // read-only page bytes safely act as a mask, without allocating a mask.
      operation.fg_fix_rgb_val.r=uint8_t((solidColor>>11)<<3);
      operation.fg_fix_rgb_val.g=uint8_t(((solidColor>>5)&63)<<2);
      operation.fg_fix_rgb_val.b=uint8_t((solidColor&31)<<3);
    }
    operation.out.buffer=target;operation.out.buffer_size=outBytes;
    operation.out.pic_w=targetStride;operation.out.pic_h=height;operation.out.blend_cm=PPA_BLEND_COLOR_MODE_RGB565;
    operation.mode=PPA_TRANS_MODE_BLOCKING;
    const int64_t started=esp_timer_get_time();const auto error=ppa_do_blend(client_,&operation);
    stats_.driverMicros+=uint64_t(esp_timer_get_time()-started);
    if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return false;}
    const auto sync=esp_cache_msync(target,outBytes,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    if(sync!=ESP_OK){stats_.lastError=sync;++stats_.failures;return false;}
    stats_.lastError=ESP_OK;++stats_.completed;return true;
#else
    (void)target;(void)targetStride;(void)targetBytes;(void)foreground;(void)foregroundStride;
    (void)background;(void)backgroundStride;(void)width;(void)height;(void)alpha;(void)solid;(void)solidColor;(void)sourceOrder;
    ++stats_.rejected;return false;
#endif
  }
  const uint16_t *borrowSources(const uint16_t *foreground,size_t foregroundStride,
             const uint16_t *background,size_t backgroundStride,size_t width,size_t height,uint8_t alpha,bool wire){
    if(!processNative(foreground,foregroundStride,background,backgroundStride,width,height,alpha))return nullptr;
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
    if(wire)convertToWire(scratch_,width,width,height);
    else {stats_.lastError=ESP_OK;++stats_.completed;}
    return scratch_;
#else
    (void)wire;
    return nullptr;
#endif
  }
  bool processNative(const uint16_t *foreground,size_t foregroundStride,const uint16_t *background,
             size_t backgroundStride,size_t width,size_t height,uint8_t alpha){
    return processRegionNative(foreground,foregroundStride,background,backgroundStride,
      height,0,0,width,height,alpha);
  }
  bool processRegionNative(const uint16_t *foreground,size_t foregroundStride,const uint16_t *background,
             size_t backgroundStride,size_t pictureHeight,size_t sourceX,size_t sourceY,
             size_t width,size_t height,uint8_t alpha){
    if(finishCopyIntoNative()!=Status::Ok)return false;
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
    // DMA picture bases and rows stay aligned. Region offsets are expressed in
    // the PPA descriptor, never by shifting the DMA base into a cache line.
    // Rejection is side-effect free and lets the caller keep its CPU fallback.
    if(!foreground||!background||!width||!height||!pictureHeight||width>=0x4000||height>=0x4000||pictureHeight>=0x4000||
       foregroundStride<width||backgroundStride<width||foregroundStride>=0x4000||backgroundStride>=0x4000||
       sourceX>foregroundStride||width>foregroundStride-sourceX||sourceX>backgroundStride||width>backgroundStride-sourceX||
       sourceY>pictureHeight||height>pictureHeight-sourceY||
       (reinterpret_cast<uintptr_t>(foreground)&63)||(reinterpret_cast<uintptr_t>(background)&63)||
       (foregroundStride*2)%64||(backgroundStride*2)%64||
       width>SIZE_MAX/height/2||pictureHeight>SIZE_MAX/foregroundStride/2||
       pictureHeight>SIZE_MAX/backgroundStride/2){++stats_.rejected;return false;}
    const size_t frontBytes=foregroundStride*pictureHeight*2,backBytes=backgroundStride*pictureHeight*2;
    if(reinterpret_cast<uintptr_t>(foreground)>UINTPTR_MAX-frontBytes||
       reinterpret_cast<uintptr_t>(background)>UINTPTR_MAX-backBytes){++stats_.rejected;return false;}
    const size_t bytes=width*height*2;
    if(bytes%64||bytes>UINT32_MAX){++stats_.rejected;return false;}
    if(!client_){
      ppa_client_config_t config={};config.oper_type=PPA_OPERATION_BLEND;
      config.max_pending_trans_num=1;config.data_burst_length=NICE_GFX_S31_PPA_BLEND_BURST==128?PPA_DATA_BURST_LENGTH_128:PPA_DATA_BURST_LENGTH_64;
      const auto error=ppa_register_client(&config,&client_);
      if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return false;}
    }
    if(bytes>capacity_){
      void *next=heap_caps_aligned_alloc(64,bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
      if(!next){++stats_.failures;stats_.lastError=ESP_ERR_NO_MEM;return false;}
      const size_t line=cacheLineSize(next);
      if(line&&(reinterpret_cast<uintptr_t>(next)%line||bytes%line)){
        heap_caps_free(next);++stats_.rejected;return false;
      }
      heap_caps_free(scratch_);scratch_=static_cast<uint16_t *>(next);capacity_=bytes;
      stats_.scratchBytes=capacity_;
    }
    ppa_blend_oper_config_t operation={};
    operation.in_fg.buffer=foreground;operation.in_fg.pic_w=foregroundStride;operation.in_fg.pic_h=pictureHeight;
    operation.in_fg.block_w=width;operation.in_fg.block_h=height;operation.in_fg.blend_cm=PPA_BLEND_COLOR_MODE_RGB565;
    operation.in_bg.buffer=background;operation.in_bg.pic_w=backgroundStride;operation.in_bg.pic_h=pictureHeight;
    operation.in_bg.block_w=width;operation.in_bg.block_h=height;operation.in_bg.blend_cm=PPA_BLEND_COLOR_MODE_RGB565;
    operation.in_fg.block_offset_x=operation.in_bg.block_offset_x=sourceX;
    operation.in_fg.block_offset_y=operation.in_bg.block_offset_y=sourceY;
    operation.out.buffer=scratch_;operation.out.buffer_size=capacity_;
    operation.out.pic_w=width;operation.out.pic_h=height;operation.out.blend_cm=PPA_BLEND_COLOR_MODE_RGB565;
    operation.fg_byte_swap=operation.bg_byte_swap=true;
    operation.fg_alpha_update_mode=PPA_ALPHA_FIX_VALUE;operation.fg_alpha_fix_val=alpha;
    operation.bg_alpha_update_mode=PPA_ALPHA_FIX_VALUE;operation.bg_alpha_fix_val=255;
    operation.mode=PPA_TRANS_MODE_BLOCKING;
    const int64_t started=esp_timer_get_time();
    const auto error=ppa_do_blend(client_,&operation);
    stats_.driverMicros+=uint64_t(esp_timer_get_time()-started);
    if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return false;}
    const size_t line=cacheLineSize(scratch_);
    if(line){
      const auto sync=esp_cache_msync(scratch_,capacity_,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
      if(sync!=ESP_OK){stats_.lastError=sync;++stats_.failures;return false;}
    }
    return true;
#else
    (void)foreground;(void)foregroundStride;(void)background;(void)backgroundStride;(void)width;(void)height;(void)alpha;
    (void)pictureHeight;(void)sourceX;(void)sourceY;
    ++stats_.rejected;return false;
#endif
  }
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
  void convertToWire(uint16_t *foreground,size_t foregroundStride,size_t width,size_t height){
    const int64_t conversionStarted=esp_timer_get_time();
    for(size_t row=0;row<height;++row){
      const uint16_t *source=scratch_+row*width;uint16_t *destination=foreground+row*foregroundStride;
      size_t x=0;
      for(;x+1<width;x+=2){
        uint32_t pair;memcpy(&pair,source+x,4);
        pair=((pair&0x00ff00ffu)<<8)|((pair>>8)&0x00ff00ffu);
        memcpy(destination+x,&pair,4);
      }
      if(x<width){const uint16_t color=source[x];destination[x]=uint16_t(color>>8|color<<8);}
    }
    stats_.conversionMicros+=uint64_t(esp_timer_get_time()-conversionStarted);
    stats_.lastError=ESP_OK;++stats_.completed;
  }
#endif
  mutable Stats stats_;
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE && defined(CONFIG_IDF_TARGET_ESP32S31)
  // BEGIN SINGLE_ASYNC_COPY_STATE
  struct AsyncCopyState {
    StaticSemaphore_t semaphoreStorage;
    SemaphoreHandle_t semaphore=nullptr;
    std::atomic<uint32_t> completedMicros{0};
    std::atomic<bool> completed{false};
    uint8_t *rows=nullptr;
    size_t rowBytes=0;
    uint32_t startedMicros=0;
    bool pending=false,callbackRegistered=false;
  };
  static bool IRAM_ATTR asyncCopyDone(ppa_client_handle_t,ppa_event_data_t*,void *userData){
    auto *state=static_cast<AsyncCopyState*>(userData);
    if(!state)return false; // Existing blocking copies use null user_data.
    state->completedMicros.store(uint32_t(esp_timer_get_time()),std::memory_order_relaxed);
    state->completed.store(true,std::memory_order_release);
    BaseType_t woken=pdFALSE;
    xSemaphoreGiveFromISR(state->semaphore,&woken);
    // No state access after waking its owner, which may drain and close.
    return woken==pdTRUE;
  }
  AsyncCopyState *asyncCopy_=nullptr;
  // END SINGLE_ASYNC_COPY_STATE
#endif
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
  esp_err_t overlaySynchronize(void *pixels,size_t bytes,int flags){
    const int64_t started=esp_timer_get_time();const esp_err_t result=esp_cache_msync(pixels,bytes,flags);
    stats_.overlaySyncMicros+=uint64_t(esp_timer_get_time()-started);++stats_.overlaySyncCalls;stats_.overlaySyncedBytes+=bytes;return result;
  }
  ppa_client_handle_t client_=nullptr;
  ppa_client_handle_t copyClient_=nullptr;
  ppa_client_handle_t fillClient_=nullptr;
  uint16_t *scratch_=nullptr;
  size_t capacity_=0;
#endif
};
}}} // namespace nice_gfx::htmlui::rgb565_ppa
#endif
