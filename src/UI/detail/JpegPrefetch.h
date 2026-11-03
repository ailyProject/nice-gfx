#pragma once
#include "JpegMedia.h"
#include "JpegPrefetchSlots.h"

#ifndef NICE_GFX_UI_JPEG_PREFETCH
#define NICE_GFX_UI_JPEG_PREFETCH 0
#endif
#if NICE_GFX_UI_JPEG_PREFETCH && NICE_GFX_UI_CAN_BORROW_JPEG_FLASH
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#define NICE_GFX_UI_HAS_JPEG_PREFETCH 1
#else
#define NICE_GFX_UI_HAS_JPEG_PREFETCH 0
#endif

namespace nice_gfx { namespace htmlui { namespace detail {
struct JpegPrefetchStats {
  uint64_t hits=0,misses=0,requests=0,completed=0,failures=0,decodeMicros=0,hardwareCalls=0;
  size_t outputBytes=0;uint32_t activeJobs=0;bool running=false;
};
struct JpegPrefetchKey {
  const uint8_t *bytes=nullptr;size_t count=0;uint16_t width=0,height=0;uint8_t sampling=0;
  JpegPrefetchKey()=default;
  explicit JpegPrefetchKey(const JpegMediaCache::FramebufferPlan &plan):bytes(plan.bytes),count(plan.count),width(plan.width),height(plan.height),sampling(plan.sampling){}
  bool operator==(const JpegPrefetchKey &other)const{return bytes==other.bytes&&count==other.count&&width==other.width&&height==other.height&&sampling==other.sampling;}
  JpegMediaCache::FramebufferPlan plan()const{return {bytes,count,width,height,sampling};}
};
// Optional S31/DROM-only look-ahead. The worker owns a separate decoder and
// four decoded PSRAM buffers. It never borrows a View, Image, LCD framebuffer,
// mutable media descriptor, or the owner's decoder statistics.
class JpegPrefetch {
  using Slots=JpegPrefetchSlots<JpegPrefetchKey,4>;
 public:
  struct Lease {
    Lease()=default;~Lease(){reset();}
    Lease(const Lease &)=delete;Lease &operator=(const Lease &)=delete;
    Lease(Lease &&other)noexcept{move(other);}Lease &operator=(Lease &&other)noexcept{if(this!=&other){reset();move(other);}return *this;}
    const uint16_t *pixels=nullptr;size_t capacityBytes=0,strideBytes=0;uint16_t width=0,height=0;
    void reset(){if(owner_)owner_->release(token_);owner_=nullptr;pixels=nullptr;capacityBytes=strideBytes=0;width=height=0;}
   private:
    friend class JpegPrefetch;JpegPrefetch *owner_=nullptr;Slots::Token token_{};
    void move(Lease &other){owner_=other.owner_;token_=other.token_;pixels=other.pixels;capacityBytes=other.capacityBytes;strideBytes=other.strideBytes;width=other.width;height=other.height;
      other.owner_=nullptr;other.pixels=nullptr;other.capacityBytes=other.strideBytes=0;other.width=other.height=0;}
  };
  explicit JpegPrefetch(size_t budget=3u*1024u*1024u):budget_(budget){}
  ~JpegPrefetch(){stop();}
  JpegPrefetch(const JpegPrefetch &)=delete;JpegPrefetch &operator=(const JpegPrefetch &)=delete;
  bool request(const JpegMediaCache::FramebufferPlan *plans,size_t count){
#if NICE_GFX_UI_HAS_JPEG_PREFETCH
    JpegPrefetchKey keys[4];size_t accepted=0;
    for(size_t i=0;plans&&i<count&&accepted<4;++i){const auto &p=plans[i];
      JpegHeader header;
      if(!p.bytes||!p.count||!p.width||!p.height||(p.width&15)||(p.height&15)||
         size_t(p.width)>SIZE_MAX/2/p.height||size_t(p.width)*p.height*2>budget_||!jpegCanBorrowFlash(p.bytes,p.count)||
         !jpegHeader(p.bytes,p.count,header)||header.width!=p.width||header.height!=p.height||header.sampling!=p.sampling)continue;
      keys[accepted++]=JpegPrefetchKey(p);
    }
    if(!accepted)return false;if(!start())return false;
    xSemaphoreTake(lock_,portMAX_DELAY);slots_.request(keys,accepted);++stats_.requests;xSemaphoreGive(lock_);
    xTaskNotifyGive(worker_);return true;
#else
    (void)plans;(void)count;return false;
#endif
  }
  bool acquire(const JpegMediaCache::FramebufferPlan &plan,Lease &lease){
    lease.reset();
#if NICE_GFX_UI_HAS_JPEG_PREFETCH
    if(!worker_)return false;
    xSemaphoreTake(lock_,portMAX_DELAY);Slots::Token token;
    const bool found=slots_.acquire(JpegPrefetchKey(plan),token);
    if(found){const auto &buffer=buffers_[token.index];lease.owner_=this;lease.token_=token;lease.pixels=buffer.pixels;lease.capacityBytes=buffer.capacity;lease.strideBytes=size_t(plan.width)*2;lease.width=plan.width;lease.height=plan.height;++stats_.hits;}
    else ++stats_.misses;
    xSemaphoreGive(lock_);return found;
#else
    (void)plan;return false;
#endif
  }
  void cancel(){
#if NICE_GFX_UI_HAS_JPEG_PREFETCH
    if(lock_){xSemaphoreTake(lock_,portMAX_DELAY);slots_.cancel();xSemaphoreGive(lock_);}
#endif
  }
  JpegPrefetchStats stats()const{
#if NICE_GFX_UI_HAS_JPEG_PREFETCH
    if(lock_){xSemaphoreTake(lock_,portMAX_DELAY);const auto result=stats_;xSemaphoreGive(lock_);return result;}
#endif
    return JpegPrefetchStats{};
  }
  JpegPrefetchStats snapshot()const{return stats();}
  bool overlaps(uintptr_t begin,size_t bytes)const{
#if NICE_GFX_UI_HAS_JPEG_PREFETCH
    if(!lock_)return false;bool overlap=false;xSemaphoreTake(lock_,portMAX_DELAY);
    for(const auto &buffer:buffers_){if(!buffer.pixels||!buffer.capacity)continue;const uintptr_t at=reinterpret_cast<uintptr_t>(buffer.pixels);
      if(begin>UINTPTR_MAX-bytes||at>UINTPTR_MAX-buffer.capacity||(at<begin+bytes&&begin<at+buffer.capacity)){overlap=true;break;}}
    xSemaphoreGive(lock_);return overlap;
#else
    (void)begin;(void)bytes;return false;
#endif
  }
 private:
  size_t budget_;
  void release(Slots::Token token){
#if NICE_GFX_UI_HAS_JPEG_PREFETCH
    if(lock_){xSemaphoreTake(lock_,portMAX_DELAY);slots_.release(token);xSemaphoreGive(lock_);if(worker_)xTaskNotifyGive(worker_);}
#else
    (void)token;
#endif
  }
  void stop(){
#if NICE_GFX_UI_HAS_JPEG_PREFETCH
    if(worker_){xSemaphoreTake(lock_,portMAX_DELAY);stopping_=true;slots_.cancel();xSemaphoreGive(lock_);xTaskNotifyGive(worker_);
      xSemaphoreTake(stopped_,portMAX_DELAY);worker_=nullptr;}
    for(auto &buffer:buffers_){heap_caps_free(buffer.pixels);buffer=Buffer{};}
    if(stopped_){vSemaphoreDelete(stopped_);stopped_=nullptr;}if(lock_){vSemaphoreDelete(lock_);lock_=nullptr;}
#endif
  }
#if NICE_GFX_UI_HAS_JPEG_PREFETCH
  struct Buffer {uint16_t *pixels=nullptr;size_t capacity=0;};
  Buffer buffers_[4]{};Slots slots_;JpegPrefetchStats stats_;
  mutable SemaphoreHandle_t lock_=nullptr;SemaphoreHandle_t stopped_=nullptr;TaskHandle_t worker_=nullptr;bool stopping_=false;
  // Only the worker calls this decoder. Its destructor runs after joining.
  JpegMediaCache decoder_;
  bool start(){
    if(worker_)return true;if(!lock_)lock_=xSemaphoreCreateMutex();if(!lock_)return false;
    if(!stopped_)stopped_=xSemaphoreCreateBinary();if(!stopped_)return false;
    stopping_=false;
    if(xTaskCreatePinnedToCore(workerEntry,"gfx-jpeg-pref",6144,this,1,&worker_,0)!=pdPASS){worker_=nullptr;return false;}
    xSemaphoreTake(lock_,portMAX_DELAY);stats_.running=true;xSemaphoreGive(lock_);return true;
  }
  static void workerEntry(void *context){static_cast<JpegPrefetch *>(context)->run();}
  void run(){
    for(;;){
      Slots::Work work;xSemaphoreTake(lock_,portMAX_DELAY);
      const bool stopping=stopping_,haveWork=!stopping&&slots_.begin(work);if(haveWork)stats_.activeJobs=1;xSemaphoreGive(lock_);
      if(stopping){xSemaphoreGive(stopped_);vTaskDelete(nullptr);return;}
      if(!haveWork){ulTaskNotifyTake(pdTRUE,portMAX_DELAY);continue;}
      const size_t required=size_t(work.key.width)*work.key.height*2;Buffer old;size_t total=0;
      xSemaphoreTake(lock_,portMAX_DELAY);old=buffers_[work.token.index];total=stats_.outputBytes;xSemaphoreGive(lock_);
      Buffer next=old;bool allocated=true;
      if(next.capacity!=required){
        // The selected slot has no read leases. Drop its old allocation first
        // so the output budget also bounds transient buffer allocation.
        next=Buffer{};
        xSemaphoreTake(lock_,portMAX_DELAY);buffers_[work.token.index]=next;stats_.outputBytes=total-old.capacity;xSemaphoreGive(lock_);
        heap_caps_free(old.pixels);
        if(required>budget_||total-old.capacity>budget_-required)allocated=false;
        else {size_t capacity=0;jpeg_decode_memory_alloc_cfg_t config{};config.buffer_direction=JPEG_DEC_ALLOC_OUTPUT_BUFFER;
          next.pixels=static_cast<uint16_t *>(jpeg_alloc_decoder_mem(required,&config,&capacity));next.capacity=capacity;
          if(!next.pixels||capacity!=required){heap_caps_free(next.pixels);next=Buffer{};allocated=false;}}
        xSemaphoreTake(lock_,portMAX_DELAY);buffers_[work.token.index]=next;stats_.outputBytes=total-old.capacity+next.capacity;xSemaphoreGive(lock_);
      }
      Status status=Status::NoMemory;
      if(allocated){JpegDecodeTarget target{next.pixels,next.capacity,size_t(work.key.width)*2,work.key.width,work.key.height};status=decoder_.decodeFramebuffer(work.key.plan(),target);}
      const auto decodeStats=decoder_.stats();
      xSemaphoreTake(lock_,portMAX_DELAY);slots_.complete(work,status==Status::Ok);
      if(status==Status::Ok)++stats_.completed;else ++stats_.failures;
      stats_.activeJobs=0;stats_.decodeMicros=decodeStats.decodeMicros;stats_.hardwareCalls=decodeStats.hardwareDecodeCalls;xSemaphoreGive(lock_);
    }
  }
#endif
};
} } }
