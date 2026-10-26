#pragma once
#include "../../Display/Transport.h"
#include "../../Core/PixelCodec.h"
#include "../Common/BusTypes.h"
#include "FastPin.h"
#include <SPI.h>

namespace nice_gfx {
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
using ArduinoSpiClass = SPIClassRP2040;
#else
using ArduinoSpiClass = SPIClass;
#endif

// Common transaction and RGB conversion logic. All calls into a bus are spans,
// including software SPI; there is no virtual dispatch for individual pixels.
class StreamBus : public Transport {
 public:
  Status command(uint8_t c, const uint8_t *p, size_t n) override { return send(c, 8, p, n); }
  Status command16(uint16_t c, const uint8_t *p, size_t n) override { return send(c, 16, p, n); }
  Status data(const uint8_t *p, size_t n) override { return send(0, 0, p, n); }
  Status pixels(uint8_t c, PixelView v, bool) override { return image(c, 8, v); }
  Status pixels16(uint16_t c, PixelView v, bool) override { return image(c, 16, v); }
  Status dataPixels(PixelView v, bool) override { return image(0, 0, v); }
  Status fill(uint8_t c, uint16_t color, size_t n) override { return solid(c, 8, color, n); }
  Status fill16(uint16_t c, uint16_t color, size_t n) override { return solid(c, 16, color, n); }
  Status dataFill(uint16_t color, size_t n) override { return solid(0, 0, color, n); }
  Status read(uint16_t c, uint8_t *p, size_t n, uint8_t dummy = 1, uint8_t bits = 8) override;
  Status setPixelFormat(PixelFormat format) override {
    if (format != PixelFormat::RGB565 && format != PixelFormat::RGB666 && format != PixelFormat::RGB888)
      return Status::InvalidArgument;
    format_ = format; return Status::Ok;
  }
  Status wait() override { return Status::Ok; }
  bool busy() const override { return false; }
  bool asynchronous() const override { return false; }
  void output(int pin, bool high) override { pinMode(pin, OUTPUT); digitalWrite(pin, high ? HIGH : LOW); }
  void delayMs(uint32_t ms) override { delay(ms); }
 protected:
  virtual Status start(uint16_t command, uint8_t bits, bool reading) = 0;
  virtual void finish() = 0;
  virtual void writeBlock(const uint8_t *data, size_t bytes, bool pixelData) = 0;
  virtual Status readBlock(uint8_t *data, size_t bytes) = 0;
  bool begun_ = false;
  PixelFormat format_ = PixelFormat::RGB565;
  FastPin writeStrobe_;
 private:
  Status send(uint16_t c, uint8_t bits, const uint8_t *p, size_t n);
  Status image(uint16_t c, uint8_t bits, PixelView view);
  Status solid(uint16_t c, uint8_t bits, uint16_t color, size_t count);
};

// Uses the selected Arduino core's hardware peripheral and bulk write API.
// CI13XX's SPIClass is a software SPI implementation provided by that core.
class ArduinoSpi : public StreamBus {
 public:
  ArduinoSpi(ArduinoSpiClass &spi, SpiPins pins) : spi_(spi), config_(pins) {}
  ArduinoSpi(ArduinoSpiClass &spi, const SpiConfig &config) : spi_(spi), config_(config) {}
  ~ArduinoSpi() override { end(); }
  Status begin() override;
  Status commands(const uint8_t *data,size_t bytes) override;
  void end() override;
  bool validOutputPin(int pin) const override;
  TransportInfo info() const override;
 protected:
  Status start(uint16_t c, uint8_t bits, bool reading) override;
  void finish() override;
  void writeBlock(const uint8_t *p, size_t n, bool) override;
  Status readBlock(uint8_t *p, size_t n) override;
 private:
  Status beginPeripheral();
  ArduinoSpiClass &spi_;
  SpiConfig config_;
  FastPin cs_, dc_;
  int8_t ownerSlot_ = -1;
  FastPin sdaClock_;
  bool sdaReading_ = false;
};

class SoftwareSpi : public StreamBus {
 public:
  explicit SoftwareSpi(SpiPins pins) : config_(pins) {}
  explicit SoftwareSpi(const SpiConfig &config) : config_(config) {}
  ~SoftwareSpi() override { end(); }
  Status begin() override;
  Status commands(const uint8_t *data,size_t bytes) override;
  void end() override { if (begun_) cs_.write(true); begun_ = false; }
  bool validOutputPin(int pin) const override;
  TransportInfo info() const override;
 protected:
  Status start(uint16_t c, uint8_t bits, bool reading) override;
  void finish() override;
  void writeBlock(const uint8_t *p, size_t n, bool) override;
  Status readBlock(uint8_t *p, size_t n) override;
 private:
  uint8_t exchange(uint8_t value, bool read);
  SpiConfig config_;
  FastPin clock_, mosi_, cs_, dc_;
  uint32_t halfPeriodUs_ = 0;
  bool sdaReading_ = false;
};

class ParallelBus : public StreamBus {
 public:
  Status begin() override;
  void end() override;
  Status register16(uint16_t command, uint16_t value, uint8_t bits=8) override;
  Status readWords(uint16_t command, uint16_t *data, size_t count, uint8_t dummy=1, uint8_t bits=8) override;
  Status fill(uint8_t c,uint16_t color,size_t count) override { return solidFast(c,8,color,count); }
  Status fill16(uint16_t c,uint16_t color,size_t count) override { return solidFast(c,16,color,count); }
  Status dataFill(uint16_t color,size_t count) override { return solidFast(0,0,color,count); }
  bool validOutputPin(int pin) const override;
  TransportInfo info() const override;
 protected:
  ParallelBus(const ParallelConfig &config, uint8_t width) : config_(config), width_(width) {}
  ~ParallelBus() override { end(); }
  Status start(uint16_t c, uint8_t bits, bool reading) override;
  void finish() override;
  void writeBlock(const uint8_t *p, size_t n, bool pixelData) override;
  Status readBlock(uint8_t *p, size_t n) override;
 private:
  void word(uint16_t value);
  void writePulse();
  void waitWritePhase();
  Status solidFast(uint16_t c,uint8_t bits,uint16_t color,size_t count);
  void direction(bool output);
  ParallelConfig config_;
  uint8_t width_, groups_ = 0;
  FastPin data_[16], wr_, rd_, cs_, dc_;
  struct Group {
    volatile uint32_t *set, *clear;
    uint32_t mask, table[4][16];
    bool combined;
  } *group_ = nullptr;
  bool reading_ = false;
  bool direct_ = false;
  uint32_t halfCycles_ = 0;
  uint32_t writeDelayUs_ = 0;
};
class Parallel8 : public ParallelBus {
 public:
  explicit Parallel8(ParallelPins pins) : ParallelBus(ParallelConfig(pins), 8) {}
  explicit Parallel8(const ParallelConfig &config) : ParallelBus(config, 8) {}
};
class Parallel16 : public ParallelBus {
 public:
  explicit Parallel16(ParallelPins pins) : ParallelBus(ParallelConfig(pins), 16) {}
  explicit Parallel16(const ParallelConfig &config) : ParallelBus(config, 16) {}
};
}
