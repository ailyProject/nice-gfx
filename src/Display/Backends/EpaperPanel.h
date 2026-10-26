#pragma once
#include "PackedMonoPanel.h"
#include "../../Drivers/Display/Epaper/SSD16xx.h"

namespace nice_gfx {
struct EpaperIo {
  // Return true while BUSY is asserted. Physical completion is measured,
  // independently of animation durations and renderer DMA ownership.
  bool (*busy)(void *)=nullptr;
  void *context=nullptr;
  Status (*begin)(void *)=nullptr;
  EpaperIo(bool (*read)(void *)=nullptr,void *ctx=nullptr,Status (*init)(void *)=nullptr)
      :busy(read),context(ctx),begin(init){}
};

class EpaperPanel:public PackedMonoPanel {
 public:
  EpaperPanel(Transport &bus,const EpaperProfile &profile,EpaperIo io,int reset=-1)
      :PackedMonoPanel(bus,profile.width,profile.height,MonoPacking::RowMsb,reset),profile_(profile),io_(io){}
  ~EpaperPanel()override{end();}
  Status begin()override{
    if(ready_)return state_==State::Sleeping?wake():record(state_==State::Fault?Status::IoError:Status::Ok);
    if(!io_.busy||!profile_.fullLut||profile_.lutBytes!=159||!profile_.minRefreshMs||
       !profile_.partialMinRefreshMs||profile_.partialMinRefreshMs>0x7FFFFFFFUL||
       profile_.busyTimeoutMs<1000||profile_.busyTimeoutMs>0x7FFFFFFFUL||profile_.minRefreshMs>0x7FFFFFFFUL||
       (profile_.controller!=EpaperController::SSD1680&&profile_.controller!=EpaperController::SSD1681)||
       (profile_.controller==EpaperController::SSD1680?(nativeWidth_>176||nativeHeight_>296):(nativeWidth_>200||nativeHeight_>200))||
       !validReset())return record(Status::InvalidArgument);
    Status s=allocate(0xFF);if(s!=Status::Ok)return s;
    if(io_.begin){s=io_.begin(io_.context);if(s!=Status::Ok)return record(s);}
    s=bus_.begin();if(s!=Status::Ok)return record(s);
    ready_=true;updates_=partialUpdates_=partialsSinceFull_=0;pending_=false;restart();return record(Status::Ok);
  }
  // end/destruction cannot run a cooperative physical wait. Applications that
  // power-gate hardware must poll to idle, sleep(), then poll to sleeping().
  void end()override{if(ready_)bus_.wait();bus_.end();ready_=false;frameOpen_=false;pending_=false;state_=State::Off;}
  uint32_t id()const override{return profile_.controller==EpaperController::SSD1680?0x1680:0x1681;}
  DisplayCapabilities capabilities()const override{
    DisplayCapabilities c;c.colorMode=DisplayColorMode::Monochrome;c.refreshMode=DisplayRefreshMode::Epaper;
    c.bitsPerPixel=1;c.nativeWidth=nativeWidth_;c.nativeHeight=nativeHeight_;c.partialRefresh=canPartial();
    c.minRefreshMs=c.partialRefresh?profile_.partialMinRefreshMs:profile_.minRefreshMs;
    c.recommendedFrameMs=c.partialRefresh?1000:profile_.minRefreshMs;return c;
  }
  bool refreshBusy()const override{return ready_&&state_!=State::Idle&&state_!=State::Fault&&state_!=State::Sleeping;}
  bool pendingRefresh()const{return pending_;}
  bool sleeping()const{return state_==State::Sleeping;}
  uint32_t updateCount()const{return updates_;}
  uint32_t partialUpdateCount()const{return partialUpdates_;}
  uint16_t consecutivePartialUpdates()const{return partialsSinceFull_;}
  bool lastRefreshPartial()const{return lastPartial_;}
  const EpaperProfile &profile()const{return profile_;}
  EpaperRefreshOptions refreshOptions()const{return policy_;}
  Status setRefreshOptions(EpaperRefreshOptions options){
    if(!options.maxPartialUpdates||!options.partialAreaPercent||options.partialAreaPercent>100)return record(Status::InvalidArgument);
    policy_=options;return record(Status::Ok);
  }
  void setPartialRefresh(bool enabled){policy_.partial=enabled;}
  Status requestFullRefresh(){if(!ready_)return record(Status::NotInitialized);forceFull_=true;pending_=true;return record(Status::Ok);}
  Status finish()override{
    if(!ready_)return record(Status::NotInitialized);if(state_==State::Fault)return record(Status::IoError);
    if(!frameOpen_&&dirty_)pending_=true;
    if(state_==State::Sleeping&&pending_)return wake();
    // Accept into private RAM; no SPI, BUSY reads or delay here.
    return record(Status::Ok);
  }
  Status recover(){if(!ready_)return record(Status::NotInitialized);if(frameOpen_||state_==State::SleepWait)return record(Status::Busy);
    bus_.end();const Status s=bus_.begin();if(s!=Status::Ok)return fail(s);
    restart();pending_=true;dirty_=true;return record(Status::Ok);}
  Status wake(){if(!ready_)return record(Status::NotInitialized);if(state_==State::SleepWait)return record(Status::Busy);
    if(state_!=State::Sleeping)return record(Status::Ok);restart();pending_=true;dirty_=true;return record(Status::Ok);}
  Status sleep(){
    if(!ready_)return record(Status::NotInitialized);if(state_==State::Sleeping)return record(Status::Ok);
    if(reset_<0)return record(Status::Unsupported); // Deep sleep exits by hardware RESET.
    if(state_!=State::Idle||frameOpen_||pending_||dirty_)return record(Status::Busy);
    const uint8_t code=1;const Status s=bus_.command(0x10,&code,1);
    if(s!=Status::Ok)return fail(s);enter(State::SleepWait,lastNow_);return record(Status::Ok);
  }
  Status poll(uint32_t now)override{
    if(!ready_)return record(Status::NotInitialized);if(state_==State::Fault)return record(Status::IoError);
    lastNow_=now;if(!clockStarted_){clockStarted_=true;stageAt_=now;lastSample_=now-5;}
    Status s=Status::Ok;
    switch(state_){
      case State::ResetHigh:if(elapsed(now,stageAt_)>=20){bus_.output(reset_,false);enter(State::ResetLow,now);}break;
      case State::ResetLow:if(elapsed(now,stageAt_)>=5){bus_.output(reset_,true);enter(State::ResetRelease,now);}break;
      case State::ResetRelease:if(elapsed(now,stageAt_)>=20)enter(State::BeforeReset,now);break;
      case State::BeforeReset:if(idle(now)){s=bus_.command(0x12,nullptr,0);if(s==Status::Ok)enter(State::AfterReset,now);}break;
      case State::AfterReset:if(idle(now)){s=ssd16xx::configure(bus_,profile_);if(s==Status::Ok)enter(profile_.temperatureLoad?State::Temperature:State::LoadLut,now);}break;
      case State::Temperature:if(idle(now))enter(State::LoadLut,now);break;
      case State::LoadLut:s=ssd16xx::loadLut(bus_,profile_.fullLut);if(s==Status::Ok)enter(State::LutWait,now);break;
      case State::LutWait:if(idle(now)){s=ssd16xx::voltages(bus_,profile_.fullLut);if(s==Status::Ok){waveformPartial_=false;enter(State::Idle,now);}}break;
      case State::Idle:s=dispatch(now);break;
      case State::PartialLow:if(elapsed(now,stageAt_)>=2){bus_.output(reset_,true);enter(State::PartialHigh,now);}break;
      case State::PartialHigh:if(elapsed(now,stageAt_)>=2){s=ssd16xx::loadLut(bus_,profile_.partialLut);if(s==Status::Ok)enter(State::PartialLutWait,now);}break;
      case State::PartialLutWait:if(idle(now)){
        if(profile_.controller==EpaperController::SSD1681)s=ssd16xx::voltages(bus_,profile_.partialLut);
        if(s==Status::Ok)s=ssd16xx::preparePartial(bus_);if(s==Status::Ok){waveformPartial_=true;enter(State::PartialPrepareWait,now);}}break;
      case State::PartialPrepareWait:if(idle(now))enter(State::PartialSend,now);break;
      case State::PartialSend:
        if(frameOpen_)break;
        if(!forceFull_&&!memcmp(frame_,sent_,bytes_)){pending_=dirty_=false;enter(State::Idle,now);break;}
        if(needsFull(changedWindow())){restart();pending_=true;break;}
        s=submit(now,true);break;
      case State::Refreshing:if(idle(now)){sentValid_=true;enter(State::Idle,now);}break;
      case State::SleepWait:if(elapsed(now,stageAt_)>=200){if(reset_>=0&&profile_.controller==EpaperController::SSD1681)bus_.output(reset_,false);
        sentValid_=false;enter(State::Sleeping,now);}break;
      case State::Sleeping:if(pending_)s=wake();break;
      default:break;
    }
    if(s!=Status::Ok)return fail(s);return state_==State::Fault?Status::IoError:Status::Ok;
  }
 private:
  enum class State:uint8_t{Off,ResetHigh,ResetLow,ResetRelease,BeforeReset,AfterReset,Temperature,LoadLut,LutWait,Idle,
    PartialLow,PartialHigh,PartialLutWait,PartialPrepareWait,PartialSend,Refreshing,SleepWait,Sleeping,Fault};
  static uint32_t elapsed(uint32_t now,uint32_t start){return uint32_t(now-start);}
  bool canPartial()const{return policy_.partial&&profile_.partialLut&&reset_>=0;}
  void enter(State state,uint32_t now){state_=state;stageAt_=now;idleStarted_=false;lastSample_=now-5;}
  void restart(){state_=reset_>=0?State::ResetHigh:State::BeforeReset;clockStarted_=false;idleStarted_=false;
    sentValid_=false;forceFull_=true;waveformPartial_=false;if(reset_>=0)bus_.output(reset_,true);}
  bool idle(uint32_t now){
    if(elapsed(now,stageAt_)>=profile_.busyTimeoutMs){fail(Status::IoError);return false;}
    if(elapsed(now,lastSample_)<5)return false;lastSample_=now;
    if(io_.busy(io_.context)){idleStarted_=false;return false;}
    if(!idleStarted_){idleStarted_=true;idleAt_=now;return false;}
    return elapsed(now,idleAt_)>=profile_.idleSettleMs;
  }
  Status fail(Status s){state_=State::Fault;sentValid_=false;pending_=true;return record(s);}
  EpaperWindow changedWindow()const{
    const unsigned stride=nativeWidth_/8;unsigned left=stride,right=0,top=nativeHeight_,bottom=0;
    for(unsigned y=0;y<nativeHeight_;++y)for(unsigned x=0;x<stride;++x){const size_t at=size_t(y)*stride+x;
      if(frame_[at]!=sent_[at]){if(x<left)left=x;if(x>=right)right=x+1;if(y<top)top=y;if(y>=bottom)bottom=y+1;}}
    return right?EpaperWindow(uint16_t(left*8),uint16_t(top),uint16_t((right-left)*8),uint16_t(bottom-top)):EpaperWindow();
  }
  bool needsFull(EpaperWindow r)const{return !sentValid_||forceFull_||!canPartial()||partialsSinceFull_>=policy_.maxPartialUpdates||
    uint64_t(r.width)*r.height*100>uint64_t(nativeWidth_)*nativeHeight_*policy_.partialAreaPercent;}
  Status dispatch(uint32_t now){
    if(!pending_||frameOpen_)return Status::Ok;
    if(sentValid_&&!forceFull_&&!memcmp(frame_,sent_,bytes_)){pending_=dirty_=false;return Status::Ok;}
    const bool full=needsFull(changedWindow());const uint32_t cadence=full?profile_.minRefreshMs:profile_.partialMinRefreshMs;
    if(updates_&&elapsed(now,lastStart_)<cadence)return Status::Ok;
    if(full){if(waveformPartial_){restart();pending_=true;return Status::Ok;}return submit(now,false);}
    // Vendor-qualified partial update requires its reset pulse; externally
    // initialized modules without RESET retain the conservative full path.
    bus_.output(reset_,false);enter(State::PartialLow,now);return Status::Ok;
  }
  Status submit(uint32_t now,bool partial){
    const EpaperWindow r=partial?changedWindow():EpaperWindow(0,0,nativeWidth_,nativeHeight_);
    Status s=partial?ssd16xx::partial(bus_,profile_,frame_,r):ssd16xx::full(bus_,profile_,frame_);
    if(s!=Status::Ok)return s;
    memcpy(sent_,frame_,bytes_);pending_=dirty_=forceFull_=false;lastStart_=now;++updates_;lastPartial_=partial;
    if(partial){++partialUpdates_;++partialsSinceFull_;}else partialsSinceFull_=0;
    enter(State::Refreshing,now);return Status::Ok;
  }
  EpaperProfile profile_;EpaperIo io_;EpaperRefreshOptions policy_;State state_=State::Off;
  bool pending_=false,clockStarted_=false,idleStarted_=false,forceFull_=false,waveformPartial_=false,lastPartial_=false;
  uint32_t stageAt_=0,idleAt_=0,lastSample_=0,lastStart_=0,lastNow_=0,updates_=0,partialUpdates_=0;
  uint16_t partialsSinceFull_=0;
};
} // namespace nice_gfx
