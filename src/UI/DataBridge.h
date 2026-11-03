#pragma once
#include "View.h"
#include <atomic>
#include <algorithm>
#include <cmath>
#include <math.h>
#include <cstring>

namespace nice_gfx { namespace htmlui {

enum class DataCommand : uint8_t { Value, Text, Sample, Key, Start, Pause, Resume, Commit, Cancel };
struct DataMessage {
  DataCommand command=DataCommand::Value;
  Id target=0;
  int32_t value=0;
  uint8_t series=0;
  char text[64]{};
};
// Bounded SPSC handoff: one producer owns push(), the UI task owns drain().
// Use FreeRtosDataBridge for multiple producers. No View call runs on a worker.
template<size_t Capacity=8> class DataBridge {
  static_assert(Capacity>0,"DataBridge needs a positive capacity");
 public:
  bool push(const DataMessage &message) {
    const size_t head=head_.load(std::memory_order_relaxed),next=(head+1)%(Capacity+1);
    if(next==tail_.load(std::memory_order_acquire))return false;
    slots_[head]=message;slots_[head].text[63]=0;head_.store(next,std::memory_order_release);return true;
  }
  bool pop(DataMessage &message) {
    const size_t tail=tail_.load(std::memory_order_relaxed);
    if(tail==head_.load(std::memory_order_acquire))return false;
    message=slots_[tail];tail_.store((tail+1)%(Capacity+1),std::memory_order_release);return true;
  }
  size_t drain(View &view,size_t budget=Capacity) {DataMessage message;size_t count=0;while(count<budget&&pop(message)){apply(view,message);++count;}return count;}
  static bool apply(View &view,const DataMessage &message) {
    switch(message.command){
      case DataCommand::Value:return view.setValue(message.target,message.value);
      case DataCommand::Text:return view.setText(message.target,message.text);
      case DataCommand::Sample:return view.appendChartValue(message.target,message.series,message.value);
      case DataCommand::Key:return message.value>=int32_t(InputAction::None)&&message.value<=int32_t(InputAction::Callback)&&view.inputAction(InputAction(message.value),message.target);
      case DataCommand::Start:return view.startBehavior(message.target);
      case DataCommand::Pause:return view.pauseBehavior(message.target,true);
      case DataCommand::Resume:return view.pauseBehavior(message.target,false);
      case DataCommand::Commit:return view.finishBehavior(message.target,true);
      case DataCommand::Cancel:return view.finishBehavior(message.target,false);
    }
    return false;
  }
  static bool scaledFloat(float value,uint8_t decimals,int32_t &result) {
    if(!std::isfinite(value)||decimals>6)return false;
    double scale=1;for(uint8_t i=0;i<decimals;++i)scale*=10;
    // Some embedded libstdc++ builds expose C99 round only in the global namespace.
    const double rounded=::round(double(value)*scale);
    if(rounded<-2147483648.0||rounded>2147483647.0)return false;
    result=int32_t(rounded);return true;
  }
 private:
  DataMessage slots_[Capacity+1]{};
  std::atomic<size_t> head_{0},tail_{0};
};

enum class DataAccess : uint8_t { ReadOnly, ReadWrite };
enum class DataNotify : uint8_t { Step, Commit };
using DataChanged = void (*)(void *,int32_t);
// Explicit transaction adapter for int/scaled-float/enum/switch values.
// UI events choose adjust/commit/cancel; application-owned storage stays typed.
class NumericBinding {
 public:
  NumericBinding(int32_t &storage,int32_t minimum,int32_t maximum,int32_t step=1,
      DataAccess access=DataAccess::ReadWrite,DataNotify notify=DataNotify::Step,
      DataChanged callback=nullptr,void *context=nullptr)
    :storage_(storage),original_(storage),minimum_(minimum),maximum_(maximum),step_(step>0?step:1),access_(access),notify_(notify),callback_(callback),context_(context){}
  bool begin(){if(access_==DataAccess::ReadOnly||minimum_>maximum_)return false;original_=storage_;editing_=true;return true;}
  bool adjust(int direction){if(!editing_)return false;const int64_t next=int64_t(storage_)+int64_t(direction)*step_;const int32_t value=int32_t(std::max<int64_t>(minimum_,std::min<int64_t>(maximum_,next)));if(value==storage_)return true;storage_=value;if(notify_==DataNotify::Step&&callback_)callback_(context_,value);return true;}
  bool commit(){if(!editing_)return false;editing_=false;if(notify_==DataNotify::Commit&&storage_!=original_&&callback_)callback_(context_,storage_);return true;}
  bool cancel(){if(!editing_)return false;editing_=false;const bool changed=storage_!=original_;storage_=original_;if(changed&&notify_==DataNotify::Step&&callback_)callback_(context_,storage_);return true;}
  int32_t value()const{return storage_;}
 private:
  int32_t &storage_;int32_t original_,minimum_,maximum_,step_;DataAccess access_;DataNotify notify_;DataChanged callback_;void *context_;bool editing_=false;
};

} }
