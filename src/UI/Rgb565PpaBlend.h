#ifndef NICE_GFX_RGB565_PPA_BLEND_H
#define NICE_GFX_RGB565_PPA_BLEND_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
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
  bool close(){
#if NICE_GFX_S31_PPA_BLEND_AVAILABLE
    // Blocking operations have completed before return. If the SDK reports an
    // unfinished transaction, retain its memory rather than free DMA storage.
    if(client_&&ppa_unregister_client(client_)!=ESP_OK)return false;
    client_=nullptr;heap_caps_free(scratch_);scratch_=nullptr;capacity_=0;stats_.scratchBytes=0;
#endif
    return true;
  }
 private:
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
    if(!foreground||!background||!width||!height||width>65535||height>65535||
       foregroundStride<width||backgroundStride<width||foregroundStride>65535||backgroundStride>65535||
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
  uint16_t *scratch_=nullptr;
  size_t capacity_=0;
#endif
};
}}} // namespace nice_gfx::htmlui::rgb565_ppa
#endif
