#pragma once
#include <stddef.h>
#include <stdint.h>

#ifndef NICE_GFX_UI_SYSTEM_MODAL_RLE_WORKER
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_SYSTEM_MODAL_RLE_WORKER 1
#else
#define NICE_GFX_UI_SYSTEM_MODAL_RLE_WORKER 0
#endif
#endif

#if NICE_GFX_UI_SYSTEM_MODAL_RLE_WORKER && (defined(ARDUINO_ARCH_ESP32) || NICE_GFX_NATIVE_IDF) && defined(CONFIG_IDF_TARGET_ESP32S31)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_attr.h"
#include "esp_memory_utils.h"
#include <atomic>
#if !defined(CONFIG_FREERTOS_UNICORE) && configNUMBER_OF_CORES>1 && configSUPPORT_STATIC_ALLOCATION && defined(CONFIG_CACHE_L1_DCACHE_LINE_SIZE) && CONFIG_CACHE_L1_DCACHE_LINE_SIZE==64
#define NICE_GFX_DETAIL_SYSTEM_MODAL_ROW_WORKER 1
#endif
#endif
#ifndef NICE_GFX_DETAIL_SYSTEM_MODAL_ROW_WORKER
#define NICE_GFX_DETAIL_SYSTEM_MODAL_ROW_WORKER 0
#endif

namespace nice_gfx { namespace platform { namespace system_modal_row_worker {
using Function=void(*)(void*)noexcept;

#if NICE_GFX_DETAIL_SYSTEM_MODAL_ROW_WORKER
struct Request {Function function=nullptr;void *context=nullptr;};
struct State {
  std::atomic<bool> busy{false},complete{false};
  TaskHandle_t task=nullptr;QueueHandle_t requests=nullptr;SemaphoreHandle_t done=nullptr;
  StaticTask_t taskControl{};StaticQueue_t queueControl{};StaticSemaphore_t semaphoreControl{};
  alignas(16) StackType_t stack[4096]{};
  alignas(16) uint8_t queueBytes[sizeof(Request)]{};
};
// One internal arena serves one submitted job at a time. Its lifetime is the
// whole boot. The task is never deleted and idle cleanup cannot outlive it.
inline State &sharedState(){static DRAM_ATTR State state{};return state;}
inline void taskEntry(void *context){
  auto &state=*static_cast<State*>(context);
  for(;;){
    Request request;
    if(xQueueReceive(state.requests,&request,portMAX_DELAY)!=pdTRUE)continue;
    request.function(request.context);
    state.complete.store(true,std::memory_order_release);
    xSemaphoreGive(state.done);
  }
}
inline bool ensureStarted(State &state){
  if(state.task)return true;
  const auto *end=reinterpret_cast<const uint8_t*>(&state)+sizeof(state)-1;
  if(!esp_ptr_internal(&state)||!esp_ptr_internal(end))return false;
  if(!state.requests)state.requests=xQueueCreateStatic(1,sizeof(Request),state.queueBytes,&state.queueControl);
  if(!state.done)state.done=xSemaphoreCreateBinaryStatic(&state.semaphoreControl);
  if(!state.requests||!state.done)return false;
  state.task=xTaskCreateStaticPinnedToCore(taskEntry,"gfx-modal-rle",sizeof(state.stack),&state,
    tskIDLE_PRIORITY+1,state.stack,&state.taskControl,0);
  return state.task!=nullptr;
}
#endif

class Ticket {
 public:
  Ticket()=default;
  Ticket(const Ticket&)=delete;Ticket&operator=(const Ticket&)=delete;
  Ticket(Ticket &&other)noexcept{
#if NICE_GFX_DETAIL_SYSTEM_MODAL_ROW_WORKER
    state_=other.state_;other.state_=nullptr;
#else
    (void)other;
#endif
  }
  ~Ticket(){join();}
  explicit operator bool()const{
#if NICE_GFX_DETAIL_SYSTEM_MODAL_ROW_WORKER
    return state_!=nullptr;
#else
    return false;
#endif
  }
  void join()noexcept{
#if NICE_GFX_DETAIL_SYSTEM_MODAL_ROW_WORKER
    if(!state_)return;
    // A submitted job must drain before its stack descriptors, sources or
    // target can be released. A timeout must never trigger concurrent repaint.
    while(!state_->complete.load(std::memory_order_acquire))xSemaphoreTake(state_->done,portMAX_DELAY);
    state_->busy.store(false,std::memory_order_release);state_=nullptr;
#endif
  }
 private:
#if NICE_GFX_DETAIL_SYSTEM_MODAL_ROW_WORKER
  State *state_=nullptr;
  explicit Ticket(State &state):state_(&state){}
#endif
  friend Ticket tryBegin(Function,void*);
};

inline Ticket tryBegin(Function function,void *context){
#if NICE_GFX_DETAIL_SYSTEM_MODAL_ROW_WORKER
  if(!function||xPortGetCoreID()!=1||!xPortCanYield())return {};
  auto &state=sharedState();bool expected=false;
  if(!state.busy.compare_exchange_strong(expected,true,std::memory_order_acquire))return {};
  if(!ensureStarted(state)){state.busy.store(false,std::memory_order_release);return {};}
  while(xSemaphoreTake(state.done,0)==pdTRUE){}
  state.complete.store(false,std::memory_order_relaxed);
  const Request request{function,context};
  if(xQueueSend(state.requests,&request,0)!=pdTRUE){state.busy.store(false,std::memory_order_release);return {};}
  return Ticket(state);
#else
  (void)function;(void)context;return {};
#endif
}
}}}
