#pragma once
#include "../../Core/BuildConfig.h"
#include "../../Display/RetainedRepaint.h"
#ifndef NICE_GFX_P4_MIPI_PARTIAL_BOUNDARY_GUARD
#define NICE_GFX_P4_MIPI_PARTIAL_BOUNDARY_GUARD 1
#endif
#ifndef NICE_GFX_P4_MIPI_INHERIT_PREFETCH
#define NICE_GFX_P4_MIPI_INHERIT_PREFETCH 0
#endif
#define NICE_GFX_P4_MIPI_INHERIT_PREFETCH_STATS_VERSION 1
// Private bounded strategy selection; original DMA/fences remain unchanged.
#ifndef NICE_GFX_P4_MIPI_NO_PPA_FULL_INHERIT
#define NICE_GFX_P4_MIPI_NO_PPA_FULL_INHERIT 0
#endif
#define NICE_GFX_P4_MIPI_NO_PPA_FULL_INHERIT_STATS_VERSION 1
#ifndef NICE_GFX_P4_MIPI_DELEGATE_SDK_FLUSH
#define NICE_GFX_P4_MIPI_DELEGATE_SDK_FLUSH 1
#endif
#ifndef NICE_GFX_P4_MIPI_SDK_FLUSH_AUDIT_ID
#define NICE_GFX_P4_MIPI_SDK_FLUSH_AUDIT_ID 0xb774170f
#endif
#define NICE_GFX_P4_MIPI_SDK_FLUSH_STATS_VERSION 1
#define NICE_GFX_P4_MIPI_PARTIAL_INHERIT_PPA_STATS_VERSION 1
#ifndef NICE_GFX_P4_MIPI_PARTIAL_INHERIT_PPA
#define NICE_GFX_P4_MIPI_PARTIAL_INHERIT_PPA 1
#endif
#ifndef NICE_GFX_P4_MIPI_PARTIAL_INHERIT_MIN_PIXELS
#define NICE_GFX_P4_MIPI_PARTIAL_INHERIT_MIN_PIXELS 8192
#endif
// Capability marker for diagnostics shared with canonical baseline builds.
#define NICE_GFX_P4_MIPI_FULL_INHERIT_GDMA_STATS_VERSION 2
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
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE && defined(CONFIG_IDF_TARGET_ESP32P4) && ESP_IDF_VERSION == ESP_IDF_VERSION_VAL(5,5,5) && defined(CONFIG_ESP_REV_MIN_FULL) && CONFIG_ESP_REV_MIN_FULL >= 300
#define NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE 1
#include "freertos/task.h"
#include "NativePresentationState.h"
#else
#define NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE 0
#endif
#if SOC_LCD_RGB_SUPPORTED
#include "esp_lcd_panel_rgb.h"
#endif
#ifndef NICE_GFX_S31_PIPELINED_FRAMEBUFFERS
#define NICE_GFX_S31_PIPELINED_FRAMEBUFFERS 1
#endif
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS && defined(CONFIG_IDF_TARGET_ESP32S31) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6,1,0)
#define NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE 1
#include "RgbAtomicPipeline.h"
#else
#define NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE 0
#endif
#if SOC_PPA_SUPPORTED && (defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32P4))
#include "driver/ppa.h"
#endif
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#include "esp_async_memcpy.h"
#endif
#if SOC_MIPI_DSI_SUPPORTED
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"
#endif

