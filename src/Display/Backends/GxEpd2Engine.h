#pragma once
#include "GxEpd2Hal.h"
#include "../../Core/Platform.h"
#include <stdlib.h>
#if defined(ESP32)||defined(ARDUINO_ARCH_ESP32)||defined(ESP_PLATFORM)
#define NGFX_GXEPD2_WORKER_PLATFORM 1
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#elif !defined(ARDUINO)&&!defined(__AVR__)
#define NGFX_GXEPD2_WORKER_PLATFORM 2
#include <atomic>
#include <thread>
#else
#define NGFX_GXEPD2_WORKER_PLATFORM 0
#endif
namespace nice_gfx { namespace detail {
template<class T>class GxEpd2Atomic{
 public:explicit GxEpd2Atomic(T value):value_(value){}
  T load()const{
#if NGFX_GXEPD2_WORKER_PLATFORM
    return value_.load(std::memory_order_acquire);
#else
    return value_;
#endif
  }
  void store(T value){
#if NGFX_GXEPD2_WORKER_PLATFORM
    value_.store(value,std::memory_order_release);
#else
    value_=value;
#endif
  }
 private:
#if NGFX_GXEPD2_WORKER_PLATFORM
  std::atomic<T> value_;
#else
  T value_;
#endif
};
inline void *gxEpd2Alloc(size_t bytes,bool psram){
  if(psram&&platform::psramFound()){void*p=platform::callocPsram(bytes);if(p)return p;}
  return malloc(bytes);
}
} // namespace detail

// Owned bytecode for read-free drivers; the same HAL executes read-dependent
// drivers on a worker. No guessed OTP, temperature, device-info or VCOM reads.
class GxEpd2Engine:public GxEpd2Hal{
 public:
  GxEpd2Engine(const GxEpd2Config &c,GxEpd2Io &io):config_(c),io_(io),failure_(Status::Ok),cancel_(false){}
  ~GxEpd2Engine(){free(tape_);}
  Status status()const{return failure_.load();}
  void reset(){failure_.store(Status::Ok);cancel_.store(false);clearTape();directSize_=0;recordSize_=0;runCount_=0;}
  void cancel(){cancel_.store(true);}
  bool cancelled()const{return cancel_.load();}
  void direct(bool enabled,bool busyLevel=false){direct_=enabled;busyLevel_=busyLevel;directSize_=0;}
  void beginRecording(){direct_=false;clearTape();recordSize_=0;runCount_=0;settingsValid_=false;recordWork_=0;recordYieldAt_=io_.nowMicros();}
  void finishRecording(){flushRecorded();}
  void finishDirect(){flushDirect();}
  bool playing()const{return position_<size_||delayActive_;}
  size_t tapeBytes()const{return size_;}
  size_t tapeCapacity()const{return capacity_;}
  void releaseTape(){clearTape();free(tape_);tape_=nullptr;capacity_=0;}
  void fault(Status s)override{if(s!=Status::Ok&&status()==Status::Ok)failure_.store(s==Status::Busy?Status::IoError:s);}
  int clockPin()const override{return config_.pins.clock;}
  int mosiPin()const override{return config_.pins.mosi;}
  int misoPin()const override{return config_.pins.miso;}
  int extraCsPin(unsigned i)const override{return i<3?config_.pins.extraCs[i]:-1;}
  void pinMode(int pin,uint8_t mode)override{
    if(direct_){flushDirect();if(usable())accept(io_.pinMode(pin,mode));}
    else{Header h={};h.op=Mode;h.pin=pin;h.arg=mode;emit(h);}
  }
  void writePin(int pin,bool high)override{
    if(pin<0)return;
    if(direct_){flushDirect();if(usable())accept(io_.writePin(pin,high));}
    else{Header h={};h.op=Pin;h.pin=pin;h.arg=high?1:0;emit(h);}
  }
  int readPin(int pin)override{
    if(!direct_){fault(Status::Unsupported);return 0;}
    flushDirect();if(!usable())return isBusyPin(pin)?!busyLevel_:0;
    bool value=false;accept(io_.readPin(pin,value));return value?1:0;
  }
  void delayMs(uint32_t ms)override{
    if(direct_){flushDirect();while(ms&&usable()){const uint32_t n=ms>5?5:ms;io_.delayMs(n);ms-=n;}}
    else if(ms){Header h={};h.op=Delay;h.a=ms;emit(h);}
  }
  void delayUs(uint32_t us)override{
    if(direct_){flushDirect();if(usable())io_.delayUs(us);}
    else if(us){Header h={};h.op=MicroDelay;h.a=us;emit(h);}
  }
  uint32_t micros()override{return io_.nowMicros();}
  void yield()override{if(direct_&&usable())io_.yieldThread();}
  void spiEnable(bool enabled)override{
    if(direct_){flushDirect();if(usable())accept(io_.spiEnable(enabled));}
    else{Header h={};h.op=Enable;h.arg=enabled?1:0;emit(h);settingsValid_=false;}
  }
  void spiSettings(uint32_t hz,uint8_t mode)override{
    if(config_.busClockHz&&hz>config_.busClockHz)hz=config_.busClockHz;
    if(direct_){flushDirect();if(usable())accept(io_.spiSettings(hz,mode));}
    else if(!settingsValid_||hz!=recordHz_||mode!=recordMode_){Header h={};h.op=Settings;h.a=hz;h.arg=mode;emit(h);
      recordHz_=hz;recordMode_=mode;settingsValid_=true;}
  }
  void spiWrite(uint8_t value)override{
    if(!usable())return;
    if(direct_){directData_[directSize_++]=value;if(directSize_==sizeof(directData_))flushDirect();}
    else if(runCount_&&value==runValue_){if(runCount_==UINT32_MAX)flushRun();++runCount_;}
    else{flushRun();runValue_=value;runCount_=1;}
    if(!direct_)cooperateRecording();
  }
  uint8_t spiTransfer(uint8_t value)override{
    if(!direct_){fault(Status::Unsupported);return 0;}flushDirect();uint8_t result=0;
    if(usable())accept(io_.transfer(&value,&result,1));return result;
  }
  void spiFlush()override{if(direct_)flushDirect();else flushRecorded();}
  void spiByteTransaction(int cs,uint8_t mask,uint8_t value)override{
    if(direct_){GxEpd2Hal::spiByteTransaction(cs,mask,value);return;}
    flushRecorded();if(!usable())return;cooperateRecording();
    if(lastTxnValid_){Header previous;memcpy(&previous,tape_+lastTxn_,sizeof(previous));
      if(previous.op==ByteTransactions&&previous.pin==cs&&previous.arg==mask&&previous.bytes<256&&
        lastTxn_+sizeof(previous)+previous.bytes==size_){
        if(!reserve(1))return;tape_[size_++]=value;++previous.bytes;memcpy(tape_+lastTxn_,&previous,sizeof(previous));return;
      }}
    Header h={};h.op=ByteTransactions;h.pin=cs;h.arg=mask;h.bytes=1;const size_t at=size_;rawEmit(h,&value);
    if(usable()){lastTxn_=at;lastTxnValid_=true;}
  }
  void selectCs(uint8_t mask,bool active)override{
    writePin(config_.pins.cs,active&&(mask&1)?false:true);
    for(unsigned i=0;i<3;++i)writePin(config_.pins.extraCs[i],active&&(mask&(2U<<i))?false:true);
  }
  void waitBusy(int pin,bool level,uint32_t timeout,uint32_t fallback)override{
    if(direct_){flushDirect();if(pin<0){delayMs(fallback);return;}
      delayMs(1);const uint32_t start=micros();
      while(usable()&&readPin(pin)==int(level)){if(uint32_t(micros()-start)>=timeout){fault(Status::IoError);break;}delayMs(1);yield();}
    }else{delayMs(1);Header h={};h.op=Wait;h.pin=pin;h.arg=level?1:0;h.a=timeout;h.b=fallback;emit(h);}
  }
  void waitBusyCommand(int pin,bool level,uint32_t timeout,uint32_t fallback,uint8_t command,int cs,int dc,uint16_t interval)override{
    PollCommand spec={int16_t(cs),int16_t(dc),interval,command,0};
    if(direct_){const uint32_t start=micros();do{writePin(dc,false);writePin(cs,false);spiWrite(command);writePin(cs,true);writePin(dc,true);
      delayMs(interval);if(pin<0){delayMs(fallback);return;}if(readPin(pin)!=int(level))return;
      if(uint32_t(micros()-start)>=timeout){fault(Status::IoError);return;}}while(usable());
    }else{Header h={};h.op=WaitCommand;h.pin=pin;h.arg=level?1:0;h.a=timeout;h.b=fallback;
      h.bytes=sizeof(spec);emit(h,&spec);}
  }
  Status poll(uint32_t nowMs){
    if(status()!=Status::Ok)return status();const uint32_t start=io_.nowMicros();size_t bytes=0;
    if(delayActive_){if(uint32_t(nowMs-delayAt_)<delayMs_)return Status::Ok;delayActive_=false;}
    while(position_<size_){Header h;memcpy(&h,tape_+position_,sizeof(h));const uint8_t *payload=tape_+position_+sizeof(h);
      Status result=Status::Ok;
      switch(h.op){
        case Mode:result=io_.pinMode(h.pin,h.arg);break;
        case Pin:result=io_.writePin(h.pin,h.arg!=0);break;
        case Enable:result=io_.spiEnable(h.arg!=0);break;
        case Settings:result=io_.spiSettings(h.a,h.arg);break;
        case Delay:case MicroDelay:{const uint32_t ms=h.op==Delay?h.a:(h.a+999)/1000;
          if(h.op==MicroDelay&&h.a<1000)io_.delayUs(h.a);
          else if(ms){delayActive_=true;delayAt_=nowMs;delayMs_=ms+1+uint32_t(io_.nowMicros()-start)/1000;}
          break;}
        case Wait:case WaitCommand:{
          if(!waitActive_){waitActive_=true;waitAt_=nowMs;commandPending_=false;}
          if(h.pin<0){delayActive_=true;delayAt_=nowMs;delayMs_=h.b+1;waitActive_=false;break;}
          if(uint64_t(uint32_t(nowMs-waitAt_))*1000>=h.a){fault(Status::IoError);return status();}
          if(h.op==WaitCommand){PollCommand spec;memcpy(&spec,payload,sizeof(spec));
            if(!commandPending_){result=pollCommand(spec);if(result!=Status::Ok)break;
              commandPending_=true;commandAt_=nowMs;return Status::Ok;}
            if(uint32_t(nowMs-commandAt_)<uint32_t(spec.interval)+1)return Status::Ok;
          }
          {bool active=false;result=io_.readPin(h.pin,active);if(result!=Status::Ok)break;
            if(active==(h.arg!=0)){commandPending_=false;return Status::Ok;}}
          waitActive_=false;commandPending_=false;break;}
        case ByteTransactions:{
          if(bytes>=config_.maxDataBytesPerPoll)return Status::Ok;
          while(dataOffset_<h.bytes){
            if(transactionStage_==0){result=replayCs(h.pin,h.arg,true);if(result!=Status::Ok)break;transactionStage_=1;}
            if(transactionStage_==1){result=io_.transfer(payload+dataOffset_,nullptr,1);if(result!=Status::Ok)break;transactionStage_=2;++bytes;}
            result=replayCs(h.pin,h.arg,false);if(result!=Status::Ok)break;transactionStage_=0;++dataOffset_;
            if(dataOffset_<h.bytes&&(bytes>=config_.maxDataBytesPerPoll||uint32_t(io_.nowMicros()-start)>=config_.pollBudgetMicros))return Status::Ok;
          }
          if(result!=Status::Ok)break;dataOffset_=0;break;}
        case Data:case Repeat:{
          if(bytes>=config_.maxDataBytesPerPoll)return Status::Ok;
          size_t remain=(h.op==Data?h.bytes:h.a)-dataOffset_,n=config_.maxDataBytesPerPoll-bytes;
          if(n>remain)n=remain;if(n>sizeof(repeatData_))n=sizeof(repeatData_);
          const uint8_t *source=repeatData_;
          if(h.op==Repeat)memset(repeatData_,h.arg,n);else source=payload+dataOffset_;
          result=io_.transfer(source,nullptr,n);if(result!=Status::Ok)break;
          bytes+=n;dataOffset_+=n;if(dataOffset_<(h.op==Data?h.bytes:h.a)){
            if(bytes>=config_.maxDataBytesPerPoll||uint32_t(io_.nowMicros()-start)>=config_.pollBudgetMicros)return Status::Ok;
            continue;}
          dataOffset_=0;break;}
        default:result=Status::IoError;break;
      }
      if(result==Status::Busy)return Status::Ok;
      if(result!=Status::Ok){fault(result);return status();}
      position_+=sizeof(h)+h.bytes;
      if(delayActive_||bytes>=config_.maxDataBytesPerPoll||uint32_t(io_.nowMicros()-start)>=config_.pollBudgetMicros)return Status::Ok;
    }return Status::Ok;
  }
 private:
  enum:uint8_t{Mode,Pin,Delay,MicroDelay,Enable,Settings,Wait,Data,Repeat,WaitCommand,ByteTransactions};
  struct Header{uint8_t op,arg;uint16_t bytes;int32_t pin;uint32_t a,b;};
  struct PollCommand{int16_t cs,dc;uint16_t interval;uint8_t command,reserved;};
  bool usable()const{return status()==Status::Ok&&!cancelled();}
  void cooperateRecording(){
#if NGFX_GXEPD2_WORKER_PLATFORM==1
    if(++recordWork_>=512){recordWork_=0;if(uint32_t(io_.nowMicros()-recordYieldAt_)>=config_.pollBudgetMicros){
      io_.yieldThread();recordYieldAt_=io_.nowMicros();}}
#endif
  }
  bool isBusyPin(int pin)const{if(pin==config_.pins.busy)return true;for(int p:config_.pins.extraBusy)if(p>=0&&p==pin)return true;return false;}
  void accept(Status s){if(s!=Status::Ok)fault(s);}
  void clearTape(){size_=position_=dataOffset_=0;waitActive_=delayActive_=commandPending_=lastTxnValid_=false;transactionStage_=0;}
  bool reserve(size_t n){if(!usable())return false;
    if(n>config_.maxTapeBytes||size_>config_.maxTapeBytes-n){fault(Status::NoMemory);return false;}
    if(size_+n<=capacity_)return true;size_t next=capacity_?capacity_:256;
    while(next<size_+n){if(next>config_.maxTapeBytes/2){next=config_.maxTapeBytes;break;}next*=2;}
    if(next>config_.maxTapeBytes)next=config_.maxTapeBytes;
    uint8_t *memory=static_cast<uint8_t*>(detail::gxEpd2Alloc(next,config_.preferPsram));
    if(!memory){fault(Status::NoMemory);return false;}if(size_)memcpy(memory,tape_,size_);free(tape_);tape_=memory;capacity_=next;return true;
  }
  void rawEmit(const Header &h,const void*data=nullptr){lastTxnValid_=false;if(!reserve(sizeof(h)+h.bytes))return;
    memcpy(tape_+size_,&h,sizeof(h));size_+=sizeof(h);if(h.bytes){memcpy(tape_+size_,data,h.bytes);size_+=h.bytes;}}
  void emit(const Header &h,const void*data=nullptr){flushRecorded();rawEmit(h,data);}
  void appendLiteral(uint8_t value){recordData_[recordSize_++]=value;if(recordSize_==sizeof(recordData_))flushLiteral();}
  void flushLiteral(){if(!recordSize_)return;Header h={};h.op=Data;h.bytes=uint16_t(recordSize_);rawEmit(h,recordData_);recordSize_=0;}
  void flushRun(){if(!runCount_)return;
    if(runCount_>=8){flushLiteral();Header h={};h.op=Repeat;h.arg=runValue_;h.a=runCount_;rawEmit(h);}
    else for(uint32_t i=0;i<runCount_;++i)appendLiteral(runValue_);runCount_=0;}
  void flushRecorded(){flushRun();flushLiteral();}
  void flushDirect(){if(directSize_){if(usable())accept(io_.transfer(directData_,nullptr,directSize_));directSize_=0;}}
  Status pollCommand(const PollCommand &s){Status result=io_.writePin(s.dc,false);if(result!=Status::Ok)return result;
    result=io_.writePin(s.cs,false);if(result!=Status::Ok)return result;
    result=io_.transfer(&s.command,nullptr,1);const Status deselected=io_.writePin(s.cs,true);const Status dc=io_.writePin(s.dc,true);
    return result!=Status::Ok?result:deselected!=Status::Ok?deselected:dc;}
  Status replayCs(int cs,uint8_t mask,bool active){
    if(mask==1)return io_.writePin(cs,!active);
    Status s=io_.writePin(config_.pins.cs,!(active&&(mask&1)));if(s!=Status::Ok)return s;
    for(unsigned i=0;i<3;++i)if(config_.pins.extraCs[i]>=0){s=io_.writePin(config_.pins.extraCs[i],!(active&&(mask&(2U<<i))));if(s!=Status::Ok)return s;}
    return Status::Ok;
  }
  const GxEpd2Config &config_;GxEpd2Io &io_;
  detail::GxEpd2Atomic<Status> failure_;detail::GxEpd2Atomic<bool> cancel_;
  uint8_t *tape_=nullptr;size_t size_=0,capacity_=0,position_=0,dataOffset_=0;
  uint8_t recordData_[256],directData_[256],repeatData_[256];
  size_t recordSize_=0,directSize_=0;uint8_t runValue_=0;uint32_t runCount_=0;
  bool direct_=false,busyLevel_=false,settingsValid_=false,waitActive_=false,delayActive_=false,commandPending_=false;
  uint32_t recordHz_=0,waitAt_=0,delayAt_=0,delayMs_=0,commandAt_=0;uint8_t recordMode_=0;
  size_t lastTxn_=0;bool lastTxnValid_=false;uint8_t transactionStage_=0;
  uint16_t recordWork_=0;uint32_t recordYieldAt_=0;
};
} // namespace nice_gfx
