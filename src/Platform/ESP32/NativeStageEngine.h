#pragma once
#include "../../Display/Transport.h"
#include <atomic>
#include <new>
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_cache.h"
#include "esp_memory_utils.h"
#include "driver/ppa.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_attr.h"
namespace nice_gfx { namespace detail {
struct NativeStageEngine;
// The callback receives an immutable generation value, not a reused mutable
// context slot. Registry and every callback-touched object are internal.
DRAM_ATTR static std::atomic<NativeStageEngine*> nativeStageEngines[4]{};
DRAM_ATTR static std::atomic<uint32_t> nativeStageNextGeneration{1};
struct NativeStageEngine {
 static constexpr size_t StageBytes=98304,GuardWords=32;static constexpr unsigned MaxSignatures=16;
 struct Signature {uint16_t x,y,w,h,pitch,stride,height,width;uint64_t owner;};
 StaticSemaphore_t semaphoreStorage{};SemaphoreHandle_t done=nullptr;ppa_client_handle_t client=nullptr;TaskHandle_t ownerTask=nullptr;
 std::atomic<uint32_t> expected{0},completedGeneration{0};bool pending=false,disabled=false;unsigned registrySlot=4;
 uint16_t*shadow=nullptr,*probe=nullptr;size_t planeWords=0,probeBytes=0,frameWidth=0,frameHeight=0,frameStride=0;Signature signatures[MaxSignatures]{};unsigned signatureCount=0;
 // Binding is per persistent client + this owned stage allocation/domain.
 // It is not a claim that every geometry was empirically sampled. Per-op SDK
 // descriptors, ROI bounds, row fences and dual-MB guard remain mandatory.
 const uint16_t*qualifiedAllocation=nullptr;uint64_t qualifiedOwner=0;
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
 uint16_t*nativelyVerified[3]{};
#else
 uint16_t*nativelyVerified[2]{};
#endif
 NativeStageProviderStats stats{};using Poison=void(*)(void*,Status);
 static bool internal(const void*p,size_t n){auto a=reinterpret_cast<uintptr_t>(p);return p&&n&&a<=UINTPTR_MAX-(n-1)&&esp_ptr_internal(p)&&esp_ptr_internal(reinterpret_cast<void*>(a+n-1));}
 static bool external(const void*p,size_t n){auto a=reinterpret_cast<uintptr_t>(p);return p&&n&&a<=UINTPTR_MAX-(n-1)&&esp_ptr_external_ram(p)&&esp_ptr_external_ram(reinterpret_cast<void*>(a+n-1));}
 static uint32_t generation(){uint32_t n=nativeStageNextGeneration.load(std::memory_order_relaxed);while(n&&n<UINT32_MAX){if(nativeStageNextGeneration.compare_exchange_weak(n,n+1,std::memory_order_relaxed))return n;}return 0;}
 static bool IRAM_ATTR callback(ppa_client_handle_t client,ppa_event_data_t*,void*opaque){
  const uint32_t tag=uint32_t(reinterpret_cast<uintptr_t>(opaque));if(!tag)return false;
  for(auto&slot:nativeStageEngines){auto*s=slot.load(std::memory_order_acquire);if(!s||s->client!=client||s->expected.load(std::memory_order_acquire)!=tag)continue;
   s->completedGeneration.store(tag,std::memory_order_release);auto semaphore=s->done;
   if(xPortInIsrContext()){BaseType_t woken=pdFALSE;xSemaphoreGiveFromISR(semaphore,&woken);return woken==pdTRUE;}xSemaphoreGive(semaphore);return false;
  }return false;
 }
 static bool safeTail(unsigned w,unsigned h){unsigned left=w%64u?w%64u:64u;return left*(h%16u?h%16u:16u)!=96u&&left*(h%32u?h%32u:32u)!=96u;}
 static bool validSource(const NativeStageSource&s,const DirtyRect&r){
  if(!s.allocation||s.allocationBytes!=StageBytes||!s.owner||!s.snapshot||!s.pixels.valid()||s.pixels.order!=PixelOrder::Native||s.pixels.width!=size_t(r.width)||s.pixels.height!=size_t(r.height)||
    s.pixels.stride<size_t(r.width)+2||s.pixels.stride>0x3fff||s.pixels.stride%32||size_t(r.height)+2>(StageBytes/2-2*GuardWords)/s.pixels.stride||
    (reinterpret_cast<uintptr_t>(s.allocation)&63u)||!internal(s.allocation,StageBytes))return false;
  return reinterpret_cast<uintptr_t>(s.pixels.data)==reinterpret_cast<uintptr_t>(s.allocation)+(GuardWords+s.pixels.stride+1)*2;
 }
 static bool geometry(const NativeFramebuffer&f,const DirtyRect&r,const NativeStageSource&s){
  return r.x>=0&&r.y>=0&&r.width>0&&r.height>0&&validSource(s,r)&&f.fencedAtomicMipiRmw&&f.pixels&&f.width&&f.height&&f.width<=f.stride&&f.stride<=0x3fff&&f.height<=0x3fff&&f.height<=SIZE_MAX/f.stride/2&&f.bytes==f.stride*f.height*2&&
   size_t(r.x)<=f.width&&size_t(r.width)<=f.width-size_t(r.x)&&size_t(r.y)<=f.height&&size_t(r.height)<=f.height-size_t(r.y)&&!(reinterpret_cast<uintptr_t>(f.pixels)&63u)&&!(f.stride*2%64u)&&external(f.pixels,f.bytes)&&safeTail(r.width,r.height);
 }
 static Status create(NativeStageEngine*&out,const NativeFramebuffer&frame){
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL
  return createOperation(out,frame,false);
#else
  void*p=heap_caps_aligned_alloc(64,sizeof(NativeStageEngine),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);if(!internal(p,sizeof(NativeStageEngine))){if(p)heap_caps_free(p);return Status::Unsupported;}
  auto*s=new(p) NativeStageEngine();out=s;s->ownerTask=xTaskGetCurrentTaskHandle();s->done=xSemaphoreCreateBinaryStatic(&s->semaphoreStorage);s->frameWidth=frame.width;s->frameHeight=frame.height;s->frameStride=frame.stride;s->planeWords=frame.stride*frame.height;s->probeBytes=(s->planeWords+2*GuardWords)*2;
  s->shadow=static_cast<uint16_t*>(heap_caps_aligned_alloc(64,StageBytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));s->probe=static_cast<uint16_t*>(heap_caps_aligned_alloc(64,s->probeBytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if(!s->done||!internal(&s->semaphoreStorage,sizeof(s->semaphoreStorage))||!internal(s->done,sizeof(StaticSemaphore_t))||!external(s->shadow,StageBytes)||!external(s->probe,s->probeBytes)||
    !internal(nativeStageEngines,sizeof(nativeStageEngines))||!internal(&nativeStageNextGeneration,sizeof(nativeStageNextGeneration))){dispose(out);return Status::Unsupported;}
  for(unsigned i=0;i<4;++i){NativeStageEngine*empty=nullptr;if(nativeStageEngines[i].compare_exchange_strong(empty,s,std::memory_order_release)){s->registrySlot=i;break;}}
  if(s->registrySlot==4){dispose(out);return Status::Unsupported;}
  ppa_client_config_t config{};config.oper_type=PPA_OPERATION_SRM;config.max_pending_trans_num=1;if(ppa_register_client(&config,&s->client)!=ESP_OK){s->client=nullptr;dispose(out);return Status::Unsupported;}
  ppa_event_callbacks_t callbacks{};callbacks.on_trans_done=callback;if(ppa_client_register_event_callbacks(s->client,&callbacks)!=ESP_OK){const auto cleaned=dispose(out);return cleaned==Status::Ok?Status::Unsupported:Status::IoError;}return Status::Ok;
 #endif
 }
 static Status dispose(NativeStageEngine*&s){if(!s)return Status::Ok;
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL
  if(s->preparing)return Status::IoError;
#endif
  while(s->pending){if(xSemaphoreTake(s->done,portMAX_DELAY)==pdTRUE&&s->completedGeneration.load(std::memory_order_acquire)==s->expected.load(std::memory_order_acquire))s->pending=false;}
  if(s->client){if(ppa_unregister_client(s->client)!=ESP_OK)return Status::IoError;s->client=nullptr;}if(s->registrySlot<4)nativeStageEngines[s->registrySlot].store(nullptr,std::memory_order_release);
  if(s->done)vSemaphoreDelete(s->done);if(s->shadow)heap_caps_free(s->shadow);if(s->probe)heap_caps_free(s->probe);s->~NativeStageEngine();heap_caps_free(s);s=nullptr;return Status::Ok;
 }
 void fail(Poison poison,void*context){disabled=true;stats.disabled=true;++stats.errors;poison(context,Status::IoError);}
 uint32_t now(NativeStageClock clock){if(!clock)return 0;++stats.clockReads;return clock();}
 bool sync(void*p,size_t n,int flags,NativeStageClock clock){stats.cacheBytes+=n;uint32_t at=now(clock);bool ok=esp_cache_msync(p,n,flags)==ESP_OK;if(clock)stats.cacheMicros+=uint32_t(now(clock)-at);return ok;}
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL
// Source-less FILL has its own instance/client. The original SRM source and
// tail predicates remain specific to the guarded internal stage.
bool fillOnly=false,preparing=false;
void*fillRows=nullptr;
size_t fillRowBytes=0;
uint32_t fillGeneration=0;
bool fillFailed=false;

static bool fillGeometry(const NativeFramebuffer&frame,const DirtyRect&area){
  return frame.fencedAtomicMipiRmw&&frame.pixels&&frame.width&&frame.height&&
    frame.width<=frame.stride&&frame.stride<0x4000&&frame.height<0x4000&&
    frame.height<=SIZE_MAX/frame.stride/2&&frame.bytes==frame.stride*frame.height*2&&
    !(reinterpret_cast<uintptr_t>(frame.pixels)&63u)&&!(frame.stride*2%64u)&&
    external(frame.pixels,frame.bytes)&&area.x>=0&&area.y>=0&&area.width>0&&area.height>0&&
    size_t(area.x)<=frame.width&&size_t(area.width)<=frame.width-size_t(area.x)&&
    size_t(area.y)<=frame.height&&size_t(area.height)<=frame.height-size_t(area.y);
}

static Status createOperation(NativeStageEngine*&out,const NativeFramebuffer&frame,bool filling){
  void*memory=heap_caps_aligned_alloc(64,sizeof(NativeStageEngine),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
  if(!internal(memory,sizeof(NativeStageEngine))){if(memory)heap_caps_free(memory);return Status::Unsupported;}
  auto*engine=new(memory) NativeStageEngine();out=engine;engine->fillOnly=filling;
  engine->ownerTask=xTaskGetCurrentTaskHandle();
  engine->done=xSemaphoreCreateBinaryStatic(&engine->semaphoreStorage);
  engine->frameWidth=frame.width;engine->frameHeight=frame.height;engine->frameStride=frame.stride;
  engine->planeWords=frame.stride*frame.height;engine->probeBytes=(engine->planeWords+2*GuardWords)*2;
  if(!filling)engine->shadow=static_cast<uint16_t*>(heap_caps_aligned_alloc(64,StageBytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  engine->probe=static_cast<uint16_t*>(heap_caps_aligned_alloc(64,engine->probeBytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if(!engine->done||!internal(&engine->semaphoreStorage,sizeof(engine->semaphoreStorage))||
     !internal(engine->done,sizeof(StaticSemaphore_t))||(!filling&&!external(engine->shadow,StageBytes))||
     !external(engine->probe,engine->probeBytes)||!internal(nativeStageEngines,sizeof(nativeStageEngines))||
     !internal(&nativeStageNextGeneration,sizeof(nativeStageNextGeneration))){dispose(out);return Status::Unsupported;}
  for(unsigned index=0;index<4;++index){
    NativeStageEngine*empty=nullptr;
    if(nativeStageEngines[index].compare_exchange_strong(empty,engine,std::memory_order_release)){engine->registrySlot=index;break;}
  }
  if(engine->registrySlot==4){dispose(out);return Status::Unsupported;}
  ppa_client_config_t config{};config.oper_type=filling?PPA_OPERATION_FILL:PPA_OPERATION_SRM;config.max_pending_trans_num=1;
  if(ppa_register_client(&config,&engine->client)!=ESP_OK){engine->client=nullptr;dispose(out);return Status::Unsupported;}
  ppa_event_callbacks_t callbacks{};callbacks.on_trans_done=callback;
  if(ppa_client_register_event_callbacks(engine->client,&callbacks)!=ESP_OK){
    const auto cleaned=dispose(out);return cleaned==Status::Ok?Status::Unsupported:Status::IoError;
  }
  return Status::Ok;
}

static Status createFill(NativeStageEngine*&out,const NativeFramebuffer&frame){
  return createOperation(out,frame,true);
}

Status drainFill(Poison poison,void*context){
  if(preparing&&!pending){fail(poison,context);return Status::IoError;}
  if(!pending)return disabled?Status::IoError:Status::Ok;
  if(!fillOnly||ownerTask!=xTaskGetCurrentTaskHandle()||!fillGeneration||
     expected.load(std::memory_order_acquire)!=fillGeneration){fail(poison,context);return Status::IoError;}
  bool timedOut=false;TickType_t ticks=pdMS_TO_TICKS(250);if(!ticks)ticks=1;
  for(;;){
    if(xSemaphoreTake(done,ticks)==pdTRUE&&completedGeneration.load(std::memory_order_acquire)==fillGeneration)break;
    if(!timedOut){timedOut=true;++stats.timeouts;fail(poison,context);}
    fillFailed=true;
  }
  pending=false;stats.pending=false;expected.store(0,std::memory_order_release);++stats.callbacks;
  if(fillFailed||disabled)return Status::IoError;
  if(!sync(fillRows,fillRowBytes,ESP_CACHE_MSYNC_FLAG_DIR_M2C,nullptr)){fail(poison,context);return Status::IoError;}
  return Status::Ok;
}

#if NICE_GFX_P4_NATIVE_FILL_DELEGATE_WARM_C2M
bool canDelegateFillC2M(const NativeFramebuffer&frame,const uint16_t*target,
    const void*rows,size_t rowBytes,bool cold) const {
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE && defined(CONFIG_IDF_TARGET_ESP32P4) && ESP_IDF_VERSION == 0x050505 && CONFIG_ESP_REV_MIN_FULL >= 300 && CONFIG_CACHE_L1_CACHE_LINE_SIZE == 64 && CONFIG_CACHE_L2_CACHE_LINE_SIZE == 64
  if(cold||!fillOnly||!signatureCount||target!=frame.pixels||
     frame.width!=1024||frame.height!=600||frame.stride!=1024||frame.bytes!=1228800||
     !(nativelyVerified[0]==target||nativelyVerified[1]==target||nativelyVerified[2]==target))return false;
  const uintptr_t first=reinterpret_cast<uintptr_t>(frame.pixels);
  const uintptr_t start=reinterpret_cast<uintptr_t>(rows);
  if(first<SOC_EXTRAM_LOW||first>=SOC_EXTRAM_HIGH||frame.bytes>size_t(SOC_EXTRAM_HIGH-first)||
     (first&63u)||(start&63u)||!rowBytes||(rowBytes&63u)||start<first||
     start-first>frame.bytes||rowBytes>frame.bytes-size_t(start-first))return false;
  // Pinned SDK FILL writes back and invalidates this same full-row span before DMA.
  // These exact-domain inputs exclude every regular cache invalid-argument exit.
  return external(rows,rowBytes);
#else
  (void)frame;(void)target;(void)rows;(void)rowBytes;(void)cold;
  return false;
#endif
}
#endif

NativeStageTransfer transferFill(const NativeFramebuffer&frame,const DirtyRect&area,uint16_t color,
    uint16_t*target,bool cold,Poison poison,void*context){
  auto*rows=target+size_t(area.y)*frame.stride;
  const size_t rowBytes=size_t(area.height)*frame.stride*2;
  const uint32_t tag=generation();
  if(!tag)return {};
#if NICE_GFX_P4_NATIVE_FILL_DELEGATE_WARM_C2M
  if(!canDelegateFillC2M(frame,target,rows,rowBytes,cold)&&
     !sync(rows,rowBytes,ESP_CACHE_MSYNC_FLAG_DIR_C2M,nullptr)){fail(poison,context);return {Status::IoError,false,false};}
#else
  if(!sync(rows,rowBytes,ESP_CACHE_MSYNC_FLAG_DIR_C2M,nullptr)){fail(poison,context);return {Status::IoError,false,false};}
#endif
  if(disabled)return {Status::IoError,false,false};
  ppa_fill_oper_config_t operation{};
  operation.out.buffer=target;operation.out.buffer_size=frame.bytes;
  operation.out.pic_w=frame.stride;operation.out.pic_h=frame.height;
  operation.out.block_offset_x=area.x;operation.out.block_offset_y=area.y;
  operation.out.fill_cm=PPA_FILL_COLOR_MODE_RGB565;
  operation.fill_block_w=area.width;operation.fill_block_h=area.height;
  operation.fill_argb_color.a=255;operation.fill_argb_color.r=uint8_t((color>>11)<<3);
  operation.fill_argb_color.g=uint8_t(((color>>5)&63)<<2);operation.fill_argb_color.b=uint8_t((color&31)<<3);
  operation.mode=PPA_TRANS_MODE_NON_BLOCKING;operation.user_data=reinterpret_cast<void*>(uintptr_t(tag));
  while(xSemaphoreTake(done,0)==pdTRUE){}
  completedGeneration.store(0,std::memory_order_relaxed);
  fillRows=cold?probe:rows;fillRowBytes=cold?probeBytes:rowBytes;
  fillGeneration=tag;fillFailed=false;
  expected.store(tag,std::memory_order_release);pending=true;stats.pending=true;++stats.invocations;
  // A non-OK SDK return can be an uncertain acceptance. Only the matching
  // callback releases this operation, including the API's queue-failure case.
  if(ppa_do_fill(client,&operation)!=ESP_OK){fillFailed=true;fail(poison,context);}
  const Status status=drainFill(poison,context);
  return {status,true,!pending};
}

NativeStageTransfer runFill(const NativeFramebuffer&frame,const DirtyRect&area,uint16_t color,
    Poison poison,void*context){
  if(disabled)return {Status::IoError,false,false};
  if(!fillOnly||preparing||pending||ownerTask!=xTaskGetCurrentTaskHandle()){
    fail(poison,context);return {Status::IoError,false,false};
  }
  if(xPortInIsrContext()||!xPortCanYield()||xPortGetCoreID()!=1||xTaskGetCoreID(nullptr)!=1||
     !fillGeometry(frame,area)||frame.stride*frame.height!=planeWords||frame.width!=frameWidth||
     frame.height!=frameHeight||frame.stride!=frameStride||area.pixels()<4096)return {};
  preparing=true;
  struct Preparation{NativeStageEngine&engine;~Preparation()noexcept{engine.preparing=false;}} preparation{*this};
  bool nativeVerified=false,targetSlotFree=false;
  for(const auto*target:nativelyVerified){nativeVerified|=target==frame.pixels;targetSlotFree|=target==nullptr;}
  if(!nativeVerified&&!targetSlotFree)return {};
  bool accepted=false;
  if(!signatureCount){
    const uint16_t colors[]={0,0xffff,0xf800,0x07e0,0x001f,0xa55a,0x5aa5,0x0821};
    // The board profile covers independent FILL geometry boundaries in the
    // same guarded allocation. Other frame geometries retain first-ROI scope.
    // Coordinates are x, y, width, height. Constant POD avoids a local-static
    // constructor/guard and adds no engine field or allocation.
    static constexpr int32_t boardAreas[8][4]={
      {1,1,544,35},{31,3,31,133},{32,17,32,128},{31,29,33,125},
      {961,533,63,67},{0,0,64,64},{959,535,65,65},{0,0,1024,600}
    };
    const bool boardProfile=frame.width==1024&&frame.height==600&&frame.stride==1024;
    const unsigned modes=boardProfile?16u:8u;
    for(unsigned mode=0;mode<modes;++mode)for(unsigned trial=0;trial<2;++trial){
      DirtyRect probeArea=area;
      uint16_t color=colors[mode%8u];
      uint32_t seed=0x93a7u+mode*97u+trial*31u;
      if(mode>=8){
        const auto&coordinates=boardAreas[mode-8];
        probeArea=DirtyRect(coordinates[0],coordinates[1],coordinates[2],coordinates[3]);
        color=trial?0x5aa5:0xa55a;
        seed=0xc1e5u+(mode-8)*97u+trial*31u;
      }
      if(!fillGeometry(frame,probeArea)){
        fail(poison,context);return {Status::IoError,accepted,accepted};
      }
      for(size_t index=0;index<probeBytes/2;++index)probe[index]=uint16_t(index*40503u^(index>>5)^seed);
      if(!sync(probe,probeBytes,ESP_CACHE_MSYNC_FLAG_DIR_C2M,nullptr)){
        fail(poison,context);return {Status::IoError,accepted,accepted};
      }
      auto result=transferFill(frame,probeArea,color,probe+GuardWords,true,poison,context);
      accepted|=result.accepted;
      if(result.status!=Status::Ok)return {Status::IoError,accepted,accepted&&!pending};
      ++stats.coldTransfers;
      for(size_t index=0;index<probeBytes/2;++index){
        uint16_t wanted=uint16_t(index*40503u^(index>>5)^seed);
        if(index>=GuardWords&&index<GuardWords+planeWords){
          const size_t pixel=index-GuardWords,y=pixel/frame.stride,x=pixel%frame.stride;
          if(x>=size_t(probeArea.x)&&x<size_t(probeArea.x+probeArea.width)&&y>=size_t(probeArea.y)&&y<size_t(probeArea.y+probeArea.height))wanted=color;
        }
        ++stats.coldWords;
        if(probe[index]!=wanted){fail(poison,context);return {Status::IoError,true,true};}
      }
    }
    signatureCount=1;++stats.qualificationCount;
  }
  auto result=transferFill(frame,area,color,frame.pixels,false,poison,context);
  accepted|=result.accepted;
  if(result.status!=Status::Ok)return {Status::IoError,accepted,accepted&&!pending};
  stats.nativePixels+=area.pixels();
  if(!nativeVerified){
    for(int32_t y=0;y<area.height;++y)for(int32_t x=0;x<area.width;++x){
      ++stats.verifiedNativeWords;
      if(frame.pixels[size_t(area.y+y)*frame.stride+area.x+x]!=color){fail(poison,context);return {Status::IoError,true,true};}
    }
    for(auto&target:nativelyVerified)if(!target){target=frame.pixels;break;}
  }
  return {Status::Ok,true,true};
}
#endif
 NativeStageTransfer transfer(const NativeFramebuffer&frame,const DirtyRect&r,const NativeStageSource&s,uint16_t*target,bool cold,Poison poison,void*context){
  auto*rows=target+size_t(r.y)*frame.stride;const size_t rowBytes=size_t(r.height)*frame.stride*2;const uint32_t tag=generation();
  if(!tag)return {};if(!sync(s.allocation,StageBytes,ESP_CACHE_MSYNC_FLAG_DIR_C2M,s.clock)||!sync(rows,rowBytes,ESP_CACHE_MSYNC_FLAG_DIR_C2M,s.clock)){fail(poison,context);return {Status::IoError,false,false};}
  ppa_srm_oper_config_t op{};op.in.buffer=s.allocation+GuardWords;op.in.pic_w=s.pixels.stride;op.in.pic_h=r.height+2;op.in.block_offset_x=op.in.block_offset_y=1;op.in.block_w=r.width;op.in.block_h=r.height;op.in.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
  op.out.buffer=target;op.out.buffer_size=frame.bytes;op.out.pic_w=frame.stride;op.out.pic_h=frame.height;op.out.block_offset_x=r.x;op.out.block_offset_y=r.y;op.out.srm_cm=PPA_SRM_COLOR_MODE_RGB565;op.rotation_angle=PPA_SRM_ROTATION_ANGLE_0;op.scale_x=op.scale_y=1.0f;op.mode=PPA_TRANS_MODE_NON_BLOCKING;op.user_data=reinterpret_cast<void*>(uintptr_t(tag));
  while(xSemaphoreTake(done,0)==pdTRUE){}completedGeneration.store(0,std::memory_order_relaxed);expected.store(tag,std::memory_order_release);pending=true;stats.pending=true;++stats.invocations;const uint32_t at=now(s.clock);
  bool failed=ppa_do_scale_rotate_mirror(client,&op)!=ESP_OK;if(failed)fail(poison,context);bool timedOut=false;TickType_t ticks=pdMS_TO_TICKS(250);if(!ticks)ticks=1;
  for(;;){if(xSemaphoreTake(done,ticks)==pdTRUE&&completedGeneration.load(std::memory_order_acquire)==tag)break;if(!timedOut){timedOut=true;++stats.timeouts;fail(poison,context);}failed=true;}
  pending=false;stats.pending=false;expected.store(0,std::memory_order_release);++stats.callbacks;if(s.clock)stats.driverMicros+=uint32_t(now(s.clock)-at);
  if(failed||disabled)return {Status::IoError,true,true};if(!sync(cold?probe:rows,cold?probeBytes:rowBytes,ESP_CACHE_MSYNC_FLAG_DIR_M2C,s.clock)||!sync(s.allocation,StageBytes,ESP_CACHE_MSYNC_FLAG_DIR_M2C,s.clock)){fail(poison,context);return {Status::IoError,true,true};}return {Status::Ok,true,true};
 }
 static uint16_t pattern(size_t x,size_t y,unsigned mode){switch(mode){case 0:return uint16_t(((x&1)?0xf800:0x001f)^((y&1)?0x07e0:0));case 1:return uint16_t(y*197u+x*13u);case 2:return uint16_t((x%31u==0||y%17u==0)?0xffff:0x0821);default:return uint16_t((x*109u)^(y*311u)^((x+y)&1?0xa55a:0x5aa5));}}
 NativeStageTransfer run(const NativeFramebuffer&frame,const DirtyRect&r,const NativeStageSource&s,Poison poison,void*context){
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL
  if(fillOnly){fail(poison,context);return {Status::IoError,false,false};}
#endif
  if(disabled)return {Status::IoError,false,false};if(pending||ownerTask!=xTaskGetCurrentTaskHandle()){fail(poison,context);return {Status::IoError,false,false};}if(xPortInIsrContext()||!xPortCanYield()||xPortGetCoreID()!=1||xTaskGetCoreID(nullptr)!=1||!geometry(frame,r,s)||frame.stride*frame.height!=planeWords||frame.width!=frameWidth||frame.height!=frameHeight||frame.stride!=frameStride)return {};
#if NICE_GFX_NATIVE_STAGE_DIAGNOSTIC_VALIDATION
  const Signature signature{uint16_t(r.x),uint16_t(r.y),uint16_t(r.width),uint16_t(r.height),uint16_t(s.pixels.stride),uint16_t(frame.stride),uint16_t(frame.height),uint16_t(frame.width),s.owner};bool verified=false;for(unsigned i=0;i<signatureCount;++i)verified|=!memcmp(&signatures[i],&signature,sizeof(signature));
  if(!verified&&signatureCount==MaxSignatures)return {};const bool nativeVerified=verified;
#else
  if(r.pixels()<4096){++stats.smallStripeFallbacks;return {};}
  if(signatureCount&&(qualifiedAllocation!=s.allocation||qualifiedOwner!=s.owner))return {};
  const bool verified=signatureCount!=0;
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
   bool nativeVerified=false;
   bool targetSlotFree=false;
   for(const auto*target:nativelyVerified){
    nativeVerified|=target==frame.pixels;
    targetSlotFree|=target==nullptr;
   }
   if(!nativeVerified&&!targetSlotFree)return {};
#else
   const bool nativeVerified=nativelyVerified[0]==frame.pixels||nativelyVerified[1]==frame.pixels;
   if(!nativeVerified&&nativelyVerified[0]&&nativelyVerified[1])return {};
#endif
#endif
  bool accepted=false;NativeStageTransfer result{};
  if(!verified){const uint32_t coldAt=now(s.clock);memcpy(shadow,s.allocation,StageBytes);
   for(unsigned mode=0;mode<4;++mode)for(unsigned pad=0;pad<2;++pad)for(unsigned trial=0;trial<2;++trial){
    const uint16_t padding=pad?0xffff:0;for(size_t i=0;i<StageBytes/2;++i)s.allocation[i]=padding;auto*valid=const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(s.pixels.data));for(int y=0;y<r.height;++y)for(int x=0;x<r.width;++x)valid[size_t(y)*s.pixels.stride+x]=pattern(x,y,mode);
    const uint32_t seed=0x72b1u+mode*97+pad*31+trial*19;for(size_t i=0;i<probeBytes/2;++i)probe[i]=uint16_t(i*40503u^(i>>5)^seed);
    if(!sync(probe,probeBytes,ESP_CACHE_MSYNC_FLAG_DIR_C2M,s.clock)){fail(poison,context);result={Status::IoError,accepted,accepted};goto restore;}
    result=transfer(frame,r,s,probe+GuardWords,true,poison,context);accepted|=result.accepted;if(result.status!=Status::Ok){if(accepted&&result.status==Status::Unsupported){fail(poison,context);result.status=Status::IoError;}goto restore;}
    ++stats.coldTransfers;for(size_t i=0;i<probeBytes/2;++i){uint16_t want=uint16_t(i*40503u^(i>>5)^seed);if(i>=GuardWords&&i<GuardWords+planeWords){const size_t p=i-GuardWords,y=p/frame.stride,x=p%frame.stride;if(x>=size_t(r.x)&&x<size_t(r.x+r.width)&&y>=size_t(r.y)&&y<size_t(r.y+r.height))want=pattern(x-r.x,y-r.y,mode);}++stats.coldWords;if(probe[i]!=want){fail(poison,context);result={Status::IoError,true,true};goto restore;}}
    for(size_t i=0;i<StageBytes/2;++i){const size_t row=i>=GuardWords?(i-GuardWords)/s.pixels.stride:SIZE_MAX,col=i>=GuardWords?(i-GuardWords)%s.pixels.stride:SIZE_MAX;const bool active=row>=1&&row<size_t(r.height)+1&&col>=1&&col<size_t(r.width)+1;const uint16_t want=active?pattern(col-1,row-1,mode):padding;++stats.sourceWords;if(s.allocation[i]!=want){fail(poison,context);result={Status::IoError,true,true};goto restore;}}
   }
restore: memcpy(s.allocation,shadow,StageBytes);if(!sync(s.allocation,StageBytes,ESP_CACHE_MSYNC_FLAG_DIR_C2M,s.clock)){fail(poison,context);result={Status::IoError,accepted,accepted};}if(s.clock)stats.coldMicros+=uint32_t(now(s.clock)-coldAt);
   if(result.status!=Status::Ok)return {result.status,accepted,accepted};
  }
  result=transfer(frame,r,s,frame.pixels,false,poison,context);accepted|=result.accepted;if(result.status!=Status::Ok)return {accepted?Status::IoError:result.status,accepted,accepted};
  stats.nativePixels+=r.pixels();const unsigned bucket=r.pixels()<4096?0:r.pixels()<8192?1:r.pixels()<16384?2:r.pixels()<32768?3:r.pixels()<65536?4:5;++stats.areaBuckets[bucket];
  if(!nativeVerified){const auto*input=reinterpret_cast<const uint16_t*>(s.pixels.data);for(int y=0;y<r.height;++y)for(int x=0;x<r.width;++x){++stats.verifiedNativeWords;if(frame.pixels[size_t(r.y+y)*frame.stride+r.x+x]!=input[size_t(y)*s.pixels.stride+x]){fail(poison,context);return {Status::IoError,true,true};}}}
#if NICE_GFX_NATIVE_STAGE_DIAGNOSTIC_VALIDATION
  if(!verified){signatures[signatureCount++]=signature;++stats.qualificationCount;}
#else
  if(!verified){qualifiedAllocation=s.allocation;qualifiedOwner=s.owner;signatureCount=1;++stats.qualificationCount;}
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
   if(!nativeVerified){
    for(auto&target:nativelyVerified){
     if(!target){target=frame.pixels;break;}
    }
   }
#else
   if(!nativeVerified)nativelyVerified[nativelyVerified[0]?1:0]=frame.pixels;
#endif
#endif
  return {Status::Ok,true,true};
 }
};
}}
