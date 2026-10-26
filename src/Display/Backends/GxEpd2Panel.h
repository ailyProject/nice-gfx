#pragma once
#include "../Backend.h"
#include "GxEpd2Engine.h"
#include "../../Drivers/Display/GxEpd2/NgfxGxEpd2_EPD.h"
#include "../../Graphics/MonoConvert.h"

namespace nice_gfx {
// Indexed, immutable physical snapshots keep slow panel work independent of UI
// rendering. Read-free protocols are replayed cooperatively; seven upstream
// protocols that actually read the panel execute on a dedicated worker.
class GxEpd2Panel:public Backend{
 public:
  GxEpd2Panel(const GxEpd2Profile &p,const GxEpd2Config &c,GxEpd2Io &io)
    :profile_(p),config_(c),io_(io),engine_(config_,io_),workerRunning_(false){
    palette_=p.palette;paletteCount_=p.paletteCount;
    if(!palette_)palette_=gxEpd2Palette(p.encoding,paletteCount_);
    bpp_=paletteCount_<=2?1:paletteCount_<=4?2:4;
  }
  ~GxEpd2Panel()override{end();free(frame_);free(snapshot_);free(backup_);free(planes_);free(grayRows_);}
  Status begin()override{
    if(ready_)return result(status());
    if(!profile_.create||!profile_.upload||!profile_.width||!profile_.height||
      profile_.width>32767||profile_.height>32767||profile_.storageWidth<profile_.width||profile_.storageWidth>32767||
      !palette_||paletteCount_<2||paletteCount_>16||!config_.maxTapeBytes||
      !config_.pollBudgetMicros||!config_.maxDataBytesPerPoll||config_.busClockHz>80000000UL)
      return result(Status::InvalidArgument);
#if !NGFX_GXEPD2_WORKER_PLATFORM
    if(profile_.worker)return result(Status::Unsupported);
#endif
    Status s=validatePins();if(s!=Status::Ok)return result(s);
    s=allocate();if(s!=Status::Ok)return result(s);
    engine_.reset();s=io_.begin(profile_,config_);if(s!=Status::Ok){io_.end();return result(s);}ioOpen_=true;
    {GxEpd2HalScope scope(&engine_);driver_=profile_.create(config_);}
    if(!driver_){io_.end();ioOpen_=false;return result(Status::NoMemory);}
    driver_->bindHal(&engine_,config_.vcomMillivolts);
    if(profile_.worker)driver_->enableRead(config_.pins.clock,config_.pins.mosi);
    ready_=true;initialized_=false;snapshotValid_=false;hasFrameStart_=false;job_=Job::None;
    s=startJob(Job::Init);if(s!=Status::Ok){end();return result(s);}return result(Status::Ok);
  }
  void end()override{
    if(frameOpen_)abortFrame();engine_.cancel();joinWorker();
    delete driver_;driver_=nullptr;if(ioOpen_)io_.end();ioOpen_=false;
    ready_=initialized_=false;job_=Job::None;pending_=false;snapshotValid_=false;engine_.releaseTape();
  }
  Status recover(){end();last_=Status::Ok;const Status s=begin();if(s==Status::Ok)pending_=true;return s;}
  bool ready()const override{return ready_;}
  uint16_t width()const override{return rotation_&1?profile_.height:profile_.width;}
  uint16_t height()const override{return rotation_&1?profile_.width:profile_.height;}
  uint8_t rotation()const override{return rotation_;}
  Status setRotation(uint8_t value)override{if(value>3)return result(Status::InvalidArgument);if(frameOpen_)return result(Status::Busy);
    rotation_=value;windowValid_=false;return result(Status::Ok);}
  int resetPin()const override{return config_.pins.reset;}
  Status status()const override{return engine_.status()!=Status::Ok?engine_.status():last_;}
  bool refreshBusy()const override{return ready_&&(job_!=Job::None||pending_);}
  bool frameOpen()const override{return frameOpen_;}
  DisplayCapabilities capabilities()const override{
    DisplayCapabilities c;c.colorMode=profile_.encoding==GxEpd2ColorEncoding::Mono?DisplayColorMode::Monochrome:DisplayColorMode::Rgb;
    c.refreshMode=DisplayRefreshMode::Epaper;c.bitsPerPixel=bpp_;c.nativeWidth=profile_.width;c.nativeHeight=profile_.height;
    c.minRefreshMs=c.recommendedFrameMs=refreshInterval();c.partialRefresh=false;return c;
  }
  const GxEpd2Profile &profile()const{return profile_;}
  const uint16_t *palette()const{return palette_;}
  uint8_t paletteCount()const{return paletteCount_;}
  size_t framebufferBytes()const{return bytes_;}
  // The capture worker publishes its owned tape only when its release store
  // completes. Diagnostics never inspect a buffer that is still being grown.
  size_t tapeBytes()const{return workerRunning_.load()?0:engine_.tapeBytes();}
  size_t tapeCapacity()const{return workerRunning_.load()?0:engine_.tapeCapacity();}
  Status window(int32_t x0,int32_t y0,int32_t x1,int32_t y1)override{
    if(!ready_)return result(Status::NotInitialized);
    if(x0<0||y0<0||x1<x0||y1<y0||x1>=width()||y1>=height())return result(Status::InvalidArgument);
    windowX_=uint16_t(x0);windowY_=uint16_t(y0);spanWidth_=uint16_t(x1-x0+1);spanHeight_=uint16_t(y1-y0+1);
    cursor_=0;windowValid_=true;return result(Status::Ok);
  }
  Status pixels(PixelView view,bool=false)override{
    if(!ready_)return result(Status::NotInitialized);
    if(!windowValid_||!view.valid()||(view.order!=PixelOrder::Native&&view.order!=PixelOrder::Wire)||
      view.height>SIZE_MAX/view.width||view.width*view.height>remaining())return result(Status::InvalidArgument);
    uint16_t x=uint16_t(windowX_+cursor_%spanWidth_),y=uint16_t(windowY_+cursor_/spanWidth_);
    for(size_t row=0;row<view.height;++row){const uint8_t *source=view.data+row*view.stride*2;
      for(size_t col=0;col<view.width;++col){const uint16_t color=monoRead565(source+col*2,view.order);setLogical(x,y,quantize(color));
        if(++x==windowX_+spanWidth_){x=windowX_;++y;}}
    }cursor_+=uint32_t(view.width*view.height);return result(Status::Ok);
  }
  Status fill(uint16_t color,uint32_t count)override{
    if(!ready_)return result(Status::NotInitialized);if(!windowValid_||count>remaining())return result(Status::InvalidArgument);
    const uint8_t index=quantize(color);uint16_t x=uint16_t(windowX_+cursor_%spanWidth_),y=uint16_t(windowY_+cursor_/spanWidth_);
    uint32_t left=count;while(left){const uint16_t n=uint16_t(left<uint32_t(windowX_+spanWidth_-x)?left:windowX_+spanWidth_-x);
      if(!rotation_)fillNative(x,y,n,index);else for(uint16_t i=0;i<n;++i)setLogical(uint16_t(x+i),y,index);
      left-=n;x=windowX_;++y;}cursor_+=count;return result(Status::Ok);
  }
  Status readRect(int32_t x,int32_t y,size_t w,size_t h,uint16_t*out)override{
    if(!ready_)return result(Status::NotInitialized);
    if(!out||!w||!h||x<0||y<0||w>width()||h>height()||size_t(x)>width()-w||size_t(y)>height()-h||w>SIZE_MAX/2||h>SIZE_MAX/(w*2))
      return result(Status::InvalidArgument);
    for(size_t row=0;row<h;++row)for(size_t col=0;col<w;++col){uint16_t nx,ny;native(uint16_t(x+col),uint16_t(y+row),nx,ny);
      const uint8_t index=getIndex(frame_,nx,ny);*out++=palette_[index<paletteCount_?index:1];}
    return result(Status::Ok);
  }
  Status beginFrame(const DirtyRect*,size_t)override{
    if(!ready_)return result(Status::NotInitialized);if(frameOpen_)return result(Status::Busy);
    if(!backup_)backup_=static_cast<uint8_t*>(detail::gxEpd2Alloc(bytes_,config_.preferPsram));
    if(!backup_)return result(Status::NoMemory);memcpy(backup_,frame_,bytes_);pendingBefore_=pending_;frameOpen_=true;return result(Status::Ok);
  }
  Status commitFrame()override{if(!frameOpen_)return result(Status::InvalidArgument);frameOpen_=false;return finish();}
  Status abortFrame()override{if(frameOpen_){memcpy(frame_,backup_,bytes_);frameOpen_=false;pending_=pendingBefore_;}return result(Status::Ok);}
  Status finish()override{
    if(!ready_)return result(Status::NotInitialized);if(engine_.status()!=Status::Ok)return engine_.status();
    if(!frameOpen_)pending_=true;return result(Status::Ok);
  }
  Status poll(uint32_t nowMs)override{
    if(!ready_)return result(Status::NotInitialized);if(engine_.status()!=Status::Ok)return engine_.status();
    if(job_!=Job::None){
      if(workerRunning_.load())return Status::Ok;
      if(profile_.worker||captureWorker_)joinWorker();
      if(!profile_.worker){const Status s=engine_.poll(nowMs);if(s!=Status::Ok)return s;if(engine_.playing())return Status::Ok;}
      if(engine_.status()!=Status::Ok)return engine_.status();
      if(job_==Job::Init){initialized_=true;engine_.releaseTape();}
      if(job_==Job::Frame)snapshotValid_=true;
      job_=Job::None;
    }
    if(!initialized_||!pending_||frameOpen_)return Status::Ok;
    if(snapshotValid_&&!memcmp(frame_,snapshot_,bytes_)){pending_=false;return Status::Ok;}
    if(hasFrameStart_&&uint32_t(nowMs-lastFrameStart_)<refreshInterval())return Status::Ok;
    memcpy(snapshot_,frame_,bytes_);pending_=false;lastFrameStart_=nowMs;hasFrameStart_=true;
    if(planes_)preparePlanes();return startJob(Job::Frame);
  }
 private:
  enum class Job:uint8_t{None,Init,Frame};
  Status result(Status s){last_=s;return s;}
  uint32_t refreshInterval()const{return profile_.fullRefreshMs<1000?1000:profile_.fullRefreshMs;}
  uint32_t remaining()const{return uint32_t(spanWidth_)*spanHeight_-cursor_;}
  uint8_t quantize(uint16_t color){if(cacheValid_&&color==cacheColor_)return cacheIndex_;
    cacheValid_=true;cacheColor_=color;cacheIndex_=gxEpd2Quantize(color,palette_,paletteCount_);return cacheIndex_;}
  void native(uint16_t x,uint16_t y,uint16_t &nx,uint16_t &ny)const{
    switch(rotation_){case 1:nx=uint16_t(profile_.width-1-y);ny=x;break;
      case 2:nx=uint16_t(profile_.width-1-x);ny=uint16_t(profile_.height-1-y);break;
      case 3:nx=y;ny=uint16_t(profile_.height-1-x);break;default:nx=x;ny=y;break;}
  }
  uint8_t getIndex(const uint8_t *buffer,uint16_t x,uint16_t y)const{
    const size_t bit=size_t(x)*bpp_;const uint8_t shift=uint8_t(8-bpp_-(bit&7));
    return uint8_t((buffer[size_t(y)*stride_+(bit>>3)]>>shift)&((1U<<bpp_)-1));
  }
  void setNative(uint16_t x,uint16_t y,uint8_t index){const size_t bit=size_t(x)*bpp_;const uint8_t shift=uint8_t(8-bpp_-(bit&7));
    uint8_t &value=frame_[size_t(y)*stride_+(bit>>3)];const uint8_t mask=uint8_t(((1U<<bpp_)-1)<<shift);
    value=uint8_t((value&~mask)|(index<<shift));}
  void setLogical(uint16_t x,uint16_t y,uint8_t index){uint16_t nx,ny;native(x,y,nx,ny);setNative(nx,ny,index);}
  void fillNative(uint16_t x,uint16_t y,uint16_t n,uint8_t index){
    const uint8_t group=uint8_t(8/bpp_);while(n&&(x%group)){setNative(x++,y,index);--n;}
    if(n>=group){const size_t bytes=n/group;uint8_t packed=index;for(uint8_t bits=bpp_;bits<8;bits=uint8_t(bits*2))packed=uint8_t(packed|(packed<<bits));
      memset(frame_+size_t(y)*stride_+size_t(x)/group,packed,bytes);const uint16_t pixels=uint16_t(bytes*group);x=uint16_t(x+pixels);n=uint16_t(n-pixels);}
    while(n--)setNative(x++,y,index);
  }
  Status validatePins()const{
    const GxEpd2Pins&p=config_.pins;if(p.clock<0||p.mosi<0||p.cs<0||(p.dc<0&&profile_.encoding!=GxEpd2ColorEncoding::Gray16)||p.busy<0||
      (profile_.requiresMiso&&p.miso<0))return Status::InvalidArgument;
    int pins[19]={p.clock,p.mosi,p.miso,p.cs,p.dc,p.reset,p.busy,
      p.extraCs[0],p.extraCs[1],p.extraCs[2],p.extraDc[0],p.extraDc[1],p.extraDc[2],
      p.extraReset[0],p.extraReset[1],p.extraReset[2],p.extraBusy[0],p.extraBusy[1],p.extraBusy[2]};
    for(unsigned i=0;i<12;++i)if((profile_.requiredExtraPins&(1U<<i))&&pins[7+i]<0)return Status::InvalidArgument;
    for(unsigned i=0;i<19;++i){if(pins[i]<0)continue;const bool input=i==2||i==6||i>=16;
      if(!io_.validPin(pins[i],input))return Status::InvalidArgument;
      for(unsigned j=0;j<i;++j)if(pins[j]>=0&&io_.physicalPin(pins[j])==io_.physicalPin(pins[i]))return Status::InvalidArgument;
    }return Status::Ok;
  }
  Status allocate(){
    if(profile_.storageWidth>(SIZE_MAX-7)/bpp_)return Status::NoMemory;
    const size_t stride=(size_t(profile_.storageWidth)*bpp_+7)/8;if(profile_.height>SIZE_MAX/stride)return Status::NoMemory;
    stride_=stride;bytes_=stride*profile_.height;
    if(!frame_){frame_=static_cast<uint8_t*>(detail::gxEpd2Alloc(bytes_,config_.preferPsram));if(!frame_)return Status::NoMemory;
      const uint8_t white=gxEpd2Quantize(0xffff,palette_,paletteCount_);uint8_t packed=white;
      for(uint8_t bits=bpp_;bits<8;bits=uint8_t(bits*2))packed=uint8_t(packed|(packed<<bits));memset(frame_,packed,bytes_);
    }
    if(!snapshot_)snapshot_=static_cast<uint8_t*>(detail::gxEpd2Alloc(bytes_,config_.preferPsram));if(!snapshot_)return Status::NoMemory;
    if(profile_.encoding==GxEpd2ColorEncoding::BlackRed||profile_.encoding==GxEpd2ColorEncoding::BlackYellow){
      planeStride_=(size_t(profile_.storageWidth)+7)/8;if(profile_.height>SIZE_MAX/planeStride_/2)return Status::NoMemory;
      planeBytes_=planeStride_*profile_.height;if(!planes_)planes_=static_cast<uint8_t*>(detail::gxEpd2Alloc(planeBytes_*2,config_.preferPsram));
      if(!planes_)return Status::NoMemory;
    }
    if(profile_.encoding==GxEpd2ColorEncoding::Gray16&&!grayRows_){if(profile_.storageWidth>SIZE_MAX/16)return Status::NoMemory;
      grayRows_=static_cast<uint8_t*>(detail::gxEpd2Alloc(size_t(profile_.storageWidth)*16,config_.preferPsram));if(!grayRows_)return Status::NoMemory;}
    return Status::Ok;
  }
  void preparePlanes(){memset(planes_,0xff,planeBytes_*2);
    for(uint16_t y=0;y<profile_.height;++y)for(uint16_t x=0;x<profile_.storageWidth;++x){const uint8_t index=getIndex(snapshot_,x,y);
      const size_t offset=size_t(y)*planeStride_+(x>>3);const uint8_t mask=uint8_t(0x80U>>(x&7));
      if(index==0)planes_[offset]&=uint8_t(~mask);else if(index==2)planes_[planeBytes_+offset]&=uint8_t(~mask);}
  }
  void executeJob(){GxEpd2HalScope scope(&engine_);
    if(job_==Job::Init)driver_->init(0,true,config_.resetDurationMs,config_.pulldownReset);
    else if(job_==Job::Frame){
      if(profile_.encoding==GxEpd2ColorEncoding::Gray16){
        for(uint16_t y=0;y<profile_.height&&!engine_.cancelled()&&engine_.status()==Status::Ok;y=uint16_t(y+16)){
          const uint16_t rows=uint16_t(profile_.height-y<16?profile_.height-y:16);
          for(uint16_t r=0;r<rows;++r)for(uint16_t x=0;x<profile_.storageWidth;++x)
            grayRows_[size_t(r)*profile_.storageWidth+x]=uint8_t(getIndex(snapshot_,x,uint16_t(y+r))*17);
          profile_.upload(*driver_,grayRows_,nullptr,0,int16_t(y),int16_t(profile_.storageWidth),int16_t(rows));
        }
      }else profile_.upload(*driver_,planes_?planes_:snapshot_,planes_?planes_+planeBytes_:nullptr,0,0,int16_t(profile_.storageWidth),int16_t(profile_.height));
      if(engine_.status()==Status::Ok&&!engine_.cancelled())driver_->refresh(false);
      if(engine_.status()==Status::Ok&&!engine_.cancelled())driver_->powerOff();
    }
  }
  Status startJob(Job job){job_=job;
    captureWorker_=false;
    if(!profile_.worker){engine_.beginRecording();
#if NGFX_GXEPD2_WORKER_PLATFORM==1
      captureWorker_=true;
#else
      executeJob();engine_.finishRecording();return engine_.status();
#endif
    }
#if NGFX_GXEPD2_WORKER_PLATFORM
    if(profile_.worker)engine_.direct(true,driver_->busyActiveLevel());workerRunning_.store(true);
#if NGFX_GXEPD2_WORKER_PLATFORM==1
    if(xTaskCreate(workerEntry,"ngfx-epd",4096,this,1,nullptr)!=pdPASS){workerRunning_.store(false);engine_.fault(Status::NoMemory);return engine_.status();}
#else
    worker_=std::thread(workerEntry,this);
#endif
    return Status::Ok;
#else
    engine_.fault(Status::Unsupported);return engine_.status();
#endif
  }
  static void workerEntry(void*context){GxEpd2Panel*self=static_cast<GxEpd2Panel*>(context);self->executeJob();
    if(self->captureWorker_)self->engine_.finishRecording();else self->engine_.finishDirect();self->workerRunning_.store(false);
#if NGFX_GXEPD2_WORKER_PLATFORM==1
    vTaskDelete(nullptr);
#endif
  }
  void joinWorker(){
#if NGFX_GXEPD2_WORKER_PLATFORM==1
    while(workerRunning_.load())vTaskDelay(1);
#elif NGFX_GXEPD2_WORKER_PLATFORM==2
    if(worker_.joinable())worker_.join();
#endif
  }
  GxEpd2Profile profile_;GxEpd2Config config_;GxEpd2Io &io_;GxEpd2Engine engine_;NgfxGxEpd2_EPD *driver_=nullptr;
  const uint16_t *palette_=nullptr;uint8_t paletteCount_=0,bpp_=1,rotation_=0;
  uint8_t *frame_=nullptr,*snapshot_=nullptr,*backup_=nullptr,*planes_=nullptr,*grayRows_=nullptr;
  size_t stride_=0,bytes_=0,planeStride_=0,planeBytes_=0;
  uint16_t windowX_=0,windowY_=0,spanWidth_=0,spanHeight_=0;uint32_t cursor_=0;
  bool ready_=false,initialized_=false,ioOpen_=false,pending_=false,frameOpen_=false,pendingBefore_=false;
  bool windowValid_=false,snapshotValid_=false,cacheValid_=false,hasFrameStart_=false;
  uint16_t cacheColor_=0;uint8_t cacheIndex_=0;uint32_t lastFrameStart_=0;
  Job job_=Job::None;Status last_=Status::Ok;detail::GxEpd2Atomic<bool> workerRunning_;
  bool captureWorker_=false;
#if NGFX_GXEPD2_WORKER_PLATFORM==2
  std::thread worker_;
#endif
};
} // namespace nice_gfx
