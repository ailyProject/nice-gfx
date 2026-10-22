#pragma once
#include "../../Core/BuildConfig.h"
#include "../../Core/I2c.h"
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#if NICE_GFX_NATIVE_IDF
#include <driver/i2c_master.h>
#include "../../Core/Platform.h"
#include <esp_rom_sys.h>
#else
#include "../Arduino/I2cWire.h"
#endif
namespace nice_gfx { namespace detail {
#if NICE_GFX_NATIVE_IDF
class BoardI2c : public I2cBus {
 public:
  BoardI2c(int sda,int scl):sda_(sda),scl_(scl) {}
  ~BoardI2c(){if(device_)i2c_master_bus_rm_device(device_);if(bus_)i2c_del_master_bus(bus_);}
  BoardI2c(const BoardI2c&)=delete;
  BoardI2c &operator=(const BoardI2c&)=delete;
  Status begin(uint8_t address) override {
    if(address<8||address>0x77)return Status::InvalidArgument;
    if(!bus_) {
      i2c_master_bus_config_t c={};c.i2c_port=I2C_NUM_0;
      c.sda_io_num=gpio_num_t(sda_);c.scl_io_num=gpio_num_t(scl_);
      c.clk_source=I2C_CLK_SRC_DEFAULT;c.glitch_ignore_cnt=7;c.flags.enable_internal_pullup=true;
      if(i2c_new_master_bus(&c,&bus_)!=ESP_OK)return Status::IoError;
    }
    if(device_&&address_==address)return Status::Ok;
    if(device_){i2c_master_bus_rm_device(device_);device_=nullptr;}
    i2c_device_config_t c={};c.dev_addr_length=I2C_ADDR_BIT_LEN_7;
    c.device_address=address;c.scl_speed_hz=400000;
    if(i2c_master_bus_add_device(bus_,&c,&device_)!=ESP_OK)return Status::IoError;
    address_=address;return Status::Ok;
  }
  Status transfer(uint8_t address,const uint8_t *tx,size_t nt,uint8_t *rx,size_t nr,bool=true) override {
    if((nt&&!tx)||(nr&&!rx)||(!nt&&!nr))return Status::InvalidArgument;
    const Status s=begin(address);if(s!=Status::Ok)return s;
    const esp_err_t r=nt&&nr?i2c_master_transmit_receive(device_,tx,nt,rx,nr,100):
      nt?i2c_master_transmit(device_,tx,nt,100):i2c_master_receive(device_,rx,nr,100);
    return r==ESP_OK?Status::Ok:Status::IoError;
  }
  void delayMs(uint16_t ms) override {platform::delayMs(ms);}
  void delayUs(uint16_t us) override {esp_rom_delay_us(us);}
 private:
  int sda_,scl_;uint8_t address_=0;
  i2c_master_bus_handle_t bus_=nullptr;i2c_master_dev_handle_t device_=nullptr;
};
#else
class BoardI2c : public I2cWire {
 public:
  BoardI2c(int sda,int scl):I2cWire(Wire),sda_(sda),scl_(scl) {}
  Status begin(uint8_t address) override {
    if(!ready_){if(!Wire.begin(sda_,scl_,400000))return Status::IoError;Wire.setTimeOut(20);ready_=true;}
    return I2cWire::begin(address);
  }
 private:int sda_,scl_;bool ready_=false;
};
#endif
} } // namespace nice_gfx::detail
#endif
