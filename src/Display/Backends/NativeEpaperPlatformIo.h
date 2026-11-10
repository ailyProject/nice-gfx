#pragma once
#include "../../Core/BuildConfig.h"
#include "../../Drivers/Display/Epaper/NativeTypes.h"
#include <new>

#if NICE_GFX_NATIVE_IDF
#include "../../Platform/Common/BusTypes.h"
#else
#include "../../Platform/Arduino/Bus.h"
#endif
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include "../../Platform/ESP32/Esp32Spi.h"
#include <driver/gpio.h>
#include <esp_timer.h>
#elif defined(ARDUINO_ARCH_STM32) && !defined(USE_HALV2_DRIVER)
#include "../../Platform/STM32/Stm32Spi.h"
#elif defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
#include "../../Platform/RP2040/Rp2040Pio.h"
#endif

namespace nice_gfx {
namespace detail {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
using NativeEpaperSpiBus=Esp32Spi;
#elif defined(ARDUINO_ARCH_STM32) && !defined(USE_HALV2_DRIVER)
using NativeEpaperSpiBus=Stm32Spi;
#elif defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
using NativeEpaperSpiBus=Rp2040PioSpi;
#else
using NativeEpaperSpiBus=ArduinoSpi;
#endif
} // namespace detail

// One owned bus serves every controller on a panel. The bus has no hardware
// CS: each synchronous span explicitly selects its destination. This avoids
// claiming a second SPI host (or a second owner of Arduino's SPI object) for
// a second controller wired to the same clock and data pins.
class NativeEpaperPlatformIo:public NativeEpaperIo {
 public:
  NativeEpaperPlatformIo()=default;
  ~NativeEpaperPlatformIo() override {end();}
  NativeEpaperPlatformIo(const NativeEpaperPlatformIo&)=delete;
  NativeEpaperPlatformIo &operator=(const NativeEpaperPlatformIo&)=delete;

