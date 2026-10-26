#pragma once
#include "PackedMonoPanel.h"
#include "../U8g2Types.h"
#if defined(__AVR__)
#include <avr/interrupt.h>
#else
#include <atomic>
#endif

namespace nice_gfx { namespace detail {
// Some upstream CAD and BYTE functions contain scratch/static state. Serialize
// each complete driver call and each physical BYTE operation across instances.
class U8g2Gate {
 public:
  U8g2Gate() {
#if defined(__AVR__)
    const uint8_t saved=SREG;cli();held_=!flag();if(held_)flag()=true;SREG=saved;
#else
    held_=!flag().test_and_set(std::memory_order_acquire);
#endif
  }
  ~U8g2Gate(){if(held_){
#if defined(__AVR__)
    const uint8_t saved=SREG;cli();flag()=false;SREG=saved;
#else
    flag().clear(std::memory_order_release);
#endif
  }}
  bool held()const{return held_;}
 private:
#if defined(__AVR__)
  static volatile bool &flag(){static volatile bool value=false;return value;}
#else
  static std::atomic_flag &flag(){static std::atomic_flag value=ATOMIC_FLAG_INIT;return value;}
#endif
  bool held_;
};
// PackedMonoPanel's RGB conversion never sends on this placeholder transport.
class U8g2NullTransport:public Transport {
 public:
  Status begin()override{return Status::Ok;}void end()override{}
  Status command(uint8_t,const uint8_t*,size_t)override{return Status::Unsupported;}
  Status pixels(uint8_t,PixelView,bool)override{return Status::Unsupported;}
  Status fill(uint8_t,uint16_t,size_t)override{return Status::Unsupported;}
  Status wait()override{return Status::Ok;}bool busy()const override{return false;}
  bool asynchronous()const override{return false;}void output(int,bool)override{}
  void delayMs(uint32_t)override{}
};
struct U8g2NullStorage{U8g2NullTransport transport;};
} // namespace detail

// All source pixels are consumed synchronously. Physical work is an owned,
// bounded tape of BYTE/GPIO operations; long upstream delays become deadlines.
// Continue poll(nowMs) even when the UI is unchanged, including initialization.
class U8g2Panel:private detail::U8g2NullStorage,public PackedMonoPanel {
 public:
  U8g2Panel(const U8g2Profile &profile,const U8g2Config &config,U8g2Io &io)
      :PackedMonoPanel(detail::U8g2NullStorage::transport,0,0,MonoPacking::PageLsb,config.pins.reset),
       profile_(profile),config_(config),io_(io){memset(&engine_,0,sizeof(engine_));}
  ~U8g2Panel()override{end();free(tape_);free(tileRow_);}
  Status begin()override{
    if(ready_)return record(fault_);
    if(!profile_.display||!profile_.cad||!profile_.byte||!config_.maxTapeBytes||
       !config_.pollBudgetMicros||!config_.maxDataBytesPerPoll||config_.i2cAddress>127||
       config_.busClockHz>80000000UL||
       (profile_.interface==U8g2Interface::I2c&&config_.busClockHz>1000000UL))
      return fail(Status::InvalidArgument);
    detail::U8g2Gate gate;if(!gate.held())return record(Status::Busy);
    fault_=Status::Ok;capturing_=true;clearTape();
    ngfx_u8x8_Setup(&engine_,profile_.display,profile_.cad,recordByte,recordGpio);
    engine_.user_ptr=this;
    if(!engine_.display_info){capturing_=false;return fail(Status::Unsupported);}
    const ngfx_u8x8_display_info_t &info=*engine_.display_info;
    if(!info.pixel_width||!info.pixel_height||!info.tile_width||!info.tile_height||
       uint32_t(info.pixel_width)>uint32_t(info.tile_width)*8||
       uint32_t(info.pixel_height)>uint32_t(info.tile_height)*8){capturing_=false;return fail(Status::Unsupported);}
    nativeWidth_=info.pixel_width;nativeHeight_=info.pixel_height;
    Status s=validatePins();if(s!=Status::Ok){capturing_=false;return fail(s);}
    if(!frame_){s=allocate(0);if(s!=Status::Ok){capturing_=false;return fail(s);}}
    if(!tileRow_){tileRow_=static_cast<uint8_t*>(malloc(size_t(info.tile_width)*8));
      if(!tileRow_){capturing_=false;return fail(Status::NoMemory);}}
    if(config_.busClockHz)engine_.bus_clock=config_.busClockHz;
    if(config_.i2cAddress)engine_.i2c_address=uint8_t(config_.i2cAddress<<1);
    for(unsigned i=0;i<NGFX_U8X8_PIN_CNT;++i){const int pin=pinFor(i);
      engine_.pins[i]=pin>=0&&pin<255?uint8_t(pin):NGFX_U8X8_PIN_NONE;}
    s=io_.begin(profile_,config_,info);if(s!=Status::Ok){capturing_=false;return fail(s);}
    ioOpen_=true;
    const uint8_t handled=engine_.display_cb(&engine_,NGFX_U8X8_MSG_DISPLAY_INIT,0,nullptr);
    engine_.display_cb(&engine_,NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE,0,nullptr);
    capturing_=false;
    if(!handled||fault_!=Status::Ok)return fail(fault_==Status::Ok?Status::Unsupported:fault_);
    ready_=true;initialized_=false;pending_=false;sentValid_=false;dirty_=true;
    phase_=Phase::Init;return record(Status::Ok);
  }
  void end()override{
    PackedMonoPanel::abortFrame();
    if(ioOpen_)io_.end();ioOpen_=false;ready_=false;initialized_=false;
    capturing_=false;executing_=false;pending_=false;phase_=Phase::None;
    waiting_=false;clearTape();sentValid_=false;
  }
  // Reinitialize the interface/controller, retaining the newest desired pixels.
  Status recover(){end();fault_=Status::Ok;const Status s=begin();if(s==Status::Ok)pending_=true;return s;}
  Status status()const override{return fault_!=Status::Ok?fault_:PackedMonoPanel::status();}
  bool asynchronous()const override{return false;}
  bool refreshBusy()const override{return ready_&&(phase_!=Phase::None||pending_);}
  DisplayCapabilities capabilities()const override{
    DisplayCapabilities c;c.colorMode=DisplayColorMode::Monochrome;c.bitsPerPixel=1;
    c.refreshMode=profile_.slowRefresh?DisplayRefreshMode::Epaper:DisplayRefreshMode::Immediate;
    c.nativeWidth=nativeWidth_;c.nativeHeight=nativeHeight_;
    c.partialRefresh=!profile_.slowRefresh&&!profile_.fullFrameOnly;
    c.minRefreshMs=profile_.slowRefresh?2000:0;
    c.recommendedFrameMs=profile_.slowRefresh?2000:33;return c;
  }
  const U8g2Profile &profile()const{return profile_;}
  const ngfx_u8x8_t &upstream()const{return engine_;}
  size_t tapeBytes()const{return tapeSize_;}
  Status finish()override{
    if(!ready_)return record(Status::NotInitialized);
    if(fault_!=Status::Ok)return fault_;
    if(!frameOpen_&&dirty_)pending_=true;
    return record(Status::Ok);
  }
  Status setContrast(uint8_t value){return control(NGFX_U8X8_MSG_DISPLAY_SET_CONTRAST,value);}
  Status powerSave(bool sleep){return control(NGFX_U8X8_MSG_DISPLAY_SET_POWER_SAVE,sleep?1:0);}
  Status poll(uint32_t nowMs)override{
    if(!ready_)return record(Status::NotInitialized);
    if(fault_!=Status::Ok)return fault_;
    const uint32_t start=io_.nowMicros();
    if(waiting_){if(uint32_t(nowMs-waitStart_)<waitMs_)return Status::Ok;waiting_=false;}
    if(phase_==Phase::None){
      if(!pending_||frameOpen_)return Status::Ok;
      if(sentValid_&&!memcmp(frame_,sent_,bytes_)){pending_=false;dirty_=false;return Status::Ok;}
      if(profile_.slowRefresh&&hasFrameStart_&&uint32_t(nowMs-lastFrameStart_)<2000)return Status::Ok;
      startFrame();lastFrameStart_=nowMs;hasFrameStart_=true;
    }
    if(phase_==Phase::Frame&&!tapeSize_){const Status s=recordFrameChunk();if(s!=Status::Ok)return s;}
    size_t consumed=0;
    if(uint32_t(io_.nowMicros()-start)>=config_.pollBudgetMicros)return Status::Ok;
    while(tapeRead_<tapeSize_){
      const uint8_t *op=tape_+tapeRead_;const bool gpio=op[0]!=0;
      const uint8_t msg=op[1],arg=op[2];
      if(gpio&&isDelay(msg)){
        const uint32_t ns=delayNanos(msg,arg);
        tapeRead_+=4;
        if(ns>=1000000UL){waiting_=true;waitStart_=nowMs;
          // nowMs has only millisecond precision and was sampled before this
          // poll. Include CPU/transfer time plus one quantization tick, so even
          // a 1 ms reset pulse cannot finish early near a tick boundary.
          waitMs_=(ns+999999UL)/1000000UL+1+uint32_t(io_.nowMicros()-start)/1000;
          return Status::Ok;}
        io_.delayNs(ns);
      }else{
        detail::U8g2Gate gate;if(!gate.held())return Status::Ok;
        Status s=Status::Ok;
        if(gpio){s=executeGpio(msg,arg);if(s==Status::Busy)return Status::Ok;
          if(s!=Status::Ok)return fail(s);tapeRead_+=4;}
        else{
          engine_.i2c_address=op[3];uint8_t count=arg;void *data=nullptr;
          if(msg==NGFX_U8X8_MSG_BYTE_SEND){
            size_t available=byteBudget();if(consumed>=available)return Status::Ok;
            available-=consumed;const size_t remaining=size_t(arg)-sendOffset_;
            if(available>io_.maxBurstBytes())available=io_.maxBurstBytes();
            if(!available)return fail(Status::InvalidArgument);
            count=uint8_t(remaining<available?remaining:available);
            data=tape_+tapeRead_+4+sendOffset_;
          }
          executing_=true;
          // Restore upstream function-static state before a separately polled
          // SEND, so interleaved display objects cannot inherit each other.
          if(msg==NGFX_U8X8_MSG_BYTE_SEND){
            if(profile_.byte==ngfx_u8x8_byte_3wire_sw_spi)
              profile_.byte(&engine_,NGFX_U8X8_MSG_BYTE_SET_DC,lastDc_,nullptr);
            if(profile_.byte==ngfx_u8x8_byte_sed1520)
              profile_.byte(&engine_,NGFX_U8X8_MSG_BYTE_START_TRANSFER,lastStart_,nullptr);
          }
          s=io_.byte(&engine_,msg,count,data);
          if(s==Status::Unsupported&&profile_.interface!=U8g2Interface::Serial&&profile_.interface!=U8g2Interface::Custom){
            const uint8_t ok=profile_.byte(&engine_,msg,count,data);s=ok?Status::Ok:Status::Unsupported;}
          executing_=false;
          if(fault_!=Status::Ok)s=fault_;
          if(s==Status::Busy)return Status::Ok;
          if(s!=Status::Ok)return fail(s);
          if(msg==NGFX_U8X8_MSG_BYTE_SET_DC)lastDc_=arg;
          if(msg==NGFX_U8X8_MSG_BYTE_START_TRANSFER)lastStart_=arg;
          if(msg==NGFX_U8X8_MSG_BYTE_SEND){consumed+=count;sendOffset_+=count;
            if(sendOffset_==arg){tapeRead_+=4+arg;sendOffset_=0;}}
          else tapeRead_+=4;
        }
        if(s!=Status::Ok)return fail(s);
      }
      if(consumed>=byteBudget()||uint32_t(io_.nowMicros()-start)>=config_.pollBudgetMicros)return Status::Ok;
    }
    const Phase finished=phase_;clearTape();
    if(finished==Phase::Frame&&!frameRecordingDone_)return record(Status::Ok);
    phase_=Phase::None;
    if(finished==Phase::Frame)sentValid_=true;
    if(finished==Phase::Init){initialized_=true;
      // Initialization may contain several vendor-prescribed EPD clear cycles;
      // do not retain that rare high-water allocation for ordinary page updates.
      free(tape_);tape_=nullptr;tapeCapacity_=0;
    }
    return record(Status::Ok);
  }
 private:
  enum class Phase:uint8_t{None,Init,Frame,Control};
  static uint8_t recordByte(ngfx_u8x8_t *u,uint8_t msg,uint8_t arg,void *data){
    U8g2Panel *p=static_cast<U8g2Panel*>(u->user_ptr);
    return p&&p->appendOp(false,msg,arg,u->i2c_address,data)?1:0;
  }
  static uint8_t recordGpio(ngfx_u8x8_t *u,uint8_t msg,uint8_t arg,void *){
    U8g2Panel *p=static_cast<U8g2Panel*>(u->user_ptr);if(!p)return 0;
    if(p->capturing_)return p->appendOp(true,msg,arg,0,nullptr)?1:0;
    if(p->executing_){const Status s=p->executeGpio(msg,arg);if(s!=Status::Ok)p->fail(s);return s==Status::Ok?1:0;}
    return 0;
  }
  bool appendOp(bool gpio,uint8_t msg,uint8_t arg,uint8_t address,void *data){
    if(fault_!=Status::Ok)return false;
    if(!gpio&&msg==NGFX_U8X8_MSG_BYTE_SET_DC){
      if(recordDcValid_&&recordDc_==arg)return true;
      recordDc_=arg;recordDcValid_=true;
    }else if((!gpio&&msg==NGFX_U8X8_MSG_BYTE_INIT)||
      (gpio&&msg==NGFX_U8X8_MSG_GPIO_DC))recordDcValid_=false;
    const size_t payload=!gpio&&msg==NGFX_U8X8_MSG_BYTE_SEND?arg:0;
    if(payload&&!data){fail(Status::InvalidArgument);return false;}
    if(payload&&lastOpValid_){uint8_t *prior=tape_+lastOp_;
      if(!prior[0]&&prior[1]==NGFX_U8X8_MSG_BYTE_SEND&&prior[3]==address&&unsigned(prior[2])+arg<=255){
        if(!reserveTape(payload))return false;
        prior=tape_+lastOp_;prior[2]=uint8_t(prior[2]+arg);memcpy(tape_+tapeSize_,data,payload);
        tapeSize_+=payload;return true;
      }
    }
    const size_t add=4+payload;
    if(!reserveTape(add))return false;
    uint8_t *op=tape_+tapeSize_;op[0]=gpio?1:0;op[1]=msg;op[2]=arg;op[3]=address;
    lastOp_=tapeSize_;lastOpValid_=true;
    if(payload)memcpy(op+4,data,payload);tapeSize_+=add;return true;
  }
  bool reserveTape(size_t add){
    if(add>config_.maxTapeBytes||tapeSize_>config_.maxTapeBytes-add){fail(Status::NoMemory);return false;}
    if(tapeSize_+add>tapeCapacity_){size_t capacity=tapeCapacity_?tapeCapacity_:256;
      while(capacity<tapeSize_+add){if(capacity>config_.maxTapeBytes/2){capacity=config_.maxTapeBytes;break;}capacity*=2;}
      if(capacity>config_.maxTapeBytes)capacity=config_.maxTapeBytes;
      void *next=realloc(tape_,capacity);if(!next){fail(Status::NoMemory);return false;}
      tape_=static_cast<uint8_t*>(next);tapeCapacity_=capacity;
    }
    return true;
  }
  void clearTape(){tapeSize_=0;tapeRead_=0;sendOffset_=0;waiting_=false;lastOpValid_=false;recordDcValid_=false;}
  void startFrame(){
    memset(changedPages_,0,sizeof(changedPages_));
    const unsigned pages=engine_.display_info->tile_height;
    for(unsigned page=0;page<pages;++page){const size_t at=size_t(page)*nativeWidth_;
      if(!sentValid_||profile_.slowRefresh||profile_.fullFrameOnly||
        (at<bytes_&&memcmp(frame_+at,sent_+at,nativeWidth_)))changedPages_[page/8]|=uint8_t(1U<<(page&7));}
    memcpy(sent_,frame_,bytes_);sentValid_=false;dirty_=false;pending_=false;
    nextPage_=0;frameRecordingDone_=false;phase_=Phase::Frame;clearTape();
  }
  Status recordFrameChunk(){
    detail::U8g2Gate gate;if(!gate.held())return Status::Ok;
    clearTape();capturing_=true;const unsigned tw=engine_.display_info->tile_width;
    const unsigned pages=engine_.display_info->tile_height;
    while(nextPage_<pages&&!(changedPages_[nextPage_/8]&(1U<<(nextPage_&7))))++nextPage_;
    if(nextPage_<pages){const unsigned page=nextPage_++;
      memset(tileRow_,0,size_t(tw)*8);
      for(unsigned x=0;x<nativeWidth_;++x){const unsigned y0=page*8;
        uint8_t value=y0<nativeHeight_?sent_[size_t(page)*nativeWidth_+x]:0;
        if(y0<nativeHeight_&&profile_.inputOneIsBlack)value=uint8_t(~value);
        if(y0<nativeHeight_&&nativeHeight_-y0<8)value&=uint8_t((1U<<(nativeHeight_-y0))-1);
        if(profile_.horizontalTiles){for(unsigned bit=0;bit<8;++bit)if(value&(1U<<bit))
          tileRow_[size_t(bit)*tw+x/8]|=uint8_t(0x80U>>(x&7));}
        else tileRow_[x]=value;
      }
      if(!ngfx_u8x8_DrawTile(&engine_,0,uint8_t(page),uint8_t(tw),tileRow_))fail(Status::Unsupported);
    }else{
      if(profile_.refreshCommand&&fault_==Status::Ok)
        engine_.display_cb(&engine_,NGFX_U8X8_MSG_DISPLAY_REFRESH,0,nullptr);
      frameRecordingDone_=true;
    }
    capturing_=false;
    if(fault_!=Status::Ok)return fault_;
    return Status::Ok;
  }
  Status control(uint8_t msg,uint8_t arg){
    if(!ready_)return record(Status::NotInitialized);
    if(fault_!=Status::Ok)return fault_;
    if(phase_!=Phase::None||pending_||frameOpen_)return record(Status::Busy);
    detail::U8g2Gate gate;if(!gate.held())return record(Status::Busy);
    clearTape();capturing_=true;const bool handled=engine_.display_cb(&engine_,msg,arg,nullptr)!=0;capturing_=false;
    if(fault_!=Status::Ok)return fault_;
    if(!handled){clearTape();return record(Status::Unsupported);}
    phase_=Phase::Control;return record(Status::Ok);
  }
  int pinFor(unsigned role)const{
    const U8g2Pins &p=config_.pins;
    if(role<8){if(profile_.interface==U8g2Interface::Spi4||profile_.interface==U8g2Interface::Spi3)
      return role==0?p.clock:role==1?p.mosi:-1;
      if(profile_.interface==U8g2Interface::Serial)return role==1?p.tx:-1;
      return p.data[role];}
    switch(role){case NGFX_U8X8_PIN_E:return p.enable;case NGFX_U8X8_PIN_CS:return p.cs;
      case NGFX_U8X8_PIN_DC:return p.dc;case NGFX_U8X8_PIN_RESET:return p.reset;
      case NGFX_U8X8_PIN_I2C_CLOCK:return p.scl;case NGFX_U8X8_PIN_I2C_DATA:return p.sda;
      case NGFX_U8X8_PIN_CS1:return p.cs1;case NGFX_U8X8_PIN_CS2:return p.cs2;default:return -1;}
  }
  Status validatePins()const{
    for(unsigned i=0;i<16;++i){const int pin=pinFor(i);
      if(pin<-1||((profile_.requiredPins&(uint32_t(1)<<i))&&pin<0)||
         (pin>=0&&!io_.validPin(pin)))return Status::InvalidArgument;
      if(pin>=0){const int physical=io_.physicalPin(pin);if(physical<0)return Status::InvalidArgument;
        for(unsigned j=0;j<i;++j){const int other=pinFor(j);
          if(other>=0&&physical==io_.physicalPin(other))return Status::InvalidArgument;}}
    }return Status::Ok;
  }
  Status executeGpio(uint8_t msg,uint8_t arg){
    if(isDelay(msg)){io_.delayNs(delayNanos(msg,arg));return Status::Ok;}
    if(msg==NGFX_U8X8_MSG_GPIO_AND_DELAY_INIT){
      for(unsigned i=0;i<16;++i){const int pin=pinFor(i);if(pin<0)continue;
        const Status s=io_.configure(pin,i==NGFX_U8X8_PIN_I2C_CLOCK||i==NGFX_U8X8_PIN_I2C_DATA);
        if(s!=Status::Ok)return s;}return Status::Ok;
    }
    if(msg>=NGFX_U8X8_MSG_GPIO(0)&&msg<NGFX_U8X8_MSG_GPIO(16)){
      const unsigned role=msg-NGFX_U8X8_MSG_GPIO(0);const int pin=pinFor(role);
      if(pin<0)return Status::Ok; // Optional CS/reset can be wired permanently.
      return io_.write(pin,arg!=0,role==NGFX_U8X8_PIN_I2C_CLOCK||role==NGFX_U8X8_PIN_I2C_DATA);
    }
    return Status::Unsupported;
  }
  static bool isDelay(uint8_t msg){return msg>=NGFX_U8X8_MSG_DELAY_MILLI&&msg<=NGFX_U8X8_MSG_DELAY_I2C;}
  uint32_t delayNanos(uint8_t msg,uint8_t arg)const{
    switch(msg){case NGFX_U8X8_MSG_DELAY_MILLI:return uint32_t(arg)*1000000UL;
      case NGFX_U8X8_MSG_DELAY_10MICRO:return uint32_t(arg)*10000UL;
      case NGFX_U8X8_MSG_DELAY_100NANO:return uint32_t(arg)*100UL;
      case NGFX_U8X8_MSG_DELAY_NANO:{
        if(config_.busClockHz&&(profile_.interface==U8g2Interface::Spi3||profile_.interface==U8g2Interface::Spi4)){
          const uint32_t half=(500000000UL+config_.busClockHz-1)/config_.busClockHz;
          return half>arg?half:arg;
        }return arg;}
      case NGFX_U8X8_MSG_DELAY_I2C:{const uint32_t hz=config_.busClockHz?config_.busClockHz:uint32_t(arg)*100000UL;
        return hz?(500000000UL+hz-1)/hz:5000;}
      default:return 0;}
  }
  size_t byteBudget()const{
    uint32_t hz=config_.busClockHz?config_.busClockHz:(profile_.interface==U8g2Interface::I2c?
      uint32_t(engine_.display_info->i2c_bus_clock_100kHz)*100000UL:engine_.display_info->sck_clock_hz);
    if(!hz)hz=100000;
    uint64_t timed=uint64_t(config_.pollBudgetMicros)*hz/(profile_.interface==U8g2Interface::I2c?9000000UL:8000000UL);
    if(!timed)timed=1;if(timed>config_.maxDataBytesPerPoll)timed=config_.maxDataBytesPerPoll;
    return size_t(timed);
  }
  Status fail(Status s){if(s!=Status::Ok&&s!=Status::Busy&&fault_==Status::Ok)fault_=s;
    if(s!=Status::Ok&&s!=Status::Busy){sentValid_=false;if(ioOpen_)io_.end();ioOpen_=false;}
    return record(s);}
  U8g2Profile profile_;U8g2Config config_;U8g2Io &io_;ngfx_u8x8_t engine_;
  uint8_t *tape_=nullptr,*tileRow_=nullptr;
  size_t tapeSize_=0,tapeCapacity_=0,tapeRead_=0,sendOffset_=0,lastOp_=0;
  uint8_t changedPages_[32]={0},nextPage_=0,recordDc_=0;
  bool lastOpValid_=false,recordDcValid_=false,frameRecordingDone_=false;
  Status fault_=Status::Ok;Phase phase_=Phase::None;
  bool capturing_=false,executing_=false,ioOpen_=false,initialized_=false,pending_=false,waiting_=false;
  uint32_t waitStart_=0,waitMs_=0,lastFrameStart_=0;bool hasFrameStart_=false;
  uint8_t lastDc_=0,lastStart_=0;
};
} // namespace nice_gfx
