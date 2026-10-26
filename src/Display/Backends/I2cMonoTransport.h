#pragma once
#include "../Transport.h"
#include "../../Core/I2c.h"

namespace nice_gfx {

// Portable SSD1306/SH1106 I2C framing. The default packet is 32 bytes including
// its control byte, fitting small Arduino Wire buffers. Larger buses may opt in
// to payloads up to 128 bytes; no transfer ever borrows caller memory.
class I2cMonoTransport : public Transport {
 public:
  using Output = void (*)(void *,int,bool);
  using OutputValid = bool (*)(int);
  I2cMonoTransport(I2cBus &bus,uint8_t address=0x3C,uint8_t payload=31,
                   Output output=nullptr,void *context=nullptr,OutputValid valid=nullptr)
      :bus_(bus),address_(address),payload_(payload),output_(output),context_(context),valid_(valid){}
  Status begin() override {
    if(address_<8||address_>0x77||!payload_||payload_>128)return Status::InvalidArgument;
    const Status s=bus_.begin(address_);ready_=s==Status::Ok;return s;
  }
  void end() override {ready_=false;}
  Status command(uint8_t value,const uint8_t *data,size_t bytes) override {
    if(bytes&&!data)return Status::InvalidArgument;
    Status s=commands(&value,1);return s==Status::Ok?commands(data,bytes):s;
  }
  Status commands(const uint8_t *data,size_t bytes) override {return send(0,data,bytes);}
  Status data(const uint8_t *data,size_t bytes) override {return send(0x40,data,bytes);}
  Status pixels(uint8_t,PixelView,bool) override {return Status::Unsupported;}
  Status fill(uint8_t,uint16_t,size_t) override {return Status::Unsupported;}
  Status wait() override {return Status::Ok;}
  bool busy() const override {return false;}
  bool asynchronous() const override {return false;}
  bool validOutputPin(int pin) const override {return output_&&pin>=0&&(!valid_||valid_(pin));}
  void output(int pin,bool high) override {if(output_)output_(context_,pin,high);}
  void delayMs(uint32_t ms) override {while(ms){const uint16_t n=ms>65535?65535:uint16_t(ms);bus_.delayMs(n);ms-=n;}}
  uint8_t address()const{return address_;}
 private:
  Status send(uint8_t control,const uint8_t *data,size_t bytes){
    if(!ready_)return Status::NotInitialized;if(bytes&&!data)return Status::InvalidArgument;
    uint8_t packet[129];packet[0]=control;
    while(bytes){const size_t n=bytes<payload_?bytes:payload_;memcpy(packet+1,data,n);
      const Status s=bus_.transfer(address_,packet,n+1,nullptr,0);if(s!=Status::Ok)return s;data+=n;bytes-=n;}
    return Status::Ok;
  }
  I2cBus &bus_;uint8_t address_,payload_;Output output_;void *context_;OutputValid valid_;bool ready_=false;
};

} // namespace nice_gfx
