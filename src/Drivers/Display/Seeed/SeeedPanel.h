#pragma once
#include "../GxEpd2/NgfxGxEpd2_EPD.h"
#include "Driver.h"
#include "Driver_SSD1677.h"
#include "../../../Platform/Common/UiMemory.h"
namespace nice_gfx { namespace seeed_protocol {
// Bridge the vendor bus to the existing instance-local command recorder/worker.
class HalBus:public IBus {
 public:
  HalBus(int cs,int dc,bool dual):cs_(cs),dc_(dc),dual_(dual){}
  bool begin()override{return true;} void end()override{}
  void beginWrite()override{if(depth_++==0)select(true);}
  void endWrite()override{if(depth_&&--depth_==0){hal()->spiFlush();select(false);}}
  void beginRead()override{beginWrite();} void endRead()override{endWrite();}
  void writeCommand(uint8_t c)override{
    const bool own=!depth_;if(own)beginWrite();
    if(dc_>=0)hal()->writePin(dc_,false);hal()->spiWrite(c);hal()->spiFlush();
    if(dc_>=0)hal()->writePin(dc_,true);if(own)endWrite();
  }
  void writeData(uint8_t value)override{
    if(depth_)hal()->spiWrite(value);else hal()->spiByteTransaction(cs_,mask_,value);
  }
  void writeData(const uint8_t*p,size_t n)override{beginWrite();for(size_t i=0;i<n;++i)hal()->spiWrite(p[i]);endWrite();}
  uint8_t readData()override{return hal()->spiTransfer(0);}
  void setFrequency(uint32_t hz)override{hz_=hz;hal()->spiSettings(hz,0);}
  uint32_t frequency()const override{return hz_;}
  bool supportsSecondaryChipSelect()const override{return dual_;}
  void selectChip(ChipSelectTarget target)override{
    if(depth_){hal()->fault(Status::InvalidArgument);return;}
    mask_=target==ChipSelectTarget::Both?3:target==ChipSelectTarget::Secondary?2:1;
  }
  BusCapabilities capabilities()const override{BusCapabilities c;c.readable=true;return c;}
 private:
  static GxEpd2Hal*hal(){return gxEpd2CurrentHal();}
  void select(bool active){if(mask_==1)hal()->writePin(cs_,!active);else hal()->selectCs(mask_,active);}
  int cs_,dc_;bool dual_;unsigned depth_=0;uint8_t mask_=1;uint32_t hz_=4000000;
};
inline void configureDriver(IDriver&){}
inline void configureDriver(Driver_SSD1677&d){d.setInvertMono(true);}

template<class Driver,uint16_t W,uint16_t H,uint8_t Bits,bool BusyHigh,bool Dual=false>
class Panel:public NgfxGxEpd2_EPD {
 public:
  Panel(int16_t cs,int16_t dc,int16_t rst,int16_t busy)
    :NgfxGxEpd2_EPD(cs,dc,rst,busy,BusyHigh,180000000UL,W,H,static_cast<NgfxGxEpd2::Panel>(0),Bits!=1,false,false),
     driver_(W,H),bus_(cs,dc,Dual){driver_.setBusyPin(busy);driver_.setResetPin(rst);configureDriver(driver_);}
  ~Panel()override{platform::freeUiMemory(converted_);platform::freeUiMemory(previous_);}
  void init(uint32_t hz=0)override{init(hz,true,10,false);}
  void init(uint32_t hz,bool,uint16_t=10,bool=false)override{
    ready_=false;written_=false;
    if(auto*h=gxEpd2CurrentHal())h->spiEnable(true);
    _ngfxPinMode(_cs,NGFX_GXEPD2_OUTPUT);_ngfxDigitalWrite(_cs,1);
    if(Dual){const int cs2=gxEpd2CurrentHal()->extraCsPin(0);if(cs2<0){fault(Status::InvalidArgument);return;}_ngfxPinMode(cs2,NGFX_GXEPD2_OUTPUT);_ngfxDigitalWrite(cs2,1);}
    if(_dc>=0){_ngfxPinMode(_dc,NGFX_GXEPD2_OUTPUT);_ngfxDigitalWrite(_dc,1);}
    bus_.setFrequency(hz?hz:4000000);driver_.clearOperationError();
    if(!driver_.init(bus_)){fault(Status::IoError);return;}check();ready_=true;
  }
  void writeImage(const uint8_t*p,int16_t x,int16_t y,int16_t w,int16_t h,bool inv=false,bool mirror=false,bool pgm=false)override{
    if(Bits!=1||inv||mirror||pgm||!full(p,x,y,w,h)){unsupported();return;}
    if(!prepare())return;
    const size_t bytes=size_t(W/8)*H;
    if(!previous_){previous_=static_cast<uint8_t*>(platform::allocateRetained(bytes));if(!previous_){fault(Status::NoMemory);return;}memset(previous_,0xff,bytes);}
    driver_.setAddrWindow(0,0,W-1,H-1);driver_.pushOldColors(previous_,bytes);driver_.pushNewColors(p,bytes);
    memcpy(previous_,p,bytes);check();written_=true;
  }
  // NICE_GFX packed palette indices -> Seeed's RGB332-derived nibble codes.
  void writeNative(const uint8_t*p,const uint8_t*,int16_t x,int16_t y,int16_t w,int16_t h,bool inv=false,bool mirror=false,bool pgm=false){
    if(Bits==1||inv||mirror||pgm||!full(p,x,y,w,h)){unsupported();return;}
    const size_t bytes=size_t(W/2)*H;
    if(!converted_){converted_=static_cast<uint8_t*>(platform::allocateRetained(bytes));if(!converted_){fault(Status::NoMemory);return;}}
    static const uint8_t codes[]={15,0,11,6,13,2};
    for(size_t i=0;i<bytes;++i){
      const uint8_t a=Bits==2?uint8_t((p[i/2]>>(i%2?2:6))&3):p[i]>>4;
      const uint8_t b=Bits==2?uint8_t((p[i/2]>>(i%2?0:4))&3):p[i]&15;
      if(a>=(Bits==2?4:6)||b>=(Bits==2?4:6)){fault(Status::InvalidArgument);return;}
      converted_[i]=uint8_t((codes[a]<<4)|codes[b]);
    }
    if(!prepare())return;driver_.setAddrWindow(0,0,W-1,H-1);driver_.pushNewColors(converted_,bytes);check();written_=true;
  }
  void refresh(bool partial=false)override{if(partial||!written_){unsupported();return;}driver_.update();check();written_=false;}
  void powerOff()override{if(ready_){driver_.sleep();check();ready_=false;}}
  void hibernate()override{powerOff();}
 private:
  static void fault(Status s){if(auto*h=gxEpd2CurrentHal())h->fault(s);}
  bool full(const uint8_t*p,int16_t x,int16_t y,int16_t w,int16_t h)const{return p&&x==0&&y==0&&w==W&&h==H;}
  bool prepare(){if(!ready_)init();return ready_;}
  void check(){if(driver_.lastOperationError()!=DriverOperationError::None)fault(Status::IoError);}
  Driver driver_;HalBus bus_;bool ready_=false,written_=false;uint8_t*converted_=nullptr,*previous_=nullptr;
};
} }
