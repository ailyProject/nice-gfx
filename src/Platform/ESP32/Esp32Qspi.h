#pragma once
#include "../../Display/Transport.h"
#include "../../Display/QspiProtocol.h"
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include "../../Core/PixelCodec.h"
#include "Esp32Platform.h"
#include <driver/spi_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <sys/types.h>

// IDF 6's libc exposes a legacy `quad` macro. Read and remove it before the
// public config declaration so later system includes cannot rename its field.
#ifdef quad
#undef quad
#endif

namespace nice_gfx {
struct QspiPins {
  int clock, d0, d1, d2, d3, cs;
  QspiPins(int clk, int io0, int io1, int io2, int io3, int select)
      : clock(clk), d0(io0), d1(io1), d2(io2), d3(io3), cs(select) {}
};
struct QspiConfig {
  QspiPins pins;
  uint32_t frequency = 40000000;
  spi_host_device_t host = SPI2_HOST;
  bool autoHost = false;
  size_t chunkBytes = 4096; // 4..32768, aligned to 4; copied chunks use <=4 KiB.
  uint8_t mode = 0;
  uint32_t readFrequency = 10000000;
  // Single-lane operation shares the same DMA queue and staging implementation.
  bool quad = true;
  int dc = -1;
  bool paddedCommands = false;
  int writeStrobePin = -1; // Optional WR on a 16-bit RPI SPI bridge.
  bool threeWire = false;
  QspiProtocol protocol;
  explicit QspiConfig(QspiPins p) : pins(p) {}
};

class Esp32Qspi : public Transport {
 public:
  explicit Esp32Qspi(QspiPins pins) : config_(pins) {}
  explicit Esp32Qspi(const QspiConfig &config) : config_(config) {}
  ~Esp32Qspi() override { end(); }
  Status begin() override;
  void end() override;
  Status command(uint8_t command, const uint8_t *data, size_t bytes) override;
  Status command16(uint16_t command, const uint8_t *data, size_t bytes) override;
  Status data(const uint8_t *data, size_t bytes) override;
  Status read(uint16_t command, uint8_t *data, size_t bytes, uint8_t dummy = 1, uint8_t bits = 8) override;
  Status setPixelFormat(PixelFormat format) override;
  Status pixels(uint8_t command, PixelView view, bool async) override;
  Status pixels16(uint16_t command, PixelView view, bool async) override;
  Status dataPixels(PixelView view, bool async) override;
  Status fill(uint8_t command, uint16_t color, size_t count) override;
  Status fill16(uint16_t command, uint16_t color, size_t count) override;
  Status dataFill(uint16_t color, size_t count) override;
  Status wait() override;
  bool busy() const override;
  bool asynchronous() const override { return true; }
  // The worker retains arbitrary-sized surfaces and pipelines bounded DMA
  // staging for PSRAM/strided input; internal wire-order RAM is zero-copy.
  bool supportsFrameBlit() const override { return true; }
  bool canFrameBlit(PixelView view) const override;
  bool validOutputPin(int pin) const override;
  TransportInfo info() const override;
  void output(int pin, bool high) override;
  void delayMs(uint32_t ms) override;
  const QspiConfig &config() const { return config_; }
 private:
  static void workerEntry(void *self);
  void worker();
  Status transfer();
  Status transmitSmall(uint16_t command, uint8_t bits, const uint8_t *data, size_t bytes);
  Status sendCommand(uint16_t command, uint8_t bits, const uint8_t *data, size_t bytes);
  Status prefix(uint16_t command, uint8_t bits, bool keepActive);
  Status setFrequency(uint32_t frequency);
  void configureFrequency(spi_transaction_t &transaction);
  Status image(uint16_t command, uint8_t bits, PixelView view, bool async);
  Status solid(uint16_t command, uint8_t bits, uint16_t color, size_t count);
  Status solidStrobe(uint16_t command, uint8_t bits, uint16_t color, size_t count);
  Status submit(bool async);
  QspiConfig config_;
  spi_device_handle_t device_ = nullptr;
  uint32_t currentFrequency_ = 0;
  uint32_t pendingFrequency_ = 0;
  bool ownsBus_ = false, stopping_ = false;
  TaskHandle_t worker_ = nullptr;
  SemaphoreHandle_t done_ = nullptr;
  uint8_t *staging_[2] = {nullptr, nullptr};
  size_t stagingBytes_ = 0;
  spi_transaction_ext_t transactions_[2] = {};
  struct Job {
    PixelView view = PixelView(nullptr, 0);
    size_t count = 0;
    uint16_t color = 0;
    uint16_t command = 0;
    uint8_t commandBits = 8;
    bool solid = false;
  } job_;
  Status result_ = Status::Ok;
  PixelFormat format_ = PixelFormat::RGB565;
  Esp32Pin writeStrobe_;
};
} // namespace nice_gfx
#endif
