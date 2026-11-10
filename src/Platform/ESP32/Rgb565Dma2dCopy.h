#pragma once
// SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
// SPDX-License-Identifier: Apache-2.0
// Internal Native565 M2M adapter derived from IDF v5.5.5 esp_async_fbcpy.c.
// SDK DMA2D owns channel arbitration; this owner retains only its descriptors.
#include "../../Display/Transport.h"
#ifndef NICE_GFX_P4_DMA2D_COPY
#define NICE_GFX_P4_DMA2D_COPY NICE_GFX_P4_SYSTEM_MODAL_ACCELERATION_DEFAULT
#endif
#ifndef NICE_GFX_P4_DMA2D_COPY_ODD_ROI
#define NICE_GFX_P4_DMA2D_COPY_ODD_ROI NICE_GFX_P4_SYSTEM_MODAL_ACCELERATION_DEFAULT
#endif
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include <sdkconfig.h>
#include <esp_idf_version.h>
#include <soc/soc_caps.h>
#endif
#if defined(CONFIG_IDF_TARGET_ESP32P4) && defined(ESP_IDF_VERSION) && defined(ESP_IDF_VERSION_VAL)
#if ESP_IDF_VERSION == ESP_IDF_VERSION_VAL(5,5,5) && SOC_DMA2D_SUPPORTED && __has_include("esp_private/dma2d.h") && __has_include("esp_private/esp_cache_private.h") && __has_include("esp_flash_encrypt.h")
#define NICE_GFX_DETAIL_RGB565_DMA2D 1
#endif
#endif
#ifndef NICE_GFX_DETAIL_RGB565_DMA2D
#define NICE_GFX_DETAIL_RGB565_DMA2D 0
#endif
#if NICE_GFX_DETAIL_RGB565_DMA2D
#include <esp_private/dma2d.h>
#include <esp_private/esp_cache_private.h>
#include <soc/dma2d_channel.h>
#include <hal/dma2d_ll.h>
#include <esp_cache.h>
#include <esp_heap_caps.h>
#include <esp_memory_utils.h>
#include <esp_flash_encrypt.h>
#include <esp_timer.h>
#include <esp_attr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <atomic>
#include <new>
#endif
namespace nice_gfx { namespace platform { namespace rgb565_dma2d {
struct Stats {
  uint64_t attempts=0,completed=0,pixels=0,rejected=0,failures=0;
  uint64_t installAttempts=0,installFailures=0,eofTransfers=0,cancelled=0,timeouts=0;
  uint64_t sourceSyncMicros=0,targetPreSyncMicros=0,descriptorSyncMicros=0;
  uint64_t submitMicros=0,waitMicros=0,postSyncMicros=0;
  int32_t lastError=0;
  bool ready=false,pending=false,poisoned=false;
};
// Serialized owner task only. The caller owns both complete pictures until
// copy/drain returns. Mapping, damage and presentation remain the caller's job.
// Configure explicitly in the owner TU with the two qualification macros;
// their values never alter this class's layout or inline implementation.
class Context {
 public:
  Context()=default;
  explicit Context(bool enabled,bool allowOdd=false):enabled_(enabled),allowOdd_(allowOdd){}
  Context(const Context&)=delete;Context&operator=(const Context&)=delete;
  ~Context(){(void)end();}
  static constexpr bool available(){return NICE_GFX_DETAIL_RGB565_DMA2D!=0;}
  bool configure(bool enabled,bool allowOdd=false){
    if(state_)return enabled_==enabled&&allowOdd_==allowOdd;
    enabled_=enabled;allowOdd_=allowOdd;return true;
  }
  const Stats &stats()const{return stats_;}
  bool canCopyIntoNative(const NativeFramebuffer &target,int32_t x,int32_t y,
      PixelView source,size_t sourceBytes,size_t sourceX,size_t sourceY,size_t width,size_t height)const{
#if NICE_GFX_DETAIL_RGB565_DMA2D
    Plan plan;return enabled_&&!stats_.poisoned&&ordinaryOwner()&&
      prepare(target,x,y,source,sourceBytes,sourceX,sourceY,width,height,plan);
#else
    (void)target;(void)x;(void)y;(void)source;(void)sourceBytes;(void)sourceX;(void)sourceY;(void)width;(void)height;return false;
#endif
  }
  Status copyIntoNative(const NativeFramebuffer &target,int32_t x,int32_t y,
      PixelView source,size_t sourceBytes,size_t sourceX,size_t sourceY,size_t width,size_t height){
    ++stats_.attempts;
#if NICE_GFX_DETAIL_RGB565_DMA2D
    if(stats_.poisoned)return Status::IoError;
    if(!enabled_||!ordinaryOwner()){++stats_.rejected;return Status::Unsupported;}
    if(state_&&state_->accepted)return Status::Busy;
    Plan plan;
    if(!prepare(target,x,y,source,sourceBytes,sourceX,sourceY,width,height,plan)){++stats_.rejected;return Status::Unsupported;}
    if(!state_){
      ++stats_.installAttempts;int32_t error=ESP_OK;
      if(!install(error)){++stats_.installFailures;++stats_.rejected;stats_.lastError=error;return Status::Unsupported;}
    }
    auto &s=*state_;s.rows=plan.outputRows;s.rowBytes=plan.outputBytes;s.pixels=uint64_t(width)*height;
    uint32_t at=clock();esp_err_t error=esp_cache_msync(const_cast<void*>(plan.inputRows),plan.inputBytes,ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    stats_.sourceSyncMicros+=uint32_t(clock()-at);at=clock();
    if(error==ESP_OK)error=esp_cache_msync(plan.outputRows,plan.outputBytes,ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    if(error==ESP_OK)error=esp_cache_msync(plan.outputRows,plan.outputBytes,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    stats_.targetPreSyncMicros+=uint32_t(clock()-at);
    if(error!=ESP_OK)return fail(error);
    fillDescriptor(*s.tx,const_cast<uint8_t*>(source.data),source.stride,source.height,sourceX,sourceY,width,height);
    fillDescriptor(*s.rx,target.pixels,target.stride,target.height,size_t(x),size_t(y),width,height);
    at=clock();error=esp_cache_msync(s.tx,s.descBytes,ESP_CACHE_MSYNC_FLAG_DIR_C2M|ESP_CACHE_MSYNC_FLAG_INVALIDATE);
    if(error==ESP_OK)error=esp_cache_msync(s.rx,s.descBytes,ESP_CACHE_MSYNC_FLAG_DIR_C2M|ESP_CACHE_MSYNC_FLAG_INVALIDATE);
    stats_.descriptorSyncMicros+=uint32_t(clock()-at);
    if(error!=ESP_OK)return fail(error);
    while(xSemaphoreTake(s.semaphore,0)==pdTRUE){}
    s.fence.store(None,std::memory_order_relaxed);s.error.store(ESP_OK,std::memory_order_relaxed);
    s.startAttempted.store(false,std::memory_order_relaxed);s.picked.store(false,std::memory_order_relaxed);
    s.accepted=true;stats_.pending=true;at=clock();
    error=dma2d_enqueue(s.pool,&s.config,s.transaction);stats_.submitMicros+=uint32_t(clock()-at);
    if(error!=ESP_OK){
      if(s.picked.load(std::memory_order_acquire)){s.error.store(error,std::memory_order_release);return drain();}
      s.accepted=false;stats_.pending=false;return fail(error);
    }
    return drain();
#else
    (void)target;(void)x;(void)y;(void)source;(void)sourceBytes;(void)sourceX;(void)sourceY;(void)width;(void)height;
    ++stats_.rejected;return Status::Unsupported;
#endif
  }
  // A timeout poisons the owner but NEVER ends an accepted source lease.
  // After any start attempt only real EOF can retire it. No CPU/PPA fallback.
  Status drain(){
#if NICE_GFX_DETAIL_RGB565_DMA2D
    if(!state_||!state_->accepted)return stats_.poisoned?Status::IoError:Status::Ok;
    if(!ordinaryOwner())return Status::Busy;
    auto &s=*state_;const uint32_t at=clock();bool timedOut=false;
    while(s.fence.load(std::memory_order_acquire)==None||s.pickedActive.load(std::memory_order_acquire)){
      xSemaphoreTake(s.semaphore,1);
      if(!timedOut&&uint32_t(clock()-at)>=500000){++stats_.timeouts;stats_.poisoned=true;stats_.lastError=s.error.load(std::memory_order_acquire);if(stats_.lastError==ESP_OK)stats_.lastError=ESP_ERR_TIMEOUT;timedOut=true;}
    }
    stats_.waitMicros+=uint32_t(clock()-at);
    const uint32_t fence=s.fence.load(std::memory_order_acquire);
    s.accepted=false;stats_.pending=false;
    esp_err_t error=s.error.load(std::memory_order_acquire);
    if(fence==Cancelled){++stats_.cancelled;return fail(error==ESP_OK?ESP_FAIL:error);}
    ++stats_.eofTransfers;const uint32_t syncAt=clock();
    // Even a faulted EOF must make the completed DMA output CPU-coherent.
    const esp_err_t synced=esp_cache_msync(s.rows,s.rowBytes,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    const esp_err_t descSynced=esp_cache_msync(s.rx,s.descBytes,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    stats_.postSyncMicros+=uint32_t(clock()-syncAt);
    if(error==ESP_OK)error=synced;if(error==ESP_OK)error=descSynced;
    if(error==ESP_OK&&s.rx->err_eof)error=ESP_FAIL;
    if(error!=ESP_OK)return fail(error);
    if(stats_.poisoned){++stats_.failures;return Status::IoError;}
    ++stats_.completed;stats_.pixels+=s.pixels;stats_.lastError=ESP_OK;return Status::Ok;
#else
    return Status::Ok;
#endif
  }
  // Busy preserves every allocation. Pool release is attempted only after a
  // real EOF or a confirmed pre-start cancellation. Destruction uses this too.
  Status end(){
#if NICE_GFX_DETAIL_RGB565_DMA2D
    if(!state_)return Status::Ok;
    if(!ordinaryOwner())return Status::Busy;
    const Status result=drain();if(state_->accepted)return Status::Busy;
    auto *s=state_;
    if(s->pool){const auto error=dma2d_release_pool(s->pool);if(error!=ESP_OK)return fail(error);s->pool=nullptr;}
    dispose(s);state_=nullptr;stats_.ready=stats_.pending=false;return result;
#else
    return Status::Ok;
#endif
  }
 private:
  bool enabled_=false,allowOdd_=false;Stats stats_{};
#if NICE_GFX_DETAIL_RGB565_DMA2D
  enum Fence:uint32_t {None,Eof,Cancelled};
  struct Plan {const void *inputRows=nullptr;void *outputRows=nullptr;size_t inputBytes=0,outputBytes=0;};
  struct State {
    dma2d_pool_handle_t pool=nullptr;dma2d_trans_t *transaction=nullptr;
    dma2d_descriptor_t *tx=nullptr,*rx=nullptr;size_t descBytes=0;
    SemaphoreHandle_t semaphore=nullptr;StaticSemaphore_t semaphoreStorage{};
    TaskHandle_t owner=nullptr;dma2d_trans_config_t config{};
    std::atomic<uint32_t> fence{None};std::atomic<int32_t> error{ESP_OK};
    std::atomic<bool> picked{false},startAttempted{false},pickedActive{false};
    bool accepted=false;void *rows=nullptr;size_t rowBytes=0;uint64_t pixels=0;
  };
  State *state_=nullptr;
  static uint32_t clock(){return uint32_t(esp_timer_get_time());}
  bool ordinaryOwner()const{return !xPortInIsrContext()&&xPortCanYield()&&(!state_||state_->owner==xTaskGetCurrentTaskHandle());}
  static bool ram(uintptr_t first,size_t bytes,bool &external){
    if(!bytes||first>UINTPTR_MAX-bytes)return false;
    const void *begin=reinterpret_cast<void*>(first),*last=reinterpret_cast<void*>(first+bytes-1);
    external=esp_ptr_external_ram(begin)&&esp_ptr_external_ram(last);
    if(external)return esp_ptr_dma_ext_capable(begin)&&esp_ptr_dma_ext_capable(last);
    return esp_ptr_internal(begin)&&esp_ptr_internal(last)&&esp_ptr_dma_capable(begin)&&esp_ptr_dma_capable(last);
  }
  bool prepare(const NativeFramebuffer&t,int32_t x,int32_t y,PixelView s,size_t capacity,
      size_t sx,size_t sy,size_t w,size_t h,Plan &p)const{
    if(!t.pixels||!t.width||!t.height||t.stride<t.width||!s.valid()||s.order!=PixelOrder::Native||
       !w||!h||x<0||y<0||t.width>=0x4000||t.height>=0x4000||t.stride>=0x4000||s.stride>=0x4000||s.height>=0x4000||
       size_t(x)>t.width||w>t.width-size_t(x)||size_t(y)>t.height||h>t.height-size_t(y)||
       sx>s.width||w>s.width-sx||sy>s.height||h>s.height-sy||
       t.height>SIZE_MAX/t.stride/2||s.height>SIZE_MAX/s.stride/2)return false;
    const size_t outBytes=t.stride*t.height*2,inBytes=s.stride*s.height*2;
    const uintptr_t out=reinterpret_cast<uintptr_t>(t.pixels),in=reinterpret_cast<uintptr_t>(s.data);
    if(t.bytes<outBytes||capacity<inBytes||out>UINTPTR_MAX-t.bytes||in>UINTPTR_MAX-capacity||
       (out<in+capacity&&in<out+t.bytes))return false;
    bool outExternal=false,inExternal=false;
    if(!ram(out,outBytes,outExternal)||!ram(in,inBytes,inExternal))return false;
    // This qualified implementation does not claim encrypted/ECC PSRAM.
    if((outExternal||inExternal)&&esp_flash_encryption_enabled())return false;
#if defined(CONFIG_SPIRAM_ECC_ENABLE) && CONFIG_SPIRAM_ECC_ENABLE
    if(outExternal||inExternal)return false;
#endif
    size_t outLine=0,inLine=0;
    if(esp_cache_get_alignment(outExternal?MALLOC_CAP_SPIRAM:MALLOC_CAP_INTERNAL,&outLine)!=ESP_OK||
       esp_cache_get_alignment(inExternal?MALLOC_CAP_SPIRAM:MALLOC_CAP_INTERNAL,&inLine)!=ESP_OK||
       !outLine||!inLine||(outLine&(outLine-1))||(inLine&(inLine-1))||
       out%outLine||in%inLine||outBytes%outLine||inBytes%inLine||
       (t.stride*2)%outLine||(s.stride*2)%inLine)return false;
    if(!allowOdd_&&((sx|size_t(x)|w)&1u))return false;
    p.inputRows=s.data+sy*s.stride*2;p.inputBytes=h*s.stride*2;
    p.outputRows=t.pixels+size_t(y)*t.stride;p.outputBytes=h*t.stride*2;return true;
  }
  static void fillDescriptor(dma2d_descriptor_t &d,void *base,size_t stride,size_t rows,size_t x,size_t y,size_t w,size_t h){
    memset(&d,0,sizeof(d));d.buffer=base;d.next=nullptr;d.dma2d_en=1;d.suc_eof=1;d.owner=DMA2D_DESCRIPTOR_BUFFER_OWNER_DMA;
    d.ha_length=stride;d.va_size=rows;d.hb_length=w;d.vb_size=h;d.x=x;d.y=y;
    d.pbyte=DMA2D_DESCRIPTOR_PBYTE_2B0_PER_PIXEL;d.mode=DMA2D_DESCRIPTOR_BLOCK_RW_MODE_SINGLE;
  }
  static bool IRAM_ATTR completed(dma2d_channel_handle_t,dma2d_event_data_t*,void *arg){
    auto&s=*static_cast<State*>(arg);BaseType_t wake=pdFALSE;xSemaphoreGiveFromISR(s.semaphore,&wake);
    // SDK555 has already cleared transaction references and freed channels.
    // Neither this callback nor its caller touches State after this store.
    s.fence.store(Eof,std::memory_order_release);return wake==pdTRUE;
  }
  static bool IRAM_ATTR picked(uint32_t n,const dma2d_trans_channel_info_t*channels,void*arg){
    auto&s=*static_cast<State*>(arg);s.pickedActive.store(true,std::memory_order_release);s.picked.store(true,std::memory_order_release);
    dma2d_channel_handle_t tx=nullptr,rx=nullptr;
    for(uint32_t i=0;i<n;++i){if(channels[i].dir==DMA2D_CHANNEL_DIRECTION_TX)tx=channels[i].chan;else if(channels[i].dir==DMA2D_CHANNEL_DIRECTION_RX)rx=channels[i].chan;}
    esp_err_t e=n==2&&tx&&rx?ESP_OK:ESP_ERR_INVALID_STATE;
    dma2d_trigger_t trigger{};trigger.periph=DMA2D_TRIG_PERIPH_M2M;trigger.periph_sel_id=SOC_DMA2D_TRIG_PERIPH_M2M_TX;
    bool txConnected=false,rxConnected=false;
    if(e==ESP_OK){e=dma2d_connect(tx,&trigger);txConnected=e==ESP_OK;}
    trigger.periph_sel_id=SOC_DMA2D_TRIG_PERIPH_M2M_RX;
    if(e==ESP_OK){e=dma2d_connect(rx,&trigger);rxConnected=e==ESP_OK;}
    dma2d_transfer_ability_t ability{};ability.access_ext_mem=true;ability.data_burst_length=128;ability.desc_burst_en=true;ability.mb_size=DMA2D_MACRO_BLOCK_SIZE_NONE;
    if(e==ESP_OK)e=dma2d_set_transfer_ability(tx,&ability);if(e==ESP_OK)e=dma2d_set_transfer_ability(rx,&ability);
    dma2d_rx_event_callbacks_t callbacks{};callbacks.on_recv_eof=completed;
    if(e==ESP_OK)e=dma2d_register_rx_event_callbacks(rx,&callbacks,&s);
    if(e==ESP_OK)e=dma2d_set_desc_addr(tx,reinterpret_cast<intptr_t>(s.tx));if(e==ESP_OK)e=dma2d_set_desc_addr(rx,reinterpret_cast<intptr_t>(s.rx));
    if(e!=ESP_OK){
      s.error.store(e,std::memory_order_release);bool yield=false;
      // Only this just-picked request, before ANY start attempt. This is a
      // confirmed no-source-read cancellation, never synthetic EOF.
      // SDK free_up_channels requires valid connected peripheral IDs.
      // A connect failure cannot safely be cancelled: retain it fail-closed.
      if(txConnected&&rxConnected&&dma2d_force_end(s.transaction,&yield)==ESP_OK){
        if(xPortInIsrContext()){BaseType_t wake=pdFALSE;xSemaphoreGiveFromISR(s.semaphore,&wake);yield|=wake==pdTRUE;}
        else xSemaphoreGive(s.semaphore);
        s.fence.store(Cancelled,std::memory_order_release);
      }
      s.pickedActive.store(false,std::memory_order_release);return yield;
    }
    s.startAttempted.store(true,std::memory_order_release);
    e=dma2d_start(tx);if(e==ESP_OK)e=dma2d_start(rx);
    if(e!=ESP_OK)s.error.store(e,std::memory_order_release);
    s.pickedActive.store(false,std::memory_order_release);return false;
  }
  static void dispose(State*s){
    if(s->semaphore)vSemaphoreDelete(s->semaphore);heap_caps_free(s->tx);heap_caps_free(s->rx);heap_caps_free(s->transaction);s->~State();heap_caps_free(s);
  }
  bool install(int32_t &error){
    error=ESP_OK;void *memory=heap_caps_calloc(1,sizeof(State),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
    if(!memory){error=ESP_ERR_NO_MEM;return false;}auto*s=new(memory) State;s->owner=xTaskGetCurrentTaskHandle();
    s->semaphore=xSemaphoreCreateBinaryStatic(&s->semaphoreStorage);size_t line=0;
    if(esp_cache_get_alignment(MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA,&line)!=ESP_OK||!line||(line&(line-1))||line>128){error=ESP_ERR_NOT_SUPPORTED;dispose(s);return false;}
    const size_t align=line>DMA2D_LL_DESC_ALIGNMENT?line:DMA2D_LL_DESC_ALIGNMENT;s->descBytes=(sizeof(dma2d_descriptor_t)+align-1)&~(align-1);
    s->tx=static_cast<dma2d_descriptor_t*>(heap_caps_aligned_calloc(align,1,s->descBytes,MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA|MALLOC_CAP_8BIT));
    s->rx=static_cast<dma2d_descriptor_t*>(heap_caps_aligned_calloc(align,1,s->descBytes,MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA|MALLOC_CAP_8BIT));
    s->transaction=static_cast<dma2d_trans_t*>(heap_caps_calloc(1,dma2d_get_trans_elm_size(),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
    if(!s->semaphore||!s->tx||!s->rx||!s->transaction){error=ESP_ERR_NO_MEM;dispose(s);return false;}
    dma2d_pool_config_t config{};dma2d_pool_handle_t pool=nullptr;error=dma2d_acquire_pool(&config,&pool);
    if(error!=ESP_OK){dispose(s);return false;}s->pool=pool;s->config.tx_channel_num=1;s->config.rx_channel_num=1;
    s->config.channel_flags=DMA2D_CHANNEL_FUNCTION_FLAG_SIBLING;s->config.on_job_picked=picked;s->config.user_config=s;
    state_=s;stats_.ready=true;return true;
  }
  Status fail(int32_t error){stats_.lastError=error;++stats_.failures;stats_.poisoned=true;return Status::IoError;}
#else
  void *state_=nullptr;
#endif
};
}}} // namespace nice_gfx::platform::rgb565_dma2d



