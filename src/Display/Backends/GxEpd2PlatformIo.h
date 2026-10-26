#pragma once
#include "../GxEpd2Types.h"
#include "../../Core/Platform.h"
#if NICE_GFX_NATIVE_IDF
#include <driver/gpio.h>
#include <esp_rom_sys.h>
#else
#include "../../Platform/Arduino/FastPin.h"
#include <SPI.h>
#endif
#if defined(ESP_PLATFORM)||defined(ESP32)||defined(ARDUINO_ARCH_ESP32)
#include <driver/spi_master.h>
#include <esp_heap_caps.h>
#define NGFX_GXEPD2_ESP_SPI 1
#else
#define NGFX_GXEPD2_ESP_SPI 0
#endif

namespace nice_gfx {
// Full duplex is required by IT8951. ESP32 owns a fresh SDK host and never
// keeps an SDK bus lock across a UI tick. GPIO CS remains source-controlled.
class GxEpd2PlatformIo:public GxEpd2Io{
 public:
  ~GxEpd2PlatformIo()override{end();}
  Status begin(const GxEpd2Profile&,const GxEpd2Config &c)override{
    end();pins_=c.pins;hz_=c.busClockHz&&c.busClockHz<4000000UL?c.busClockHz:4000000UL;mode_=0;configured_=true;
#if !NGFX_GXEPD2_ESP_SPI && defined(ARDUINO)
    // Other Arduino cores retain their SPIClass pin mapping. Reject a mapping
    // that cannot be honored instead of silently clocking unrelated pins.
    if(!mapArduinoPins(pins_.clock,pins_.mosi,pins_.miso))return Status::Unsupported;
#endif
    return Status::Ok;
  }
  void end()override{
    if(configured_){writePin(pins_.cs,true);for(int pin:pins_.extraCs)writePin(pin,true);}
    spiEnable(false);configured_=false;
#if NGFX_GXEPD2_ESP_SPI
    free(staging_);staging_=nullptr;
#endif
  }
  bool validPin(int pin,bool input=false)const override{
#if NICE_GFX_NATIVE_IDF
    return input?GPIO_IS_VALID_GPIO(pin):GPIO_IS_VALID_OUTPUT_GPIO(pin);
#else
    return input?FastPin::validInput(pin):FastPin::valid(pin);
#endif
  }
  int physicalPin(int pin)const override{
#if NICE_GFX_NATIVE_IDF
    return pin;
#else
    return FastPin::gpioNumber(pin);
#endif
  }
  Status pinMode(int pin,uint8_t mode)override{
    if(pin<0)return Status::Ok;if(mode>2||!validPin(pin,mode!=1))return Status::InvalidArgument;
#if NICE_GFX_NATIVE_IDF
    const gpio_num_t gpio=gpio_num_t(pin);
    if(gpio_set_direction(gpio,mode==1?GPIO_MODE_OUTPUT:GPIO_MODE_INPUT)!=ESP_OK)return Status::IoError;
    return gpio_set_pull_mode(gpio,mode==2?GPIO_PULLUP_ONLY:GPIO_FLOATING)==ESP_OK?Status::Ok:Status::IoError;
#else
    ::pinMode(pin,mode==1?OUTPUT:mode==2?INPUT_PULLUP:INPUT);return Status::Ok;
#endif
  }
  Status writePin(int pin,bool high)override{
    if(pin<0)return Status::Ok;if(!validPin(pin))return Status::InvalidArgument;
#if NICE_GFX_NATIVE_IDF
    return gpio_set_level(gpio_num_t(pin),high?1:0)==ESP_OK?Status::Ok:Status::IoError;
#else
    ::digitalWrite(pin,high?HIGH:LOW);return Status::Ok;
#endif
  }
  Status readPin(int pin,bool &high)override{
    if(!validPin(pin,true))return Status::InvalidArgument;
#if NICE_GFX_NATIVE_IDF
    high=gpio_get_level(gpio_num_t(pin))!=0;
#else
    high=::digitalRead(pin)!=LOW;
#endif
    return Status::Ok;
  }
  Status spiEnable(bool enabled)override{
    if(enabled==enabled_)return Status::Ok;
#if NGFX_GXEPD2_ESP_SPI
    if(!enabled){if(device_){spi_bus_remove_device(device_);device_=nullptr;}
      if(owns_){spi_bus_free(host_);owns_=false;}enabled_=false;return Status::Ok;}
    if(!configured_)return Status::NotInitialized;
    spi_bus_config_t bus={};bus.sclk_io_num=physicalPin(pins_.clock);bus.mosi_io_num=physicalPin(pins_.mosi);
    bus.miso_io_num=pins_.miso<0?-1:physicalPin(pins_.miso);bus.quadwp_io_num=bus.quadhd_io_num=-1;disableExtra(bus,0);bus.max_transfer_sz=256;
    host_=SPI2_HOST;esp_err_t e=spi_bus_initialize(host_,&bus,SPI_DMA_CH_AUTO);
#if !defined(SOC_SPI_PERIPH_NUM)||SOC_SPI_PERIPH_NUM>2
    if(e==ESP_ERR_INVALID_STATE){host_=SPI3_HOST;e=spi_bus_initialize(host_,&bus,SPI_DMA_CH_AUTO);}
#endif
    if(e!=ESP_OK)return error(e);owns_=true;enabled_=true;
    if(!staging_)staging_=static_cast<uint8_t*>(heap_caps_malloc(512,MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL));
    if(!staging_){spiEnable(false);return Status::NoMemory;}
    const Status s=addDevice();if(s!=Status::Ok){spiEnable(false);return s;}
    enabled_=true;return Status::Ok;
#elif defined(ARDUINO)
    if(enabled){SPI.begin();enabled_=true;}else{SPI.end();enabled_=false;}return Status::Ok;
#else
    (void)enabled;return Status::Unsupported;
#endif
  }
  Status spiSettings(uint32_t hz,uint8_t mode)override{
    if(!hz||hz>80000000UL||mode>3)return Status::InvalidArgument;
    if(hz_==hz&&mode_==mode)return Status::Ok;hz_=hz;mode_=mode;
#if NGFX_GXEPD2_ESP_SPI
    if(enabled_){if(device_){if(spi_bus_remove_device(device_)!=ESP_OK)return Status::IoError;device_=nullptr;}return addDevice();}
#endif
    return Status::Ok;
  }
  Status transfer(const uint8_t *tx,uint8_t *rx,size_t bytes)override{
    if(!enabled_||(bytes&&!tx))return Status::InvalidArgument;
#if NGFX_GXEPD2_ESP_SPI
    if(!device_||!staging_)return Status::NotInitialized;
    while(bytes){const size_t n=bytes>256?256:bytes;memcpy(staging_,tx,n);spi_transaction_t t={};
      t.length=n*8;t.tx_buffer=staging_;if(rx){t.rxlength=t.length;t.rx_buffer=staging_+256;}
      const esp_err_t e=spi_device_polling_transmit(device_,&t);if(e!=ESP_OK)return error(e);
      if(rx){memcpy(rx,staging_+256,n);rx+=n;}tx+=n;bytes-=n;}return Status::Ok;
#elif defined(ARDUINO)
    const uint8_t modes[]={SPI_MODE0,SPI_MODE1,SPI_MODE2,SPI_MODE3};SPI.beginTransaction(SPISettings(hz_,MSBFIRST,modes[mode_]));
    for(size_t i=0;i<bytes;++i){const uint8_t value=SPI.transfer(tx[i]);if(rx)rx[i]=value;}SPI.endTransaction();return Status::Ok;
#else
    (void)tx;(void)rx;(void)bytes;return Status::Unsupported;
#endif
  }
  void delayMs(uint32_t ms)override{platform::delayMs(ms);}
  void delayUs(uint32_t us)override{
#if NICE_GFX_NATIVE_IDF
    esp_rom_delay_us(us);
#else
    while(us>=60000UL){::delayMicroseconds(60000);us-=60000UL;}if(us)::delayMicroseconds(uint16_t(us));
#endif
  }
  uint32_t nowMicros()const override{return platform::micros();}
  void yieldThread()override{
#if NGFX_GXEPD2_ESP_SPI
    vTaskDelay(1);
#elif defined(ARDUINO)
    ::yield();
#endif
  }
 private:
#if NGFX_GXEPD2_ESP_SPI
  static Status error(esp_err_t e){return e==ESP_ERR_NO_MEM?Status::NoMemory:Status::IoError;}
  Status addDevice(){spi_device_interface_config_t d={};d.mode=mode_;d.clock_speed_hz=hz_;d.spics_io_num=-1;d.queue_size=1;
    d.flags=SPI_DEVICE_NO_DUMMY;return spi_bus_add_device(host_,&d,&device_)==ESP_OK?Status::Ok:Status::IoError;}
  template<class T>static auto disableExtra(T&b,int)->decltype(b.data4_io_num=-1,void()){
    b.data4_io_num=b.data5_io_num=b.data6_io_num=b.data7_io_num=-1;}
  template<class T>static void disableExtra(T&,long){}
  spi_host_device_t host_=SPI2_HOST;spi_device_handle_t device_=nullptr;bool owns_=false;uint8_t *staging_=nullptr;
#elif defined(ARDUINO)
  static bool mapArduinoPins(int clock,int mosi,int miso){
    return clock==SCK&&mosi==MOSI&&(miso<0||miso==MISO);
  }
#endif
  GxEpd2Pins pins_;uint32_t hz_=4000000UL;uint8_t mode_=0;bool configured_=false,enabled_=false;
};
} // namespace nice_gfx
