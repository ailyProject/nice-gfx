#pragma once
#include "../../Display/FramebufferTransport.h"
#include "../../Display/VideoControllerTypes.h"

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include "esp_idf_version.h"
#include "soc/soc_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_dev.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_attr.h"
#include <atomic>
#if SOC_LCD_RGB_SUPPORTED
#include "esp_lcd_panel_rgb.h"
#endif
#if SOC_PPA_SUPPORTED && defined(CONFIG_IDF_TARGET_ESP32S31)
#include "driver/ppa.h"
#endif
#if SOC_MIPI_DSI_SUPPORTED
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"
#endif

namespace nice_gfx {

// GFX controller options; there is no borrowed component/vendor configuration.
struct VideoPanelConfig {
  unsigned bits_per_pixel = 16;
  int rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
  struct { unsigned reset_active_high : 1; } flags = {};
};

// GPIO-only command channel for RGB panels that use a ninth command/data bit.
// The names also match the editor's existing hardware fields. Command and
// parameter widths are one byte; expect_clk_speed limits the bit-banged clock.
struct VideoSpiConfig {
  struct { int cs_gpio_num = -1, scl_gpio_num = -1, sda_gpio_num = -1; } line_config;
  uint32_t expect_clk_speed = 1000000;
  uint8_t spi_mode = 0, lcd_cmd_bytes = 1, lcd_param_bytes = 1;
  struct {
    unsigned use_dc_bit : 1;
    unsigned lsb_first : 1;
    unsigned dc_zero_on_data : 1;
    unsigned cs_high_active : 1;
  } flags = {1, 0, 0, 0};
};

#if SOC_LCD_RGB_SUPPORTED
class VideoScanStats {
  std::atomic<uint32_t> frames_{0}, period_{0};
 public:
  void reset(){frames_.store(0,std::memory_order_relaxed);period_.store(0,std::memory_order_relaxed);}
  void IRAM_ATTR record(uint32_t delta){
    frames_.fetch_add(1,std::memory_order_relaxed);
    if(delta>=1000&&delta<500000){const uint32_t old=period_.load(std::memory_order_relaxed);period_.store(old?(old*7+delta)/8:delta,std::memory_order_relaxed);}
  }
  uint32_t frames() const{return frames_.load(std::memory_order_relaxed);}
  uint32_t periodMicros() const{return period_.load(std::memory_order_relaxed);}
};
struct VideoCopyStats {
  uint64_t cpuPixels=0, ppaPixels=0;
  uint32_t ppaTransfers=0, ppaFailures=0, ppaVerificationFailures=0;
  bool ppaReady=false;
  uint64_t ppaFillPixels=0;
  uint32_t ppaFillTransfers=0, ppaFillFailures=0, ppaFillVerificationFailures=0;
  bool ppaFillReady=false;
  uint64_t mappedPixels=0;
  uint32_t mappedTransfers=0;
  uint64_t inheritedPixels=0,inheritMicros=0,linkWaitMicros=0;
  uint32_t frameCommits=0,frameSwitches=0,frameAborts=0,linkTimeouts=0;
  uint64_t ppaCopyDriverMicros=0,ppaCopySyncMicros=0;
};
struct RgbConfig {
  VideoController controller = VideoController::ST7262;
  int reset = -1;
  VideoPanelConfig panel;
  esp_lcd_rgb_panel_config_t rgb = {};
  VideoSpiConfig io3wire;
  // A non-null init.commands replaces the controller's default program.
  // Keep that program and its data alive until begin() has returned.
  VideoInitOverride init;
  bool createIo = true, displayOnInit = true;
  // Optional synchronous command transport supplied by the application.
  // The caller owns context through shutdown(); errors abort initialization.
  // Set createIo=false when supplying this callback. Pixel data stays on RGB.
  VideoWrite commandWrite = nullptr;
  void *commandContext = nullptr;
  // Optional S31 hardware copy/byte swap and large solid fills. Unsupported targets or failed
  // transactions retain the CPU path. The diagnostics storage is borrowed.
  bool usePpa = false;
  // S31 RGB stream mode, or S3 with internal bounce buffers: render complete
  // batches into an inactive framebuffer and publish once. Requires one
  // additional screen-sized PSRAM buffer. S3 releases the old source only
  // after the bounce engine has latched the new source on its interrupt core.
  bool frameAtomic = false;
  VideoCopyStats *copyStats = nullptr;
  VideoScanStats *scanStats = nullptr;
  // Optional initialization affinity, so RGB/bounce interrupts can run on a
  // different CPU from the UI. -1 preserves the caller's core. No UI calls
  // cross tasks; the temporary initialization task completes before begin.
  int8_t interruptCore = -1;
  RgbConfig();
};
#endif

#if SOC_MIPI_DSI_SUPPORTED
struct MipiConfig {
  VideoController controller = VideoController::ILI9881C;
  int reset = -1;
  VideoPanelConfig panel;
  esp_lcd_dsi_bus_config_t dsi = {};
  esp_lcd_dbi_io_config_t dbi = {};
  esp_lcd_dpi_panel_config_t dpi = {};
  esp_ldo_channel_config_t ldo = {};
  VideoInitOverride init;
  bool acquireLdo = true, displayOnInit = true;
  MipiConfig();
};
#endif

// Controller commands are executed by GFX's native programs. Only the SoC RGB
// and DPI hardware engines use ESP-IDF panel handles. Source-copy completion
// permits staging reuse. Opt-in atomic RGB additionally fences inactive-buffer
// reuse on S31 DMA link completion or S3 bounce source-latch completion.
#if SOC_LCD_RGB_SUPPORTED || SOC_MIPI_DSI_SUPPORTED
class VideoTransport : public FramebufferTransport {
 public:
  ~VideoTransport() override { end(); }
  void end() override;
  Status shutdown();
  Status status() const { return result_; }
  Status command(uint8_t c, const uint8_t *data, size_t bytes) override;
  Status wait() override;
  Status present() override;
  Status beginFrame(const DirtyRect *regions,size_t count) override;
  Status commitFrame() override;
  Status abortFrame() override;
  Status mapNativeFramebuffer(NativeFramebuffer &frame) override;
  bool busy() const override { return pending_; }
  bool supportsFrameBlit() const override {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    // The RGB framebuffer also accepts strided CPU copies on S3. Hardware PPA
    // is not required to skip scanline staging and expose native rendering.
    return rgbFramebuffer_ != nullptr;
#elif SOC_PPA_SUPPORTED && defined(CONFIG_IDF_TARGET_ESP32S31)
    return rgbFramebuffer_ && ppaCopy_;
#else
    return false;
#endif
  }
  bool validOutputPin(int pin) const override;
  void output(int pin, bool high) override;
  void delayMs(uint32_t ms) override;
 protected:
  explicit VideoTransport(VideoInterface interface) : interface_(interface) {}
  Status initializeHardware(VideoController controller, const VideoPanelConfig &panel,
                            const VideoInitOverride &init, int reset,
                            size_t width, size_t height, unsigned lanes,
                            bool displayOnInit, bool hasDisplayPin);
  Status prepareCommandSpi(const VideoSpiConfig &config);
  Status fail(esp_err_t error);
  Status fail(Status reason);
  Status drawBitmap(int x0, int y0, int x1, int y1, const uint16_t *pixels) override;
  Status drawPixelView(int x, int y, PixelView view) override;
  bool directPixelView() const override { return rgbFramebuffer_ != nullptr; }
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
  bool directFill() const override { return rgbFramebuffer_ != nullptr; }
  Status drawFillRect(int x, int y, size_t width, size_t height, uint16_t color) override;
  Status synchronizeFramebuffer(int x, int y, size_t width, size_t height, bool cpuWritten);
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
  Status beginAtomicFrame(const DirtyRect *regions,size_t count,bool explicitFrame);
  Status ensureAtomicFrame();
  Status waitAtomicFlip();
  Status inheritAtomicRect(const DirtyRect &area);
  static constexpr size_t AtomicMaxDamage=16,AtomicMaxInherit=64;
  bool frameAtomic_=false,frameOpen_=false,frameExplicit_=false,staleFull_=true,frameDamageFull_=true;
  std::atomic<bool> flipPending_{false};
  SemaphoreHandle_t frameCompleted_=nullptr;
  uint16_t *rgbFrames_[2]={nullptr,nullptr};
  uint8_t frontIndex_=0;
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  int atomicCore_=-1;
  esp_err_t atomicSubmitError_=ESP_OK;
  static void publishS3Atomic(void *context);
#endif
  DirtyRect staleDamage_[AtomicMaxDamage],frameDamage_[AtomicMaxDamage];
  size_t staleCount_=0,frameDamageCount_=0;
#endif
  uint16_t *rgbFramebuffer_ = nullptr;
#if SOC_LCD_RGB_SUPPORTED
  VideoCopyStats *copyStats_ = nullptr;
  VideoScanStats *scanStats_ = nullptr;
  uint32_t lastVsync_ = 0;
#endif
#if SOC_PPA_SUPPORTED && defined(CONFIG_IDF_TARGET_ESP32S31)
  ppa_client_handle_t ppaCopy_ = nullptr;
  ppa_client_handle_t ppaFill_ = nullptr;
  bool ppaVerified_ = false, ppaPartialVerified_ = false;
  bool ppaFillVerified_ = false, ppaFillPartialVerified_ = false, ppaFillRejected_ = false;
  bool copyWithPpa(int x, int y, PixelView view);
  bool fillWithPpa(int x, int y, size_t width, size_t height, uint16_t color);
#endif
  static Status convert(esp_err_t error);
  esp_lcd_panel_handle_t panel_ = nullptr;
  esp_lcd_panel_io_handle_t io_ = nullptr;
  void *dsiBus_ = nullptr, *ldo_ = nullptr;
  bool refreshOnDemand_ = false;
  VideoWrite commandWrite_ = nullptr;
  void *commandContext_ = nullptr;
 private:
  static Status writeCommand(uint8_t c, const uint8_t *data, size_t bytes, void *context);
  static void controllerDelay(uint32_t ms, void *context);
  Status transmitCommand(uint8_t c, const uint8_t *data, size_t bytes);
  Status display(bool on);
  void spiByte(uint8_t byte, bool data);
  void spiBit(bool high);
  static bool IRAM_ATTR complete(void *context);
#if SOC_LCD_RGB_SUPPORTED
  static bool IRAM_ATTR rgbDone(esp_lcd_panel_handle_t,
                               const esp_lcd_rgb_panel_event_data_t *, void *context);
  static bool IRAM_ATTR rgbVsync(esp_lcd_panel_handle_t,
                               const esp_lcd_rgb_panel_event_data_t *, void *context);
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
  static bool IRAM_ATTR rgbFrameDone(esp_lcd_panel_handle_t,
                               const esp_lcd_rgb_panel_event_data_t *, void *context);
#endif
#endif
#if SOC_MIPI_DSI_SUPPORTED
  static bool IRAM_ATTR dpiDone(esp_lcd_panel_handle_t,
                               esp_lcd_dpi_panel_event_data_t *, void *context);
#endif
  VideoInterface interface_;
  VideoController controller_ = VideoController::ST7262;
  SemaphoreHandle_t completed_ = nullptr;
  uint16_t *scratch_ = nullptr;
  volatile bool pending_ = false;
  bool hasSpi_ = false, hasDisplayPin_ = false, swapRedBlue_ = false;
  VideoSpiConfig spi_;
  uint32_t halfPeriodUs_ = 1;
  Status result_ = Status::Ok;
};

#if SOC_LCD_RGB_SUPPORTED
class Esp32Rgb : public VideoTransport {
 public:
  explicit Esp32Rgb(const RgbConfig &config) : VideoTransport(VideoInterface::RGB), config_(config) {}
  Status begin() override;
  Status setPixelClockHz(uint32_t hz) override;
  TransportInfo info() const override;
 private:
  Status beginConfigured();
  RgbConfig config_;
};
#endif
#if SOC_MIPI_DSI_SUPPORTED
class Esp32Mipi : public VideoTransport {
 public:
  explicit Esp32Mipi(const MipiConfig &config) : VideoTransport(VideoInterface::MIPI), config_(config) {}
  Status begin() override;
  TransportInfo info() const override;
 private:
  MipiConfig config_;
};
#endif
#endif // SOC_LCD_RGB_SUPPORTED || SOC_MIPI_DSI_SUPPORTED
} // namespace nice_gfx
#endif
