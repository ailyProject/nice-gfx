#pragma once
#include "../Backend.h"
#include "../../Drivers/Display/Epaper/NativeTypes.h"
#include "../../Drivers/Display/Epaper/IT8951.h"
#include "../../Graphics/MonoConvert.h"
#include "../../Platform/Common/UiMemory.h"
#include <string.h>

namespace nice_gfx {

// Packed target + immutable transfer snapshot. No RGB565 framebuffer, command
// recorder, worker task or full-size wire-conversion allocation. The drawing
// thread may prepare the next image during SPI upload and optical refresh.
class NativeEpaperPanel final : public Backend {
 public:
  NativeEpaperPanel(const NativeEpaperProfile &profile,const NativeEpaperConfig &config,NativeEpaperIo &io)
      :profile_(profile),config_(config),io_(io){}
  ~NativeEpaperPanel() override {end();release(frame_);release(snapshot_);release(backup_);}
  Status begin() override {
    if(ready_)return record(state_==State::Fault?Status::IoError:Status::Ok);
    if(!profile_.documented||(!isIt()&&!profile_.protocol)||config_.vcomMillivolts!=-1)return record(Status::Unsupported);
    const auto &p=protocol();
    if(!profile_.width||!profile_.height||profile_.width>profile_.storageWidth||
       profile_.width>INT16_MAX||profile_.height>INT16_MAX||profile_.storageWidth>INT16_MAX||
       uint8_t(profile_.encoding)>uint8_t(NativeEpaperEncoding::Gray16)||(profile_.storageWidth&7)||
       !profile_.chipCount||profile_.chipCount>2||
       profile_.rowBytes()%profile_.chipCount||
       profile_.height>SIZE_MAX/profile_.rowBytes()||
       (!isIt()&&(!p.initSteps||p.initSteps>32||!p.planes||p.planes>8||!p.initialize||!p.startPlane||!p.refresh||
          !p.encodedBytesPerByte||p.encodedBytesPerByte>8||config_.maxDataBytesPerPoll<p.encodedBytesPerByte||
          (p.encodedBytesPerByte!=1&&!p.encode)))||
       (isIt()&&(profile_.encoding!=NativeEpaperEncoding::Gray16||profile_.chipCount!=1||
          (profile_.width&3)||config_.maxDataBytesPerPoll<2))||!profile_.minRefreshMs||profile_.minRefreshMs>0x7fffffffUL||
       profile_.busyTimeoutMs<100||profile_.busyTimeoutMs>0x7fffffffUL||
       !profile_.chipCount||profile_.chipCount>2||!config_.maxDataBytesPerPoll||
       config_.resetDurationMs<2||config_.resetDurationMs>100||config_.powerOnDelayMs>10000||config_.pollBudgetMicros>0x7fffffffUL)
      return record(Status::InvalidArgument);
    const size_t n=profile_.framebufferBytes();
    if(!frame_)frame_=allocate(n);
    if(!snapshot_)snapshot_=allocate(n);
    if(!frame_||!snapshot_){release(frame_);release(snapshot_);frame_=snapshot_=nullptr;return record(Status::NoMemory);}
    bytes_=n;memset(frame_,whiteByte(),n);memset(snapshot_,whiteByte(),n);
    const Status s=io_.begin(profile_,config_);
    if(s!=Status::Ok){io_.end();return record(s);}
    ready_=true;rotation_=0;frameOpen_=false;windowOpen_=false;
    dirtyFirst_=0;dirtyEnd_=bytes_;pending_=true;snapshotValid_=false;updates_=0;
    restart();return state_==State::Fault?status_:record(Status::Ok);
  }
  void end() override {
    if(ready_)io_.end();ready_=false;frameOpen_=false;windowOpen_=false;pending_=false;state_=State::Off;
  }
  bool ready() const override {return ready_;}
  uint16_t width() const override {return rotation_&1?profile_.height:profile_.width;}
  uint16_t height() const override {return rotation_&1?profile_.width:profile_.height;}
  uint8_t rotation() const override {return rotation_;}
  int resetPin() const override {return config_.pins.reset;}
  Status status() const override {return status_;}
  uint32_t id() const override {return 0xe000U+uint8_t(profile_.controller);}
  TransportInfo transportInfo() const override {
    TransportInfo p;p.frequency=config_.busClockHz;p.clock=config_.pins.clock;p.cs=config_.pins.cs;
    p.dc=config_.pins.dc;p.data[0]=config_.pins.mosi;p.read=config_.pins.miso;p.lanes=1;return p;
  }
  DisplayCapabilities capabilities() const override {
    DisplayCapabilities p;p.colorMode=profile_.encoding==NativeEpaperEncoding::Mono?
      DisplayColorMode::Monochrome:DisplayColorMode::Rgb;
    p.refreshMode=DisplayRefreshMode::Epaper;p.bitsPerPixel=profile_.bitsPerPixel();
    p.nativeWidth=profile_.width;p.nativeHeight=profile_.height;p.minRefreshMs=profile_.minRefreshMs;
    p.recommendedFrameMs=profile_.minRefreshMs;p.partialRefresh=false;return p;
  }
  // The backend retains packed copies, never the caller's RGB tile.
  bool refreshBusy() const override {return ready_&&state_!=State::Idle&&state_!=State::Sleeping&&state_!=State::Fault;}
  bool frameOpen() const override {return frameOpen_;}
  bool sleeping() const {return state_==State::Sleeping;}
  bool pendingRefresh() const {return pending_;}
  uint32_t updateCount() const {return updates_;}
  size_t packedBytes() const {return bytes_;}
  size_t storageBytes() const {return bytes_*(size_t(frame_!=nullptr)+size_t(snapshot_!=nullptr)+size_t(backup_!=nullptr));}
  const uint8_t *packedPixels() const {return frame_;}
  const NativeEpaperProfile &profile() const {return profile_;}
  Status setRotation(uint8_t value) override {
    if(!ready_)return record(Status::NotInitialized);
    if(value>3)return record(Status::InvalidArgument);
    if(frameOpen_)return record(Status::Busy);
    rotation_=value;windowOpen_=false;return record(Status::Ok);
  }
  Status window(int32_t x,int32_t y,int32_t right,int32_t bottom) override {
    if(!ready_)return record(Status::NotInitialized);
    if(x<0||y<0||right<x||bottom<y||right>=width()||bottom>=height())return record(Status::InvalidArgument);
    left_=uint16_t(x);top_=uint16_t(y);spanWidth_=uint16_t(right-x+1);spanHeight_=uint16_t(bottom-y+1);
    cursor_=0;windowOpen_=true;return record(Status::Ok);
  }
  Status pixels(PixelView view,bool=false) override {
    if(!ready_)return record(Status::NotInitialized);
    if(!view.valid()||(view.order!=PixelOrder::Native&&view.order!=PixelOrder::Wire)||!windowOpen_||
       view.height>SIZE_MAX/view.width||view.width*view.height>uint32_t(spanWidth_)*spanHeight_-cursor_)
      return record(Status::InvalidArgument);
    for(size_t y=0;y<view.height;++y){const uint8_t *row=view.data+2*y*view.stride;
      for(size_t x=0;x<view.width;++x){putAtCursor(quantize(monoRead565(row+2*x,view.order)));++cursor_;}}
    return record(Status::Ok);
  }
  Status fill(uint16_t color,uint32_t count) override {
    if(!ready_)return record(Status::NotInitialized);
    if(!windowOpen_||count>uint32_t(spanWidth_)*spanHeight_-cursor_)return record(Status::InvalidArgument);
    const uint8_t value=quantize(color),bits=profile_.bitsPerPixel();
    if(!cursor_&&left_==0&&top_==0&&spanWidth_==width()&&spanHeight_==height()&&
       count==uint32_t(width())*height()&&profile_.width==profile_.storageWidth){
      const uint8_t packed=bits==1?(value?0xff:0):bits==2?uint8_t(value*0x55):uint8_t(value*0x11);
      for(size_t i=0;i<bytes_;++i)store(i,packed);
      cursor_+=count;return record(Status::Ok);
    }
    while(count--){putAtCursor(value);++cursor_;}return record(Status::Ok);
  }
  Status readRect(int32_t x,int32_t y,size_t w,size_t h,uint16_t *out) override {
    if(!ready_)return record(Status::NotInitialized);
    if(!out||!w||!h||x<0||y<0||w>width()||h>height()||size_t(x)>width()-w||size_t(y)>height()-h)
      return record(Status::InvalidArgument);
    for(size_t row=0;row<h;++row)for(size_t col=0;col<w;++col){
      uint16_t px=uint16_t(x+col),py=uint16_t(y+row);native(px,py);*out++=decode(index(px,py));}
    return record(Status::Ok);
  }
  Status beginFrame(const DirtyRect *areas,size_t count) override {
    if(!ready_)return record(Status::NotInitialized);
    if(count&&!areas)return record(Status::InvalidArgument);
    for(size_t i=0;i<count;++i){const auto &r=areas[i];if(r.x<0||r.y<0||r.width<=0||r.height<=0||
      int64_t(r.x)+r.width>width()||int64_t(r.y)+r.height>height())return record(Status::InvalidArgument);}
    if(!count||frameOpen_)return record(Status::Ok);
    if(!backup_){backup_=allocate(bytes_);if(!backup_)return record(Status::NoMemory);}
    memcpy(backup_,frame_,bytes_);backupFirst_=dirtyFirst_;backupEnd_=dirtyEnd_;frameOpen_=true;return record(Status::Ok);
  }
  Status commitFrame() override {frameOpen_=false;windowOpen_=false;return finish();}
  Status abortFrame() override {
    if(!ready_)return record(Status::NotInitialized);
    if(frameOpen_){memcpy(frame_,backup_,bytes_);dirtyFirst_=backupFirst_;dirtyEnd_=backupEnd_;}
    frameOpen_=false;windowOpen_=false;return record(Status::Ok);
  }
  Status finish() override {
    if(!ready_)return record(Status::NotInitialized);
    if(state_==State::Fault)return record(Status::IoError);
    if(!frameOpen_&&dirtyEnd_>dirtyFirst_)pending_=true;
    if(state_==State::Sleeping&&pending_)return wake();return record(Status::Ok);
  }
  Status requestFullRefresh(){if(!ready_)return record(Status::NotInitialized);force_=pending_=true;return record(Status::Ok);}
  Status recover(){
    if(!ready_)return record(Status::NotInitialized);
    if(frameOpen_)return record(Status::Busy);
    io_.end();const Status s=io_.begin(profile_,config_);if(s!=Status::Ok)return fail(s);
    restart();force_=pending_=true;return record(Status::Ok);
  }
  Status wake(){if(!ready_)return record(Status::NotInitialized);if(state_!=State::Sleeping)return record(Status::Ok);
    restart();force_=pending_=true;return record(Status::Ok);}
  Status sleep(){
    if(!ready_)return record(Status::NotInitialized);if(state_==State::Sleeping)return record(Status::Ok);
    if(state_!=State::Idle||frameOpen_||pending_)return record(Status::Busy);
    if(!isIt()&&(config_.pins.reset<0||!protocol().sleep||!protocol().powerOff))return record(Status::Unsupported);
    chip_=0;enter(State::PowerOff,lastNow_);return record(Status::Ok);
  }
  Status poll(uint32_t now) override {
    if(!ready_)return record(Status::NotInitialized);if(state_==State::Fault)return record(Status::IoError);
    lastNow_=now;if(!clockStarted_){clockStarted_=true;stageAt_=now;}
    if(state_!=State::Idle&&state_!=State::Sleeping&&state_!=State::Off&&
       elapsed(now,stageAt_)>=profile_.busyTimeoutMs)return fail(Status::IoError);
    const auto &driver=protocol();Status s=Status::Ok;
    switch(state_){
      case State::PowerSettle:
        if(elapsed(now,stageAt_)>=config_.powerOnDelayMs){s=io_.reset(false);if(s==Status::Ok)enter(State::ResetLow,now);}break;
      case State::ResetLow:
        if(elapsed(now,stageAt_)>=config_.resetDurationMs){s=io_.reset(true);if(s==Status::Ok)enter(State::ResetRelease,now);}break;
      case State::ResetRelease:if(elapsed(now,stageAt_)>=20){initStep_=chip_=0;waitThen(State::Initialize,now);}break;
      case State::Wait:
        if(elapsed(now,stageAt_)>=profile_.busyTimeoutMs)return fail(Status::IoError);
        {bool level=false;s=io_.busy(level);if(s==Status::Ok){
          if(level==profile_.busyHigh){idleSince_=now;idleSeen_=false;}
          else if(!idleSeen_){idleSince_=now;idleSeen_=true;}
          else if(elapsed(now,idleSince_)>=5&&elapsed(now,stageAt_)>=minimumWaitMs_)enter(afterWait_,now);}}
        break;
      case State::Initialize:
        if(isIt()){s=it_.initialize(io_,profile_.width,profile_.height);if(s==Status::Ok)enter(State::Idle,now);break;}
        if(initStep_==driver.initSteps){initStep_=0;if(++chip_==profile_.chipCount){chip_=0;enter(State::Idle,now);}break;}
        s=driver.initialize(io_,profile_,initStep_,chip_);
        if(s==Status::Ok){const bool wait=(driver.waitAfterInit&(uint32_t(1)<<initStep_))!=0;++initStep_;
          if(wait)waitThen(State::Initialize,now,10);}break;
      case State::Idle:
        if(!pending_||frameOpen_)break;
        if(updates_&&elapsed(now,lastStart_)<profile_.minRefreshMs)break;
        if(!force_&&snapshotValid_&&dirtyEnd_>dirtyFirst_&&
           !memcmp(frame_+dirtyFirst_,snapshot_+dirtyFirst_,dirtyEnd_-dirtyFirst_)){
          pending_=false;dirtyFirst_=bytes_;dirtyEnd_=0;break;}
        if(force_||!snapshotValid_)memcpy(snapshot_,frame_,bytes_);
        else if(dirtyEnd_>dirtyFirst_)memcpy(snapshot_+dirtyFirst_,frame_+dirtyFirst_,dirtyEnd_-dirtyFirst_);
        pending_=force_=false;dirtyFirst_=bytes_;dirtyEnd_=0;plane_=chip_=0;offset_=0;
        enter(State::BeginPlane,now);break;
      case State::BeginPlane:
        if(!isIt()&&(driver.zeroPlanes&zeroValid_[chip_]&(uint8_t(1)<<plane_))){advancePlane(now);break;}
        s=isIt()?it_.startUpload(io_,itArea()):driver.startPlane(io_,profile_,plane_,chip_);
        if(s==Status::Ok){offset_=0;enter(State::Upload,now);}break;
      case State::Upload:s=isIt()?uploadIt():upload();if(s==Status::Ok&&offset_==bytes_/profile_.chipCount){
        if(isIt()){enter(State::FinishUpload,now);break;}
        if(driver.zeroPlanes&(uint8_t(1)<<plane_))zeroValid_[chip_]|=uint8_t(1)<<plane_;
        advancePlane(now);}break;
      case State::FinishUpload:s=it_.finishUpload(io_);if(s==Status::Ok){lastStart_=now;enter(State::Refresh,now);}break;
      case State::Refresh:
        if(isIt()){s=it_.refresh(io_,itArea());if(s==Status::Ok)enter(State::Complete,now);break;}
        s=driver.refresh(io_,profile_,chip_);
        if(s==Status::Ok&&++chip_==profile_.chipCount){chip_=0;lastStart_=now;waitThen(State::Complete,now);}break;
      case State::Complete:snapshotValid_=true;++updates_;enter(State::Idle,now);break;
      case State::PowerOff:
        s=isIt()?it_.powerOff(io_):driver.powerOff(io_,profile_,chip_);
        if(s==Status::Ok&&++chip_==profile_.chipCount){chip_=0;waitThen(State::Sleep,now);}break;
      case State::Sleep:
        s=isIt()?it_.sleep(io_):driver.sleep(io_,profile_,chip_);
        if(s==Status::Ok&&++chip_==profile_.chipCount){chip_=0;snapshotValid_=false;enter(State::Sleeping,now);}break;
      default:break;
    }
    return s==Status::Ok||s==Status::Busy?record(Status::Ok):fail(s);
  }
 private:
  enum class State:uint8_t{Off,PowerSettle,ResetLow,ResetRelease,Wait,Initialize,Idle,BeginPlane,Upload,FinishUpload,Refresh,Complete,PowerOff,Sleep,Sleeping,Fault};
  bool isIt() const {return profile_.controller==NativeEpaperController::IT8951;}
  const NativeEpaperProtocol &protocol() const {static const NativeEpaperProtocol empty;return profile_.protocol?*profile_.protocol:empty;}
  it8951::Area itArea() const {return it8951::Area(0,0,profile_.width,profile_.height);}
  static uint32_t elapsed(uint32_t a,uint32_t b){return uint32_t(a-b);}
  uint8_t *allocate(size_t n){return static_cast<uint8_t*>(config_.preferPsram ? platform::allocateRetained(n) : ::malloc(n));}
  static void release(void *p){platform::freeUiMemory(p);}
  uint8_t whiteByte() const {return profile_.encoding==NativeEpaperEncoding::Mono?0xff:
    profile_.encoding==NativeEpaperEncoding::FourColor?0x55:profile_.encoding==NativeEpaperEncoding::Gray16?0xff:0x11;}
  void enter(State s,uint32_t now){state_=s;stageAt_=now;}
  void waitThen(State s,uint32_t now,uint16_t minimum=0){afterWait_=s;minimumWaitMs_=minimum;idleSeen_=false;enter(State::Wait,now);}
  void restart(){clockStarted_=false;snapshotValid_=false;zeroValid_[0]=zeroValid_[1]=0;itChunkBytes_=0;it_.resetState();state_=State::PowerSettle;}
  void advancePlane(uint32_t now){
    if(++plane_<protocol().planes)enter(State::BeginPlane,now);
    else if(++chip_<profile_.chipCount){plane_=0;enter(State::BeginPlane,now);}
    else{chip_=0;enter(State::Refresh,now);}
  }
  Status record(Status s){status_=s;return s;}
  Status fail(Status s){io_.end();state_=State::Fault;snapshotValid_=false;pending_=true;return record(s);}
  void store(size_t at,uint8_t value){if(frame_[at]==value)return;frame_[at]=value;
    if(at<dirtyFirst_)dirtyFirst_=at;if(at>=dirtyEnd_)dirtyEnd_=at+1;}
  void native(uint16_t &x,uint16_t &y) const {const uint16_t old=x;
    switch(rotation_){case 1:x=profile_.width-1-y;y=old;break;
      case 2:x=profile_.width-1-x;y=profile_.height-1-y;break;
      case 3:x=y;y=profile_.height-1-old;break;default:break;}}
  uint8_t index(uint16_t x,uint16_t y) const {
    const uint8_t bits=profile_.bitsPerPixel(),per=8/bits,shift=uint8_t(8-bits-(x%per)*bits);
    return uint8_t((frame_[size_t(y)*profile_.rowBytes()+x/per]>>shift)&((1U<<bits)-1));
  }
  void putAtCursor(uint8_t value){uint16_t x=uint16_t(left_+cursor_%spanWidth_),y=uint16_t(top_+cursor_/spanWidth_);
    native(x,y);const uint8_t bits=profile_.bitsPerPixel(),per=8/bits,shift=uint8_t(8-bits-(x%per)*bits);
    const size_t at=size_t(y)*profile_.rowBytes()+x/per;const uint8_t mask=uint8_t(((1U<<bits)-1)<<shift);
    store(at,uint8_t((frame_[at]&~mask)|(value<<shift)));
  }
  uint8_t quantize(uint16_t color) const {
    const uint32_t r=((color>>11)*255+15)/31,g=(((color>>5)&63)*255+31)/63,b=((color&31)*255+15)/31;
    if(profile_.encoding==NativeEpaperEncoding::Mono)return r*77+g*150+b*29>=32768?1:0;
    if(profile_.encoding==NativeEpaperEncoding::Gray16)return uint8_t(((r*77+g*150+b*29)*15+32640)/65280);
    const uint16_t palette[]={0,0xffff,0xffe0,0xf800,0x001f,0x07e0};
    const uint8_t count=profile_.encoding==NativeEpaperEncoding::FourColor?4:6;uint8_t best=0;uint32_t distance=UINT32_MAX;
    for(uint8_t i=0;i<count;++i){const int32_t dr=int32_t(r)-((palette[i]>>11)*255+15)/31,
      dg=int32_t(g)-(((palette[i]>>5)&63)*255+31)/63,db=int32_t(b)-((palette[i]&31)*255+15)/31;
      const uint32_t d=uint32_t(dr*dr)*77+uint32_t(dg*dg)*150+uint32_t(db*db)*29;
      if(d<distance){distance=d;best=i;if(!d)break;}}return best;
  }
  uint16_t decode(uint8_t value) const {
    if(profile_.encoding==NativeEpaperEncoding::Mono)return value?0xffff:0;
    if(profile_.encoding==NativeEpaperEncoding::Gray16){const uint8_t c=uint8_t(value*17);return uint16_t((c>>3)<<11|(c>>2)<<5|(c>>3));}
    static const uint16_t colors[]={0,0xffff,0xffe0,0xf800,0x001f,0x07e0};return value<6?colors[value]:0;
  }
  Status upload(){
    const size_t total=bytes_/profile_.chipCount,limit=config_.maxDataBytesPerPoll;
    const uint8_t expansion=profile_.protocol->encodedBytesPerByte;
    const uint32_t began=io_.nowMicros();size_t used=0;
    do{size_t n=total-offset_;if(n>(limit-used)/expansion)n=(limit-used)/expansion;
      const bool zero=(profile_.protocol->zeroPlanes&(uint8_t(1)<<plane_))!=0;
      if((zero||profile_.protocol->encode)&&n>256/expansion)n=256/expansion;
      if(!n)break;
      // Single-chip packed data is already in bus order. Multi-chip protocols
      // consume left/right half rows without allocating another whole image.
      const uint8_t *source=snapshot_+offset_;uint8_t staging[256];
      if(profile_.chipCount==2){const size_t half=profile_.rowBytes()/2;
        n=n<half-offset_%half?n:half-offset_%half;
        source=snapshot_+(offset_/half)*profile_.rowBytes()+chip_*half+offset_%half;}
      if(zero){
        memset(staging,0,n*expansion);source=staging;
      }else if(profile_.protocol->encode){profile_.protocol->encode(source,staging,n);source=staging;}
      const Status s=io_.data(source,n*expansion,chip_);if(s!=Status::Ok)return s;
      offset_+=n;used+=n*expansion;stageAt_=lastNow_;
    }while(offset_<total&&used<limit&&(!config_.pollBudgetMicros||uint32_t(io_.nowMicros()-began)<config_.pollBudgetMicros));
    return Status::Ok;
  }
  Status uploadIt(){
    const uint32_t began=io_.nowMicros();size_t used=0;
    do{
      if(!itChunkBytes_){size_t n=bytes_-offset_;if(n>256)n=256;
        if(n>config_.maxDataBytesPerPoll-used)n=config_.maxDataBytesPerPoll-used;
        n&=~size_t(1);if(!n)break;itChunkBytes_=uint16_t(n);}
      const size_t n=itChunkBytes_;uint16_t words[128];for(size_t i=0;i<n/2;++i){
        const uint8_t *p=snapshot_+offset_+i*2;words[i]=uint16_t(uint16_t(p[0])<<8|p[1]);}
      const Status s=it_.writePixels(io_,words,n/2);if(s!=Status::Ok)return s;
      offset_+=n;used+=n;itChunkBytes_=0;stageAt_=lastNow_;
    }while(offset_<bytes_&&used<config_.maxDataBytesPerPoll&&
      (!config_.pollBudgetMicros||uint32_t(io_.nowMicros()-began)<config_.pollBudgetMicros));
    return Status::Ok;
  }
  NativeEpaperProfile profile_;NativeEpaperConfig config_;NativeEpaperIo &io_;
  it8951::Controller it_;
  Status status_=Status::Ok;State state_=State::Off,afterWait_=State::Off;
  uint8_t *frame_=nullptr,*snapshot_=nullptr,*backup_=nullptr;
  size_t bytes_=0,dirtyFirst_=0,dirtyEnd_=0,backupFirst_=0,backupEnd_=0,offset_=0;
  uint16_t left_=0,top_=0,spanWidth_=0,spanHeight_=0,itChunkBytes_=0,minimumWaitMs_=0;uint32_t cursor_=0;
  uint8_t rotation_=0,chip_=0,plane_=0,initStep_=0,zeroValid_[2]={0,0};
  bool ready_=false,frameOpen_=false,windowOpen_=false,pending_=false,force_=false,snapshotValid_=false,
    clockStarted_=false,idleSeen_=false;
  uint32_t stageAt_=0,idleSince_=0,lastNow_=0,lastStart_=0,updates_=0;
};

} // namespace nice_gfx
