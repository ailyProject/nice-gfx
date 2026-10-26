#pragma once
#include "Display.h"
#include "Backends/I2cMonoTransport.h"
#include "../Drivers/Display/Monochrome/SSD1306.h"
#include "../Drivers/Display/Monochrome/SH1106.h"
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || NICE_GFX_NATIVE_IDF
#include <driver/gpio.h>
#endif

namespace nice_gfx {
namespace detail {
inline bool monoResetValid(int pin){
#if NICE_GFX_NATIVE_IDF
  return GPIO_IS_VALID_OUTPUT_GPIO(pin);
#else
  return FastPin::valid(pin);
#endif
}
inline void monoResetOutput(void *,int pin,bool high){
#if NICE_GFX_NATIVE_IDF
  gpio_set_direction(gpio_num_t(pin),GPIO_MODE_OUTPUT);gpio_set_level(gpio_num_t(pin),high?1:0);
#else
  pinMode(pin,OUTPUT);digitalWrite(pin,high?HIGH:LOW);
#endif
}
template<class Controller>struct I2cMonoStorage {
  I2cMonoTransport bus;Controller panel;
  I2cMonoStorage(I2cBus &wire,uint8_t address,int reset,PanelOptions options,uint8_t payload)
    :bus(wire,address,payload,monoResetOutput,nullptr,monoResetValid),panel(bus,reset,options){}
};
}
// The application owns/initializes the physical I2cBus. That same bus can serve
// touch/sensors; this adapter neither reconfigures pins nor calls Wire.end().
template<class Controller>class I2cMonoDisplay:private detail::I2cMonoStorage<Controller>,public NICE_GFX {
  using Storage=detail::I2cMonoStorage<Controller>;
 public:
  I2cMonoDisplay(I2cBus &wire,uint8_t address=0x3C,int reset=-1,
      PanelOptions options=PanelOptions(),uint8_t payload=31)
    :Storage(wire,address,reset,options,payload),NICE_GFX(Storage::panel){}
  bool begin(){return beginChecked()==Status::Ok;}
};
template<class Controller>class MonoSpiDisplay:public SpiDisplay<Controller>{
 public:MonoSpiDisplay(int cs,int dc,int reset,int clock,int mosi,int miso=-1,
    uint32_t hz=Controller::spiFrequency,PanelOptions options=PanelOptions())
    :SpiDisplay<Controller>(cs,dc,reset,clock,mosi,miso,hz,options){}
};
}
using SSD1306_128x64_I2C=nice_gfx::I2cMonoDisplay<nice_gfx::SSD1306>;
using SSD1306_128x32_I2C=nice_gfx::I2cMonoDisplay<nice_gfx::SSD1306_32>;
using SH1106_128x64_I2C=nice_gfx::I2cMonoDisplay<nice_gfx::SH1106>;
using SSD1306_128x64_SPI=nice_gfx::MonoSpiDisplay<nice_gfx::SSD1306>;
using SSD1306_128x32_SPI=nice_gfx::MonoSpiDisplay<nice_gfx::SSD1306_32>;
using SH1106_128x64_SPI=nice_gfx::MonoSpiDisplay<nice_gfx::SH1106>;
