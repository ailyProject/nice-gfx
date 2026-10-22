#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "../examples/07_UI/Waveshare4B/TCA9554.h"
#include <Drivers/Display/Video/ST7701_RGB.h>
#include <Drivers/Touch/GT911.h>
using namespace nice_gfx;
struct Bus : TouchI2cBus {
  uint8_t output=0xFF,dir=0xFF,flags=0; bool fail=false; std::vector<uint8_t> edges;
  Status begin(uint8_t) override { return Status::Ok; }
  Status transfer(uint8_t address,const uint8_t *tx,size_t nt,uint8_t *rx,size_t nr,bool=true) override {
    if(fail)return Status::IoError;
    if(address==0x20){
      if(nt==1&&nr==1){rx[0]=tx[0]==1?output:dir;return Status::Ok;}
      if(nt==2&&nr==0){if(tx[0]==1){if(!(output&4)&&(tx[1]&4)&&!(tx[1]&1))edges.push_back((tx[1]>>1)&1);output=tx[1];}else if(tx[0]==3)dir=tx[1];else assert(false);return Status::Ok;}
    }
    if(address==0x5D){
      if(nt==2&&nr==1){assert(tx[0]==0x81&&tx[1]==0x4E);rx[0]=flags;return Status::Ok;}
      if(nt==2&&nr==8){const uint8_t p[]={0,100,0,200,0,4,0,0};for(unsigned i=0;i<8;i++)rx[i]=p[i];return Status::Ok;}
      if(nt==3){assert(tx[0]==0x81&&tx[1]==0x4E&&tx[2]==0);flags=0;return Status::Ok;}
    }
    assert(false);return Status::IoError;
  }
};
int main(){
  bool sawDpiMode=false;
  for(const auto &c:video::ST7701_RGB_Type1_Init)if(c.command==0x3A){
    assert(c.dataBytes==1&&c.data[0]==0x60);sawDpiMode=true;
  }
  assert(sawDpiMode);
  Bus bus;waveshare4b::Tca9554CommandBus commands(bus);
  assert(commands.command(0,nullptr,0)==Status::NotInitialized);
  assert(commands.begin()==Status::Ok);assert((bus.dir&7)==0);assert((bus.dir&0xF8)==0xF8);
  assert((bus.output&0xF8)==0xF8);assert((bus.output&7)==1);
  const auto commandWrite=waveshare4b::Tca9554CommandBus::write;
  assert(commandWrite(0x3A,nullptr,0,nullptr)==Status::InvalidArgument);
  const uint8_t data[]={0x55,0xA3};assert(commandWrite(0x3A,data,2,&commands)==Status::Ok);
  assert(bus.edges.size()==27);
  const unsigned values[]={0x03A,0x155,0x1A3};
  for(unsigned i=0;i<3;i++){unsigned word=0;for(unsigned bit=0;bit<9;bit++)word=(word<<1)|bus.edges[i*9+bit];assert(word==values[i]);}
  assert(bus.output&1);assert((bus.output&0xF8)==0xF8);
  bus.fail=true;assert(commandWrite(0x11,nullptr,0,&commands)==Status::IoError);bus.fail=false;
  Gt911 touch(bus,480,480);assert(touch.begin()==Status::Ok);RawTouch p{};
  bus.flags=0x81;assert(touch.read(p));assert(p.x==100&&p.y==200&&p.pressure==4);
  assert(touch.read(p)); // Not-ready report holds an active contact.
  bus.flags=0x80;assert(!touch.read(p));assert(touch.status()==Status::Ok);
  bus.flags=0x86;assert(!touch.read(p));assert(touch.status()==Status::IoError);assert(bus.flags==0);
  bus.fail=true;assert(!touch.read(p));assert(touch.status()==Status::IoError);
  std::puts("PASS 4B command transport: 9-bit mode0 frames, latch preservation, errors; GT911 ready/hold/release/invalid/IO error.");
}
