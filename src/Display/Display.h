#pragma once
#include "../Graphics/Gfx.h"
#include "../Core/BuildConfig.h"
#if NICE_GFX_NATIVE_IDF
#include "../Platform/Common/BusTypes.h"
#else
#include "../Platform/Arduino/Bus.h"
#endif
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include "../Platform/ESP32/Esp32Spi.h"
#include "../Platform/ESP32/Esp32Video.h"
#include "../Platform/ESP32/Esp32ScanQspi.h"
#elif defined(ARDUINO_ARCH_STM32)
#include "../Platform/STM32/Stm32Spi.h"
#elif defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
#include "../Platform/RP2040/Rp2040Pio.h"
#include "../Platform/RP2040/Rp2040Spi.h"
#endif

namespace nice_gfx {
namespace detail {
template<class Controller> inline auto configureByteTransport(SpiConfig &config,int)
    -> decltype(Controller::byteOnly,void()) { config.byteOnly=Controller::byteOnly; }
template<class Controller> inline void configureByteTransport(SpiConfig &,long) {}
// The hardware base is constructed before the graphics base and destroyed
// after it. This gives the renderer a live panel throughout its lifetime.
template<class Bus, class Controller> struct DisplayStorage {
  Bus bus;
  Controller panel;
  template<class... Args> DisplayStorage(int reset, Args&&... args)
      : bus(static_cast<Args&&>(args)...), panel(bus, reset) {}
  template<class... Args> DisplayStorage(PanelOptions options,int reset,Args&&... args)
      : bus(static_cast<Args&&>(args)...), panel(bus,reset,options) {}
};
template<class Controller> inline SpiConfig spiConfig(int cs,int dc,int clock,int mosi,int miso,uint32_t hz,
                                                    const PanelOptions &options) {
  SpiConfig c(SpiPins(clock,mosi,miso,cs,dc)); c.frequency=hz;
  c.mode=options.spiMode == -1 ? Controller::spiMode : uint8_t(options.spiMode);
  c.readFrequency=options.spiReadFrequency;
  c.paddedCommands=Controller::paddedCommands;
  c.writeStrobePin=options.writeStrobePin;
  configureByteTransport<Controller>(c,0);
  return c;
}
inline ParallelConfig parallelConfig(ParallelPins pins, const PanelOptions &options) {
  ParallelConfig c(pins);
  c.frequency=options.parallelFrequency;
  c.writeDelayUs=options.parallelWriteDelayUs;
  c.readDelayUs=options.parallelReadDelayUs;
  return c;
}
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
template<class Controller> inline auto configureControllerQspi(QspiConfig &config, int)
    -> decltype(Controller::configureQspi(config), void()) {
  Controller::configureQspi(config);
}
template<class Controller> inline void configureControllerQspi(QspiConfig &, long) {}
#endif
}

template<class Bus, class Controller>
class OwnedDisplay : private detail::DisplayStorage<Bus,Controller>, public NICE_GFX {
  using Storage=detail::DisplayStorage<Bus,Controller>;
 public:
  bool begin() { return beginChecked()==Status::Ok; }
  OwnedDisplay(const OwnedDisplay&)=delete;
  OwnedDisplay &operator=(const OwnedDisplay&)=delete;
 protected:
  template<class... Args> OwnedDisplay(int reset, Args&&... args)
      : Storage(reset,static_cast<Args&&>(args)...), NICE_GFX(Storage::panel) {}
  template<class... Args> OwnedDisplay(PanelOptions options,int reset,Args&&... args)
      : Storage(options,reset,static_cast<Args&&>(args)...), NICE_GFX(Storage::panel) {}
};

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
template<class Controller> class SpiDisplay : public OwnedDisplay<Esp32Spi,Controller> {
  using Base=OwnedDisplay<Esp32Spi,Controller>;
 public:
  // CS, DC, RESET, CLOCK, MOSI, optional MISO and frequency.
  SpiDisplay(int cs,int dc,int reset,int clock,int mosi,int miso=-1,uint32_t hz=40000000,
             PanelOptions options=PanelOptions())
      : Base(options,reset,detail::spiConfig<Controller>(cs,dc,clock,mosi,miso,hz,options)) {}
};
template<class Controller> class QspiDisplay : public OwnedDisplay<Esp32Qspi,Controller> {
  using Base=OwnedDisplay<Esp32Qspi,Controller>;
  static QspiConfig config(int cs,int clock,int d0,int d1,int d2,int d3,uint32_t hz) {
    QspiConfig c(QspiPins(clock,d0,d1,d2,d3,cs)); c.frequency=hz; c.autoHost=true;
    // Larger direct-DMA chunks avoid repeated transactions for a display tile.
    // Copied transfers still use the transport's bounded 4 KiB staging buffers.
    c.chunkBytes=32768;
    detail::configureControllerQspi<Controller>(c,0);
    return c;
  }
 public:
  // CS, RESET, CLOCK, D0, D1, D2, D3, optional frequency.
  QspiDisplay(int cs,int reset,int clock,int d0,int d1,int d2,int d3,uint32_t hz=40000000)
      : Base(reset,config(cs,clock,d0,d1,d2,d3,hz)) {}
  QspiDisplay(int cs,int reset,int clock,int d0,int d1,int d2,int d3,uint32_t hz,PanelOptions options)
      : Base(options,reset,config(cs,clock,d0,d1,d2,d3,hz)) {}
};
#elif defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
template<class Controller> class SpiDisplay : public OwnedDisplay<Rp2040PioSpi,Controller> {
  using Base=OwnedDisplay<Rp2040PioSpi,Controller>;
 public:
  SpiDisplay(int cs,int dc,int reset,int clock,int mosi,int miso=-1,uint32_t hz=40000000,
             PanelOptions options=PanelOptions())
      : Base(options,reset,detail::spiConfig<Controller>(cs,dc,clock,mosi,miso,hz,options)) {}
};
#else
#if defined(ARDUINO_ARCH_STM32) && !defined(USE_HALV2_DRIVER)
using DefaultSpiBus=Stm32Spi;
#else
using DefaultSpiBus=ArduinoSpi;
#endif
template<class Controller> class SpiDisplay : public OwnedDisplay<DefaultSpiBus,Controller> {
  using Base=OwnedDisplay<DefaultSpiBus,Controller>;
 public:
  SpiDisplay(int cs,int dc,int reset,int clock,int mosi,int miso=-1,uint32_t hz=40000000,
             PanelOptions options=PanelOptions())
      #if defined(ARDUINO_ARCH_STM32) && !defined(USE_HALV2_DRIVER)
      : Base(options,reset,detail::spiConfig<Controller>(cs,dc,clock,mosi,miso,hz,options)) {}
#else
      : Base(options,reset,SPI,detail::spiConfig<Controller>(cs,dc,clock,mosi,miso,hz,options)) {}
#endif
};
#endif

#if !NICE_GFX_NATIVE_IDF
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
using SelectedArduinoSpiBus=Rp2040Spi;
#else
using SelectedArduinoSpiBus=ArduinoSpi;
#endif
template<class Controller> class ArduinoSpiDisplay : public OwnedDisplay<SelectedArduinoSpiBus,Controller> {
  using Base=OwnedDisplay<SelectedArduinoSpiBus,Controller>;
 public:
  ArduinoSpiDisplay(ArduinoSpiClass &spi,int cs,int dc,int reset,int clock,int mosi,int miso=-1,
                    uint32_t hz=40000000,PanelOptions options=PanelOptions())
      : Base(options,reset,spi,detail::spiConfig<Controller>(cs,dc,clock,mosi,miso,hz,options)) {}
};

template<class Controller> class SoftwareSpiDisplay : public OwnedDisplay<SoftwareSpi,Controller> {
  using Base=OwnedDisplay<SoftwareSpi,Controller>;
 public:
  SoftwareSpiDisplay(int cs,int dc,int reset,int clock,int mosi,int miso=-1,uint32_t hz=1000000,
                     PanelOptions options=PanelOptions())
      : Base(options,reset,detail::spiConfig<Controller>(cs,dc,clock,mosi,miso,hz,options)) {}
};
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
using DefaultParallel8=Rp2040Parallel8;
using DefaultParallel16=Rp2040Parallel16;
#else
using DefaultParallel8=Parallel8;
using DefaultParallel16=Parallel16;
#endif
template<class Controller> class Parallel8Display : public OwnedDisplay<DefaultParallel8,Controller> {
  using Base=OwnedDisplay<DefaultParallel8,Controller>;
 public:
  Parallel8Display(int cs,int dc,int reset,int wr,int rd,
      int d0,int d1,int d2,int d3,int d4,int d5,int d6,int d7,PanelOptions options=PanelOptions())
      : Base(options,reset,detail::parallelConfig(ParallelPins(wr,rd,cs,dc,d0,d1,d2,d3,d4,d5,d6,d7),options)) {}
};
template<class Controller> class Parallel16Display : public OwnedDisplay<DefaultParallel16,Controller> {
  using Base=OwnedDisplay<DefaultParallel16,Controller>;
 public:
  Parallel16Display(int cs,int dc,int reset,int wr,int rd,
      int d0,int d1,int d2,int d3,int d4,int d5,int d6,int d7,
      int d8,int d9,int d10,int d11,int d12,int d13,int d14,int d15,PanelOptions options=PanelOptions())
      : Base(options,reset,detail::parallelConfig(ParallelPins(wr,rd,cs,dc,d0,d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,d11,d12,d13,d14,d15),options)) {}
};
#else
// These optional GPIO/Arduino transports are separate from the native ESP-IDF
// SPI, QSPI, RGB and MIPI display constructors above.
template<class Controller> class ArduinoSpiDisplay;
template<class Controller> class SoftwareSpiDisplay;
template<class Controller> class Parallel8Display;
template<class Controller> class Parallel16Display;
#endif
}
using DisplayOptions=nice_gfx::PanelOptions;
using DisplayColorOrder=nice_gfx::ColorOrder;
using ST7735Tab=nice_gfx::St7735Tab;
using DisplayStatus=nice_gfx::Status;
using DisplayPixels=nice_gfx::PixelView;
using DisplayPixelOrder=nice_gfx::PixelOrder;