  Status begin(const NativeEpaperProfile &profile,const NativeEpaperConfig &config) override {
    end();
    framed_=profile.controller==NativeEpaperController::IT8951;
    if(!validConfiguration(profile,config))return Status::InvalidArgument;
    pins_=config.pins;chips_=profile.chipCount;
    SpiConfig spi(SpiPins(pins_.clock,pins_.mosi,pins_.miso,-1,pins_.dc));
    spi.frequency=config.busClockHz;spi.readFrequency=config.busClockHz;
    spi.mode=0;spi.byteOnly=true;
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM) || \
    (defined(ARDUINO_ARCH_STM32) && !defined(USE_HALV2_DRIVER)) || \
    (defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED))
    bus_=new(std::nothrow) detail::NativeEpaperSpiBus(spi);
#else
    bus_=new(std::nothrow) detail::NativeEpaperSpiBus(SPI,spi);
#endif
    if(!bus_){chips_=0;return Status::NoMemory;}
    // Deassert every CS before routing the peripheral clock. Initialization
    // does not pulse RESET or wait for BUSY; the backend schedules those steps.
    Status s=setOutput(pins_.cs,true);
    if(s==Status::Ok&&chips_==2)s=setOutput(pins_.extraCs[0],true);
    if(s==Status::Ok&&pins_.reset>=0)s=setOutput(pins_.reset,true);
    if(s==Status::Ok)s=setInput(pins_.busy);
    if(s==Status::Ok)s=bus_->begin();
    if(s!=Status::Ok){end();return s;}
    ready_=true;return Status::Ok;
  }

  void end() override {
    if(bus_){
      if(chips_){writeOutput(pins_.cs,true);if(chips_==2)writeOutput(pins_.extraCs[0],true);}
      bus_->wait();bus_->end();delete bus_;bus_=nullptr;
    }
    ready_=false;chips_=0;packet_=Packet::None;
  }
  Status reset(bool high) override {
    if(!ready_)return Status::NotInitialized;
    return pins_.reset<0?(framed_?Status::Ok:Status::Unsupported):setOutput(pins_.reset,high);
  }
  Status busy(bool &level) override {
    if(!ready_)return Status::NotInitialized;
    return readInput(pins_.busy,level);
  }
  Status command(uint8_t command,const uint8_t *data,size_t count,uint8_t chip=0) override {
    if(!ready_)return Status::NotInitialized;
    if(framed_)return Status::Unsupported;
    if(chip>=chips_||(count&&!data))return Status::InvalidArgument;
    Status s=select(chip,false);
    if(s==Status::Ok)s=bus_->command(command,data,count);
    const Status released=select(chip,true);
    return s==Status::Ok?released:s;
  }
  Status data(const uint8_t *data,size_t count,uint8_t chip=0) override {
    if(!ready_)return Status::NotInitialized;
    if(framed_)return Status::Unsupported;
    if(chip>=chips_||(count&&!data))return Status::InvalidArgument;
    if(!count)return Status::Ok;
    Status s=select(chip,false);
    if(s==Status::Ok)s=bus_->data(data,count);
    const Status released=select(chip,true);
    return s==Status::Ok?released:s;
  }
  Status writeWords(uint16_t preamble,const uint16_t *words,size_t count) override {
    if(!ready_)return Status::NotInitialized;
    if(!framed_)return Status::Unsupported;
    if(!words||!count||count>128||(preamble!=0x6000&&preamble!=0))return Status::InvalidArgument;
    if(packet_!=Packet::None&&(packet_!=Packet::Write||preamble_!=preamble||wordCount_!=count))return Status::InvalidArgument;
    bool level=false;Status s=readInput(pins_.busy,level);if(s!=Status::Ok)return abortPacket(s);
    if(!level)return Status::Busy;
    if(packet_==Packet::None){
      s=select(0,false);if(s==Status::Ok)s=bus_->command16(preamble,nullptr,0);
      if(s!=Status::Ok)return abortPacket(s);
      packet_=Packet::Write;preamble_=preamble;wordCount_=count;
      s=readInput(pins_.busy,level);if(s!=Status::Ok)return abortPacket(s);
      if(!level)return Status::Busy;
    }
    uint8_t wire[256];for(size_t i=0;i<count;++i){wire[2*i]=uint8_t(words[i]>>8);wire[2*i+1]=uint8_t(words[i]);}
    s=bus_->data(wire,count*2);return abortPacket(s);
  }
  Status readWords(uint16_t *words,size_t count) override {
    if(!ready_)return Status::NotInitialized;
    if(!framed_)return Status::Unsupported;
    if(!words||!count||count>128)return Status::InvalidArgument;
    if(packet_!=Packet::None&&(packet_==Packet::Write||wordCount_!=count))return Status::InvalidArgument;
    bool level=false;Status s=readInput(pins_.busy,level);if(s!=Status::Ok)return abortPacket(s);
    if(!level)return Status::Busy;
    if(packet_==Packet::None){
      s=select(0,false);if(s==Status::Ok)s=bus_->command16(0x1000,nullptr,0);
      if(s!=Status::Ok)return abortPacket(s);
      wordCount_=count;packet_=Packet::ReadDummy;
      s=readInput(pins_.busy,level);if(s!=Status::Ok)return abortPacket(s);
      if(!level)return Status::Busy;
    }
    uint8_t wire[256];
    if(packet_==Packet::ReadDummy){
      s=bus_->read(0,wire,2,0,0);if(s!=Status::Ok)return abortPacket(s);
      packet_=Packet::ReadPayload;
      s=readInput(pins_.busy,level);if(s!=Status::Ok)return abortPacket(s);
      if(!level)return Status::Busy;
    }
    s=bus_->read(0,wire,count*2,0,0);
    if(s==Status::Ok)for(size_t i=0;i<count;++i)words[i]=uint16_t(uint16_t(wire[2*i])<<8|wire[2*i+1]);
    return abortPacket(s);
  }
  uint32_t nowMicros() const override {
#if NICE_GFX_NATIVE_IDF
    return uint32_t(esp_timer_get_time());
#else
    return uint32_t(micros());
#endif
  }

 protected:
  static int physicalPin(int pin) {
#if NICE_GFX_NATIVE_IDF
    return pin;
#else
    return FastPin::gpioNumber(pin);
#endif
  }
  static bool validPin(int pin,bool input=false) {
#if NICE_GFX_NATIVE_IDF
    return input?GPIO_IS_VALID_GPIO(pin):GPIO_IS_VALID_OUTPUT_GPIO(pin);
#else
    return input?FastPin::validInput(pin):FastPin::valid(pin);
#endif
  }
  static Status setOutput(int pin,bool high) {
    if(!validPin(pin))return Status::InvalidArgument;
#if NICE_GFX_NATIVE_IDF
    if(gpio_set_level(gpio_num_t(pin),high?1:0)!=ESP_OK)return Status::IoError;
    return gpio_set_direction(gpio_num_t(pin),GPIO_MODE_OUTPUT)==ESP_OK?Status::Ok:Status::IoError;
#else
    digitalWrite(pin,high?HIGH:LOW);pinMode(pin,OUTPUT);return Status::Ok;
#endif
  }
  static Status setInput(int pin) {
    if(!validPin(pin,true))return Status::InvalidArgument;
#if NICE_GFX_NATIVE_IDF
    return gpio_set_direction(gpio_num_t(pin),GPIO_MODE_INPUT)==ESP_OK?Status::Ok:Status::IoError;
#else
    pinMode(pin,INPUT);return Status::Ok;
#endif
  }
  static Status writeOutput(int pin,bool high) {
    if(!validPin(pin))return Status::InvalidArgument;
#if NICE_GFX_NATIVE_IDF
    return gpio_set_level(gpio_num_t(pin),high?1:0)==ESP_OK?Status::Ok:Status::IoError;
#else
    digitalWrite(pin,high?HIGH:LOW);return Status::Ok;
#endif
  }
  static Status readInput(int pin,bool &level) {
    if(!validPin(pin,true))return Status::InvalidArgument;
#if NICE_GFX_NATIVE_IDF
    level=gpio_get_level(gpio_num_t(pin))!=0;
#else
    level=digitalRead(pin)!=LOW;
#endif
    return Status::Ok;
  }

 private:
  static bool validConfiguration(const NativeEpaperProfile &p,const NativeEpaperConfig &c) {
    if((p.chipCount!=1&&p.chipCount!=2)||!c.busClockHz||c.busClockHz>p.maxClockHz)return false;
    const bool framed=p.controller==NativeEpaperController::IT8951;
    const NativeEpaperPins &n=c.pins;
    const int pins[]={n.clock,n.mosi,n.cs,n.dc,n.reset,n.busy,n.miso,n.extraCs[0]};
    if(n.clock<0||n.mosi<0||n.cs<0||(framed?n.dc!=-1:n.dc<0)||n.busy<0||n.reset<-1||n.miso<-1||
       (framed&&(n.miso<0||p.chipCount!=1))||
       (p.chipCount==2&&n.extraCs[0]<0)||n.extraCs[1]!=-1||n.extraCs[2]!=-1)return false;
    for(unsigned i=0;i<8;++i){
      if(i==7&&p.chipCount==1)continue;
      if(pins[i]<0)continue;
      if(!validPin(pins[i],i==5||i==6))return false;
      for(unsigned j=0;j<i;++j)if(pins[j]>=0&&physicalPin(pins[i])==physicalPin(pins[j]))return false;
    }
    return true;
  }
  Status select(uint8_t chip,bool high) {
    return writeOutput(chip?pins_.extraCs[0]:pins_.cs,high);
  }
  Status abortPacket(Status s){const Status released=select(0,true);packet_=Packet::None;
    return s==Status::Ok?released:s;}
  enum class Packet:uint8_t{None,Write,ReadDummy,ReadPayload};
  NativeEpaperPins pins_;
  detail::NativeEpaperSpiBus *bus_=nullptr;
  uint8_t chips_=0;
  bool ready_=false;
  bool framed_=false;
  Packet packet_=Packet::None;
  uint16_t preamble_=0;size_t wordCount_=0;
};
} // namespace nice_gfx
