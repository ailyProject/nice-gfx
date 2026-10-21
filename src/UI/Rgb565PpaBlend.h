#ifndef NICE_GFX_RGB565_PPA_BLEND_H
#define NICE_GFX_RGB565_PPA_BLEND_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../Display/Transport.h"
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include <soc/soc_caps.h>
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31) && defined(SOC_PPA_SUPPORTED) && SOC_PPA_SUPPORTED
#define NICE_GFX_S31_PPA_BLEND_AVAILABLE 1
#include <driver/ppa.h>
#include <esp_cache.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#else
#define NICE_GFX_S31_PPA_BLEND_AVAILABLE 0
#endif

namespace nice_gfx { namespace htmlui { namespace rgb565_ppa {
struct Stats {
  uint64_t completed=0,driverMicros=0,conversionMicros=0;
  uint64_t copyCompleted=0,copyDriverMicros=0;
  uint32_t rejected=0,failures=0;
  int lastError=0;
  size_t scratchBytes=0;
};
// Optional S31 backend. Single owner task only.
// Inputs are immutable Wire565 for the duration of the blocking PPA call.
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
    const size_t line=esp_cache_get_line_size_by_addr(target.pixels);
    if(!line||(line&(line-1))||out%line||targetBytes%line||target.bytes<targetBytes||targetBytes>UINT32_MAX||
       sourceBytes<inputBytes||(in&1)||out>UINTPTR_MAX-targetBytes||in>UINTPTR_MAX-inputBytes||
       (out<in+inputBytes&&in<out+targetBytes))return false;
    return true;
#else
    (void)target;(void)x;(void)y;(void)source;(void)sourceBytes;(void)sourceX;(void)sourceY;(void)width;(void)height;return false;
#endif
  }
  Status copyIntoNative(const NativeFramebuffer &target,int32_t x,int32_t y,PixelView source,size_t sourceBytes,
             size_t sourceX,size_t sourceY,size_t width,size_t height){
    if(!canCopyIntoNative(target,x,y,source,sourceBytes,sourceX,sourceY,width,height))return Status::Unsupported;
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
    if(!copyClient_){ppa_client_config_t config={};config.oper_type=PPA_OPERATION_SRM;config.max_pending_trans_num=1;config.data_burst_length=PPA_DATA_BURST_LENGTH_64;
      const auto error=ppa_register_client(&config,&copyClient_);if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return Status::IoError;}}
    const size_t line=esp_cache_get_line_size_by_addr(target.pixels),bytes=target.stride*target.height*2;
    const size_t begin=(size_t(y)*target.stride*2)&~(line-1),end=((size_t(y)+height)*target.stride*2+line-1)&~(line-1);
    auto *region=reinterpret_cast<uint8_t *>(target.pixels)+begin;
    // CPU outside regions and overlays may share these SDK-invalidated rows.
    if(esp_cache_msync(region,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_C2M)!=ESP_OK){++stats_.failures;return Status::IoError;}
    ppa_srm_oper_config_t op={};op.in.buffer=source.data;op.in.pic_w=source.stride;op.in.pic_h=source.height;
    op.in.block_w=width;op.in.block_h=height;op.in.block_offset_x=sourceX;op.in.block_offset_y=sourceY;op.in.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
    op.out.buffer=target.pixels;op.out.buffer_size=bytes;op.out.pic_w=target.stride;op.out.pic_h=target.height;
    op.out.block_offset_x=x;op.out.block_offset_y=y;op.out.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
    op.scale_x=op.scale_y=1.0f;op.byte_swap=source.order==PixelOrder::Wire;op.mode=PPA_TRANS_MODE_BLOCKING;
    const int64_t started=esp_timer_get_time();const auto error=ppa_do_scale_rotate_mirror(copyClient_,&op);stats_.copyDriverMicros+=uint64_t(esp_timer_get_time()-started);
    if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return Status::IoError;}
    const auto sync=esp_cache_msync(region,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    if(sync!=ESP_OK){stats_.lastError=sync;++stats_.failures;return Status::IoError;}
    ++stats_.copyCompleted;stats_.lastError=ESP_OK;return Status::Ok;
#else
    return Status::Unsupported;
#endif
  }
  bool close(){
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
    // Blocking operations have completed before return. If the SDK reports an
    // unfinished transaction, retain its memory rather than free DMA storage.
    if(copyClient_&&ppa_unregister_client(copyClient_)!=ESP_OK)return false;
    copyClient_=nullptr;
    if(client_&&ppa_unregister_client(client_)!=ESP_OK)return false;
    client_=nullptr;heap_caps_free(scratch_);scratch_=nullptr;capacity_=0;stats_.scratchBytes=0;
#endif
    return true;
  }
 private:
  bool targetBlend(uint16_t *target,size_t targetStride,size_t targetBytes,
             const uint16_t *foreground,size_t foregroundStride,const uint16_t *background,
             size_t backgroundStride,size_t width,size_t height,uint8_t alpha,bool solid,uint16_t solidColor,PixelOrder sourceOrder=PixelOrder::Wire){
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
      config.max_pending_trans_num=1;config.data_burst_length=PPA_DATA_BURST_LENGTH_64;
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
    (void)background;(void)backgroundStride;(void)width;(void)height;(void)alpha;(void)solid;(void)solidColor;
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
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
    // Prototype deliberately restricts inputs to complete aligned RGB565 rows.
    // Rejection is side-effect free and lets the caller keep its CPU fallback.
    if(!foreground||!background||!width||!height||width>=0x4000||height>=0x4000||
       foregroundStride<width||backgroundStride<width||foregroundStride>=0x4000||backgroundStride>=0x4000||
       (reinterpret_cast<uintptr_t>(foreground)&63)||(reinterpret_cast<uintptr_t>(background)&63)||
       (foregroundStride*2)%64||(backgroundStride*2)%64||
       width>SIZE_MAX/height/2||(height-1)>(SIZE_MAX/2-width)/foregroundStride||
       (height-1)>(SIZE_MAX/2-width)/backgroundStride){++stats_.rejected;return false;}
    const size_t bytes=width*height*2;
    if(bytes%64||bytes>UINT32_MAX){++stats_.rejected;return false;}
    if(!client_){
      ppa_client_config_t config={};config.oper_type=PPA_OPERATION_BLEND;
      config.max_pending_trans_num=1;config.data_burst_length=PPA_DATA_BURST_LENGTH_64;
      const auto error=ppa_register_client(&config,&client_);
      if(error!=ESP_OK){stats_.lastError=error;++stats_.failures;return false;}
    }
    if(bytes>capacity_){
      void *next=heap_caps_aligned_alloc(64,bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
      if(!next){++stats_.failures;stats_.lastError=ESP_ERR_NO_MEM;return false;}
      const size_t line=esp_cache_get_line_size_by_addr(next);
      if(line&&(reinterpret_cast<uintptr_t>(next)%line||bytes%line)){
        heap_caps_free(next);++stats_.rejected;return false;
      }
      heap_caps_free(scratch_);scratch_=static_cast<uint16_t *>(next);capacity_=bytes;
      stats_.scratchBytes=capacity_;
    }
    ppa_blend_oper_config_t operation={};
    operation.in_fg.buffer=foreground;operation.in_fg.pic_w=foregroundStride;operation.in_fg.pic_h=height;
    operation.in_fg.block_w=width;operation.in_fg.block_h=height;operation.in_fg.blend_cm=PPA_BLEND_COLOR_MODE_RGB565;
    operation.in_bg.buffer=background;operation.in_bg.pic_w=backgroundStride;operation.in_bg.pic_h=height;
    operation.in_bg.block_w=width;operation.in_bg.block_h=height;operation.in_bg.blend_cm=PPA_BLEND_COLOR_MODE_RGB565;
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
    const size_t line=esp_cache_get_line_size_by_addr(scratch_);
    if(line){
      const auto sync=esp_cache_msync(scratch_,capacity_,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
      if(sync!=ESP_OK){stats_.lastError=sync;++stats_.failures;return false;}
    }
    return true;
#else
    (void)foreground;(void)foregroundStride;(void)background;(void)backgroundStride;(void)width;(void)height;(void)alpha;
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
  Stats stats_;
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
  ppa_client_handle_t client_=nullptr;
  ppa_client_handle_t copyClient_=nullptr;
  uint16_t *scratch_=nullptr;
  size_t capacity_=0;
#endif
};
}}} // namespace nice_gfx::htmlui::rgb565_ppa
#endif
