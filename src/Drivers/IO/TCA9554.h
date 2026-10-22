#pragma once
#include "../../Core/I2c.h"

// Reusable expander and optional command transport; board wiring lives in presets.
namespace nice_gfx {

// TCA9554 eight-bit GPIO expander. Cached read/modify/write operations preserve
// unrelated pins; state changes only after a successful I2C transaction.
class Tca9554 {
 public:
  explicit Tca9554(I2cBus &bus,uint8_t address=0x20):bus_(bus),address_(address){}
  Status begin() {
    if(address_<0x20||address_>0x27)return Status::InvalidArgument;
    Status s=bus_.begin(address_);if(s!=Status::Ok)return s;
    if((s=readRegister(1,output_))!=Status::Ok)return s;
    return readRegister(3,direction_);
  }
  Status level(uint8_t mask,bool high) {return output(high?uint8_t(output_|mask):uint8_t(output_&~mask));}
  Status direction(uint8_t mask,bool input) {
    const uint8_t next=input?uint8_t(direction_|mask):uint8_t(direction_&~mask);
    const Status s=writeRegister(3,next);if(s==Status::Ok)direction_=next;return s;
  }
  Status input(uint8_t &value) {return readRegister(0,value);}
 protected:
  Status output(uint8_t value) {const Status s=writeRegister(1,value);if(s==Status::Ok)output_=value;return s;}
  Status readRegister(uint8_t reg,uint8_t &value) {return bus_.transfer(address_,&reg,1,&value,1);}
  Status writeRegister(uint8_t reg,uint8_t value) {const uint8_t data[]={reg,value};return bus_.transfer(address_,data,2,nullptr,0);}
  I2cBus &bus_;uint8_t address_,output_=0xff,direction_=0xff;
};
// Optional 9-bit mode-0 command SPI through configurable expander pins. No LCD
// controller, board pin assignments, touch reset or backlight is embedded here.
class Tca9554CommandBus : public Tca9554 {
 public:
  explicit Tca9554CommandBus(I2cBus &bus,uint8_t address=0x20,uint8_t cs=0,uint8_t data=1,uint8_t clock=2)
    :Tca9554(bus,address),csPin_(cs),dataPin_(data),clockPin_(clock){}
  Status begin() {
    ready_=false;
    if(csPin_>7||dataPin_>7||clockPin_>7||csPin_==dataPin_||csPin_==clockPin_||dataPin_==clockPin_)return Status::InvalidArgument;
    cs_=uint8_t(1u<<csPin_);data_=uint8_t(1u<<dataPin_);clock_=uint8_t(1u<<clockPin_);
    Status s=Tca9554::begin();if(s!=Status::Ok)return s;
    if((s=output(uint8_t((output_|cs_)&~(data_|clock_))))!=Status::Ok)return s;
    if((s=direction(uint8_t(cs_|data_|clock_),false))!=Status::Ok)return s;
    ready_=true;return Status::Ok;
  }
  Status command(uint8_t command,const uint8_t *data,size_t bytes) {
    if(!ready_)return Status::NotInitialized;if(bytes&&!data)return Status::InvalidArgument;
    Status s=level(cs_,false);if(s==Status::Ok)s=word(command,false);
    for(size_t i=0;i<bytes&&s==Status::Ok;++i)s=word(data[i],true);
    const Status idle=level(cs_,true);return s==Status::Ok?idle:s;
  }
  static Status write(uint8_t command,const uint8_t *data,size_t bytes,void *context) {
    return context?static_cast<Tca9554CommandBus *>(context)->command(command,data,bytes):Status::InvalidArgument;
  }
 private:
  Status word(uint8_t byte,bool data) {
    const uint16_t value=uint16_t(byte)|(data?0x100:0);
    for(int bit=8;bit>=0;--bit){Status s=level(data_,(value&(1u<<bit))!=0);if(s!=Status::Ok)return s;
      if((s=level(clock_,true))!=Status::Ok)return s;if((s=level(clock_,false))!=Status::Ok)return s;}
    return Status::Ok;
  }
  uint8_t csPin_,dataPin_,clockPin_,cs_=0,data_=0,clock_=0;bool ready_=false;
};
} // namespace nice_gfx
