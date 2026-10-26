#pragma once
#include "Display.h"
#include "Backends/EpaperPanel.h"
#include "Presets/EpaperProfiles/EpdV2.h"
#if NICE_GFX_NATIVE_IDF || defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <driver/gpio.h>
#endif

namespace nice_gfx {
namespace detail {
inline int epaperPhysicalPin(int pin){
#if NICE_GFX_NATIVE_IDF
  return pin;
#else
  return FastPin::gpioNumber(pin);
#endif
}
inline bool epaperBusyDistinct(int busy,int cs,int dc,int reset,int clock,int mosi){
  const int physical=epaperPhysicalPin(busy);
  return physical>=0&&physical!=epaperPhysicalPin(cs)&&physical!=epaperPhysicalPin(dc)&&
    physical!=epaperPhysicalPin(reset)&&physical!=epaperPhysicalPin(clock)&&physical!=epaperPhysicalPin(mosi);
}
struct BorrowedEpaperStorage {
  EpaperPanel panel;
  BorrowedEpaperStorage(Transport&bus,const EpaperProfile &profile,EpaperIo io,int reset):panel(bus,profile,io,reset){}
};
struct EpaperBusyPin {
  int pin;
  bool usable;
  explicit EpaperBusyPin(int number,bool allowed=true):pin(number),usable(allowed){}
  static bool read(void *context){const int pin=static_cast<EpaperBusyPin*>(context)->pin;
#if NICE_GFX_NATIVE_IDF
    return gpio_get_level(gpio_num_t(pin))!=0;
#else
    return digitalRead(pin)!=LOW;
#endif
  }
  static Status begin(void *context){const int pin=static_cast<EpaperBusyPin*>(context)->pin;
    if(!static_cast<EpaperBusyPin*>(context)->usable)return Status::InvalidArgument;
#if NICE_GFX_NATIVE_IDF
    if(!GPIO_IS_VALID_GPIO(pin))return Status::InvalidArgument;
#else
    if(!FastPin::validInput(pin))return Status::InvalidArgument;
#endif
#if NICE_GFX_NATIVE_IDF
    return gpio_set_direction(gpio_num_t(pin),GPIO_MODE_INPUT)==ESP_OK?Status::Ok:Status::IoError;
#else
    if(pin<0)return Status::InvalidArgument;pinMode(pin,INPUT);return Status::Ok;
#endif
  }
};
inline SpiConfig epaperSpiConfig(int cs,int dc,int clock,int mosi,uint32_t hz){
  SpiConfig config(SpiPins(clock,mosi,-1,cs,dc));config.frequency=hz;config.mode=0;config.byteOnly=true;return config;
}
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
using EpaperSpiBus=Esp32Spi;
#elif defined(ARDUINO_ARCH_STM32) && !defined(USE_HALV2_DRIVER)
using EpaperSpiBus=Stm32Spi;
#elif defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
using EpaperSpiBus=Rp2040PioSpi;
#else
using EpaperSpiBus=ArduinoSpi;
#endif
struct Waveshare1in54V2 {static const EpaperProfile &profile(){return epaper::waveshare1in54V2();}};
struct Waveshare2in9V2 {static const EpaperProfile &profile(){return epaper::waveshare2in9V2();}};
template<class Profile>struct EpaperSpiStorage {
  EpaperSpiBus bus;EpaperBusyPin busy;EpaperPanel panel;
  EpaperSpiStorage(int cs,int dc,int reset,int busyPin,int clock,int mosi,uint32_t hz)
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM) || \
    (defined(ARDUINO_ARCH_STM32) && !defined(USE_HALV2_DRIVER)) || \
    (defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED))
    :bus(epaperSpiConfig(cs,dc,clock,mosi,hz)),busy(busyPin,epaperBusyDistinct(busyPin,cs,dc,reset,clock,mosi)),
#else
    :bus(SPI,epaperSpiConfig(cs,dc,clock,mosi,hz)),busy(busyPin,epaperBusyDistinct(busyPin,cs,dc,reset,clock,mosi)),
#endif
      panel(bus,Profile::profile(),EpaperIo(EpaperBusyPin::read,&busy,EpaperBusyPin::begin),reset){}
};
}
class EpaperDisplay:private detail::BorrowedEpaperStorage,public NICE_GFX {
  using Storage=detail::BorrowedEpaperStorage;
 public:EpaperDisplay(Transport&bus,const EpaperProfile &profile,EpaperIo io,int reset=-1)
    :Storage(bus,profile,io,reset),NICE_GFX(Storage::panel){}
  bool begin(){return beginChecked()==Status::Ok;}
  EpaperPanel &epaperPanel(){return Storage::panel;}
};
// Constructor: CS, DC, RESET, BUSY, CLOCK, MOSI, optional SPI frequency.
// Keep pollDisplay(millis()) (or ui.tick()) running; begin is asynchronous.
template<class Profile>class EpaperSpiDisplay:private detail::EpaperSpiStorage<Profile>,public NICE_GFX {
  using Storage=detail::EpaperSpiStorage<Profile>;
 public:EpaperSpiDisplay(int cs,int dc,int reset,int busy,int clock,int mosi,uint32_t hz=4000000)
    :Storage(cs,dc,reset,busy,clock,mosi,hz),NICE_GFX(Storage::panel){}
  bool begin(){return beginChecked()==Status::Ok;}
  EpaperPanel &epaperPanel(){return Storage::panel;}
};
}
using SSD1681_200x200_EPD_SPI=nice_gfx::EpaperSpiDisplay<nice_gfx::detail::Waveshare1in54V2>;
using SSD1680_128x296_EPD_SPI=nice_gfx::EpaperSpiDisplay<nice_gfx::detail::Waveshare2in9V2>;
