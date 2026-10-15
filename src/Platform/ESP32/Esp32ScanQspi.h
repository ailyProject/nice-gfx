#pragma once
#include "../../Display/FramebufferTransport.h"
#include "../../Drivers/Display/Video/VideoControllers.h"

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include "soc/soc_caps.h"
#include "driver/spi_master.h"

namespace nice_gfx {

// ST77903 is a video scanner, not a GRAM panel. It owns an exclusive SPI host
// and continuously sends the framebuffer, including its configured porches.
// Pins are physical GPIO numbers. Module power/gamma values may be overridden
// with init; geometry, RGB565 and porch commands must match this configuration.
struct ScanQspiConfig {
  spi_host_device_t host = SPI2_HOST;
  int clock = -1, data[4] = {-1, -1, -1, -1}, cs = -1, reset = -1;
  uint16_t width = 400, height = 400;
  uint32_t frequency = 40000000;
  uint8_t frontPorch = 8, backPorch = 8;
  uint16_t minimumLineUs = 42;
  uint8_t batchLines = 20, batchCount = 3;
  uint32_t taskStackBytes = 4096;
  uint8_t taskPriority = 23;
  // -1 selects the initializing core, which also owns the SPI interrupt.
  int taskCore = -1;
  bool framebufferInPsram = true, resetActiveHigh = false;
  bool displayOnInit = true, bgr = false;
  VideoInitOverride init;
};

class Esp32ScanQspi : public FramebufferTransport {
 public:
  explicit Esp32ScanQspi(const ScanQspiConfig &config) : config_(config) {}
  ~Esp32ScanQspi() override { end(); }
  Status begin() override;
  void end() override;
  Status shutdown();
  Status status() const;
  Status command(uint8_t, const uint8_t *, size_t) override;
  Status wait() override;
  bool busy() const override { return false; }
  bool validOutputPin(int pin) const override;
  TransportInfo info() const override;
  void output(int pin, bool high) override;
  void delayMs(uint32_t ms) override;
  const ScanQspiConfig &config() const { return config_; }
  // A read-only native RGB565 view. Writes use the normal window API so they
  // cannot race the worker's bounded framebuffer-to-DMA copies.
  const uint16_t *framebuffer() const;
 protected:
  Status drawBitmap(int, int, int, int, const uint16_t *) override;
 private:
  struct State;
  static void workerEntry(void *);
  Status startScan();
  Status stopScan();
  Status sendRegister(uint8_t, const uint8_t *, size_t);
  Status initializeController();
  Status display(bool);
  ScanQspiConfig config_;
  State *state_ = nullptr;
  Status result_ = Status::Ok;
};
} // namespace nice_gfx
#endif
