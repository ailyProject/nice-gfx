#pragma once
#include "../Common/BusTypes.h"
#include "Esp32Qspi.h"
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
namespace nice_gfx {
struct Esp32SpiConfig : SpiConfig {
  spi_host_device_t host = SPI2_HOST;
  size_t chunkBytes = 4096;
  bool autoHost = false;
  explicit Esp32SpiConfig(SpiPins pins) : SpiConfig(pins) {}
  explicit Esp32SpiConfig(const SpiConfig &config) : SpiConfig(config) {}
};
class Esp32Spi : public Esp32Qspi {
 public:
  explicit Esp32Spi(SpiPins pins) : Esp32Qspi(convert(Esp32SpiConfig(pins),true)) {}
  explicit Esp32Spi(const SpiConfig &config) : Esp32Qspi(convert(Esp32SpiConfig(config),true)) {}
  explicit Esp32Spi(const Esp32SpiConfig &config) : Esp32Qspi(convert(config)) {}
 private:
  static QspiConfig convert(const Esp32SpiConfig &c,bool automatic=false) {
    QspiConfig q(QspiPins(c.pins.clock,c.pins.mosi,c.pins.miso,-1,-1,c.pins.cs));
    q.frequency=c.frequency; q.readFrequency=c.readFrequency; q.mode=c.mode;
    q.host=c.host; q.autoHost=c.autoHost || automatic; q.chunkBytes=c.chunkBytes; q.quad=false; q.dc=c.pins.dc;
    q.paddedCommands=c.paddedCommands;
    q.byteOnly=c.byteOnly;
    q.writeStrobePin=c.writeStrobePin;
    q.threeWire=c.pins.miso>=0 && Esp32Pin::gpioNumber(c.pins.miso)==Esp32Pin::gpioNumber(c.pins.mosi);
    if(q.threeWire) q.pins.d1=-1;
    return q;
  }
};
}
#endif
