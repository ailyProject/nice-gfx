#pragma once
#include "DataBridge.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

namespace nice_gfx { namespace htmlui {

// Optional FreeRTOS adapter. Queue copies complete messages; all rendering and
// callbacks still execute only in the UI task's drain(). Multiple producers are
// supported, and a full queue reports failure instead of overwriting old input.
template<size_t Capacity=8> class FreeRtosDataBridge {
  static_assert(Capacity>0,"FreeRtosDataBridge needs a positive capacity");
 public:
  FreeRtosDataBridge(){queue_=xQueueCreate(Capacity,sizeof(DataMessage));mutex_=xSemaphoreCreateMutex();}
  ~FreeRtosDataBridge(){if(queue_)vQueueDelete(queue_);if(mutex_)vSemaphoreDelete(mutex_);}
  FreeRtosDataBridge(const FreeRtosDataBridge&)=delete;
  FreeRtosDataBridge&operator=(const FreeRtosDataBridge&)=delete;
  bool valid()const{return queue_&&mutex_;}
  bool push(DataMessage message,TickType_t wait=0){message.text[63]=0;return queue_&&xQueueSend(queue_,&message,wait)==pdTRUE;}
  bool pop(DataMessage &message){return queue_&&xQueueReceive(queue_,&message,0)==pdTRUE;}
  size_t drain(View &view,size_t budget=Capacity){DataMessage message;size_t count=0;while(count<budget&&pop(message)){DataBridge<Capacity>::apply(view,message);++count;}return count;}
  bool lock(TickType_t wait=portMAX_DELAY){return mutex_&&xSemaphoreTake(mutex_,wait)==pdTRUE;}
  void unlock(){if(mutex_)xSemaphoreGive(mutex_);}
  // Suspend only an application-owned worker, never the calling/UI task.
  bool suspend(TaskHandle_t worker){if(!worker||worker==xTaskGetCurrentTaskHandle())return false;vTaskSuspend(worker);return true;}
  bool resume(TaskHandle_t worker){if(!worker||worker==xTaskGetCurrentTaskHandle())return false;vTaskResume(worker);return true;}
 private:
  QueueHandle_t queue_=nullptr;SemaphoreHandle_t mutex_=nullptr;
};

} }