namespace nice_gfx {
namespace detail {struct NativeStageEngine;}

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
#define NICE_GFX_P4_MIPI_REPAINT_INHERIT_STATS_VERSION 1
struct VideoCopyStats {
  bool inheritPrefetchPending=false;
  uint32_t inheritPrefetchAttempts=0,inheritPrefetchAccepted=0,inheritPrefetchCompleted=0,inheritPrefetchConsumed=0,inheritPrefetchRejected=0;
  uint64_t inheritPrefetchPixels=0,inheritPrefetchSubmitMicros=0,inheritPrefetchDrainMicros=0,inheritPrefetchPostSyncMicros=0,inheritPrefetchLifetimeMicros=0;
  uint32_t sdkFlushDelegatedCommits=0,sdkFlushManualCalls=0,sdkFlushValidationFallbacks=0;
  uint64_t sdkFlushDelegatedBytes=0,sdkFlushValidationMicros=0,sdkFlushManualMicros=0;
  uint32_t repaintInheritFrames=0,repaintInheritFallbackFrames=0,repaintInheritFragments=0;
  uint64_t repaintInheritSkippedPixels=0,repaintInheritPlanMicros=0;
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
  uint8_t framebufferCount=1;
  bool pipelinedFramebuffers=false;
  uint64_t ppaCopyDriverMicros=0,ppaCopySyncMicros=0;
  uint64_t framebufferSyncMicros=0;
  uint64_t cacheSyncBytes=0;
  uint32_t cacheSyncCalls=0,cacheBatches=0,cacheCoalescedWrites=0;
  uint64_t gdmaPixels=0;
  uint32_t gdmaTransfers=0,gdmaFailures=0,gdmaVerificationFailures=0;
  bool gdmaReady=false;
  bool partialInheritPpaReady=false;
  uint32_t partialInheritPpaCpuTransfers=0,partialInheritPpaAttempts=0,partialInheritPpaTransfers=0;
  uint32_t partialInheritPpaRejected=0,partialInheritPpaTimeouts=0,partialInheritPpaVerificationFailures=0;
#if NICE_GFX_P4_MIPI_PARTIAL_BOUNDARY_GUARD
#define NICE_GFX_P4_MIPI_PARTIAL_BOUNDARY_GUARD_STATS_VERSION 1
  uint32_t partialInheritPpaBoundaryRejects=0;
#endif
  uint64_t partialInheritPpaPixels=0,partialInheritPpaMicros=0,partialInheritPpaSyncMicros=0;
  uint64_t partialInheritPpaWaitMicros=0,partialInheritPpaVerifyMicros=0;
  // Experimental full-frame front-to-inactive inheritance, not arbitrary FB copy.
  uint32_t fullInheritCpuTransfers=0,fullInheritGdmaAttempts=0,fullInheritGdmaTransfers=0;
  uint32_t fullInheritGdmaRejected=0,fullInheritGdmaTimeouts=0,fullInheritGdmaVerificationFailures=0;
  uint64_t fullInheritGdmaMicros=0,fullInheritGdmaSyncMicros=0;
  uint64_t fullInheritGdmaWaitMicros=0,fullInheritGdmaVerifyMicros=0;
  // v2: no framebuffer mapping is needed to explain an unsupported acquisition.
  bool fullInheritGdmaReady=false;
  int32_t fullInheritGdmaInstallError=0;
  uint16_t fullInheritGdmaBurstBytes=0,fullInheritGdmaCacheLineBytes=0;
  uint16_t fullInheritGdmaSourceLowBits=0,fullInheritGdmaTargetLowBits=0;
  uint32_t fullInheritGdmaUnsupportedMask=0,fullInheritGdmaLastUnsupportedReason=0;
  // 1 ForceCPU, 2 NoEngine, 3 NoSemaphore, 4 NoCacheLine, 5 NonYieldContext,
  // 6 InvalidSize, 7 CacheAlignment, 8 BurstAlignment, 9 NotPSRAM.
  uint32_t fullInheritGdmaUnsupportedReasons[9]={};
  // Lightweight bridge keeps this diagnostics record normally copyable.
  // Readers are installed only while the originating transport is alive.
  const void *presentationContext=nullptr;
  uint32_t (*presentationSequenceReader)(const void *)=nullptr;
  bool (*presentationTimestampReader)(const void *,uint32_t,uint32_t &)=nullptr;
  uint32_t presentationSequence()const{return presentationContext&&presentationSequenceReader?
    presentationSequenceReader(presentationContext):0;}
  bool presentationTimestamp(uint32_t sequence,uint32_t &micros)const{return presentationContext&&presentationTimestampReader&&
    presentationTimestampReader(presentationContext,sequence,micros);}
  // Selection counts are not successful transfers; use existing GDMA counters.
  uint32_t noPpaFullInheritSelections=0,noPpaFullInheritIneligible=0;
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
  // Single-buffer RGB565 uses the SDK-owned scanout surface directly. CPU
  // writes still synchronize dirty cache lines before returning. Multi-buffer
  // and other output formats retain the ordinary SDK copy path.
  bool useFramebuffer = true;
  // P4 RGB565: render into the other SDK framebuffer and publish one complete
  // frame. Requires IDF >= 5.5.5 frame-buffer-complete ownership callbacks.
  // False preserves the existing single-buffer/copy paths and their timings.
  bool frameAtomic = false;
  // P4 hardware fill/copy/byte-swap, with exact CPU fallback. Small operations
  // stay on the CPU. No extra framebuffer and no change to panel timing.
  bool usePpa =
#if defined(CONFIG_IDF_TARGET_ESP32P4) && NICE_GFX_P4_DISABLE_PPA
    false;
#else
    true;
#endif
  // AXI-GDMA is faster for large contiguous native rows. Source RAM must be
  // 16-byte aligned; other views retain PPA/CPU. Uses one AXI channel pair.
  bool useGdma =
#if defined(CONFIG_IDF_TARGET_ESP32P4) && NICE_GFX_P4_DISABLE_GDMA
    false;
#else
    true;
#endif
  // Coalesce CPU cache writeback within startWrite()/endWrite() and explicit
  // frames. Allocation failure keeps immediate writeback. This is not a flip.
  bool batchWrites = true;
#if SOC_LCD_RGB_SUPPORTED
  VideoCopyStats *copyStats = nullptr;
  VideoScanStats *scanStats = nullptr;
#endif
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
  Status beginWrite() override;
  Status beginFrame(const DirtyRect *regions,size_t count) override;
  Status beginRepaintFrame(const DirtyRect *regions,size_t count) override;
  Status beginRetainedRepaintFrame(const DirtyRect *,size_t,const RetainedRepaintRequest *,size_t,RetainedRepaintReceipt *) override;
  Status mapRetainedRepaintFramebuffer(const RetainedRepaintReceipt *,size_t,NativeFramebuffer &) override;
  size_t retainedRepaintStorageBytes()const override{return sizeof(retainedTags_)+sizeof(retainedAcquiring_)+sizeof(retainedMapping_);}
  Status validateRetainedRepaintFrame(const RetainedRepaintReceipt *,size_t) override;
  Status certifyRetainedRepaintFrame(const RetainedRepaintReceipt *,size_t) override;
  Status commitFrame() override;
  Status abortFrame() override;
  Status mapNativeFramebuffer(NativeFramebuffer &frame) override;
  bool supportsNativeStage()const override;
  NativeStageTransfer copyNativeStage(const NativeFramebuffer &,const DirtyRect &,const NativeStageSource &)noexcept override;
  NativeStageProviderStats nativeStageProviderStats()const override;
  Status copySurfaceRows(const NativeFramebuffer &,int32_t,PixelView) override;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  bool supportsNativePresentationPipeline()const override;
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE && NICE_GFX_P4_NATIVE_PRESENTATION_HISTORY_COALESCE
  detail::NativePresentationState::HistoryStats nativeInheritanceHistoryStats()const {
    return mipiPresentation_.historyStats();
  }
#endif
  Status beginNativeDrawing(const DirtyRect *,size_t,bool,NativeDrawingLease &) override;
  Status mapNativeDrawing(const NativeDrawingLease &,NativeFramebuffer &) override;
  NativeStageTransfer copyNativeDrawingStage(const NativeDrawingLease &,const NativeFramebuffer &,const DirtyRect &,const NativeStageSource &)noexcept override;
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL
  NativeStageTransfer fillNativeDrawing(const NativeDrawingLease &,const NativeFramebuffer &,const DirtyRect &,uint16_t)noexcept override;
#endif
  Status syncNativeDrawing(const NativeDrawingLease &)noexcept override;
  Status sealNativeDrawing(const NativeDrawingLease &)noexcept override;
  Status abortNativeDrawing(const NativeDrawingLease &)noexcept override;
  NativePresentationTransfer submitNativePresentation(const NativeDrawingLease &)noexcept override;
  NativePresentationTransfer pollNativePresentation(const NativePresentationReceipt &,bool)noexcept override;
#endif
  // Actual S31 DMA link publications, independent of accepted submissions.
  // The opt-in pipeline retains 256 exact ISR timestamps for diagnostics.
  uint32_t presentationSequence()const;
  bool presentationTimestamp(uint32_t sequence,uint32_t &micros)const;
  bool busy() const override {
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
    if(mipiPresentationBound_)return pending_||mipiPresentation_.pending()!=detail::NativePresentationState::None||mipiPresentation_.retirementPending();
#endif
#if defined(CONFIG_IDF_TARGET_ESP32P4) && NICE_GFX_P4_MIPI_INHERIT_PREFETCH
    return pending_||mipiPrefetchActive_;
#else
    return pending_;
#endif
  }
  bool supportsFrameBlit() const override {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    // The RGB framebuffer also accepts strided CPU copies on S3. Hardware PPA
    // is not required to skip scanline staging and expose native rendering.
    return rgbFramebuffer_ != nullptr;
#elif SOC_PPA_SUPPORTED && defined(CONFIG_IDF_TARGET_ESP32S31)
    return rgbFramebuffer_ && ppaCopy_;
#elif defined(CONFIG_IDF_TARGET_ESP32P4)
    return interface_ == VideoInterface::MIPI && rgbFramebuffer_ != nullptr;
#else
    return false;
#endif
  }
  bool validOutputPin(int pin) const override;
  void output(int pin, bool high) override;
  void delayMs(uint32_t ms) override;
 protected:
  explicit VideoTransport(VideoInterface interface) : interface_(interface) {}
  Status reshapeRaster(size_t width,size_t height){return startRaster(width,height,scratch_,rasterWidth());}
  Status recordVideoResult(Status status){return result_=status;}
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
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32P4)
  // P4 RGB retains its existing SDK path; direct fills apply to mapped MIPI.
  bool directFill() const override {
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    return interface_ == VideoInterface::MIPI && rgbFramebuffer_ != nullptr;
#else
    return rgbFramebuffer_ != nullptr;
#endif
  }
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
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
  bool framePipelined_=false;
  uint16_t *pipelineThird_=nullptr;
  detail::RgbAtomicPipeline pipeline_;
  std::atomic<uint32_t> pipelineArmed_{0},pipelineAcknowledged_{0},pipelinePublished_{0};
  struct PresentationStamp {std::atomic<uint32_t> sequence{0},micros{0};};
  static constexpr size_t PresentationHistory=256;
  PresentationStamp presentationHistory_[PresentationHistory];
  uint16_t *pipelineBuffer(uint8_t index)const{return index<2?rgbFrames_[index]:index==2?pipelineThird_:nullptr;}
  Status waitPipelineFlip(bool block);
  Status beginPipelineFrame(const DirtyRect *regions,size_t count,bool explicitFrame,
    DirtyRect (&needed)[AtomicMaxInherit],DirtyRect (&next)[AtomicMaxInherit]);
  Status commitPipelineFrame();
#endif
#endif
  uint16_t *rgbFramebuffer_ = nullptr;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  struct MipiDirtySpan { uint16_t left, right; };
  MipiDirtySpan *mipiDirty_ = nullptr;
  size_t mipiDirtyTop_=0,mipiDirtyBottom_=0,ppaCacheLine_=0;
  bool mipiBatch_=false,mipiFrame_=false;
  Status flushMipiDirty();
  // P4 MIPI owns exactly two SDK allocations. Only mipiFrames_[front^1] may
  // be mapped or written; the previous front stays borrowed until the fence.
#if NICE_GFX_P4_NATIVE_STAGE_PROVIDER
  detail::NativeStageEngine*nativeStageEngine_=nullptr;
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL
  detail::NativeStageEngine*nativeFillEngine_=nullptr;
  bool mipiFillPreparing_=false;
#endif
#endif
  bool mipiAtomic_=false,mipiAtomicOpen_=false,mipiAtomicExplicit_=false,mipiAtomicPending_=false;
  bool mipiDirectSdkDpi_=false;
  bool canDelegateMipiSdkFlush(size_t bytes) const;
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  uint16_t *mipiFrames_[3]={nullptr,nullptr,nullptr};
  detail::NativePresentationState mipiPresentation_{};
  NativeDrawingLease mipiDrawingLease_{},mipiReadyLease_{};
  NativePresentationReceipt mipiPresentationReceipt_{},mipiRetiredReceipt_{};
  uint64_t mipiPresentationOwner_=0;
  TaskHandle_t mipiPresentationTask_=nullptr;
  bool mipiPresentationBound_=false,mipiTypedDrawing_=false,mipiTypedReady_=false;
  bool mipiTypedPending_=false,mipiTypedRetired_=false;
  uint8_t mipiSdkFramebufferCount_=0;
  bool validMipiDrawing(const NativeDrawingLease &)const;
  bool mipiDrawingCovers(const DirtyRect &)const;
  bool mipiPresentationTask()const;
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL
  Status drainNativeDrawingFill()noexcept;
#endif
  Status copyOnlyMipiWait();
  Status beginMipiPresentationLegacy(bool,const DirtyRect *,size_t,bool);
  Status mapMipiPresentationLegacy(NativeFramebuffer &);
  Status commitMipiPresentationLegacy();
  Status abortMipiPresentationLegacy();
  Status presentMipiPresentation();
  Status observeMipiPresentation(bool,bool &);
  Status claimMipiPresentation(bool &);
  Status quiesceMipiPresentation();
  bool delegateMipiPresentationFlush(size_t)const;
#else
  uint16_t *mipiFrames_[2]={nullptr,nullptr};
#endif
  uint8_t mipiFront_=0;
  Status mipiAtomicFailure_=Status::Ok;
  SemaphoreHandle_t mipiFrameFence_=nullptr;
  std::atomic<uint32_t> mipiFrameSequence_{0};
  uint32_t mipiFenceBaseline_=0;
  // The allocation caller is useful diagnostics, but only an observed DMA
  // callback establishes the actual interrupt core used by the SDK.
  int mipiInitCore_=-1;
  std::atomic<int> mipiInterruptCore_{-1};
  uint8_t mipiFenceCompletions_=2;
  static constexpr size_t MipiMaxDamage=16;
  bool mipiStaleFull_=true,mipiDamageFull_=true;
  DirtyRect mipiStaleDamage_[MipiMaxDamage],mipiDamage_[MipiMaxDamage];
  size_t mipiStaleCount_=0,mipiDamageCount_=0;
  Status beginMipiAtomicFrame(bool explicitFrame,const DirtyRect *regions=nullptr,size_t count=0,bool repaint=false);
  Status inheritMipiAtomicRect(const DirtyRect &rect);
  void noteMipiDamage(int x,int y,size_t width,size_t height);
  Status commitMipiAtomicFrame();
  Status waitMipiAtomicFlip();
  Status failMipiAtomic(Status status);
  void clearMipiDirty();
  async_memcpy_handle_t mipiGdma_=nullptr;
  SemaphoreHandle_t mipiGdmaDone_=nullptr;
  bool mipiGdmaVerified_=false;
  // A submitted full-inheritance transfer never escapes its blocking fence.
  bool mipiFullInheritPending_=false,mipiFullInheritVerified_=false;
#if NICE_GFX_P4_MIPI_NO_PPA_FULL_INHERIT
  bool mipiPpaConfigured_=false;
#endif
  Status copyMipiFullInheritanceGdma();
  Status drainMipiFullInheritanceGdma();
  ppa_client_handle_t mipiPartialInheritPpa_=nullptr;
  SemaphoreHandle_t mipiPartialInheritDone_=nullptr;
  bool mipiPartialInheritPending_=false,mipiPartialInheritVerified_=false;
  Status copyMipiPartialInheritancePpa(const DirtyRect &r);
  bool mipiPrefetchActive_=false,mipiPrefetchReady_=false;
  uint32_t mipiOwnershipEpoch_=0,mipiPrefetchGeneration_=0;
  size_t mipiPrefetchIndex_=MipiMaxDamage,mipiPrefetchRowBytes_=0;
  DirtyRect mipiPrefetchRect_;
  const uint16_t *mipiPrefetchSource_=nullptr;
  uint16_t *mipiPrefetchTarget_=nullptr,*mipiPrefetchRows_=nullptr;
  int64_t mipiPrefetchStarted_=0;
  Status queueMipiInheritancePrefetch(const DirtyRect &r,size_t index);
  Status startMipiInheritancePrefetch();
  Status drainMipiInheritancePrefetch();
  Status drainMipiPartialInheritancePpa();
  static bool IRAM_ATTR partialInheritPpaDone(ppa_client_handle_t,ppa_event_data_t*,void*);
  bool copyWithGdma(int x,int y,PixelView view);
  // Complete cache-aligned RAM spans only. The blocking fence covers every
  // accepted source; callers own bounds/order validation for their surface.
  bool copyContiguousGdma(uint16_t *target,const void *source,size_t bytes);
  static bool IRAM_ATTR gdmaDone(async_memcpy_handle_t,async_memcpy_event_t*,void *context);
#endif
#if SOC_LCD_RGB_SUPPORTED
  VideoCopyStats *copyStats_ = nullptr;
  VideoScanStats *scanStats_ = nullptr;
  uint32_t lastVsync_ = 0;
#endif
#if SOC_PPA_SUPPORTED && (defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32P4))
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
 protected:
  void resetRetainedRepaintCertificates(){retainedTags_.reset();}
 private:
  detail::RetainedRepaintTags retainedTags_;bool retainedAcquiring_=false,retainedMapping_=false;
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
  static bool IRAM_ATTR dpiVsync(esp_lcd_panel_handle_t,
                                esp_lcd_dpi_panel_event_data_t *, void *context);
#if defined(CONFIG_IDF_TARGET_ESP32P4) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,5,5)
  static bool IRAM_ATTR dpiFrameDone(esp_lcd_panel_handle_t,
                                    esp_lcd_dpi_panel_event_data_t *, void *context);
#endif
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
