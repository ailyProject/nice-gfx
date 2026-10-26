#pragma once
#include "../U8g2Types.h"
#include "../../Core/I2c.h"
#include <stdlib.h>
#include <string.h>

namespace nice_gfx {
// A raw SPI port keeps CS under the upstream CAD/BYTE protocol's control.
// Existing Transport::command()/data() cannot substitute for this interface:
// they may toggle CS and interpret command arguments differently.
class U8g2SpiPort {
 public:
  virtual ~U8g2SpiPort(){}
  virtual Status begin(int clock,int mosi,uint32_t hz,uint8_t mode)=0;
  virtual void end()=0;
  virtual Status start()=0;
  virtual Status send(const uint8_t *,size_t)=0;
  virtual Status stop()=0;
};

class U8g2DelegatingIo:public U8g2Io {
 public:
  explicit U8g2DelegatingIo(U8g2Io &gpio):gpio_(gpio){}
  bool validPin(int pin)const override{return gpio_.validPin(pin);}
  int physicalPin(int pin)const override{return gpio_.physicalPin(pin);}
  Status configure(int pin,bool drain)override{return gpio_.configure(pin,drain);}
  Status write(int pin,bool high,bool drain=false)override{return gpio_.write(pin,high,drain);}
  Status read(int pin,bool &value)override{return gpio_.read(pin,value);}
  void delayNs(uint32_t ns)override{gpio_.delayNs(ns);}
  uint32_t nowMicros()const override{return gpio_.nowMicros();}
 protected:U8g2Io &gpio_;
};

// Hardware acceleration for the ordinary four-wire SPI BYTE protocol only.
// Special 9-bit/parallel protocols retain their own upstream BYTE code.
// Reserve this SPI port/pin set exclusively while refreshBusy() is true: CAD
// may hold CS and the raw port's transaction across poll() calls. Never call a
// blocking touch/sensor driver on that same SPIClass during that interval.
class U8g2SpiIo:public U8g2DelegatingIo {
 public:
  U8g2SpiIo(U8g2Io &gpio,U8g2SpiPort &spi):U8g2DelegatingIo(gpio),spi_(spi){}
  size_t maxBurstBytes()const override{return 255;}
  Status begin(const U8g2Profile &p,const U8g2Config &c,const ngfx_u8x8_display_info_t &i)override{
    end();if(p.interface!=U8g2Interface::Spi4||p.byte!=ngfx_u8x8_byte_4wire_sw_spi)return Status::Unsupported;
    config_=c;info_=&i;
    Status s=gpio_.begin(p,c,i);if(s!=Status::Ok)return s;
    s=spi_.begin(c.pins.clock,c.pins.mosi,c.busClockHz?c.busClockHz:i.sck_clock_hz,i.spi_mode);
    if(s!=Status::Ok){gpio_.end();return s;}open_=true;return Status::Ok;
  }
  void end()override{
    if(active_){set(config_.pins.cs,info_?info_->chip_disable_level:1);spi_.stop();active_=false;}
    if(open_){spi_.end();gpio_.end();open_=false;}
  }
  Status configure(int pin,bool drain)override{
    if(pin==config_.pins.clock||pin==config_.pins.mosi)return Status::Ok;
    return gpio_.configure(pin,drain);
  }
  Status byte(ngfx_u8x8_t *,uint8_t msg,uint8_t arg,void *data)override{
    if(!open_)return Status::NotInitialized;
    switch(msg){
      case NGFX_U8X8_MSG_BYTE_INIT:return set(config_.pins.cs,info_->chip_disable_level);
      case NGFX_U8X8_MSG_BYTE_SET_DC:return set(config_.pins.dc,arg);
      case NGFX_U8X8_MSG_BYTE_START_TRANSFER:{
        if(active_)return Status::InvalidArgument;
        Status s=spi_.start();if(s!=Status::Ok)return s;active_=true;
        s=set(config_.pins.cs,info_->chip_enable_level);gpio_.delayNs(info_->post_chip_enable_wait_ns);return s;}
      case NGFX_U8X8_MSG_BYTE_SEND:return active_?spi_.send(static_cast<uint8_t*>(data),arg):Status::InvalidArgument;
      case NGFX_U8X8_MSG_BYTE_END_TRANSFER:{
        if(!active_)return Status::InvalidArgument;
        gpio_.delayNs(info_->pre_chip_disable_wait_ns);Status s=set(config_.pins.cs,info_->chip_disable_level);
        const Status stopped=spi_.stop();active_=false;return s==Status::Ok?stopped:s;}
      default:return Status::Unsupported;
    }
  }
 private:
  Status set(int pin,bool high){return pin<0?Status::Ok:gpio_.write(pin,high);}
  U8g2SpiPort &spi_;U8g2Config config_;const ngfx_u8x8_display_info_t *info_=nullptr;
  bool open_=false,active_=false;
};

// Collect exactly one upstream START..END transaction, including its control
// bytes, then pass it to a real ACK-reporting I2cBus. No inserted STOP/START or
// guessed per-controller framing. Owns no bus: configure its pins/frequency as
// usual, then set config.io=&adapter. maxTransaction is a bounded RAM budget.
class U8g2I2cIo:public U8g2DelegatingIo {
 public:
  U8g2I2cIo(U8g2Io &gpio,I2cBus &bus,size_t maxTransaction=4096)
    :U8g2DelegatingIo(gpio),bus_(bus),limit_(maxTransaction){}
  size_t maxBurstBytes()const override{return 255;}
  ~U8g2I2cIo()override{free(buffer_);}
  Status begin(const U8g2Profile &p,const U8g2Config &c,const ngfx_u8x8_display_info_t &i)override{
    if(p.interface!=U8g2Interface::I2c||!limit_)return Status::Unsupported;
    config_=c;size_=0;active_=false;return gpio_.begin(p,c,i);
  }
  void end()override{size_=0;active_=false;gpio_.end();}
  Status configure(int pin,bool drain)override{
    if(pin==config_.pins.scl||pin==config_.pins.sda)return Status::Ok;
    return gpio_.configure(pin,drain);
  }
  Status byte(ngfx_u8x8_t *u,uint8_t msg,uint8_t arg,void *data)override{
    switch(msg){
      case NGFX_U8X8_MSG_BYTE_INIT:case NGFX_U8X8_MSG_BYTE_SET_DC:return Status::Ok;
      case NGFX_U8X8_MSG_BYTE_START_TRANSFER:{
        if(active_)return Status::InvalidArgument;address_=u->i2c_address>>1;size_=0;
        const Status s=bus_.begin(address_);if(s==Status::Ok)active_=true;return s;}
      case NGFX_U8X8_MSG_BYTE_SEND:{
        if(!active_||(arg&&!data))return Status::InvalidArgument;
        if(arg>limit_||size_>limit_-arg)return Status::NoMemory;
        if(size_+arg>capacity_){size_t next=capacity_?capacity_:32;
          while(next<size_+arg){if(next>limit_/2){next=limit_;break;}next*=2;}
          if(next>limit_)next=limit_;void *memory=realloc(buffer_,next);
          if(!memory)return Status::NoMemory;buffer_=static_cast<uint8_t*>(memory);capacity_=next;}
        if(arg)memcpy(buffer_+size_,data,arg);size_+=arg;return Status::Ok;}
      case NGFX_U8X8_MSG_BYTE_END_TRANSFER:{
        if(!active_)return Status::InvalidArgument;active_=false;
        return size_?bus_.transfer(address_,buffer_,size_,nullptr,0,true):Status::Ok;}
      default:return Status::Unsupported;
    }
  }
 private:
  I2cBus &bus_;size_t limit_,size_=0,capacity_=0;uint8_t *buffer_=nullptr,address_=0;
  U8g2Config config_;bool active_=false;
};
} // namespace nice_gfx
