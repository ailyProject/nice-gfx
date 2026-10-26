#pragma once
#include "U8g2BusIo.h"
#include "../../Core/Platform.h"
#if NICE_GFX_NATIVE_IDF
#include <driver/gpio.h>
#include <esp_rom_sys.h>
#else
#include "../../Platform/Arduino/FastPin.h"
#include <SPI.h>
#endif
#if defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
#include <driver/spi_master.h>
#include <esp_heap_caps.h>
#endif

namespace nice_gfx {
// Portable software GPIO fallback. Upstream software I2C does not observe ACK
// or clock stretching; use U8g2I2cIo with a hardware I2cBus when that is needed.
class U8g2PlatformIo:public U8g2Io {
 public:
  bool validPin(int pin)const override{
#if NICE_GFX_NATIVE_IDF
    return GPIO_IS_VALID_OUTPUT_GPIO(pin);
#else
    return FastPin::valid(pin);
#endif
  }
  int physicalPin(int pin)const override{
#if NICE_GFX_NATIVE_IDF
    return pin;
#else
    return FastPin::gpioNumber(pin);
#endif
  }
  Status configure(int pin,bool drain)override{
    if(!validPin(pin))return Status::InvalidArgument;
#if NICE_GFX_NATIVE_IDF
    if(gpio_set_direction(gpio_num_t(pin),drain?GPIO_MODE_INPUT_OUTPUT_OD:GPIO_MODE_OUTPUT)!=ESP_OK)return Status::IoError;
    if(drain&&gpio_set_pull_mode(gpio_num_t(pin),GPIO_PULLUP_ONLY)!=ESP_OK)return Status::IoError;
    return Status::Ok;
#else
    pinMode(pin,drain?INPUT_PULLUP:OUTPUT);return Status::Ok;
#endif
  }
  Status write(int pin,bool high,bool drain=false)override{
    if(!validPin(pin))return Status::InvalidArgument;
#if NICE_GFX_NATIVE_IDF
    (void)drain;return gpio_set_level(gpio_num_t(pin),high?1:0)==ESP_OK?Status::Ok:Status::IoError;
#else
    if(drain){if(high)pinMode(pin,INPUT_PULLUP);else{digitalWrite(pin,LOW);pinMode(pin,OUTPUT);}}
    else digitalWrite(pin,high?HIGH:LOW);return Status::Ok;
#endif
  }
  Status read(int pin,bool &value)override{
#if NICE_GFX_NATIVE_IDF
    if(!GPIO_IS_VALID_GPIO(pin))return Status::InvalidArgument;value=gpio_get_level(gpio_num_t(pin))!=0;
#else
    if(!FastPin::validInput(pin))return Status::InvalidArgument;value=digitalRead(pin)!=LOW;
#endif
    return Status::Ok;
  }
  void delayNs(uint32_t ns)override{
    // Round upward on portable platforms: never violate the controller's
    // minimum timing. Hardware SPI avoids these per-edge software waits.
    if(!ns)return;const uint32_t us=(ns+999)/1000;
#if NICE_GFX_NATIVE_IDF
    esp_rom_delay_us(us);
#else
    while(ns>=60000000UL){delayMicroseconds(60000);ns-=60000000UL;}
    if(ns)delayMicroseconds(uint16_t((ns+999)/1000));
    (void)us;
#endif
  }
  uint32_t nowMicros()const override{return platform::micros();}
};

#if !NICE_GFX_NATIVE_IDF
// A usable raw SPIClass bridge for hardware acceleration. On ESP32 the
// configured pins are routed explicitly. Other Arduino cores use the SPIClass
// instance's configured pins; select/remap them using that core's API first.
class U8g2ArduinoSpiPort:public U8g2SpiPort {
 public:
  explicit U8g2ArduinoSpiPort(SPIClass &spi):spi_(spi){}
  Status begin(int clock,int mosi,uint32_t hz,uint8_t mode)override{
    if(!hz||mode>3)return Status::InvalidArgument;
    hz_=hz;mode_=mode;
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    spi_.begin(clock,-1,mosi,-1);
#else
    (void)clock;(void)mosi;spi_.begin();
#endif
    return Status::Ok;
  }
  void end()override{if(active_)stop();}
  Status start()override{if(active_)return Status::Busy;
    const uint8_t modes[]={SPI_MODE0,SPI_MODE1,SPI_MODE2,SPI_MODE3};
    spi_.beginTransaction(SPISettings(hz_,MSBFIRST,modes[mode_]));active_=true;return Status::Ok;}
  Status send(const uint8_t *data,size_t bytes)override{
    if(!active_||(bytes&&!data))return Status::InvalidArgument;
    for(size_t i=0;i<bytes;++i)spi_.transfer(data[i]);return Status::Ok;
  }
  Status stop()override{if(active_){spi_.endTransaction();active_=false;}return Status::Ok;}
 private:SPIClass &spi_;uint32_t hz_=0;uint8_t mode_=0;bool active_=false;
};
#endif

#if defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
// Native SDK raw-byte port; manual chip select permits active-high modules and
// keeps one upstream transaction selected across successive poll() calls.
class U8g2Esp32SpiPort:public U8g2SpiPort {
 public:
  ~U8g2Esp32SpiPort()override{end();}
  Status begin(int clock,int mosi,uint32_t hz,uint8_t mode)override{
    end();if(!hz||hz>80000000UL||mode>3)return Status::InvalidArgument;
#if !NICE_GFX_NATIVE_IDF
    clock=FastPin::gpioNumber(clock);mosi=FastPin::gpioNumber(mosi);
#endif
    if(!GPIO_IS_VALID_OUTPUT_GPIO(clock)||!GPIO_IS_VALID_OUTPUT_GPIO(mosi)||clock==mosi)return Status::InvalidArgument;
    spi_bus_config_t bus={};bus.sclk_io_num=clock;bus.mosi_io_num=mosi;bus.miso_io_num=-1;
    bus.quadwp_io_num=-1;bus.quadhd_io_num=-1;disableExtra(bus,0);bus.max_transfer_sz=4096;
    host_=SPI2_HOST;esp_err_t error=spi_bus_initialize(host_,&bus,SPI_DMA_CH_AUTO);
#if !defined(SOC_SPI_PERIPH_NUM) || SOC_SPI_PERIPH_NUM>2
    if(error==ESP_ERR_INVALID_STATE){host_=SPI3_HOST;error=spi_bus_initialize(host_,&bus,SPI_DMA_CH_AUTO);}
#endif
    if(error!=ESP_OK)return error==ESP_ERR_NO_MEM?Status::NoMemory:Status::IoError;
    owns_=true;staging_=static_cast<uint8_t*>(heap_caps_malloc(256,MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL));
    if(!staging_){end();return Status::NoMemory;}
    spi_device_interface_config_t device={};device.mode=mode;device.clock_speed_hz=hz;
    device.spics_io_num=-1;device.queue_size=1;device.flags=SPI_DEVICE_HALFDUPLEX|SPI_DEVICE_NO_DUMMY;
    error=spi_bus_add_device(host_,&device,&device_);
    if(error!=ESP_OK){end();return error==ESP_ERR_NO_MEM?Status::NoMemory:Status::IoError;}return Status::Ok;
  }
  void end()override{if(active_)stop();if(device_){spi_bus_remove_device(device_);device_=nullptr;}
    if(owns_){spi_bus_free(host_);owns_=false;}free(staging_);staging_=nullptr;}
  Status start()override{
    if(!device_)return Status::NotInitialized;if(active_)return Status::Busy;
    // This port owns a newly created host with exactly one private device.
    // Polling transfers acquire/release the SDK bus themselves; keep no SDK
    // mutex held across a UI tick or a vendor's long delay sequence.
    active_=true;return Status::Ok;
  }
  Status send(const uint8_t *data,size_t bytes)override{
    if(!active_||(bytes&&!data))return Status::InvalidArgument;
    while(bytes){const size_t n=bytes>256?256:bytes;spi_transaction_t transfer={};
      // One small persistent aligned buffer avoids per-transfer DMA copies and
      // accepts the transient/unaligned source buffers used by upstream CAD.
      memcpy(staging_,data,n);transfer.tx_buffer=staging_;transfer.length=n*8;
      if(spi_device_polling_transmit(device_,&transfer)!=ESP_OK)return Status::IoError;data+=n;bytes-=n;}
    return Status::Ok;
  }
  Status stop()override{active_=false;return Status::Ok;}
 private:
  template<class Bus>static auto disableExtra(Bus &b,int)->decltype(b.data4_io_num=-1,void()){
    b.data4_io_num=b.data5_io_num=b.data6_io_num=b.data7_io_num=-1;}
  template<class Bus>static void disableExtra(Bus &,long){}
  spi_host_device_t host_=SPI2_HOST;spi_device_handle_t device_=nullptr;uint8_t *staging_=nullptr;
  bool owns_=false,active_=false;
};
#endif
} // namespace nice_gfx
