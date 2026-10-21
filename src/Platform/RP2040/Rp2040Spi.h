#pragma once
#include "../Arduino/Bus.h"
#include "../Common/BufferedDmaBus.h"
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
namespace nice_gfx {
// Native SPI0/SPI1 DMA is also available when pins follow the peripheral route.
class Rp2040Spi : public BufferedDmaBus<ArduinoSpi> {
 public:
  Rp2040Spi(SPIClassRP2040 &spi,SpiPins pins) : BufferedDmaBus<ArduinoSpi>(spi,pins),spi_(spi) {}
  Rp2040Spi(SPIClassRP2040 &spi,const SpiConfig &config) : BufferedDmaBus<ArduinoSpi>(spi,config),spi_(spi) {}
  ~Rp2040Spi() override { end(); }
  Status begin() override {
    if (begun_) return Status::Ok;
    Status s=ArduinoSpi::begin(); if (s!=Status::Ok) return s;
    s=allocateDmaBuffers(); if (s!=Status::Ok) ArduinoSpi::end(); return s;
  }
  void end() override { releaseDmaBuffers(); ArduinoSpi::end(); }
 protected:
  Status dmaWrite(const uint8_t *data,size_t bytes) override {
    if (!spi_.transferAsync(data,nullptr,bytes)) return Status::IoError;
    started_=millis(); active_=true; return Status::Ok;
  }
  Status dmaPoll() override {
    if (!active_) return Status::Ok;
    if (spi_.finishedAsync()) { active_=false; return Status::Ok; }
    if (uint32_t(millis()-started_)<5000) return Status::Busy;
    spi_.abortAsync(); active_=false; return Status::IoError;
  }
 private:
  SPIClassRP2040 &spi_;
  uint32_t started_=0;
  bool active_=false;
};
}
#endif
