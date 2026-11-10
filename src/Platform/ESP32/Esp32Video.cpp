#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include "Esp32Video.h"
#include "../../Display/RepaintInheritance.h"
#if (defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)) && (SOC_LCD_RGB_SUPPORTED || SOC_MIPI_DSI_SUPPORTED)
#include "../../Drivers/Display/Video/VideoControllers.h"
#include "Esp32Platform.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#include "esp_ipc.h"
#endif
#include "freertos/task.h"
#include <string.h>
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32P4)
#include "esp_cache.h"
#endif
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#include "esp_private/esp_cache_private.h"
#include "hal/cache_hal.h"
#include "esp_memory_utils.h"
#endif

#if NICE_GFX_P4_NATIVE_STAGE_PROVIDER && defined(CONFIG_IDF_TARGET_ESP32P4) && SOC_PPA_SUPPORTED && ESP_IDF_VERSION == ESP_IDF_VERSION_VAL(5,5,5)
#define NICE_GFX_NATIVE_STAGE_ENGINE_AVAILABLE 1
#include "NativeStageEngine.h"
#else
#define NICE_GFX_NATIVE_STAGE_ENGINE_AVAILABLE 0
#endif
namespace nice_gfx {
#include "NativePresentationBridge.inc.h"
bool VideoTransport::supportsNativeStage()const{
#if NICE_GFX_NATIVE_STAGE_ENGINE_AVAILABLE
  return interface_==VideoInterface::MIPI&&mipiAtomic_&&ppaCopy_&&ppaCacheLine_&&ppaCacheLine_<=64&&64%ppaCacheLine_==0&&!swapRedBlue_&&panel_&&mipiFrames_[0]&&mipiFrames_[1]&&mipiInitCore_==1;
#else
  return false;
#endif
}
NativeStageProviderStats VideoTransport::nativeStageProviderStats()const{
#if NICE_GFX_NATIVE_STAGE_ENGINE_AVAILABLE
  if(nativeStageEngine_)return nativeStageEngine_->stats;
#endif
  return {};
}
NativeStageTransfer VideoTransport::copyNativeStage(const NativeFramebuffer&frame,const DirtyRect&r,const NativeStageSource&source)noexcept{
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_)return copyNativeDrawingStage(mipiDrawingLease_,frame,r,source);
#endif
#if NICE_GFX_NATIVE_STAGE_ENGINE_AVAILABLE
  if(mipiAtomicFailure_!=Status::Ok)return {mipiAtomicFailure_,false,false};
  if(nativeStageEngine_&&(nativeStageEngine_->pending||nativeStageEngine_->ownerTask!=xTaskGetCurrentTaskHandle())){
    const auto poison=+[](void*context,Status status){static_cast<VideoTransport*>(context)->failMipiAtomic(status);};
    nativeStageEngine_->fail(poison,this);return {mipiAtomicFailure_,false,false};
  }
  // Failed ownership is not Unsupported: the caller must never CPU-fallback
  // into an unknown or currently scanned allocation.
  if(!supportsNativeStage()||!mipiAtomicOpen_||!mipiAtomicExplicit_||mipiAtomicPending_||pending_||mipiFullInheritPending_||mipiPartialInheritPending_||mipiPrefetchActive_||mipiPrefetchReady_||retainedTags_.open()||mipiFront_>1||
    mipiFrames_[0]==mipiFrames_[1]||rgbFramebuffer_!=mipiFrames_[mipiFront_^1]||!frame.fencedAtomicMipiRmw||frame.pixels!=rgbFramebuffer_||frame.width!=rasterWidth()||frame.height!=rasterHeight()||frame.stride!=rasterWidth()||frame.bytes!=rasterWidth()*rasterHeight()*2||
    !ppaCacheLine_||ppaCacheLine_>64||64%ppaCacheLine_||xPortInIsrContext()||!xPortCanYield()||xPortGetCoreID()!=1||xTaskGetCoreID(nullptr)!=1)return {failMipiAtomic(Status::IoError),false,false};
  bool declared=mipiDamageFull_;for(size_t i=0;i<mipiDamageCount_;++i){const auto&d=mipiDamage_[i];declared|=r.x>=d.x&&r.y>=d.y&&int64_t(r.x)+r.width<=int64_t(d.x)+d.width&&int64_t(r.y)+r.height<=int64_t(d.y)+d.height;}
  if(!declared||!detail::NativeStageEngine::validSource(source,r))return {failMipiAtomic(Status::IoError),false,false};
#if defined(CONFIG_SECURE_FLASH_ENC_ENABLED) && CONFIG_SECURE_FLASH_ENC_ENABLED
  return {};
#endif
  if(!detail::NativeStageEngine::geometry(frame,r,source))return {};
#if !NICE_GFX_NATIVE_STAGE_DIAGNOSTIC_VALIDATION
  if(r.pixels()<4096)return {};
#endif
  if(!nativeStageEngine_){const Status created=detail::NativeStageEngine::create(nativeStageEngine_,frame);if(created!=Status::Ok)return {created==Status::Unsupported?created:failMipiAtomic(created),false,false};}
  const auto poison=+[](void*context,Status status){static_cast<VideoTransport*>(context)->failMipiAtomic(status);};
  auto result=nativeStageEngine_->run(frame,r,source,poison,this);if(result.status!=Status::Ok&&result.status!=Status::Unsupported)failMipiAtomic(result.status);return result;
#else
  (void)frame;(void)r;(void)source;return {};
#endif
}


Status VideoTransport::convert(esp_err_t error) {
  switch (error) {
    case ESP_OK: return Status::Ok;
    case ESP_ERR_INVALID_ARG: return Status::InvalidArgument;
    case ESP_ERR_NO_MEM: return Status::NoMemory;
    case ESP_ERR_NOT_SUPPORTED: return Status::Unsupported;
    case ESP_ERR_INVALID_STATE: return Status::Busy;
    default: return Status::IoError;
  }
}

bool IRAM_ATTR VideoTransport::complete(void *context) {
  SemaphoreHandle_t semaphore = static_cast<SemaphoreHandle_t>(context);
  if (xPortInIsrContext()) {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(semaphore, &woken);
    return woken == pdTRUE;
  }
  xSemaphoreGive(semaphore);
  return false;
}
#if SOC_LCD_RGB_SUPPORTED
bool IRAM_ATTR VideoTransport::rgbDone(esp_lcd_panel_handle_t,
                                      const esp_lcd_rgb_panel_event_data_t *, void *context) {
  return complete(static_cast<VideoTransport*>(context)->completed_);
}
bool IRAM_ATTR VideoTransport::rgbVsync(esp_lcd_panel_handle_t,
                                      const esp_lcd_rgb_panel_event_data_t *,void *context){
  auto &s=*static_cast<VideoTransport*>(context);
  const uint32_t now=uint32_t(esp_timer_get_time());
  if(s.scanStats_)s.scanStats_->record(s.lastVsync_?uint32_t(now-s.lastVsync_):0);
  s.lastVsync_=now;return false;
}
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
bool IRAM_ATTR VideoTransport::rgbFrameDone(esp_lcd_panel_handle_t,
                                      const esp_lcd_rgb_panel_event_data_t *,void *context){
  auto &s=*static_cast<VideoTransport*>(context);
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
  if(s.framePipelined_){
    const uint32_t sequence=s.pipelineArmed_.load(std::memory_order_acquire);
    uint32_t previous=s.pipelineAcknowledged_.load(std::memory_order_relaxed);
    if(!sequence||!s.flipPending_.load(std::memory_order_acquire)||previous==sequence||
       !s.pipelineAcknowledged_.compare_exchange_strong(previous,sequence,std::memory_order_acq_rel))return false;
    auto &stamp=s.presentationHistory_[sequence%PresentationHistory];
    stamp.sequence.store(0,std::memory_order_release);
    stamp.micros.store(uint32_t(esp_timer_get_time()),std::memory_order_relaxed);
    stamp.sequence.store(sequence,std::memory_order_release);
    s.pipelinePublished_.store(sequence,std::memory_order_release);
    if(s.copyStats_)__atomic_add_fetch(&s.copyStats_->frameSwitches,1u,__ATOMIC_RELAXED);
    // Publish all ownership/timestamp evidence before waking the producer.
    return s.frameCompleted_?complete(s.frameCompleted_):false;
  }
#endif
  // S31 reports the AXI GDMA link switch; S3 reports the bounce engine's
  // end-of-frame source latch. Both have stopped reading the old framebuffer.
  // S3 submission is serialized against this callback on the interrupt core.
  // VSYNC and color-copy callbacks cannot release either ownership fence.
  return s.frameCompleted_&&s.flipPending_.load(std::memory_order_acquire)?complete(s.frameCompleted_):false;
}
#endif
#endif
#if SOC_MIPI_DSI_SUPPORTED
bool IRAM_ATTR VideoTransport::dpiDone(esp_lcd_panel_handle_t,
                                      esp_lcd_dpi_panel_event_data_t *, void *context) {
  return complete(static_cast<VideoTransport*>(context)->completed_);
}
#if defined(CONFIG_IDF_TARGET_ESP32P4) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,5,5)
bool IRAM_ATTR VideoTransport::dpiFrameDone(esp_lcd_panel_handle_t,
                                         esp_lcd_dpi_panel_event_data_t *,void *context){
  auto &s=*static_cast<VideoTransport*>(context);
  if(!s.mipiAtomic_)return false;
  const int core=xPortGetCoreID();
  const int previous=s.mipiInterruptCore_.load(std::memory_order_relaxed);
  if(previous==-1)s.mipiInterruptCore_.store(core,std::memory_order_release);
  else if(previous!=core)s.mipiInterruptCore_.store(-2,std::memory_order_release);
  // Count every SDK DMA-restart completion, not only callbacks while armed.
  // The producer samples this sequence AFTER draw_bitmap changes cur_fb_index.
  s.mipiFrameSequence_.fetch_add(1,std::memory_order_release);
  return s.mipiFrameFence_?complete(s.mipiFrameFence_):false;
}
#endif
bool IRAM_ATTR VideoTransport::dpiVsync(esp_lcd_panel_handle_t,
                                      esp_lcd_dpi_panel_event_data_t *, void *context) {
#if SOC_LCD_RGB_SUPPORTED
  auto &s=*static_cast<VideoTransport*>(context);
  const uint32_t now=uint32_t(esp_timer_get_time());
  if(s.scanStats_)s.scanStats_->record(s.lastVsync_?uint32_t(now-s.lastVsync_):0);
  s.lastVsync_=now;
#endif
  return false;
}
#endif

Status VideoTransport::wait() {
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_)return copyOnlyMipiWait();
#endif
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  const Status inherited=drainMipiInheritancePrefetch();if(inherited!=Status::Ok)return inherited;
  if(mipiAtomic_&&mipiAtomicFailure_!=Status::Ok)return mipiAtomicFailure_;
#endif
  if (!pending_) return Status::Ok;
  // Accepted DMA copies must finish before their staging memory can be reused.
  if (!completed_ || xSemaphoreTake(completed_, portMAX_DELAY) != pdTRUE)
    return result_ = Status::IoError;
  pending_ = false;
  return result_;
}

Status VideoTransport::present() {
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_)return presentMipiPresentation();
#endif
  if (!panel_ || !rasterReady()) return Status::NotInitialized;
  const Status s = wait();
  if (s != Status::Ok) return s;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(interface_==VideoInterface::MIPI){
    if(mipiAtomic_){
      if(mipiAtomicOpen_&&!mipiAtomicExplicit_)return commitMipiAtomicFrame();
      return waitMipiAtomicFlip();
    }
    if(mipiFrame_)return result_;
    mipiBatch_=false;
    return flushMipiDirty();
  }
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
  if(frameAtomic_)return frameOpen_&&!frameExplicit_?commitFrame():result_;
#endif
#if SOC_LCD_RGB_SUPPORTED
  if (interface_ == VideoInterface::RGB && refreshOnDemand_)
    return result_ = convert(esp_lcd_rgb_panel_refresh(panel_));
#endif
  return result_;
}

Status VideoTransport::beginWrite(){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(mipiAtomic_)return beginMipiAtomicFrame(false);
  if(interface_==VideoInterface::MIPI&&mipiDirty_)mipiBatch_=true;
#endif
  return Status::Ok;
}

#if defined(CONFIG_IDF_TARGET_ESP32S3)
void VideoTransport::publishS3Atomic(void *context){
  auto &s=*static_cast<VideoTransport *>(context);
  portMUX_TYPE guard=portMUX_INITIALIZER_UNLOCKED;
  portENTER_CRITICAL(&guard);
  s.flipPending_.store(true,std::memory_order_release);
  s.atomicSubmitError_=esp_lcd_panel_draw_bitmap(s.panel_,0,0,int(s.rasterWidth()),int(s.rasterHeight()),s.rgbFramebuffer_);
  portEXIT_CRITICAL(&guard);
}
#endif
Status VideoTransport::beginFrame(const DirtyRect *regions,size_t count){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(mipiAtomic_)return beginMipiAtomicFrame(true,regions,count);
  if(interface_==VideoInterface::MIPI&&mipiDirty_){
    if(!mipiFrame_){const Status s=flushMipiDirty();if(s!=Status::Ok)return s;}
    mipiFrame_=true;
  }
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
  if(frameAtomic_)return beginAtomicFrame(regions,count,true);
#endif
  (void)regions;(void)count;return Status::Ok;
}
Status VideoTransport::beginRepaintFrame(const DirtyRect *regions,size_t count){
#if NICE_GFX_FRAME_REPAINT_INHERIT && defined(CONFIG_IDF_TARGET_ESP32P4)
  if(mipiAtomic_&&!mipiAtomicOpen_&&count&&count<=MipiMaxDamage&&regions&&
     !xPortInIsrContext()&&xPortCanYield())return beginMipiAtomicFrame(true,regions,count,true);
#else
  (void)regions;(void)count;
#endif
  return Status::Unsupported;
}
Status VideoTransport::beginRetainedRepaintFrame(const DirtyRect *damage,size_t count,const RetainedRepaintRequest *requests,size_t requested,RetainedRepaintReceipt *receipts){
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_)return Status::Unsupported;
#endif
#if NICE_GFX_UI_RETAINED_CHART_INK && NICE_GFX_FRAME_REPAINT_INHERIT && defined(CONFIG_IDF_TARGET_ESP32P4) && !NICE_GFX_P4_MIPI_INHERIT_PREFETCH
  if(!mipiAtomic_||mipiAtomicOpen_||mipiAtomicFailure_!=Status::Ok||swapRedBlue_||!damage||!count||count>MipiMaxDamage||!requests||!receipts||!requested||requested>4||xPortInIsrContext()||!xPortCanYield()||xPortGetCoreID()!=1||xTaskGetCoreID(nullptr)!=1)return Status::Unsupported;
  retainedAcquiring_=true;const auto s=beginMipiAtomicFrame(true,damage,count,true);retainedAcquiring_=false;if(s!=Status::Ok)return s;
  const auto issued=retainedTags_.begin(mipiFront_^1,reinterpret_cast<uintptr_t>(rgbFramebuffer_),requests,requested,damage,count,receipts);
  if(issued!=Status::Ok)abortFrame();return issued;
#else
  (void)damage;(void)count;(void)requests;(void)requested;(void)receipts;return Status::Unsupported;
#endif
}
Status VideoTransport::validateRetainedRepaintFrame(const RetainedRepaintReceipt*r,size_t n){return retainedTags_.validate(r,n);}
Status VideoTransport::certifyRetainedRepaintFrame(const RetainedRepaintReceipt*r,size_t n){return retainedTags_.certify(r,n);}
Status VideoTransport::mapRetainedRepaintFramebuffer(const RetainedRepaintReceipt*r,size_t n,NativeFramebuffer&frame){
  const auto valid=retainedTags_.validate(r,n);if(valid!=Status::Ok){frame=NativeFramebuffer{};return valid;}
  retainedMapping_=true;const auto s=mapNativeFramebuffer(frame);retainedMapping_=false;return s;
}
Status VideoTransport::commitFrame(){
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_)return commitMipiPresentationLegacy();
#endif
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(interface_==VideoInterface::MIPI){
    if(mipiAtomic_){const bool retained=retainedTags_.open();if(retained&&!retainedTags_.commitReady()){abortFrame();return Status::Busy;}
      const auto s=commitMipiAtomicFrame();if(retained)retainedTags_.fencedCommit(s==Status::Ok);return s;}
    mipiFrame_=mipiBatch_=false;
    const Status s=flushMipiDirty();
    if(s==Status::Ok&&copyStats_)++copyStats_->frameCommits;
    return s;
  }
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
  if(frameAtomic_){
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
    if(framePipelined_)return commitPipelineFrame();
#endif
    const Status s=wait();if(s!=Status::Ok)return s;
    if(!frameOpen_)return result_;
    if(!panel_||!frameCompleted_||flipPending_.load(std::memory_order_acquire))return result_=Status::Busy;
    while(xSemaphoreTake(frameCompleted_,0)==pdTRUE){}
    // Arm before draw_bitmap: the link-switch interrupt may arrive before
    // that SDK call returns. Keep the just-submitted buffer immutable until
    // the next acquisition consumes the independent frame-complete fence.
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    // S3's bounce completion latches the next source BEFORE our callback.
    // Publish on that same core with its IRQ masked, so an old-source latch
    // cannot race the submission and falsely release the old front buffer.
    esp_err_t error=ESP_OK;
    if(xPortGetCoreID()==atomicCore_)publishS3Atomic(this);
    else error=esp_ipc_call_blocking(atomicCore_,publishS3Atomic,this);
    if(error==ESP_OK)error=atomicSubmitError_;
#else
    flipPending_.store(true,std::memory_order_release);
    const esp_err_t error=esp_lcd_panel_draw_bitmap(panel_,0,0,int(rasterWidth()),int(rasterHeight()),rgbFramebuffer_);
#endif
    if(error!=ESP_OK){
      // A failed submission has uncertain DMA ownership. Do not disarm or
      // recycle either buffer. A later wait may recover a completion; shutdown
      // can always stop/delete the panel before releasing its callback state.
      frameOpen_=frameExplicit_=false;staleFull_=true;staleCount_=0;
      return result_=convert(error);
    }
    staleFull_=frameDamageFull_;staleCount_=frameDamageCount_;
    for(size_t i=0;i<staleCount_;++i)staleDamage_[i]=frameDamage_[i];
    if(copyStats_)++copyStats_->frameCommits;
    frameOpen_=frameExplicit_=false;return result_=Status::Ok;
  }
#endif
  return present();
}
Status VideoTransport::abortFrame(){
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_)return abortMipiPresentationLegacy();
#endif
#if NICE_GFX_NATIVE_STAGE_ENGINE_AVAILABLE
  if(nativeStageEngine_&&nativeStageEngine_->pending)return failMipiAtomic(Status::IoError);
#endif
  retainedTags_.abort();
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(interface_==VideoInterface::MIPI){
    if(mipiAtomic_){
      const bool prefetched=mipiPrefetchActive_||mipiPrefetchReady_;
      const Status inherited=drainMipiInheritancePrefetch();if(inherited!=Status::Ok)return inherited;
      if(prefetched){mipiPrefetchReady_=false;mipiStaleFull_=true;mipiStaleCount_=0;}
      if(mipiAtomicFailure_!=Status::Ok)return mipiAtomicFailure_;
      const Status s=wait();if(s!=Status::Ok)return failMipiAtomic(s);
      if(mipiAtomicPending_)return waitMipiAtomicFlip();
      // Discard the inactive partial frame. The next acquisition re-inherits
      // the entire last published frame; no partial/aborted frame is submitted.
      if(mipiAtomicOpen_){mipiStaleFull_=true;mipiStaleCount_=0;}
      mipiAtomicOpen_=mipiAtomicExplicit_=mipiFrame_=mipiBatch_=false;
      clearMipiDirty();
      if(copyStats_)++copyStats_->frameAborts;
      return result_=Status::Ok;
    }
    // Single-buffer batching cannot roll back pixels. Publish accepted CPU
    // writes even on abort, so later DMA does not observe stale memory.
    mipiFrame_=mipiBatch_=false;
    if(copyStats_)++copyStats_->frameAborts;
    return flushMipiDirty();
  }
#endif
  const Status s=wait();
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
  if(frameAtomic_&&frameOpen_){
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
    if(framePipelined_)pipeline_.abort();
#endif
    frameOpen_=frameExplicit_=false;staleFull_=true;staleCount_=0;if(copyStats_)++copyStats_->frameAborts;}
#endif
  return s;
}

#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
Status VideoTransport::waitAtomicFlip(){
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
  if(framePipelined_)return waitPipelineFlip(true);
#endif
  if(!flipPending_.load(std::memory_order_acquire))return Status::Ok;
  const int64_t started=esp_timer_get_time();
  const bool done=frameCompleted_&&xSemaphoreTake(frameCompleted_,pdMS_TO_TICKS(250))==pdTRUE;
  if(copyStats_)copyStats_->linkWaitMicros+=uint64_t(esp_timer_get_time()-started);
  if(!done){if(copyStats_)++copyStats_->linkTimeouts;return result_=Status::IoError;}
  // One outstanding flip, one completion, and only this producer changes
  // indices. The ISR only signals; it never mutates a still-borrowed pointer.
  frontIndex_^=1;rgbFramebuffer_=rgbFrames_[frontIndex_^1];
  flipPending_.store(false,std::memory_order_release);if(copyStats_)++copyStats_->frameSwitches;return result_=Status::Ok;
}
Status VideoTransport::inheritAtomicRect(const DirtyRect &r){
  const int64_t started=esp_timer_get_time();
  const uint16_t *base=rgbFrames_[frontIndex_];
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
  if(framePipelined_)base=pipelineBuffer(pipeline_.latest());
#endif
  const auto *front=base+size_t(r.y)*rasterWidth()+size_t(r.x);
  const PixelView view(front,size_t(r.width),size_t(r.height),rasterWidth(),PixelOrder::Native);
#if SOC_PPA_SUPPORTED
  // Front already contains the panel's channel order. The ordinary PPA copy
  // applies BGR conversion to application colors, so keep physical BGR
  // inheritance on the raw CPU copy rather than converting it a second time.
  if(!swapRedBlue_&&copyWithPpa(r.x,r.y,view)){if(copyStats_){copyStats_->inheritedPixels+=uint64_t(r.width)*r.height;copyStats_->inheritMicros+=uint64_t(esp_timer_get_time()-started);}return Status::Ok;}
#endif
  for(int32_t row=0;row<r.height;++row)memcpy(rgbFramebuffer_+size_t(r.y+row)*rasterWidth()+size_t(r.x),front+size_t(row)*rasterWidth(),size_t(r.width)*2);
  const Status result=synchronizeFramebuffer(r.x,r.y,size_t(r.width),size_t(r.height),true);
  if(copyStats_){copyStats_->inheritedPixels+=uint64_t(r.width)*r.height;copyStats_->inheritMicros+=uint64_t(esp_timer_get_time()-started);copyStats_->cpuPixels+=uint64_t(r.width)*r.height;}
  return result;
}
Status VideoTransport::beginAtomicFrame(const DirtyRect *regions,size_t count,bool explicitFrame){
  if(frameOpen_){if(explicitFrame)frameExplicit_=true;return Status::Ok;}
  if(!panel_||!rasterReady()||!rgbFrames_[0]||!rgbFrames_[1])return Status::NotInitialized;
  if(count&&!regions)return Status::InvalidArgument;
  const bool bounded=count&&count<=AtomicMaxDamage;
  if(bounded)for(size_t i=0;i<count;++i){const auto &r=regions[i];
    if(r.x<0||r.y<0||r.width<=0||r.height<=0||int64_t(r.x)+r.width>rasterWidth()||int64_t(r.y)+r.height>rasterHeight())return Status::InvalidArgument;
  }
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
  DirtyRect needed[AtomicMaxInherit],next[AtomicMaxInherit];
  if(framePipelined_)return beginPipelineFrame(regions,bounded?count:0,explicitFrame,needed,next);
#else
  DirtyRect needed[AtomicMaxInherit],next[AtomicMaxInherit];
#endif
  Status s=wait();if(s==Status::Ok)s=waitAtomicFlip();if(s!=Status::Ok)return s;
  frameDamageFull_=!bounded;frameDamageCount_=bounded?count:0;
  for(size_t i=0;i<frameDamageCount_;++i)frameDamage_[i]=regions[i];
  size_t neededCount=0;
  const DirtyRect full(0,0,int32_t(rasterWidth()),int32_t(rasterHeight()));
  if(staleFull_)needed[neededCount++]=full;
  else for(size_t i=0;i<staleCount_;++i)needed[neededCount++]=staleDamage_[i];
  bool overflow=false;
  // The back buffer differs from front only inside the previous frame's
  // damage. New declared damage is guaranteed to be replaced by the caller,
  // so inherit just stale-minus-new. In particular, steady map ROI needs no
  // frame clone. Bounded fragmentation falls back to one complete safe copy.
  for(size_t cut=0;bounded&&cut<count&&!overflow;++cut){size_t n=0;const auto &b=regions[cut];
    for(size_t i=0;i<neededCount&&!overflow;++i){const auto &a=needed[i];
      const int32_t left=a.x>b.x?a.x:b.x,top=a.y>b.y?a.y:b.y;
      const int32_t right=a.x+a.width<b.x+b.width?a.x+a.width:b.x+b.width;
      const int32_t bottom=a.y+a.height<b.y+b.height?a.y+a.height:b.y+b.height;
      const auto add=[&](int32_t x,int32_t y,int32_t w,int32_t h){if(w<=0||h<=0)return;if(n==AtomicMaxInherit){overflow=true;return;}next[n++]=DirtyRect(x,y,w,h);};
      if(right<=left||bottom<=top)add(a.x,a.y,a.width,a.height);
      else {add(a.x,a.y,a.width,top-a.y);add(a.x,bottom,a.width,a.y+a.height-bottom);add(a.x,top,left-a.x,bottom-top);add(right,top,a.x+a.width-right,bottom-top);}
    }
    if(!overflow){neededCount=n;for(size_t i=0;i<n;++i)needed[i]=next[i];}
  }
  if(overflow){needed[0]=full;neededCount=1;}
  // Inheritance never publishes and consumes every source synchronously.
  // An aborted/incomplete frame must re-inherit its whole back buffer.
  staleFull_=true;staleCount_=0;
  for(size_t i=0;i<neededCount;++i)if((s=inheritAtomicRect(needed[i]))!=Status::Ok)return s;
  frameOpen_=true;frameExplicit_=explicitFrame;return result_=Status::Ok;
}
Status VideoTransport::ensureAtomicFrame(){return !frameAtomic_||frameOpen_?Status::Ok:beginAtomicFrame(nullptr,0,false);}
#endif

uint32_t VideoTransport::presentationSequence()const{
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
  return framePipelined_?pipelinePublished_.load(std::memory_order_acquire):0;
#else
  return 0;
#endif
}
bool VideoTransport::presentationTimestamp(uint32_t sequence,uint32_t &micros)const{
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
  if(!framePipelined_||!sequence)return false;
  const auto &stamp=presentationHistory_[sequence%PresentationHistory];
  if(stamp.sequence.load(std::memory_order_acquire)!=sequence)return false;
  const uint32_t time=stamp.micros.load(std::memory_order_relaxed);
  if(stamp.sequence.load(std::memory_order_acquire)!=sequence)return false;
  micros=time;return true;
#else
  (void)sequence;(void)micros;return false;
#endif
}

#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
Status VideoTransport::waitPipelineFlip(bool block){
  if(pipeline_.faulted())return result_=Status::IoError;
  if(!flipPending_.load(std::memory_order_acquire))return Status::Ok;
  const int64_t started=esp_timer_get_time();
  const bool done=frameCompleted_&&xSemaphoreTake(frameCompleted_,block?pdMS_TO_TICKS(250):0)==pdTRUE;
  if(block&&copyStats_)copyStats_->linkWaitMicros+=uint64_t(esp_timer_get_time()-started);
  if(!done){if(!block)return Status::Ok;if(copyStats_)++copyStats_->linkTimeouts;pipeline_.poison();return result_=Status::IoError;}
  if(pipelineAcknowledged_.load(std::memory_order_acquire)!=pipeline_.generation()||!pipeline_.acknowledge()){
    pipeline_.poison();return result_=Status::IoError;
  }
  frontIndex_=pipeline_.front();flipPending_.store(false,std::memory_order_release);
  // A third buffer may already be under CPU paint. Its mapped pointer must
  // not change when the older pending frame becomes the new displayed front.
  return result_=Status::Ok;
}
Status VideoTransport::beginPipelineFrame(const DirtyRect *regions,size_t count,bool explicitFrame,
    DirtyRect (&needed)[AtomicMaxInherit],DirtyRect (&next)[AtomicMaxInherit]){
  Status status=wait();if(status==Status::Ok)status=waitPipelineFlip(false);if(status!=Status::Ok)return status;
  if(!pipelineThird_||!pipeline_.acquire())return result_=Status::IoError;
  rgbFramebuffer_=pipelineBuffer(pipeline_.drawing());
  frameDamageFull_=!count;frameDamageCount_=count;
  for(size_t i=0;i<count;++i)frameDamage_[i]=regions[i];
  const DirtyRect full(0,0,int32_t(rasterWidth()),int32_t(rasterHeight()));
  const size_t inherit=detail::RgbAtomicPipeline::inheritance(full,regions,count,needed,next);
  for(size_t i=0;i<inherit;++i)if((status=inheritAtomicRect(needed[i]))!=Status::Ok){pipeline_.abort();return status;}
  frameOpen_=true;frameExplicit_=explicitFrame;return result_=Status::Ok;
}
Status VideoTransport::commitPipelineFrame(){
  Status status=wait();if(status!=Status::Ok)return status;if(!frameOpen_)return result_;
  // The SDK exposes one pending DMA-link request, not a multi-frame queue.
  // Only this final submission fence waits; painting used the free third FB.
  status=waitPipelineFlip(true);if(status!=Status::Ok)return status;
  if(!panel_||!frameCompleted_||!pipeline_.canSubmit())return result_=Status::Busy;
  while(xSemaphoreTake(frameCompleted_,0)==pdTRUE){}
  if(!pipeline_.submit())return result_=Status::Busy;
  pipelineArmed_.store(pipeline_.generation(),std::memory_order_release);
  flipPending_.store(true,std::memory_order_release);
  const esp_err_t error=esp_lcd_panel_draw_bitmap(panel_,0,0,int(rasterWidth()),int(rasterHeight()),rgbFramebuffer_);
  frameOpen_=frameExplicit_=false;
  if(error!=ESP_OK){pipeline_.poison();return result_=convert(error);} // Uncertain DMA ownership: never recycle.
  if(copyStats_)++copyStats_->frameCommits;
  return result_=Status::Ok;
}
#endif

Status VideoTransport::mapNativeFramebuffer(NativeFramebuffer &frame) {
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_)return mapMipiPresentationLegacy(frame);
#endif
  frame = NativeFramebuffer();
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32P4)
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(interface_ != VideoInterface::MIPI)return Status::Unsupported;
#endif
  // Atomic RGB maps its inactive buffer; single-buffer RGB565 MIPI maps the
  // SDK-owned scanout surface. BGR and multi-buffer DPI remain unmapped.
  if (!panel_ || !rasterReady()) return Status::NotInitialized;
  if (!rgbFramebuffer_ || swapRedBlue_) return Status::Unsupported;
  const Status s = wait();
  if (s != Status::Ok) return s;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(mipiAtomic_){const Status acquired=beginMipiAtomicFrame(false);if(acquired!=Status::Ok)return acquired;}
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
  const Status acquired=ensureAtomicFrame();if(acquired!=Status::Ok)return acquired;
#endif
  if(!retainedMapping_)retainedTags_.ordinaryDamage(
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    mipiFront_^1,
#else
    0,
#endif
    nullptr,0);
  frame.pixels = rgbFramebuffer_;
  frame.width = frame.stride = rasterWidth(); frame.height = rasterHeight();
  frame.bytes = frame.stride * frame.height * sizeof(uint16_t);
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  frame.fencedAtomicMipiRmw=mipiAtomic_&&mipiAtomicOpen_&&!mipiAtomicPending_&&!pending_&&
    !mipiFullInheritPending_&&!mipiPartialInheritPending_&&mipiAtomicFailure_==Status::Ok&&mipiFront_<2&&
    mipiFrames_[0]&&mipiFrames_[1]&&mipiFrames_[0]!=mipiFrames_[1]&&rgbFramebuffer_==mipiFrames_[mipiFront_^1];
#endif
  return Status::Ok;
#else
  return Status::Unsupported;
#endif
}

Status VideoTransport::shutdown() {
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL && NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE && NICE_GFX_NATIVE_STAGE_ENGINE_AVAILABLE
  if(mipiFillPreparing_)return result_=failMipiAtomic(Status::IoError);
  const Status filled=drainNativeDrawingFill();
  if(filled!=Status::Ok&&nativeFillEngine_&&(nativeFillEngine_->preparing||nativeFillEngine_->pending))return result_=filled;
#endif
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_){const Status quiet=quiesceMipiPresentation();if(quiet!=Status::Ok)return result_=quiet;}
#endif
#if NICE_GFX_NATIVE_STAGE_ENGINE_AVAILABLE
  const Status stageReleased=detail::NativeStageEngine::dispose(nativeStageEngine_);if(stageReleased!=Status::Ok)return result_=stageReleased;
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL
  const Status fillReleased=detail::NativeStageEngine::dispose(nativeFillEngine_);if(fillReleased!=Status::Ok)return result_=fillReleased;
#endif
#endif
  retainedTags_.reset();
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  drainMipiInheritancePrefetch();
  // Never uninstall an engine/free SDK FBs while an accepted copy owns them.
  // Normal operation drains synchronously. This guard also covers cleanup.
  if(mipiFullInheritPending_)drainMipiFullInheritanceGdma();
  if(mipiPartialInheritPending_)drainMipiPartialInheritancePpa();
#endif
  const Status first = wait();
  if (first != Status::Ok && pending_) return result_ = first;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(mipiGdma_){const Status s=convert(esp_async_memcpy_uninstall(mipiGdma_));if(s!=Status::Ok)return result_=s;mipiGdma_=nullptr;}
  if(mipiGdmaDone_){vSemaphoreDelete(mipiGdmaDone_);mipiGdmaDone_=nullptr;}
  mipiGdmaVerified_=false;
  mipiFullInheritPending_=mipiFullInheritVerified_=false;
#if NICE_GFX_P4_MIPI_NO_PPA_FULL_INHERIT
  mipiPpaConfigured_=false;
#endif
  if(mipiPartialInheritPpa_){const Status s=convert(ppa_unregister_client(mipiPartialInheritPpa_));if(s!=Status::Ok)return result_=s;mipiPartialInheritPpa_=nullptr;}
  if(mipiPartialInheritDone_){vSemaphoreDelete(mipiPartialInheritDone_);mipiPartialInheritDone_=nullptr;}
  mipiPartialInheritPending_=mipiPartialInheritVerified_=false;
  mipiPrefetchActive_=mipiPrefetchReady_=false;mipiOwnershipEpoch_=mipiPrefetchGeneration_=0;
  mipiPrefetchSource_=nullptr;mipiPrefetchTarget_=mipiPrefetchRows_=nullptr;mipiPrefetchIndex_=MipiMaxDamage;mipiPrefetchRowBytes_=0;
  if(copyStats_)copyStats_->inheritPrefetchPending=false;
  if(copyStats_)copyStats_->partialInheritPpaReady=false;
#endif
#if SOC_PPA_SUPPORTED && (defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32P4))
  if(ppaFill_){const Status s=convert(ppa_unregister_client(ppaFill_));if(s!=Status::Ok)return result_=s;ppaFill_=nullptr;}
  if(ppaCopy_){const Status s=convert(ppa_unregister_client(ppaCopy_));if(s!=Status::Ok)return result_=s;ppaCopy_=nullptr;}
  ppaVerified_=ppaPartialVerified_=false;
  ppaFillVerified_=ppaFillPartialVerified_=ppaFillRejected_=false;
#endif
#if SOC_LCD_RGB_SUPPORTED
  if(copyStats_){copyStats_->ppaReady=false;copyStats_->ppaFillReady=false;copyStats_->gdmaReady=false;copyStats_->fullInheritGdmaReady=false;}
#endif
  stopRaster();
  if (panel_) {
    const Status s = convert(esp_lcd_panel_del(panel_));
    // A failed delete may leave DMA running. Its resources and callback
    // semaphore must survive, and a later explicit shutdown can retry.
    if (s != Status::Ok) return result_ = first == Status::Ok ? s : first;
    panel_ = nullptr; rgbFramebuffer_ = nullptr;
  }
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(mipiDirty_){heap_caps_free(mipiDirty_);mipiDirty_=nullptr;}
  mipiDirtyTop_=mipiDirtyBottom_=ppaCacheLine_=0;
  mipiBatch_=mipiFrame_=false;
  // panel_del above has stopped DMA and callbacks before releasing the fence.
  if(mipiFrameFence_){vSemaphoreDelete(mipiFrameFence_);mipiFrameFence_=nullptr;}
  mipiAtomic_=mipiAtomicOpen_=mipiAtomicExplicit_=mipiAtomicPending_=false;
  mipiFrames_[0]=mipiFrames_[1]=nullptr;mipiFront_=0;mipiDirectSdkDpi_=false;
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  mipiFrames_[2]=nullptr;mipiSdkFramebufferCount_=0;mipiPresentationBound_=false;
  mipiPresentation_=detail::NativePresentationState{};mipiPresentationOwner_=0;mipiPresentationTask_=nullptr;
  mipiDrawingLease_=mipiReadyLease_={};mipiPresentationReceipt_=mipiRetiredReceipt_={};
  mipiTypedDrawing_=mipiTypedReady_=mipiTypedPending_=mipiTypedRetired_=false;
#endif
  mipiAtomicFailure_=Status::Ok;mipiFrameSequence_.store(0);mipiFenceBaseline_=0;
  mipiInitCore_=-1;mipiInterruptCore_.store(-1);mipiFenceCompletions_=2;
  mipiStaleFull_=mipiDamageFull_=true;mipiStaleCount_=mipiDamageCount_=0;
#endif
#if SOC_LCD_RGB_SUPPORTED
  if(copyStats_){copyStats_->presentationContext=nullptr;copyStats_->presentationSequenceReader=nullptr;copyStats_->presentationTimestampReader=nullptr;}
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
  frameAtomic_=frameOpen_=frameExplicit_=false;flipPending_.store(false,std::memory_order_release);
  rgbFrames_[0]=rgbFrames_[1]=nullptr;frontIndex_=0;staleFull_=true;staleCount_=frameDamageCount_=0;
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
  framePipelined_=false;pipelineThird_=nullptr;pipeline_=detail::RgbAtomicPipeline{};
  pipelineArmed_.store(0);pipelineAcknowledged_.store(0);pipelinePublished_.store(0);
  for(auto &stamp:presentationHistory_){stamp.sequence.store(0);stamp.micros.store(0);}
#endif
  if(frameCompleted_){vSemaphoreDelete(frameCompleted_);frameCompleted_=nullptr;}
#endif
  if (io_) {
    const Status s = convert(esp_lcd_panel_io_del(io_));
    if (s != Status::Ok) return result_ = first == Status::Ok ? s : first;
    io_ = nullptr;
  }
#if SOC_MIPI_DSI_SUPPORTED
  if (dsiBus_) {
    const Status s = convert(esp_lcd_del_dsi_bus(static_cast<esp_lcd_dsi_bus_handle_t>(dsiBus_)));
    if (s != Status::Ok) return result_ = first == Status::Ok ? s : first;
    dsiBus_ = nullptr;
  }
  if (ldo_) {
    const Status s = convert(esp_ldo_release_channel(static_cast<esp_ldo_channel_handle_t>(ldo_)));
    if (s != Status::Ok) return result_ = first == Status::Ok ? s : first;
    ldo_ = nullptr;
  }
#endif
  if (completed_) { vSemaphoreDelete(completed_); completed_ = nullptr; }
  if (scratch_) { heap_caps_free(scratch_); scratch_ = nullptr; }
  hasSpi_ = hasDisplayPin_ = refreshOnDemand_ = swapRedBlue_ = false;
#if SOC_LCD_RGB_SUPPORTED
  copyStats_=nullptr;scanStats_=nullptr;lastVsync_=0;
#endif
  commandWrite_ = nullptr; commandContext_ = nullptr;
  return result_ = first;
}
void VideoTransport::end() {
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_){
    // A destructor cannot release this while an uncertain SDK operation still
    // retains callback context. Missing proof intentionally blocks this prototype.
    while(shutdown()!=Status::Ok&&panel_)vTaskDelay(1);
    return;
  }
#endif
  shutdown();
}
Status VideoTransport::fail(Status reason) { shutdown(); return result_ = reason; }
Status VideoTransport::fail(esp_err_t error) { return fail(convert(error)); }

Status VideoTransport::prepareCommandSpi(const VideoSpiConfig &config) {
  if (config.spi_mode > 3 || !config.expect_clk_speed ||
      config.lcd_cmd_bytes != 1 || config.lcd_param_bytes != 1 || !config.flags.use_dc_bit)
    return Status::InvalidArgument;
  const int pins[] = {config.line_config.cs_gpio_num, config.line_config.scl_gpio_num,
                      config.line_config.sda_gpio_num};
  for (size_t i = 0; i < 3; ++i) {
    if (i == 0 && pins[i] == -1) continue;
    if (!validOutputPin(pins[i])) return Status::InvalidArgument;
    for (size_t j = 0; j < i; ++j) if (pins[i] == pins[j]) return Status::InvalidArgument;
  }
  spi_ = config;
  halfPeriodUs_ = 500000UL / config.expect_clk_speed + (500000UL % config.expect_clk_speed != 0);
  if (!halfPeriodUs_) halfPeriodUs_ = 1;
  if (pins[0] >= 0) output(pins[0], !spi_.flags.cs_high_active);
  output(pins[1], (spi_.spi_mode & 2) != 0);
  output(pins[2], false);
  hasSpi_ = true;
  return Status::Ok;
}

void VideoTransport::spiBit(bool high) {
  const bool idle = (spi_.spi_mode & 2) != 0;
  const int clock = spi_.line_config.scl_gpio_num, data = spi_.line_config.sda_gpio_num;
  if (spi_.spi_mode & 1) {
    esp32_platform::write(clock, !idle);
    esp32_platform::write(data, high);
    esp32_platform::delayMicros(halfPeriodUs_);
    esp32_platform::write(clock, idle);
    esp32_platform::delayMicros(halfPeriodUs_);
  } else {
    esp32_platform::write(data, high);
    esp32_platform::delayMicros(halfPeriodUs_);
    esp32_platform::write(clock, !idle);
    esp32_platform::delayMicros(halfPeriodUs_);
    esp32_platform::write(clock, idle);
  }
}
void VideoTransport::spiByte(uint8_t byte, bool data) {
  const bool dc = spi_.flags.dc_zero_on_data ? !data : data;
  spiBit(dc);
  if (spi_.flags.lsb_first) for (unsigned bit = 0; bit < 8; ++bit) spiBit((byte >> bit) & 1);
  else for (unsigned bit = 0; bit < 8; ++bit) spiBit((byte >> (7 - bit)) & 1);
}
Status VideoTransport::transmitCommand(uint8_t c, const uint8_t *data, size_t bytes) {
  if (bytes && !data) return Status::InvalidArgument;
  if (commandWrite_) return commandWrite_(c, data, bytes, commandContext_);
  if (interface_ == VideoInterface::MIPI) {
    if (!io_) return Status::NotInitialized;
    return convert(esp_lcd_panel_io_tx_param(io_, c, data, bytes));
  }
  if (!hasSpi_) return Status::NotInitialized;
  const int cs = spi_.line_config.cs_gpio_num;
  if (cs >= 0) esp32_platform::write(cs, spi_.flags.cs_high_active);
  spiByte(c, false);
  for (size_t i = 0; i < bytes; ++i) spiByte(data[i], true);
  if (cs >= 0) esp32_platform::write(cs, !spi_.flags.cs_high_active);
  return Status::Ok;
}
Status VideoTransport::writeCommand(uint8_t c, const uint8_t *data, size_t bytes, void *context) {
  return static_cast<VideoTransport *>(context)->transmitCommand(c, data, bytes);
}
void VideoTransport::controllerDelay(uint32_t ms, void *context) {
  static_cast<VideoTransport *>(context)->delayMs(ms);
}

Status VideoTransport::initializeHardware(VideoController controller, const VideoPanelConfig &options,
                                          const VideoInitOverride &init, int reset,
                                          size_t width, size_t height, unsigned lanes,
                                          bool displayOnInit, bool hasDisplayPin) {
  if (!panel_ || !width || !height || width > INT16_MAX || height > INT16_MAX)
    return fail(Status::InvalidArgument);
  const auto *profile = videoControllerProfile(controller, interface_);
  if (!profile) return fail(Status::Unsupported);
  controller_ = controller; hasDisplayPin_ = hasDisplayPin;
  swapRedBlue_ = interface_ == VideoInterface::RGB && profile->noCommand &&
                 options.rgb_ele_order == LCD_RGB_ELEMENT_ORDER_BGR;
  const size_t bytes = (width * sizeof(uint16_t) + 63) & ~size_t(63);
  scratch_ = static_cast<uint16_t *>(heap_caps_aligned_alloc(64, bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
  if (!scratch_) return fail(ESP_ERR_NO_MEM);
  completed_ = xSemaphoreCreateBinary();
  if (!completed_) return fail(ESP_ERR_NO_MEM);
  esp_err_t error = ESP_OK;
#if SOC_LCD_RGB_SUPPORTED
  if (interface_ == VideoInterface::RGB) {
    esp_lcd_rgb_panel_event_callbacks_t callbacks = {};
    callbacks.on_color_trans_done = rgbDone;
    callbacks.on_vsync = rgbVsync;
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
    if(frameAtomic_)callbacks.on_frame_buf_complete=rgbFrameDone;
#endif
    error = esp_lcd_rgb_panel_register_event_callbacks(panel_, &callbacks, this);
  }
#endif
#if SOC_MIPI_DSI_SUPPORTED
  if (interface_ == VideoInterface::MIPI) {
    esp_lcd_dpi_panel_event_callbacks_t callbacks = {};
    callbacks.on_color_trans_done = dpiDone;
    callbacks.on_vsync = dpiVsync;
#if defined(CONFIG_IDF_TARGET_ESP32P4) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,5,5)
    if(mipiAtomic_)callbacks.on_frame_buf_complete=dpiFrameDone;
#endif
    error = esp_lcd_dpi_panel_register_event_callbacks(panel_, &callbacks, this);
  }
#endif
  if (error != ESP_OK) return fail(error);
  // RGB exposes a peripheral reset operation. Native DPI exposes no reset
  // handler: its external controller is reset below through GPIO or commands.
  if (interface_ == VideoInterface::RGB) {
    error = esp_lcd_panel_reset(panel_);
    if (error != ESP_OK) return fail(error);
  }
  if (reset >= 0) {
    const bool active = options.flags.reset_active_high;
    output(reset, !active); delayMs(profile->resetPreMs);
    output(reset, active); delayMs(profile->resetActiveMs);
    output(reset, !active); delayMs(profile->resetIdleMs);
  } else {
    const Status resetStatus = videoReset(controller, interface_, writeCommand, controllerDelay, this);
    if (resetStatus != Status::Ok) return fail(resetStatus);
  }
  if (profile->initRequiresVideo) {
    error = esp_lcd_panel_init(panel_);
    if (error != ESP_OK) return fail(error);
  }
  Status s = videoInitialize(controller, interface_, options.bits_per_pixel, lanes,
                             options.rgb_ele_order == LCD_RGB_ELEMENT_ORDER_BGR,
                             init, writeCommand, controllerDelay, this, width, height);
  if (s != Status::Ok) return fail(s);
  if (!profile->initRequiresVideo) {
    error = esp_lcd_panel_init(panel_);
    if (error != ESP_OK) return fail(error);
  }
  if (displayOnInit && (!profile->noCommand || hasDisplayPin)) {
    s = display(true);
    if (s != Status::Ok) return fail(s);
  }
  s = startRaster(width, height, scratch_, width);
  if (s != Status::Ok) return fail(s);
  if (refreshOnDemand_ && displayOnInit) {
    s = present();
    if (s != Status::Ok) return fail(s);
  }
  return result_ = Status::Ok;
}

Status VideoTransport::drawBitmap(int x0, int y0, int x1, int y1, const uint16_t *pixels) {
  if (!panel_ || !rasterReady()) return Status::NotInitialized;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(mipiAtomic_)return drawPixelView(x0,y0,PixelView(pixels,size_t(x1-x0),size_t(y1-y0),size_t(x1-x0),PixelOrder::Native));
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
  if(frameAtomic_)return drawPixelView(x0,y0,PixelView(pixels,size_t(x1-x0),size_t(y1-y0),size_t(x1-x0),PixelOrder::Native));
#endif
  const Status s = wait();
  if (s != Status::Ok) return s;
  while (xSemaphoreTake(completed_, 0) == pdTRUE) {}
  if (swapRedBlue_) {
    const size_t count = size_t(x1 - x0) * size_t(y1 - y0);
    for (size_t i = 0; i < count; ++i) {
      const uint16_t color = pixels[i];
      scratch_[i] = uint16_t((color & 0x07E0) | ((color & 0x001F) << 11) | (color >> 11));
    }
    pixels = scratch_;
  }
  pending_ = true;
#if SOC_LCD_RGB_SUPPORTED
  if(copyStats_)copyStats_->cpuPixels+=uint64_t(x1-x0)*(y1-y0);
#endif
  result_ = convert(esp_lcd_panel_draw_bitmap(panel_, x0, y0, x1, y1, pixels));
  if (result_ != Status::Ok) { pending_ = false; return result_; }
  return wait();
}

Status VideoTransport::copySurfaceRows(const NativeFramebuffer &frame,int32_t y,PixelView source){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(mipiAtomic_&&mipiAtomicFailure_!=Status::Ok)return mipiAtomicFailure_;
  if(interface_!=VideoInterface::MIPI||pending_||!mipiGdma_||!mipiGdmaDone_||xPortInIsrContext()||!xPortCanYield()||
     !source.valid()||source.order!=PixelOrder::Native||y<0||!frame.pixels||!frame.width||!frame.height||
     frame.stride!=frame.width||source.width!=frame.width||source.stride!=source.width||
     size_t(y)>frame.height||source.height>frame.height-size_t(y)||frame.height>SIZE_MAX/frame.stride/2||
     frame.bytes<frame.stride*frame.height*2||source.height>SIZE_MAX/source.width/2||source.width*source.height<2048)return Status::Unsupported;
  const uintptr_t target=reinterpret_cast<uintptr_t>(frame.pixels),display=reinterpret_cast<uintptr_t>(rgbFramebuffer_);
  const size_t bytes=frame.stride*frame.height*2,displayBytes=rasterWidth()*rasterHeight()*2;
  if(target>UINTPTR_MAX-bytes||(rgbFramebuffer_&&target<display+displayBytes&&display<target+bytes))return Status::Unsupported;
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiAtomic_)for(unsigned i=0;i<(mipiPresentationBound_?3u:2u);++i){
#else
  if(mipiAtomic_)for(unsigned i=0;i<2;++i){
#endif
    const uintptr_t sdk=reinterpret_cast<uintptr_t>(mipiFrames_[i]);
    if(sdk&&target<sdk+displayBytes&&sdk<target+bytes)return Status::Unsupported;
  }
  // The existing per-display AXI engine already owns cache synchronization,
  // first-transfer verification and a fence for every accepted source. This
  // independent target never enters the transport's dirty/present lifecycle.
  return copyContiguousGdma(frame.pixels+size_t(y)*frame.stride,source.data,source.width*source.height*2)?Status::Ok:Status::Unsupported;
#else
  (void)frame;(void)y;(void)source;return Status::Unsupported;
#endif
}

#if defined(CONFIG_IDF_TARGET_ESP32P4)
bool IRAM_ATTR VideoTransport::gdmaDone(async_memcpy_handle_t,async_memcpy_event_t*,void *context){
  return complete(context);
}
bool VideoTransport::copyWithGdma(int x,int y,PixelView view){
  if(mipiAtomic_&&(!mipiAtomicOpen_||mipiAtomicPending_||mipiAtomicFailure_!=Status::Ok))return false;
  if(!mipiGdma_||!mipiGdmaDone_||swapRedBlue_||view.order!=PixelOrder::Native||
     x!=0||view.width!=rasterWidth()||view.stride!=view.width||view.width*view.height<2048||
     (reinterpret_cast<uintptr_t>(view.data)&15)||
     (!esp_ptr_internal(view.data)&&!esp_ptr_external_ram(view.data)))return false;
  const size_t bytes=view.width*view.height*2;
  auto *out=rgbFramebuffer_+size_t(y)*rasterWidth();
  // DMA invalidation must stay inside the framebuffer allocation and cannot
  // discard neighboring dirty pixels. Only complete cache-aligned rows qualify.
  if(!ppaCacheLine_||reinterpret_cast<uintptr_t>(out)%ppaCacheLine_||bytes%ppaCacheLine_)return false;
  const uintptr_t source=reinterpret_cast<uintptr_t>(view.data),base=reinterpret_cast<uintptr_t>(rgbFramebuffer_);
  const size_t frameBytes=rasterWidth()*rasterHeight()*2;
  if(source<base+frameBytes&&base<source+bytes)return false;
  return copyContiguousGdma(out,view.data,bytes);
}
bool VideoTransport::copyContiguousGdma(uint16_t *out,const void *input,size_t bytes){
  if(!mipiGdma_||!mipiGdmaDone_||!out||!input||!bytes||!ppaCacheLine_||
     reinterpret_cast<uintptr_t>(out)%ppaCacheLine_||bytes%ppaCacheLine_||(reinterpret_cast<uintptr_t>(input)&15))return false;
  const uintptr_t source=reinterpret_cast<uintptr_t>(input),target=reinterpret_cast<uintptr_t>(out);
  if(source>UINTPTR_MAX-bytes||target>UINTPTR_MAX-bytes||(source<target+bytes&&target<source+bytes))return false;
  const auto ram=[](uintptr_t address,size_t count){return (esp_ptr_internal(reinterpret_cast<void*>(address))&&esp_ptr_internal(reinterpret_cast<void*>(address+count-1)))||
    (esp_ptr_external_ram(reinterpret_cast<void*>(address))&&esp_ptr_external_ram(reinterpret_cast<void*>(address+count-1)));};
  if(!ram(source,bytes)||!ram(target,bytes))return false;
  while(xSemaphoreTake(mipiGdmaDone_,0)==pdTRUE){}
  const auto accepted=esp_async_memcpy(mipiGdma_,out,const_cast<void*>(input),bytes,gdmaDone,mipiGdmaDone_);
  if(accepted!=ESP_OK){if(copyStats_)++copyStats_->gdmaFailures;return false;}
  // Always fence an accepted source before returning or taking a CPU fallback.
  xSemaphoreTake(mipiGdmaDone_,portMAX_DELAY);
  if(esp_cache_msync(out,bytes,ESP_CACHE_MSYNC_FLAG_DIR_M2C)!=ESP_OK){if(copyStats_)++copyStats_->gdmaFailures;return false;}
  if(!mipiGdmaVerified_){
    if(memcmp(out,input,bytes)){
      if(copyStats_){++copyStats_->gdmaVerificationFailures;copyStats_->gdmaReady=false;}
      if(esp_async_memcpy_uninstall(mipiGdma_)==ESP_OK)mipiGdma_=nullptr;
      return false;
    }
    mipiGdmaVerified_=true;
  }
  if(copyStats_){++copyStats_->gdmaTransfers;copyStats_->gdmaPixels+=bytes/2;}
  return true;
}

Status VideoTransport::failMipiAtomic(Status status){
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL && NICE_GFX_NATIVE_STAGE_ENGINE_AVAILABLE
  if(nativeFillEngine_){nativeFillEngine_->disabled=true;nativeFillEngine_->stats.disabled=true;}
#endif
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_)mipiPresentation_.fault();
#endif
  retainedTags_.reset();
  // Ownership after an SDK submission error or missing DMA fence is unknown.
  // Keep both SDK buffers unavailable to writers until shutdown stops the panel.
  if(status==Status::Ok)status=Status::IoError;
  if(mipiAtomicFailure_==Status::Ok)mipiAtomicFailure_=status;
  return result_=mipiAtomicFailure_;
}
void VideoTransport::clearMipiDirty(){
  if(mipiDirty_)for(size_t row=0;row<rasterHeight();++row)
    mipiDirty_[row]={uint16_t(rasterWidth()),0};
  mipiDirtyTop_=rasterHeight();mipiDirtyBottom_=0;
}
Status VideoTransport::waitMipiAtomicFlip(){
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_){bool retired=false;return observeMipiPresentation(true,retired);}
#endif
  if(mipiAtomicFailure_!=Status::Ok)return mipiAtomicFailure_;
  if(!mipiAtomicPending_)return Status::Ok;
  if(!mipiFrameFence_||xPortInIsrContext()||!xPortCanYield())return failMipiAtomic(Status::IoError);
  const int64_t started=esp_timer_get_time();
  while(uint32_t(mipiFrameSequence_.load(std::memory_order_acquire)-mipiFenceBaseline_)<mipiFenceCompletions_){
    const int64_t remaining=250000-(esp_timer_get_time()-started);
    if(remaining<=0){
      if(copyStats_)++copyStats_->linkTimeouts;
      return failMipiAtomic(Status::IoError);
    }
    TickType_t ticks=pdMS_TO_TICKS(uint32_t((remaining+999)/1000));if(!ticks)ticks=1;
    // The semaphore is only a wakeup. Sequence arithmetic is the fence and
    // remains correct when notifications coalesce or an ISR runs before wait.
    xSemaphoreTake(mipiFrameFence_,ticks);
  }
  if(copyStats_){++copyStats_->frameSwitches;copyStats_->linkWaitMicros+=uint64_t(esp_timer_get_time()-started);}
  mipiFront_^=1;rgbFramebuffer_=mipiFrames_[mipiFront_^1];
  ++mipiOwnershipEpoch_; // Logical ownership generation, not a VSYNC count.
  mipiAtomicPending_=false;
  return result_=Status::Ok;
}
void VideoTransport::noteMipiDamage(int x,int y,size_t width,size_t height){
  if(mipiDamageFull_||!width||!height)return;
  if(x<0||y<0||width>rasterWidth()||height>rasterHeight()||
     size_t(x)>rasterWidth()-width||size_t(y)>rasterHeight()-height){
    mipiDamageFull_=true;mipiDamageCount_=0;return;
  }
  DirtyRect merged(x,y,int32_t(width),int32_t(height));
  // A declared native region normally contains every primitive. Include any
  // extra draw/fill rectangle as well, even when it exceeds the declaration.
  for(size_t i=0;i<mipiDamageCount_;){
    const auto &r=mipiDamage_[i];
    if(merged.x>=r.x&&merged.y>=r.y&&merged.x+merged.width<=r.x+r.width&&merged.y+merged.height<=r.y+r.height)return;
    if(merged.x>r.x+r.width||r.x>merged.x+merged.width||merged.y>r.y+r.height||r.y>merged.y+merged.height){++i;continue;}
    const int32_t left=merged.x<r.x?merged.x:r.x,top=merged.y<r.y?merged.y:r.y;
    const int32_t right=merged.x+merged.width>r.x+r.width?merged.x+merged.width:r.x+r.width;
    const int32_t bottom=merged.y+merged.height>r.y+r.height?merged.y+merged.height:r.y+r.height;
    merged=DirtyRect(left,top,right-left,bottom-top);
    mipiDamage_[i]=mipiDamage_[--mipiDamageCount_];i=0;
  }
  if(mipiDamageCount_==MipiMaxDamage){mipiDamageFull_=true;mipiDamageCount_=0;return;}
  mipiDamage_[mipiDamageCount_++]=merged;
}
// Candidate is intentionally a separate, parameterless SDK-pair capability.
// The general surface copy path and its SDK-FB exclusion remain unchanged.
Status VideoTransport::drainMipiFullInheritanceGdma(){
  if(!mipiFullInheritPending_)return mipiAtomicFailure_;
  if(!mipiGdmaDone_||xPortInIsrContext()||!xPortCanYield()){
    // Returning here could let a destructor release an accepted DMA's memory.
    // This is a misuse of the single-task transport; fail-stop, never recycle.
    failMipiAtomic(Status::IoError);
    for(;;){}
  }
  const int64_t started=copyStats_?esp_timer_get_time():0;
  bool timedOut=false;
  TickType_t ticks=pdMS_TO_TICKS(250);if(!ticks)ticks=1;
  // A timeout poisons this transport, but does NOT cancel DMA or grant ownership.
  // Keep draining, yielding in FreeRTOS, until the actual completion callback.
  // A permanently lost callback intentionally stalls and fails the host timeout;
  // no watchdog is fed here and no UI/CPU fallback can run past this fence.
  while(xSemaphoreTake(mipiGdmaDone_,ticks)!=pdTRUE){
    if(!timedOut){
      timedOut=true;
      if(copyStats_){++copyStats_->fullInheritGdmaTimeouts;++copyStats_->gdmaFailures;}
      failMipiAtomic(Status::IoError);
    }
  }
  mipiFullInheritPending_=false;
  if(copyStats_)copyStats_->fullInheritGdmaWaitMicros+=uint64_t(esp_timer_get_time()-started);
  return mipiAtomicFailure_;
}
Status VideoTransport::copyMipiFullInheritanceGdma(){
  const auto unsupported=[&](unsigned reason){
    if(copyStats_){copyStats_->fullInheritGdmaLastUnsupportedReason=reason;
      copyStats_->fullInheritGdmaUnsupportedMask|=uint32_t(1u<<(reason-1));
      ++copyStats_->fullInheritGdmaUnsupportedReasons[reason-1];}
    return Status::Unsupported;
  };
  if(copyStats_){
    copyStats_->fullInheritGdmaReady=mipiGdma_&&mipiGdmaDone_;
    copyStats_->fullInheritGdmaBurstBytes=64;copyStats_->fullInheritGdmaCacheLineBytes=uint16_t(ppaCacheLine_);
    copyStats_->fullInheritGdmaSourceLowBits=mipiFront_<2?uint16_t(reinterpret_cast<uintptr_t>(mipiFrames_[mipiFront_])&127u):0;
    copyStats_->fullInheritGdmaTargetLowBits=mipiFront_<2?uint16_t(reinterpret_cast<uintptr_t>(mipiFrames_[mipiFront_^1])&127u):0;
  }
#if defined(NICE_GFX_P4_MIPI_FORCE_CPU_FULL_INHERIT) && NICE_GFX_P4_MIPI_FORCE_CPU_FULL_INHERIT
  return unsupported(1);
#else
  // These are ownership errors, not a reason to write using a CPU fallback.
  if(!mipiAtomic_||mipiAtomicOpen_||mipiAtomicPending_||pending_||mipiFullInheritPending_||mipiPartialInheritPending_||
     mipiAtomicFailure_!=Status::Ok||mipiFront_>1||!panel_||
     !mipiFrames_[mipiFront_]||!mipiFrames_[mipiFront_^1]||
     mipiFrames_[mipiFront_]==mipiFrames_[mipiFront_^1]||
     rgbFramebuffer_!=mipiFrames_[mipiFront_^1])return Status::IoError;
  if(!ppaCacheLine_)return unsupported(4);
  if(!mipiGdma_)return unsupported(2);
  if(!mipiGdmaDone_)return unsupported(3);
  if(xPortInIsrContext()||!xPortCanYield())return unsupported(5);
  const size_t width=rasterWidth(),height=rasterHeight();
  if(!width||!height||width>SIZE_MAX/height/sizeof(uint16_t))return unsupported(6);
  const size_t bytes=width*height*sizeof(uint16_t);
  auto *target=mipiFrames_[mipiFront_^1];
  const auto *source=mipiFrames_[mipiFront_];
  const uintptr_t from=reinterpret_cast<uintptr_t>(source),to=reinterpret_cast<uintptr_t>(target);
  // All three constraints hold for 1024x600 SDK allocations. Other layouts use
  // the established CPU path: no expanded invalidation or cropped DMA writes.
  if(from>UINTPTR_MAX-bytes||to>UINTPTR_MAX-bytes||
     (from<to+bytes&&to<from+bytes))return Status::IoError;
  if(from%ppaCacheLine_||to%ppaCacheLine_||bytes%ppaCacheLine_)return unsupported(7);
  // This display's shared AXI memcpy channel is INSTALLED with a 64-byte burst.
  // Keep a burst guard, while accepting SDK FBs at offset64 modulo128.
  if((from&63)||(to&63)||(bytes&63))return unsupported(8);
  if(!esp_ptr_external_ram(source)||!esp_ptr_external_ram(source+width*height-1)||
     !esp_ptr_external_ram(target)||!esp_ptr_external_ram(target+width*height-1))return unsupported(9);
  const int64_t started=copyStats_?esp_timer_get_time():0;
  auto sync=[&](void *address,int flags){
    const int64_t at=copyStats_?esp_timer_get_time():0;
    const Status status=convert(esp_cache_msync(address,bytes,flags));
    if(copyStats_){++copyStats_->cacheSyncCalls;copyStats_->cacheSyncBytes+=bytes;
      copyStats_->fullInheritGdmaSyncMicros+=uint64_t(esp_timer_get_time()-at);}
    return status;
  };
  // Preserve all dirty CPU cache lines before DMA; flush the old inactive image
  // too so a later dirty eviction cannot overwrite the incoming DMA image.
  Status status=sync(const_cast<uint16_t*>(source),ESP_CACHE_MSYNC_FLAG_DIR_C2M);
  if(status==Status::Ok)status=sync(target,ESP_CACHE_MSYNC_FLAG_DIR_C2M);
  if(status!=Status::Ok)return status;
  while(xSemaphoreTake(mipiGdmaDone_,0)==pdTRUE){}
  if(copyStats_)++copyStats_->fullInheritGdmaAttempts;
  mipiFullInheritPending_=true;
  const esp_err_t accepted=esp_async_memcpy(mipiGdma_,target,const_cast<uint16_t*>(source),bytes,gdmaDone,mipiGdmaDone_);
  if(accepted!=ESP_OK){
    // The API did not accept this request. There is no DMA source lifetime to
    // fence, and the untouched target is safe for the original CPU inheritance.
    mipiFullInheritPending_=false;
    if(copyStats_){++copyStats_->fullInheritGdmaRejected;++copyStats_->gdmaFailures;}
    return Status::Unsupported;
  }
  status=drainMipiFullInheritanceGdma();
  // Always invalidate only after completion, including a late completion after
  // timeout. Failure remains poisoned and no following painter is permitted.
  const Status synced=sync(target,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
  if(status==Status::Ok)status=synced;
  if(status==Status::Ok&&!mipiFullInheritVerified_){
    const int64_t at=copyStats_?esp_timer_get_time():0;
    const bool mismatch=memcmp(target,source,bytes)!=0;
    if(copyStats_)copyStats_->fullInheritGdmaVerifyMicros+=uint64_t(esp_timer_get_time()-at);
    if(mismatch){
      if(copyStats_){++copyStats_->fullInheritGdmaVerificationFailures;
        ++copyStats_->gdmaVerificationFailures;++copyStats_->gdmaFailures;}
      status=Status::IoError;
    }else mipiFullInheritVerified_=true;
  }
  if(copyStats_){
    copyStats_->fullInheritGdmaMicros+=uint64_t(esp_timer_get_time()-started);
    if(status==Status::Ok){++copyStats_->fullInheritGdmaTransfers;
      ++copyStats_->gdmaTransfers;copyStats_->gdmaPixels+=bytes/2;}
  }
  // Caller permanently poisons atomic ownership on any accepted-copy error.
  return status;
#endif
}

bool IRAM_ATTR VideoTransport::partialInheritPpaDone(ppa_client_handle_t,
    ppa_event_data_t*,void *context){return complete(context);}

Status VideoTransport::drainMipiPartialInheritancePpa(){
  if(!mipiPartialInheritPending_)return mipiAtomicFailure_;
  if(!mipiPartialInheritDone_||xPortInIsrContext()||!xPortCanYield()){
    failMipiAtomic(Status::IoError);for(;;){} // Accepted DMA cannot be cancelled by returning.
  }
  const int64_t started=copyStats_?esp_timer_get_time():0;
  bool timedOut=false;TickType_t ticks=pdMS_TO_TICKS(250);if(!ticks)ticks=1;
  while(xSemaphoreTake(mipiPartialInheritDone_,ticks)!=pdTRUE){
    if(!timedOut){timedOut=true;
      if(copyStats_){++copyStats_->partialInheritPpaTimeouts;++copyStats_->ppaFailures;}
      failMipiAtomic(Status::IoError);
    }
  }
  mipiPartialInheritPending_=false;
  if(copyStats_)copyStats_->partialInheritPpaWaitMicros+=uint64_t(esp_timer_get_time()-started);
  return mipiAtomicFailure_;
}

Status VideoTransport::copyMipiPartialInheritancePpa(const DirtyRect &r){
#if !NICE_GFX_P4_MIPI_PARTIAL_INHERIT_PPA || ESP_IDF_VERSION != ESP_IDF_VERSION_VAL(5,5,5)
  (void)r;return Status::Unsupported;
#else
  // Only beginFrame's already-fenced SDK pair may enter this capability.
  // Do not relax the independent-surface copy API's SDK allocation exclusion.
  if(!mipiAtomic_||mipiAtomicOpen_||mipiAtomicPending_||pending_||mipiFullInheritPending_||
     mipiPartialInheritPending_||mipiAtomicFailure_!=Status::Ok||mipiFront_>1||!panel_||
     !mipiFrames_[mipiFront_]||!mipiFrames_[mipiFront_^1]||
     mipiFrames_[mipiFront_]==mipiFrames_[mipiFront_^1]||
     rgbFramebuffer_!=mipiFrames_[mipiFront_^1])return Status::IoError;
  const size_t width=rasterWidth(),height=rasterHeight();
  if(!width||!height||width>SIZE_MAX/height/2||r.x<0||r.y<0||r.width<=0||r.height<=0||
     size_t(r.width)>width||size_t(r.height)>height||size_t(r.x)>width-size_t(r.width)||
     size_t(r.y)>height-size_t(r.height))return Status::IoError;
  const size_t pixels=size_t(r.width)*size_t(r.height),bytes=width*height*2;
  if((r.x==0&&r.y==0&&size_t(r.width)==width&&size_t(r.height)==height)||
     pixels<NICE_GFX_P4_MIPI_PARTIAL_INHERIT_MIN_PIXELS||r.width<128||r.height<2||
     width>0x3FFF||height>0x3FFF||!mipiPartialInheritPpa_||!mipiPartialInheritDone_||
     !ppaCacheLine_||xPortInIsrContext()||!xPortCanYield())return Status::Unsupported;
#if defined(CONFIG_SECURE_FLASH_ENC_ENABLED) && CONFIG_SECURE_FLASH_ENC_ENABLED
  return Status::Unsupported; // SDK documents PSRAM SRM as unsupported with flash encryption.
#endif
  auto *back=mipiFrames_[mipiFront_^1];const auto *front=mipiFrames_[mipiFront_];
  const uintptr_t from=reinterpret_cast<uintptr_t>(front),to=reinterpret_cast<uintptr_t>(back);
  if(from>UINTPTR_MAX-bytes||to>UINTPTR_MAX-bytes||(from<to+bytes&&to<from+bytes))return Status::IoError;
  if(from%ppaCacheLine_||to%ppaCacheLine_||bytes%ppaCacheLine_||(width*2)%ppaCacheLine_||
     !esp_ptr_external_ram(front)||!esp_ptr_external_ram(front+width*height-1)||
     !esp_ptr_external_ram(back)||!esp_ptr_external_ram(back+width*height-1))return Status::Unsupported;
#if NICE_GFX_P4_MIPI_PARTIAL_BOUNDARY_GUARD
  // This helper submits only RGB565 identity SRM. Both supported MB sizes are
  // conservatively covered; mode is not read or forced. No DMA is accepted yet.
  const uint32_t tailWidth=uint32_t(r.width)&63u;
  const uint32_t tailHeight16=((uint32_t(r.height)-1u)&15u)+1u;
  const uint32_t tailHeight32=((uint32_t(r.height)-1u)&31u)+1u;
  if(tailWidth*tailHeight16==96u||tailWidth*tailHeight32==96u){
    if(copyStats_)++copyStats_->partialInheritPpaBoundaryRejects;
    return Status::Unsupported; // Original caller owns the fenced CPU fallback.
  }
#endif
  // Match IDF 5.5.5 SRM's complete-row input C2M and output M2C window.
  // A whole allocated frame is passed to PPA; crop offsets never pretend an
  // interior pointer owns width*height bytes beyond its allocation.
  const size_t offset=size_t(r.y)*width,windowBytes=size_t(r.height)*width*2;
  auto *targetRows=back+offset;const auto *sourceRows=front+offset;
  const int64_t started=copyStats_?esp_timer_get_time():0;
  const auto sync=[&](void *address,int flags){
    const int64_t at=copyStats_?esp_timer_get_time():0;
    const Status s=convert(esp_cache_msync(address,windowBytes,flags));
    if(copyStats_){++copyStats_->cacheSyncCalls;copyStats_->cacheSyncBytes+=windowBytes;
      copyStats_->partialInheritPpaSyncMicros+=uint64_t(esp_timer_get_time()-at);}
    return s;
  };
  const auto finish=[&](Status s){if(copyStats_)copyStats_->partialInheritPpaMicros+=uint64_t(esp_timer_get_time()-started);return s;};
  Status status=sync(const_cast<uint16_t*>(sourceRows),ESP_CACHE_MSYNC_FLAG_DIR_C2M);
  if(status==Status::Ok)status=sync(targetRows,ESP_CACHE_MSYNC_FLAG_DIR_C2M);
  if(status!=Status::Ok)return finish(status);
  ppa_srm_oper_config_t operation={};
  operation.in.buffer=front;operation.in.pic_w=width;operation.in.pic_h=height;
  operation.in.block_offset_x=r.x;operation.in.block_offset_y=r.y;
  operation.in.block_w=r.width;operation.in.block_h=r.height;operation.in.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
  operation.out.buffer=back;operation.out.buffer_size=bytes;operation.out.pic_w=width;operation.out.pic_h=height;
  operation.out.block_offset_x=r.x;operation.out.block_offset_y=r.y;operation.out.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
  operation.rotation_angle=PPA_SRM_ROTATION_ANGLE_0;operation.scale_x=operation.scale_y=1.0f;
  operation.mode=PPA_TRANS_MODE_NON_BLOCKING;operation.user_data=mipiPartialInheritDone_;
  while(xSemaphoreTake(mipiPartialInheritDone_,0)==pdTRUE){}
  if(copyStats_)++copyStats_->partialInheritPpaAttempts;
  mipiPartialInheritPending_=true;
  const esp_err_t accepted=ppa_do_scale_rotate_mirror(mipiPartialInheritPpa_,&operation);
  if(accepted!=ESP_OK){
    // Audited IDF 5.5.5 validates first, or removes/recycles a failed enqueue.
    // No non-OK return owns this transaction. Only documented rejections may
    // fall back; unexpected errors still poison the atomic transport.
    mipiPartialInheritPending_=false;
    if(copyStats_){++copyStats_->partialInheritPpaRejected;++copyStats_->ppaFailures;}
    return finish(accepted==ESP_ERR_INVALID_ARG||accepted==ESP_FAIL?Status::Unsupported:Status::IoError);
  }
  status=drainMipiPartialInheritancePpa();
  const Status synced=sync(targetRows,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
  if(status==Status::Ok)status=synced;
  if(status==Status::Ok&&!mipiPartialInheritVerified_){
    const int64_t at=copyStats_?esp_timer_get_time():0;bool mismatch=false;
    for(int32_t row=0;row<r.height&&!mismatch;++row){const size_t index=size_t(r.y+row)*width+size_t(r.x);
      mismatch=memcmp(back+index,front+index,size_t(r.width)*2)!=0;}
    if(copyStats_)copyStats_->partialInheritPpaVerifyMicros+=uint64_t(esp_timer_get_time()-at);
    if(mismatch){status=Status::IoError;if(copyStats_){++copyStats_->partialInheritPpaVerificationFailures;++copyStats_->ppaVerificationFailures;++copyStats_->ppaFailures;}}
    else mipiPartialInheritVerified_=true;
  }
  if(status==Status::Ok&&copyStats_){++copyStats_->partialInheritPpaTransfers;
    copyStats_->partialInheritPpaPixels+=pixels;++copyStats_->ppaTransfers;copyStats_->ppaPixels+=pixels;}
  return finish(status);
#endif
}

Status VideoTransport::queueMipiInheritancePrefetch(const DirtyRect &r,size_t index){
#if !NICE_GFX_P4_MIPI_INHERIT_PREFETCH || !NICE_GFX_P4_MIPI_PARTIAL_INHERIT_PPA || ESP_IDF_VERSION != ESP_IDF_VERSION_VAL(5,5,5)
  (void)r;(void)index;return Status::Unsupported;
#else
  if(!mipiPartialInheritVerified_||mipiPrefetchActive_||mipiPrefetchReady_||index>=MipiMaxDamage||
     mipiInitCore_!=1||xPortInIsrContext()||!xPortCanYield()||xPortGetCoreID()!=1||xTaskGetCoreID(nullptr)!=1)return Status::Unsupported;
  // Only beginFrame's already-fenced SDK pair may enter this capability.
  // Do not relax the independent-surface copy API's SDK allocation exclusion.
  if(!mipiAtomic_||mipiAtomicOpen_||mipiAtomicPending_||pending_||mipiFullInheritPending_||
     mipiPartialInheritPending_||mipiAtomicFailure_!=Status::Ok||mipiFront_>1||!panel_||
     !mipiFrames_[mipiFront_]||!mipiFrames_[mipiFront_^1]||
     mipiFrames_[mipiFront_]==mipiFrames_[mipiFront_^1]||
     rgbFramebuffer_!=mipiFrames_[mipiFront_^1])return Status::IoError;
  const size_t width=rasterWidth(),height=rasterHeight();
  if(!width||!height||width>SIZE_MAX/height/2||r.x<0||r.y<0||r.width<=0||r.height<=0||
     size_t(r.width)>width||size_t(r.height)>height||size_t(r.x)>width-size_t(r.width)||
     size_t(r.y)>height-size_t(r.height))return Status::IoError;
  const size_t pixels=size_t(r.width)*size_t(r.height),bytes=width*height*2;
  if((r.x==0&&r.y==0&&size_t(r.width)==width&&size_t(r.height)==height)||
     pixels<NICE_GFX_P4_MIPI_PARTIAL_INHERIT_MIN_PIXELS||r.width<128||r.height<2||
     width>0x3FFF||height>0x3FFF||!mipiPartialInheritPpa_||!mipiPartialInheritDone_||
     !ppaCacheLine_||xPortInIsrContext()||!xPortCanYield())return Status::Unsupported;
#if defined(CONFIG_SECURE_FLASH_ENC_ENABLED) && CONFIG_SECURE_FLASH_ENC_ENABLED
  return Status::Unsupported; // SDK documents PSRAM SRM as unsupported with flash encryption.
#endif
  auto *back=mipiFrames_[mipiFront_^1];const auto *front=mipiFrames_[mipiFront_];
  const uintptr_t from=reinterpret_cast<uintptr_t>(front),to=reinterpret_cast<uintptr_t>(back);
  if(from>UINTPTR_MAX-bytes||to>UINTPTR_MAX-bytes||(from<to+bytes&&to<from+bytes))return Status::IoError;
  if(from%ppaCacheLine_||to%ppaCacheLine_||bytes%ppaCacheLine_||(width*2)%ppaCacheLine_||
     !esp_ptr_external_ram(front)||!esp_ptr_external_ram(front+width*height-1)||
     !esp_ptr_external_ram(back)||!esp_ptr_external_ram(back+width*height-1))return Status::Unsupported;
  // Match IDF 5.5.5 SRM's complete-row input C2M and output M2C window.
  // A whole allocated frame is passed to PPA; crop offsets never pretend an
  // interior pointer owns width*height bytes beyond its allocation.
  const size_t offset=size_t(r.y)*width,windowBytes=size_t(r.height)*width*2;
  auto *targetRows=back+offset;const auto *sourceRows=front+offset;
  const int64_t started=copyStats_?esp_timer_get_time():0;
  const auto sync=[&](void *address,int flags){
    const int64_t at=copyStats_?esp_timer_get_time():0;
    const Status s=convert(esp_cache_msync(address,windowBytes,flags));
    if(copyStats_){++copyStats_->cacheSyncCalls;copyStats_->cacheSyncBytes+=windowBytes;
      copyStats_->partialInheritPpaSyncMicros+=uint64_t(esp_timer_get_time()-at);}
    return s;
  };
  const auto finish=[&](Status s){if(copyStats_){const uint64_t elapsed=uint64_t(esp_timer_get_time()-started);copyStats_->partialInheritPpaMicros+=elapsed;copyStats_->inheritMicros+=elapsed;copyStats_->inheritPrefetchSubmitMicros+=elapsed;}return s;};
  Status status=sync(const_cast<uint16_t*>(sourceRows),ESP_CACHE_MSYNC_FLAG_DIR_C2M);
  if(status==Status::Ok)status=sync(targetRows,ESP_CACHE_MSYNC_FLAG_DIR_C2M);
  if(status!=Status::Ok)return finish(status);
  ppa_srm_oper_config_t operation={};
  operation.in.buffer=front;operation.in.pic_w=width;operation.in.pic_h=height;
  operation.in.block_offset_x=r.x;operation.in.block_offset_y=r.y;
  operation.in.block_w=r.width;operation.in.block_h=r.height;operation.in.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
  operation.out.buffer=back;operation.out.buffer_size=bytes;operation.out.pic_w=width;operation.out.pic_h=height;
  operation.out.block_offset_x=r.x;operation.out.block_offset_y=r.y;operation.out.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
  operation.rotation_angle=PPA_SRM_ROTATION_ANGLE_0;operation.scale_x=operation.scale_y=1.0f;
  operation.mode=PPA_TRANS_MODE_NON_BLOCKING;operation.user_data=mipiPartialInheritDone_;
  while(xSemaphoreTake(mipiPartialInheritDone_,0)==pdTRUE){}
  if(copyStats_){++copyStats_->partialInheritPpaAttempts;++copyStats_->inheritPrefetchAttempts;}
  // Record all accepted-lease metadata BEFORE the SDK may callback.
  mipiPrefetchRect_=r;mipiPrefetchIndex_=index;mipiPrefetchGeneration_=mipiOwnershipEpoch_;
  mipiPrefetchSource_=front;mipiPrefetchTarget_=back;mipiPrefetchRows_=targetRows;mipiPrefetchRowBytes_=windowBytes;
  mipiPrefetchStarted_=esp_timer_get_time();mipiPrefetchActive_=true;
  mipiPartialInheritPending_=true;
  const esp_err_t accepted=ppa_do_scale_rotate_mirror(mipiPartialInheritPpa_,&operation);
  if(accepted!=ESP_OK){
    // Audited IDF 5.5.5 validates first, or removes/recycles a failed enqueue.
    // No non-OK return owns this transaction. Only documented rejections may
    // fall back; unexpected errors still poison the atomic transport.
    mipiPartialInheritPending_=false;mipiPrefetchActive_=false;
    if(copyStats_){++copyStats_->partialInheritPpaRejected;++copyStats_->ppaFailures;++copyStats_->inheritPrefetchRejected;}
    return finish(accepted==ESP_ERR_INVALID_ARG||accepted==ESP_FAIL?Status::Unsupported:Status::IoError);
  }
  if(copyStats_){++copyStats_->inheritPrefetchAccepted;copyStats_->inheritPrefetchPending=true;}
  return finish(Status::Ok);
#endif
}

Status VideoTransport::startMipiInheritancePrefetch(){
#if !NICE_GFX_P4_MIPI_INHERIT_PREFETCH
  return Status::Ok;
#else
  // The caller has already completed the original one/two-callback flip
  // fence. No SDK submission is ever initiated by the ISR callback.
  if(!mipiAtomic_||mipiAtomicOpen_||mipiAtomicPending_||mipiAtomicFailure_!=Status::Ok||
     mipiStaleFull_||!mipiStaleCount_||mipiStaleCount_>MipiMaxDamage||
     mipiPrefetchActive_||mipiPrefetchReady_||!mipiPartialInheritVerified_||
     mipiInitCore_!=1||xPortInIsrContext()||!xPortCanYield()||
     xPortGetCoreID()!=1||xTaskGetCoreID(nullptr)!=1)return Status::Ok;
  size_t selected=MipiMaxDamage;uint64_t maximum=0;
  for(size_t i=0;i<mipiStaleCount_;++i){const auto &r=mipiStaleDamage_[i];
    if(r.x<0||r.y<0||r.width<128||r.height<2||
       size_t(r.width)>rasterWidth()||size_t(r.height)>rasterHeight()||
       size_t(r.x)>rasterWidth()-size_t(r.width)||size_t(r.y)>rasterHeight()-size_t(r.height))continue;
    const uint64_t pixels=uint64_t(r.width)*uint64_t(r.height);
    if(pixels<NICE_GFX_P4_MIPI_PARTIAL_INHERIT_MIN_PIXELS||
       (r.x==0&&r.y==0&&size_t(r.width)==rasterWidth()&&size_t(r.height)==rasterHeight()))continue;
    if(pixels>maximum){maximum=pixels;selected=i;}
  }
  if(selected==MipiMaxDamage)return Status::Ok;
  const Status status=queueMipiInheritancePrefetch(mipiStaleDamage_[selected],selected);
  return status==Status::Unsupported?Status::Ok:status==Status::Ok?Status::Ok:failMipiAtomic(status);
#endif
}

Status VideoTransport::drainMipiInheritancePrefetch(){
#if !NICE_GFX_P4_MIPI_INHERIT_PREFETCH
  return Status::Ok;
#else
  if(!mipiPrefetchActive_)return Status::Ok;
  const int64_t started=copyStats_?esp_timer_get_time():0;
  // This independently tracks an ACCEPTED source/destination lease even when
  // wait()/abort() would otherwise early-return an atomic display failure.
  // The existing drain poisons on timeout but does not release pending until
  // a real completion arrives. A never-completing request never frees FBs.
  Status status=drainMipiPartialInheritancePpa();
  const int64_t syncStarted=copyStats_?esp_timer_get_time():0;
  const Status synced=convert(esp_cache_msync(mipiPrefetchRows_,mipiPrefetchRowBytes_,ESP_CACHE_MSYNC_FLAG_DIR_M2C));
  if(copyStats_){const uint64_t elapsed=uint64_t(esp_timer_get_time()-syncStarted);
    ++copyStats_->cacheSyncCalls;copyStats_->cacheSyncBytes+=mipiPrefetchRowBytes_;
    copyStats_->partialInheritPpaSyncMicros+=elapsed;copyStats_->inheritPrefetchPostSyncMicros+=elapsed;}
  if(status==Status::Ok)status=synced;
  // Validate the lease token after quiescence, before allowing any writer.
  if(mipiPrefetchGeneration_!=mipiOwnershipEpoch_||mipiPrefetchSource_!=mipiFrames_[mipiFront_]||
     mipiPrefetchTarget_!=mipiFrames_[mipiFront_^1]||rgbFramebuffer_!=mipiPrefetchTarget_)
    status=Status::IoError;
  mipiPrefetchActive_=false;mipiPrefetchReady_=status==Status::Ok;
  if(copyStats_){copyStats_->inheritPrefetchPending=false;
    copyStats_->inheritPrefetchDrainMicros+=uint64_t(esp_timer_get_time()-started);
    copyStats_->partialInheritPpaMicros+=uint64_t(esp_timer_get_time()-started);
    copyStats_->inheritMicros+=uint64_t(esp_timer_get_time()-started);
    copyStats_->inheritPrefetchLifetimeMicros+=uint64_t(esp_timer_get_time()-mipiPrefetchStarted_);
    if(status==Status::Ok){const uint64_t pixels=mipiPrefetchRect_.pixels();
      ++copyStats_->inheritPrefetchCompleted;copyStats_->inheritPrefetchPixels+=pixels;
      ++copyStats_->partialInheritPpaTransfers;copyStats_->partialInheritPpaPixels+=pixels;
      ++copyStats_->ppaTransfers;copyStats_->ppaPixels+=pixels;copyStats_->inheritedPixels+=pixels;}}
  if(status!=Status::Ok){mipiPrefetchReady_=false;mipiStaleFull_=true;mipiStaleCount_=0;return failMipiAtomic(status);}
  return Status::Ok;
#endif
}

Status VideoTransport::inheritMipiAtomicRect(const DirtyRect &r){
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_)return failMipiAtomic(Status::IoError);
#endif
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(!mipiPresentationBound_)
#endif
  retainedTags_.written(mipiFront_^1,r);
  const int64_t started=copyStats_?esp_timer_get_time():0;
  const bool full=r.x==0&&r.y==0&&size_t(r.width)==rasterWidth()&&size_t(r.height)==rasterHeight();
  if(full){
    const Status accelerated=copyMipiFullInheritanceGdma();
    if(accelerated!=Status::Unsupported){
      if(copyStats_){copyStats_->inheritMicros+=uint64_t(esp_timer_get_time()-started);
        if(accelerated==Status::Ok)copyStats_->inheritedPixels+=uint64_t(r.width)*r.height;}
      return accelerated;
    }
    if(copyStats_)++copyStats_->fullInheritCpuTransfers;
  }else{
    const Status accelerated=copyMipiPartialInheritancePpa(r);
    if(accelerated!=Status::Unsupported){
      if(copyStats_){copyStats_->inheritMicros+=uint64_t(esp_timer_get_time()-started);
        if(accelerated==Status::Ok)copyStats_->inheritedPixels+=uint64_t(r.width)*r.height;}
      return accelerated;
    }
    if(copyStats_)++copyStats_->partialInheritPpaCpuTransfers;
  }
  auto *back=rgbFramebuffer_+size_t(r.y)*rasterWidth()+size_t(r.x);
  const auto *front=mipiFrames_[mipiFront_]+size_t(r.y)*rasterWidth()+size_t(r.x);
  if(size_t(r.width)==rasterWidth())memcpy(back,front,size_t(r.width)*size_t(r.height)*2);
  else for(int32_t row=0;row<r.height;++row)
    memcpy(back+size_t(row)*rasterWidth(),front+size_t(row)*rasterWidth(),size_t(r.width)*2);
  // Clean inherited CPU stores before PPA/GDMA can touch a shared cache line.
  // Including the unchanged gaps between these rows is safe on the inactive FB.
  const size_t bytes=((size_t(r.height)-1)*rasterWidth()+size_t(r.width))*2;
  const Status s=convert(esp_cache_msync(back,bytes,
    ESP_CACHE_MSYNC_FLAG_DIR_C2M|ESP_CACHE_MSYNC_FLAG_UNALIGNED));
  if(copyStats_){const uint64_t pixels=uint64_t(r.width)*r.height;
    copyStats_->inheritedPixels+=pixels;copyStats_->cpuPixels+=pixels;
    copyStats_->inheritMicros+=uint64_t(esp_timer_get_time()-started);
    ++copyStats_->cacheSyncCalls;copyStats_->cacheSyncBytes+=bytes;}
  return s;
}
Status VideoTransport::beginMipiAtomicFrame(bool explicitFrame,const DirtyRect *regions,size_t count,bool repaint){
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_)return beginMipiPresentationLegacy(explicitFrame,regions,count,repaint);
#endif
  const Status inherited=drainMipiInheritancePrefetch();if(inherited!=Status::Ok)return inherited;
  if(mipiAtomicFailure_!=Status::Ok)return mipiAtomicFailure_;
  if(!panel_||!rasterReady()||!mipiFrames_[0]||!mipiFrames_[1])return Status::NotInitialized;
  if(count&&!regions)return Status::InvalidArgument;
  const bool bounded=count&&count<=MipiMaxDamage;
  if(bounded)for(size_t i=0;i<count;++i){const auto &r=regions[i];
    if(r.x<0||r.y<0||r.width<=0||r.height<=0||size_t(r.width)>rasterWidth()||size_t(r.height)>rasterHeight()||
       size_t(r.x)>rasterWidth()-size_t(r.width)||size_t(r.y)>rasterHeight()-size_t(r.height))return Status::InvalidArgument;
  }
  if(mipiAtomicOpen_){
    if(explicitFrame){mipiAtomicExplicit_=true;
      if(!bounded){mipiDamageFull_=true;mipiDamageCount_=0;}
      else for(size_t i=0;i<count;++i)noteMipiDamage(regions[i].x,regions[i].y,size_t(regions[i].width),size_t(regions[i].height));
    }
    return Status::Ok;
  }
  Status s=wait();if(s==Status::Ok)s=waitMipiAtomicFlip();if(s!=Status::Ok)return s;
  rgbFramebuffer_=mipiFrames_[mipiFront_^1];
  // After a successful flip, the old front differs from the latest front only
  // inside the last frame's conservative damage. Copy all of that damage;
  // unlike stale-minus-new, this never assumes the next paint fully replaces it.
  const DirtyRect full(0,0,int32_t(rasterWidth()),int32_t(rasterHeight()));
  bool planned=false;
#if NICE_GFX_FRAME_REPAINT_INHERIT
  // Only the explicit, idle, fully-replacing Renderer contract may subtract.
  // Both original wait/fence calls above completed before acquiring this back.
  if(repaint&&explicitFrame&&bounded){
    const int64_t at=copyStats_?esp_timer_get_time():0;
    DirtyRect needed[32];size_t neededCount=0;
    const DirtyRect *stale=mipiStaleFull_?&full:mipiStaleDamage_;
    const size_t staleCount=mipiStaleFull_?1:mipiStaleCount_;
    planned=detail::subtractRepaint(stale,staleCount,regions,count,rasterWidth(),rasterHeight(),needed,neededCount);
    if(copyStats_)copyStats_->repaintInheritPlanMicros+=uint64_t(esp_timer_get_time()-at);
    if(planned){
      uint64_t before=0,after=0;for(size_t i=0;i<staleCount;++i)before+=stale[i].pixels();
      for(size_t i=0;i<neededCount;++i)after+=needed[i].pixels();
      for(size_t i=0;i<neededCount&&s==Status::Ok;++i)s=inheritMipiAtomicRect(needed[i]);
      if(copyStats_){++copyStats_->repaintInheritFrames;copyStats_->repaintInheritFragments+=neededCount;
        copyStats_->repaintInheritSkippedPixels+=before-after;}
    }else if(copyStats_)++copyStats_->repaintInheritFallbackFrames;
  }
#else
  (void)repaint;
#endif
  if(!planned){
    bool useVerifiedFull=false;
#if NICE_GFX_P4_MIPI_NO_PPA_FULL_INHERIT && !NICE_GFX_P4_MIPI_INHERIT_PREFETCH && \
    !(defined(NICE_GFX_P4_MIPI_FORCE_CPU_FULL_INHERIT) && NICE_GFX_P4_MIPI_FORCE_CPU_FULL_INHERIT)
    if(!mipiStaleFull_&&mipiStaleCount_&&mipiStaleCount_<=MipiMaxDamage){
      // The prior full copy verified this fixed SDK pair. Copying unchanged
      // pixels too is conservative: no damage is subtracted or skipped.
      useVerifiedFull=explicitFrame&&bounded&&mipiDirectSdkDpi_&&mipiAtomic_&&
        !mipiPpaConfigured_&&!ppaCopy_&&!ppaFill_&&!mipiPartialInheritPpa_&&
        mipiFullInheritVerified_&&mipiGdma_&&mipiGdmaDone_&&ppaCacheLine_==64&&
        rasterWidth()==1024&&rasterHeight()==600&&!swapRedBlue_&&
        !mipiPrefetchActive_&&!mipiPrefetchReady_&&!mipiFullInheritPending_&&!mipiPartialInheritPending_&&
        !pending_&&!mipiAtomicPending_&&mipiFront_<2&&
        mipiFrames_[0]!=mipiFrames_[1]&&rgbFramebuffer_==mipiFrames_[mipiFront_^1]&&
        !((reinterpret_cast<uintptr_t>(mipiFrames_[0])|reinterpret_cast<uintptr_t>(mipiFrames_[1]))&63u)&&
        esp_ptr_external_ram(mipiFrames_[0])&&esp_ptr_external_ram(mipiFrames_[0]+1024*600-1)&&
        esp_ptr_external_ram(mipiFrames_[1])&&esp_ptr_external_ram(mipiFrames_[1]+1024*600-1);
      if(copyStats_){if(useVerifiedFull)++copyStats_->noPpaFullInheritSelections;
        else ++copyStats_->noPpaFullInheritIneligible;}
    }
#endif
    // Reuse the existing full-copy validation, accepted-DMA drain, cache sync,
    // rejection CPU fallback and error propagation. Never invent a new fence.
    if(mipiStaleFull_||useVerifiedFull)s=inheritMipiAtomicRect(full);
    else for(size_t i=0;i<mipiStaleCount_&&s==Status::Ok;++i){const auto &r=mipiStaleDamage_[i];
#if NICE_GFX_P4_MIPI_INHERIT_PREFETCH
      if(mipiPrefetchReady_&&i==mipiPrefetchIndex_&&mipiPrefetchGeneration_==mipiOwnershipEpoch_&&
         mipiPrefetchSource_==mipiFrames_[mipiFront_]&&mipiPrefetchTarget_==rgbFramebuffer_&&
         r.x==mipiPrefetchRect_.x&&r.y==mipiPrefetchRect_.y&&r.width==mipiPrefetchRect_.width&&r.height==mipiPrefetchRect_.height){
        if(copyStats_)++copyStats_->inheritPrefetchConsumed;continue;}
#endif
      s=inheritMipiAtomicRect(r);}}
  mipiPrefetchReady_=false;
  if(s!=Status::Ok)return failMipiAtomic(s);
  if(!retainedAcquiring_)retainedTags_.ordinaryDamage(mipiFront_^1,bounded?regions:nullptr,bounded?count:0);
  // Unknown/implicit frames and aborts conservatively taint the complete back.
  mipiStaleFull_=true;mipiStaleCount_=0;
  mipiDamageFull_=!explicitFrame||!bounded;mipiDamageCount_=0;
  if(!mipiDamageFull_)for(size_t i=0;i<count;++i)
    noteMipiDamage(regions[i].x,regions[i].y,size_t(regions[i].width),size_t(regions[i].height));
  clearMipiDirty();mipiAtomicOpen_=true;mipiAtomicExplicit_=explicitFrame;
  mipiFrame_=true;mipiBatch_=false;
  return result_=Status::Ok;
}
bool VideoTransport::canDelegateMipiSdkFlush(size_t bytes) const {
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_)return delegateMipiPresentationFlush(bytes);
#endif
#if !NICE_GFX_P4_MIPI_DELEGATE_SDK_FLUSH || NICE_GFX_P4_MIPI_SDK_FLUSH_AUDIT_ID != 0xb774170f || \
    ESP_IDF_VERSION != ESP_IDF_VERSION_VAL(5,5,5) || !CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_ENABLE || \
    !defined(CONFIG_ESP_REV_MIN_FULL) || CONFIG_ESP_REV_MIN_FULL < 300
  (void)bytes;return false;
#else
  // Only this exact audited SDK's DIRECT DPI allocation path is admitted.
  // Unknown transports, SDKs, flags, geometry, pointer provenance or ranges
  // continue through the original checked full C2M call in commitFrame.
  if(interface_!=VideoInterface::MIPI||!mipiDirectSdkDpi_||!mipiAtomic_||!mipiAtomicOpen_||
     mipiAtomicPending_||pending_||mipiFullInheritPending_||mipiPartialInheritPending_||
     mipiAtomicFailure_!=Status::Ok||mipiFront_>1||!panel_||swapRedBlue_||
     !mipiFrames_[0]||!mipiFrames_[1]||mipiFrames_[0]==mipiFrames_[1]||
     rgbFramebuffer_!=mipiFrames_[mipiFront_^1]||
     xPortInIsrContext()||!xPortCanYield()||mipiInitCore_!=1||
     xPortGetCoreID()!=1||xTaskGetCoreID(nullptr)!=1)return false;
  const size_t width=rasterWidth(),height=rasterHeight();
  if(!width||!height||width>INT16_MAX||height>INT16_MAX||width>SIZE_MAX/height/2||
     bytes!=width*height*2||!bytes)return false;
  const uintptr_t address=reinterpret_cast<uintptr_t>(rgbFramebuffer_);
  if(address>UINT32_MAX||bytes>UINT32_MAX||address>UINT32_MAX-bytes||
     address%64||bytes%64||!esp_ptr_external_ram(rgbFramebuffer_)||
     !esp_ptr_external_ram(reinterpret_cast<const void*>(address+bytes-1)))return false;
  uint32_t level=0,id=0;
  // The very same read-only range validation called by esp_cache_msync.
  // Fixed flags are C2M|UNALIGNED (6), data type; all other esp_err branches
  // are excluded. HAL writeback failure is an SDK assert, not a lost error.
  return cache_hal_vaddr_to_cache_level_id(uint32_t(address),uint32_t(bytes),&level,&id)&&
    level==2&&cache_hal_get_cache_line_size(level,CACHE_TYPE_DATA)==64;
#endif
}

Status VideoTransport::commitMipiAtomicFrame(){
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_)return commitMipiPresentationLegacy();
#endif
#if NICE_GFX_NATIVE_STAGE_ENGINE_AVAILABLE
  if(nativeStageEngine_&&nativeStageEngine_->pending)return failMipiAtomic(Status::IoError);
#endif
  if(mipiAtomicFailure_!=Status::Ok)return mipiAtomicFailure_;
  if(!mipiAtomicOpen_)return waitMipiAtomicFlip();
  Status s=wait();if(s!=Status::Ok)return failMipiAtomic(s);
  if(!panel_||!mipiFrameFence_||rgbFramebuffer_!=mipiFrames_[mipiFront_^1])
    return failMipiAtomic(Status::IoError);
  // Flush the complete inactive image, including mapped native painters, once.
  // Never issue draw_bitmap for individual tiles in the atomic path.
  const size_t bytes=rasterWidth()*rasterHeight()*sizeof(uint16_t);
  const int64_t validationStarted=copyStats_?esp_timer_get_time():0;
  const bool delegated=canDelegateMipiSdkFlush(bytes);
  if(copyStats_)copyStats_->sdkFlushValidationMicros+=uint64_t(esp_timer_get_time()-validationStarted);
  if(delegated){
    // Exact audited SDK own-FB full-frame draw below performs this same full
    // C2M synchronously BEFORE cur_fb_index can change. No inheritance/cache
    // fence is removed. All possible msync esp_err validations were checked.
  }else{
    const int64_t manualStarted=copyStats_?esp_timer_get_time():0;
    s=convert(esp_cache_msync(rgbFramebuffer_,bytes,
      ESP_CACHE_MSYNC_FLAG_DIR_C2M|ESP_CACHE_MSYNC_FLAG_UNALIGNED));
    if(copyStats_){++copyStats_->sdkFlushManualCalls;
      copyStats_->sdkFlushManualMicros+=uint64_t(esp_timer_get_time()-manualStarted);
#if NICE_GFX_P4_MIPI_DELEGATE_SDK_FLUSH
      ++copyStats_->sdkFlushValidationFallbacks;
#endif
    }
    if(s!=Status::Ok)return failMipiAtomic(s);
  }
  clearMipiDirty();mipiAtomicOpen_=mipiAtomicExplicit_=mipiFrame_=mipiBatch_=false;
  mipiAtomicPending_=true;
  const int pinnedCore=xTaskGetCoreID(nullptr);
  const int interruptCore=mipiInterruptCore_.load(std::memory_order_acquire);
  const esp_err_t submitted=esp_lcd_panel_draw_bitmap(panel_,0,0,
    int(rasterWidth()),int(rasterHeight()),rgbFramebuffer_);
  if(submitted!=ESP_OK)return failMipiAtomic(convert(submitted));
  if(delegated&&copyStats_){++copyStats_->sdkFlushDelegatedCommits;copyStats_->sdkFlushDelegatedBytes+=bytes;}
  // On the same CPU a task cannot run between the SDK ISR's cur_fb_index
  // snapshot and its frame-complete callback: an old-index callback must have
  // completed before draw_bitmap can return. Therefore the NEXT callback
  // after this baseline necessarily restarts DMA on our newly submitted FB.
  // Require a genuinely pinned producer, not merely equal before/after CPU
  // samples (an unpinned task could migrate out and back during submission).
  // Unknown/other-core IRQs retain the conservative two-completion proof.
  mipiFenceCompletions_=2;
#if !defined(NICE_GFX_P4_MIPI_FORCE_TWO_FRAME_FENCE) || !NICE_GFX_P4_MIPI_FORCE_TWO_FRAME_FENCE
  if(pinnedCore>=0&&pinnedCore==xPortGetCoreID()&&pinnedCore==xTaskGetCoreID(nullptr)&&
     interruptCore==pinnedCore&&mipiInterruptCore_.load(std::memory_order_acquire)==interruptCore)
    mipiFenceCompletions_=1;
#else
  (void)pinnedCore;(void)interruptCore;
#endif
  mipiFenceBaseline_=mipiFrameSequence_.load(std::memory_order_acquire);
  mipiStaleFull_=mipiDamageFull_;mipiStaleCount_=mipiDamageCount_;
  for(size_t i=0;i<mipiStaleCount_;++i)mipiStaleDamage_[i]=mipiDamage_[i];
  if(copyStats_)++copyStats_->frameCommits;
  const Status flipped=waitMipiAtomicFlip();if(flipped!=Status::Ok)return flipped;
  return startMipiInheritancePrefetch();
}

Status VideoTransport::flushMipiDirty(){
  if(!mipiDirty_||mipiDirtyTop_>=mipiDirtyBottom_)return result_;
  const bool batch=mipiBatch_,frame=mipiFrame_;
  mipiBatch_=mipiFrame_=false;
  if(copyStats_)++copyStats_->cacheBatches;
  // Merge nearby linear ranges, including the gap between adjacent narrow
  // scanlines. One cache command is cheaper than a command for each glyph or
  // diagonal-line pixel; C2M safely preserves clean pixels in those gaps.
  size_t first=0,last=0;
  const auto sync=[&](){
    if(first==last)return Status::Ok;
    const int64_t at=copyStats_?esp_timer_get_time():0;
    const auto s=convert(esp_cache_msync(rgbFramebuffer_+first,(last-first)*2,
      ESP_CACHE_MSYNC_FLAG_DIR_C2M|ESP_CACHE_MSYNC_FLAG_UNALIGNED));
    if(copyStats_){++copyStats_->cacheSyncCalls;copyStats_->cacheSyncBytes+=(last-first)*2;copyStats_->framebufferSyncMicros+=uint64_t(esp_timer_get_time()-at);}
    return s;
  };
  for(size_t row=mipiDirtyTop_;row<mipiDirtyBottom_;++row){
    const auto span=mipiDirty_[row];if(span.left>=span.right)continue;
    const size_t begin=row*rasterWidth()+span.left,end=row*rasterWidth()+span.right;
    if(first!=last&&begin>last+512){
      const Status s=sync();if(s!=Status::Ok){mipiBatch_=batch;mipiFrame_=frame;return result_=s;}
      first=last=0;
    }
    if(first==last)first=begin;
    last=end;
  }
  const Status s=sync();if(s!=Status::Ok){mipiBatch_=batch;mipiFrame_=frame;return result_=s;}
  for(size_t row=mipiDirtyTop_;row<mipiDirtyBottom_;++row)mipiDirty_[row]={uint16_t(rasterWidth()),0};
  mipiDirtyTop_=rasterHeight();mipiDirtyBottom_=0;
  mipiBatch_=batch;mipiFrame_=frame;
  return result_=Status::Ok;
}
#endif

#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32P4)
Status VideoTransport::synchronizeFramebuffer(int x, int y, size_t width, size_t height, bool cpuWritten) {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  if (!cpuWritten || frameAtomic_) return result_ = Status::Ok;
  // Let the installed SDK preserve its bounce-buffer/cache coherency policy.
  // Passing its own framebuffer bypasses its CPU pixel copy. This remains a
  // source-completion fence, never a VSYNC wait or a framebuffer flip.
  while (xSemaphoreTake(completed_, 0) == pdTRUE) {}
  pending_ = true;
  result_ = convert(esp_lcd_panel_draw_bitmap(panel_, x, y, x+int(width), y+int(height), rgbFramebuffer_));
  if (result_ != Status::Ok) { pending_ = false; return result_; }
  return wait();
#else
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  // Preserve RGB's SDK submission/callback behavior on P4. The mapped MIPI
  // surface is already the sole scanout buffer and needs only dirty writeback.
  if(interface_ != VideoInterface::MIPI){
    while(xSemaphoreTake(completed_,0)==pdTRUE){}
    pending_=true;
    result_=convert(esp_lcd_panel_draw_bitmap(panel_,x,y,x+int(width),y+int(height),rgbFramebuffer_));
    if(result_!=Status::Ok){pending_=false;return result_;}
    return wait();
  }
#endif
  // CPU stores and blocking PPA have consumed their sources. Cache maintenance
  // does not publish an atomic back buffer: only commitFrame changes DMA links.
  if (!cpuWritten) return result_ = Status::Ok;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(mipiDirty_&&(mipiBatch_||mipiFrame_)){
    const size_t top=size_t(y),bottom=top+height;
    for(size_t row=top;row<bottom;++row){
      auto &span=mipiDirty_[row];
      if(size_t(x)<span.left)span.left=uint16_t(x);
      if(size_t(x)+width>span.right)span.right=uint16_t(size_t(x)+width);
    }
    if(top<mipiDirtyTop_)mipiDirtyTop_=top;
    if(bottom>mipiDirtyBottom_)mipiDirtyBottom_=bottom;
    if(copyStats_)++copyStats_->cacheCoalescedWrites;
    return result_=Status::Ok;
  }
#endif
#if !defined(CONFIG_IDF_TARGET_ESP32P4)
  if (!esp_cache_get_line_size_by_addr(rgbFramebuffer_)) return result_ = Status::Ok;
#endif
  struct SyncTimer {
    VideoCopyStats *stats; int64_t started;
    ~SyncTimer(){if(stats)stats->framebufferSyncMicros+=uint64_t(esp_timer_get_time()-started);}
  } syncTimer{copyStats_,copyStats_?esp_timer_get_time():0};
  auto *first = rgbFramebuffer_ + size_t(y) * rasterWidth() + size_t(x);
  const int flags = ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED;
  const auto sync=[&](void *address,size_t bytes){
    if(copyStats_){++copyStats_->cacheSyncCalls;copyStats_->cacheSyncBytes+=bytes;}
    return convert(esp_cache_msync(address,bytes,flags));
  };
  // A narrow tile must not write back every LCD scanline it intersects.
  // C2M preserves neighboring pixels sharing either boundary cache line.
  if (height == 1 || width * 2 >= rasterWidth())
    return result_ = sync(first, ((height - 1) * rasterWidth() + width) * 2);
  for (size_t row = 0; row < height; ++row) {
    const Status s = sync(first + row * rasterWidth(), width * 2);
    if (s != Status::Ok) return result_ = s;
  }
  return result_ = Status::Ok;
#endif
}

Status VideoTransport::drawFillRect(int x, int y, size_t width, size_t height, uint16_t color) {
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_&&mipiTypedDrawing_)return failMipiAtomic(Status::IoError);
#endif
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(!mipiPresentationBound_)
#endif
  if(mipiAtomic_)retainedTags_.written(mipiFront_^1,{x,y,int32_t(width),int32_t(height)});
#endif
  if (!rgbFramebuffer_) return Status::NotInitialized;
  const Status s = wait();
  if (s != Status::Ok) return s;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(mipiAtomic_){const Status acquired=beginMipiAtomicFrame(false);if(acquired!=Status::Ok)return acquired;
    noteMipiDamage(x,y,width,height);}
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_&&(!validMipiDrawing(mipiDrawingLease_)||width>INT32_MAX||height>INT32_MAX||
    !mipiDrawingCovers({x,y,int32_t(width),int32_t(height)})))return failMipiAtomic(Status::IoError);
#endif
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
  const Status acquired=ensureAtomicFrame();if(acquired!=Status::Ok)return acquired;
#endif
  if (swapRedBlue_) color = uint16_t((color & 0x07E0) | ((color & 0x001F) << 11) | (color >> 11));
#if SOC_PPA_SUPPORTED && (defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32P4))
  if (width*height>=4096 && fillWithPpa(x, y, width, height, color)) return result_ = Status::Ok;
#endif
  const auto fillSpan = [color](uint16_t *out, size_t count) {
#if defined(__GNUC__)
    // An aligned word store writes two PSRAM pixels. Avoid both staging and
    // the peripheral driver's halfword copy for every individual scanline.
    typedef uint32_t FillWord __attribute__((__may_alias__));
    if ((reinterpret_cast<uintptr_t>(out) & 3) && count) { *out++ = color; --count; }
    auto *words = reinterpret_cast<FillWord *>(out);
    const uint32_t pair = uint32_t(color) | (uint32_t(color) << 16);
    while (count >= 8) {
      words[0] = pair; words[1] = pair; words[2] = pair; words[3] = pair;
      words += 4; count -= 8;
    }
    while (count >= 2) { *words++ = pair; count -= 2; }
    out = reinterpret_cast<uint16_t *>(words);
#endif
    while (count--) *out++ = color;
  };
  uint16_t *out = rgbFramebuffer_ + size_t(y) * rasterWidth() + size_t(x);
  if (width == rasterWidth()) fillSpan(out, width * height);
  else for (size_t row = 0; row < height; ++row) fillSpan(out + row * rasterWidth(), width);
#if SOC_LCD_RGB_SUPPORTED
  if (copyStats_) copyStats_->cpuPixels += uint64_t(width) * height;
#endif
  return synchronizeFramebuffer(x, y, width, height, true);
}
#endif

Status VideoTransport::drawPixelView(int x, int y, PixelView view) {
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_&&mipiTypedDrawing_){
    const DirtyRect area(x,y,int32_t(view.width),int32_t(view.height));
    if(!validMipiDrawing(mipiDrawingLease_)||!view.valid()||view.width>INT32_MAX||view.height>INT32_MAX||
      !mipiDrawingCovers(area)||view.order!=PixelOrder::Native||view.stride!=rasterWidth()||
      view.data!=reinterpret_cast<const uint8_t*>(rgbFramebuffer_+size_t(y)*rasterWidth()+size_t(x)))
      return failMipiAtomic(Status::IoError);
    const Status copied=copyOnlyMipiWait();return copied==Status::Ok?synchronizeFramebuffer(x,y,view.width,view.height,true):copied;
  }
#endif
  if (!rgbFramebuffer_) return Status::NotInitialized;
  const Status s = wait();
  if (s != Status::Ok) return s;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(mipiAtomic_){const Status acquired=beginMipiAtomicFrame(false);if(acquired!=Status::Ok)return acquired;
    noteMipiDamage(x,y,view.width,view.height);}
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(mipiPresentationBound_&&(!validMipiDrawing(mipiDrawingLease_)||view.width>INT32_MAX||view.height>INT32_MAX||
    !mipiDrawingCovers({x,y,int32_t(view.width),int32_t(view.height)})))return failMipiAtomic(Status::IoError);
#endif
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
  const Status acquired=ensureAtomicFrame();if(acquired!=Status::Ok)return acquired;
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32P4)
  // A producer can render directly into the mapped native scanout buffer.
  // Submitting that exact rectangle commits its cache lines, without another
  // SRM transaction or overlapping memcpy. Keep the ordinary present() path.
  if (!swapRedBlue_ && view.order == PixelOrder::Native && view.stride == rasterWidth() &&
      view.data == reinterpret_cast<const uint8_t *>(rgbFramebuffer_ + size_t(y) * rasterWidth() + size_t(x))) {
#if SOC_LCD_RGB_SUPPORTED
    if (copyStats_) { ++copyStats_->mappedTransfers; copyStats_->mappedPixels += uint64_t(view.width) * view.height; }
#endif
    return synchronizeFramebuffer(x, y, view.width, view.height, true);
  }
#endif
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(!mipiPresentationBound_)
#endif
  if(mipiAtomic_)retainedTags_.written(mipiFront_^1,{x,y,int32_t(view.width),int32_t(view.height)});
#endif
  bool copied=false;
  bool dmaCopied=false;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  copied=dmaCopied=copyWithGdma(x,y,view);
#endif
#if SOC_PPA_SUPPORTED && (defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32P4))
  if(!copied)copied=dmaCopied=copyWithPpa(x,y,view);
#endif
#if SOC_LCD_RGB_SUPPORTED
  if(!copied&&copyStats_)copyStats_->cpuPixels+=uint64_t(view.width)*view.height;
#endif
  if(!copied && interface_==VideoInterface::MIPI && view.order==PixelOrder::Native &&
     !swapRedBlue_ && x==0 && view.width==rasterWidth() && view.stride==view.width){
    memcpy(rgbFramebuffer_+size_t(y)*rasterWidth(),view.data,view.width*view.height*2);
    copied=true;
  }
  if(!copied)for (size_t row = 0; row < view.height; ++row) {
    uint16_t *out = rgbFramebuffer_ + (size_t(y) + row) * rasterWidth() + size_t(x);
    const uint8_t *in = view.data + row * view.stride * 2;
    if (view.order == PixelOrder::Native && !swapRedBlue_) memcpy(out, in, view.width * 2);
    else {
      size_t i=0;
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      // PSRAM halfword stores are expensive on S31. Stream aligned words,
      // converting two wire pixels per store without a second staging copy.
      typedef uint32_t CopyWord __attribute__((__may_alias__));
      if (!(reinterpret_cast<uintptr_t>(out)&3) && !(reinterpret_cast<uintptr_t>(in)&3)) {
        auto *dst=reinterpret_cast<CopyWord *>(out);
        const auto *src=reinterpret_cast<const CopyWord *>(in);
        const auto convertPair=[&](uint32_t p){
          if(view.order==PixelOrder::Wire)p=((p&0x00FF00FFu)<<8)|((p>>8)&0x00FF00FFu);
          if(swapRedBlue_)p=(p&0x07E007E0u)|((p&0x001F001Fu)<<11)|((p>>11)&0x001F001Fu);
          return p;
        };
        for(;i+7<view.width;i+=8,src+=4,dst+=4){
          const uint32_t a=src[0],b=src[1],c=src[2],d=src[3];
          dst[0]=convertPair(a);dst[1]=convertPair(b);dst[2]=convertPair(c);dst[3]=convertPair(d);
        }
        for(;i+1<view.width;i+=2)*dst++=convertPair(*src++);
      }
#endif
      for (; i < view.width; ++i) {
      uint16_t c;
      if (view.order == PixelOrder::Wire) c = uint16_t(uint16_t(in[2*i]) << 8 | in[2*i+1]);
      else { const uint8_t *p=in+2*i; c=uint16_t(p[0] | uint16_t(p[1]) << 8); }
      if (swapRedBlue_) c=uint16_t((c & 0x07E0) | ((c & 0x001F) << 11) | (c >> 11));
      out[i]=c;
      }
    }
  }
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32P4)
  // Blocking PPA already synchronized its output; CPU writes need only the
  // dirty pixels written back, rather than every complete LCD scanline.
  return synchronizeFramebuffer(x, y, view.width, view.height,
    !dmaCopied);
#else
  // The IDF recognizes its own framebuffer and skips its pixel-by-pixel copy.
  // This still performs the required cache writeback without bounce buffers,
  // and preserves source-completion/on-demand semantics on IDF 5 and IDF 6.
  while (xSemaphoreTake(completed_, 0) == pdTRUE) {}
  pending_=true;
  result_=convert(esp_lcd_panel_draw_bitmap(panel_, x, y, x+int(view.width), y+int(view.height), rgbFramebuffer_));
  if (result_ != Status::Ok) { pending_=false; return result_; }
  return wait();
#endif
}

#if SOC_PPA_SUPPORTED && (defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32P4))
bool VideoTransport::fillWithPpa(int x,int y,size_t width,size_t height,uint16_t color){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(mipiAtomic_&&(!mipiAtomicOpen_||mipiAtomicPending_||mipiAtomicFailure_!=Status::Ok))return false;
#endif
  // Keep tiny rectangles on the CPU to avoid DMA submission/wakeup overhead.
  // Card-size erases (for example 232x132) are frequent during cached layer
  // motion. At least 8 KiB of output uses the blocking DMA path.
  // The hardware descriptor and fill block dimensions are 14-bit on S31.
  if(!ppaFill_||ppaFillRejected_||width*height<4096||rasterWidth()>0x3FFF||rasterHeight()>0x3FFF)return false;
  const size_t bytes=rasterWidth()*rasterHeight()*2;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  const size_t line=ppaCacheLine_;
#else
  const size_t line=esp_cache_get_line_size_by_addr(rgbFramebuffer_);
#endif
  if(!line||bytes%line||reinterpret_cast<uintptr_t>(rgbFramebuffer_)%line)return false;
  const size_t begin=(size_t(y)*rasterWidth()*2)&~(line-1);
  const size_t end=((size_t(y)+height)*rasterWidth()*2+line-1)&~(line-1);
  if(end>bytes)return false;
  auto *region=reinterpret_cast<uint8_t*>(rgbFramebuffer_)+begin;
  ppa_fill_oper_config_t operation={};
  operation.out.buffer=rgbFramebuffer_;operation.out.buffer_size=bytes;
  operation.out.pic_w=rasterWidth();operation.out.pic_h=rasterHeight();
  operation.out.block_offset_x=x;operation.out.block_offset_y=y;
  operation.out.fill_cm=PPA_FILL_COLOR_MODE_RGB565;
  operation.fill_block_w=width;operation.fill_block_h=height;
  // The fill register takes ARGB8888 channels, including for RGB565 output.
  // Truncation by the hardware recovers every original 5/6/5 component exactly.
  operation.fill_argb_color.a=255;
  operation.fill_argb_color.r=uint8_t((color>>11)<<3);
  operation.fill_argb_color.g=uint8_t(((color>>5)&63)<<2);
  operation.fill_argb_color.b=uint8_t((color&31)<<3);
  operation.mode=PPA_TRANS_MODE_BLOCKING;
  // Unlike SRM, the SDK fill path writes back and invalidates the complete
  // output rows before DMA, preserving dirty neighbors outside the rectangle.
  if(ppa_do_fill(ppaFill_,&operation)!=ESP_OK){if(copyStats_)++copyStats_->ppaFillFailures;return false;}
  // Scanout/bounce readers may have re-cached those rows while DMA was writing.
  if(esp_cache_msync(region,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_M2C)!=ESP_OK){if(copyStats_)++copyStats_->ppaFillFailures;return false;}
  const bool partial=x!=0||width!=rasterWidth();
  if(!ppaFillVerified_||(partial&&!ppaFillPartialVerified_)){
    bool exact=true;
    for(size_t row=0;row<height&&exact;++row)for(size_t col=0;col<width;++col)
      if(rgbFramebuffer_[(size_t(y)+row)*rasterWidth()+size_t(x)+col]!=color){exact=false;break;}
    if(!exact){
      ppaFillRejected_=true;
      if(copyStats_){++copyStats_->ppaFillVerificationFailures;copyStats_->ppaFillReady=false;}
      if(ppa_unregister_client(ppaFill_)==ESP_OK)ppaFill_=nullptr;
      return false;
    }
    ppaFillVerified_=true;if(partial)ppaFillPartialVerified_=true;
  }
  if(copyStats_){++copyStats_->ppaFillTransfers;copyStats_->ppaFillPixels+=uint64_t(width)*height;}
  return true;
}

bool VideoTransport::copyWithPpa(int x,int y,PixelView view){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(mipiAtomic_&&(!mipiAtomicOpen_||mipiAtomicPending_||mipiAtomicFailure_!=Status::Ok))return false;
#endif
  // S31 applies RGB swap before byte swap. With wire RGB565 input the two
  // operations do not commute; keep this combination on the exact CPU path.
  if(swapRedBlue_&&view.order==PixelOrder::Wire)return false;
  if(!ppaCopy_||view.width*view.height<256||view.stride>0x3FFF||view.height>0x3FFF)return false;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  // P4's setup cost exceeds CPU memcpy for small native rectangles. Byte
  // swapping benefits earlier. These cutoffs are measured on the JC4880.
  if(view.width*view.height<(view.order==PixelOrder::Wire?1024u:8192u))return false;
#endif
  const size_t bytes=rasterWidth()*rasterHeight()*2;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  const size_t line=ppaCacheLine_;
#else
  const size_t line=esp_cache_get_line_size_by_addr(rgbFramebuffer_);
#endif
  if(!line||bytes%line||reinterpret_cast<uintptr_t>(rgbFramebuffer_)%line)return false;
  const size_t begin=(size_t(y)*rasterWidth()*2)&~(line-1);
  const size_t end=((size_t(y)+view.height)*rasterWidth()*2+line-1)&~(line-1);
  if(end>bytes)return false;
  auto *region=reinterpret_cast<uint8_t*>(rgbFramebuffer_)+begin;
  // SRM's SDK implementation invalidates complete destination rows before
  // DMA. Preserve dirty neighbors for partial/cache-line-sharing writes. A
  // complete aligned row is overwritten, so its previous contents need no
  // redundant writeback (the common full-frame and full-width tile paths).
  const bool replacesCacheLines=x==0&&view.width==rasterWidth()&&
    (size_t(y)*rasterWidth()*2)%line==0&&(view.height*rasterWidth()*2)%line==0;
  if(!replacesCacheLines){
    const int64_t at=copyStats_?esp_timer_get_time():0;
    const auto sync=esp_cache_msync(region,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    if(copyStats_)copyStats_->ppaCopySyncMicros+=uint64_t(esp_timer_get_time()-at);
    if(sync!=ESP_OK)return false;
  }
  ppa_srm_oper_config_t operation={};
  operation.in.buffer=view.data;operation.in.pic_w=view.stride;operation.in.pic_h=view.height;
  operation.in.block_w=view.width;operation.in.block_h=view.height;operation.in.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
#if defined(CONFIG_IDF_TARGET_ESP32S31)
  if(frameAtomic_&&view.order==PixelOrder::Native&&view.stride==rasterWidth()){
    const uintptr_t source=reinterpret_cast<uintptr_t>(view.data);
    uint8_t buffers=2;
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
    if(framePipelined_)buffers=3;
#endif
    for(uint8_t i=0;i<buffers;++i){const uint16_t *base=rgbFrames_[i<2?i:0];
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
      if(i==2)base=pipelineThird_;
#endif
      const uintptr_t begin=reinterpret_cast<uintptr_t>(base);
      if(source<begin||source-begin>=bytes||(source&1))continue;
      const size_t offset=(source-begin)/2,sx=offset%rasterWidth(),sy=offset/rasterWidth();
      if(view.width<=rasterWidth()-sx&&view.height<=rasterHeight()-sy){
        // Give the SDK the complete allocation, not an interior pointer whose
        // declared pic_w*pic_h could run past the final cropped scanline.
        operation.in.buffer=base;operation.in.pic_w=rasterWidth();operation.in.pic_h=rasterHeight();
        operation.in.block_offset_x=sx;operation.in.block_offset_y=sy;
      }
      break;
    }
  }
#endif
  operation.out.buffer=rgbFramebuffer_;operation.out.buffer_size=bytes;
  operation.out.pic_w=rasterWidth();operation.out.pic_h=rasterHeight();
  operation.out.block_offset_x=x;operation.out.block_offset_y=y;operation.out.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
  operation.scale_x=operation.scale_y=1.0f;operation.byte_swap=view.order==PixelOrder::Wire;
  operation.rgb_swap=swapRedBlue_;operation.mode=PPA_TRANS_MODE_BLOCKING;
  const int64_t copyAt=copyStats_?esp_timer_get_time():0;
  const auto copied=ppa_do_scale_rotate_mirror(ppaCopy_,&operation);
  if(copyStats_)copyStats_->ppaCopyDriverMicros+=uint64_t(esp_timer_get_time()-copyAt);
  if(copied!=ESP_OK){if(copyStats_)++copyStats_->ppaFailures;return false;}
  // A bounce ISR could have cached these rows while DMA was writing them.
  const int64_t syncAt=copyStats_?esp_timer_get_time():0;
  const auto synced=esp_cache_msync(region,end-begin,ESP_CACHE_MSYNC_FLAG_DIR_M2C);
  if(copyStats_)copyStats_->ppaCopySyncMicros+=uint64_t(esp_timer_get_time()-syncAt);
  if(synced!=ESP_OK)return false;
  const bool partial=x!=0||view.width!=rasterWidth();
  if(!ppaVerified_||(partial&&!ppaPartialVerified_)){
    bool exact=true;
    for(size_t row=0;row<view.height&&exact;++row)for(size_t col=0;col<view.width;++col){
      const uint8_t *p=view.data+(row*view.stride+col)*2;
      uint16_t expected=view.order==PixelOrder::Wire?uint16_t(uint16_t(p[0])<<8|p[1]):uint16_t(p[0]|uint16_t(p[1])<<8);
      if(swapRedBlue_)expected=uint16_t((expected&0x07E0)|((expected&31)<<11)|(expected>>11));
      if(rgbFramebuffer_[(size_t(y)+row)*rasterWidth()+size_t(x)+col]!=expected){exact=false;break;}
    }
    if(!exact){
      if(copyStats_){++copyStats_->ppaVerificationFailures;copyStats_->ppaReady=false;}
      if(ppa_unregister_client(ppaCopy_)==ESP_OK)ppaCopy_=nullptr;
      return false;
    }
    ppaVerified_=true;if(partial)ppaPartialVerified_=true;
  }
  if(copyStats_){++copyStats_->ppaTransfers;copyStats_->ppaPixels+=uint64_t(view.width)*view.height;}
  return true;
}
#endif

Status VideoTransport::command(uint8_t c, const uint8_t *data, size_t bytes) {
  if (!rasterReady()) return Status::NotInitialized;
  if (bytes && !data) return Status::InvalidArgument;
  const Status s = wait();
  if (s != Status::Ok) return s;
  if (c == 0x2A || c == 0x2B || c == 0x2C)
    return result_ = FramebufferTransport::command(c, data, bytes);
  if (bytes) return result_ = Status::Unsupported;
  if (c == 0x28 || c == 0x29) return result_ = display(c == 0x29);
  if (c == 0x20 || c == 0x21) {
    const auto *profile = videoControllerProfile(controller_, interface_);
    if (profile && profile->noCommand)
      return result_ = convert(esp_lcd_panel_invert_color(panel_, c == 0x21));
    return result_ = videoInvert(controller_, interface_, c == 0x21, writeCommand, this);
  }
  return result_ = Status::Unsupported;
}
Status VideoTransport::display(bool on) {
  const auto *profile = videoControllerProfile(controller_, interface_);
  if (!profile) return Status::Unsupported;
  // An external DISP signal gates the RGB output independently of controller
  // DCS state. Use the native engine to drive its configured active polarity.
  if (interface_ == VideoInterface::RGB && hasDisplayPin_)
    return convert(esp_lcd_panel_disp_on_off(panel_, on));
  if (profile->noCommand) return Status::Unsupported;
  return videoDisplay(controller_, interface_, on, writeCommand, controllerDelay, this);
}
bool VideoTransport::validOutputPin(int pin) const { return GPIO_IS_VALID_OUTPUT_GPIO(pin); }
void VideoTransport::output(int pin, bool high) { esp32_platform::output(pin,high); }
void VideoTransport::delayMs(uint32_t ms) { esp32_platform::delayMillis(ms); }

#if SOC_LCD_RGB_SUPPORTED
RgbConfig::RgbConfig() {
  rgb.data_width = 16;
#if defined(ESP_IDF_VERSION_MAJOR) && ESP_IDF_VERSION_MAJOR >= 6
  rgb.in_color_format = rgb.out_color_format = LCD_COLOR_FMT_RGB565;
#else
  rgb.bits_per_pixel = 16;
#endif
  rgb.clk_src = LCD_CLK_SRC_DEFAULT;
  rgb.hsync_gpio_num = rgb.vsync_gpio_num = rgb.de_gpio_num = rgb.pclk_gpio_num = rgb.disp_gpio_num = decltype(rgb.disp_gpio_num)(-1);
  for (size_t i = 0; i < sizeof(rgb.data_gpio_nums) / sizeof(rgb.data_gpio_nums[0]); ++i)
    rgb.data_gpio_nums[i] = decltype(rgb.disp_gpio_num)(-1);
}
Status Esp32Rgb::begin() {
  if (config_.interruptCore < -1 || config_.interruptCore > 1) return Status::InvalidArgument;
  if (rasterReady()) return wait();
#if !CONFIG_FREERTOS_UNICORE
  if (config_.interruptCore >= 0 && config_.interruptCore != xPortGetCoreID()) {
    struct Request { Esp32Rgb *self; SemaphoreHandle_t done; Status result; } request{this,xSemaphoreCreateBinary(),Status::IoError};
    if (!request.done) return Status::NoMemory;
    const auto worker=[](void *context) {
      auto *r=static_cast<Request *>(context);
      r->result=r->self->beginConfigured();
      xSemaphoreGive(r->done);
      vTaskDelete(nullptr);
    };
    const BaseType_t created=xTaskCreatePinnedToCore(worker,"gfx-rgb-init",6144,&request,2,nullptr,config_.interruptCore);
    if (created!=pdPASS) {vSemaphoreDelete(request.done);return Status::NoMemory;}
    xSemaphoreTake(request.done,portMAX_DELAY);
    vSemaphoreDelete(request.done);return request.result;
  }
#else
  if (config_.interruptCore > 0) return Status::Unsupported;
#endif
  return beginConfigured();
}
Status Esp32Rgb::beginConfigured() {
  if (rasterReady()) return wait();
  const Status cleaned = shutdown();
  if (cleaned != Status::Ok) return cleaned;
  const auto *profile = videoControllerProfile(config_.controller, VideoInterface::RGB);
  if (!profile || !profile->supportsRgb) return fail(Status::Unsupported);
#if defined(ESP_IDF_VERSION_MAJOR) && ESP_IDF_VERSION_MAJOR >= 6
  // IDF 6 replaces bits_per_pixel with input/output color formats. Zero output
  // means the SDK uses the input format; other outputs require conversion.
  if (config_.rgb.in_color_format != LCD_COLOR_FMT_RGB565 ||
      (config_.rgb.out_color_format && config_.rgb.out_color_format != LCD_COLOR_FMT_RGB565))
    return fail(Status::Unsupported);
#else
  const size_t bpp = config_.rgb.bits_per_pixel ? config_.rgb.bits_per_pixel : config_.rgb.data_width;
  if (bpp != 16) return fail(Status::Unsupported);
#endif
  if (config_.panel.bits_per_pixel != 16 || config_.rgb.flags.no_fb ||
      !(profile->rgbBitsMask & (1UL << 16))) return fail(Status::Unsupported);
  if (config_.rgb.data_width != 8 && config_.rgb.data_width != 16) return fail(Status::Unsupported);
  if (config_.reset < -1 || (config_.reset >= 0 && !validOutputPin(config_.reset)) ||
      !config_.rgb.timings.h_res || !config_.rgb.timings.v_res ||
      config_.rgb.timings.h_res > INT16_MAX || config_.rgb.timings.v_res > INT16_MAX || !config_.rgb.timings.pclk_hz ||
      (config_.panel.rgb_ele_order != LCD_RGB_ELEMENT_ORDER_RGB && config_.panel.rgb_ele_order != LCD_RGB_ELEMENT_ORDER_BGR) ||
      (!config_.init.commands && config_.init.count)) return fail(Status::InvalidArgument);
  const int controlPins[] = {config_.rgb.hsync_gpio_num, config_.rgb.vsync_gpio_num,
    config_.rgb.de_gpio_num, config_.rgb.pclk_gpio_num, config_.rgb.disp_gpio_num};
  for (size_t i = 0; i < sizeof(controlPins) / sizeof(controlPins[0]); ++i)
    if (config_.reset >= 0 && config_.reset == controlPins[i]) return fail(Status::InvalidArgument);
  for (size_t i = 0; i < config_.rgb.data_width; ++i)
    if (config_.reset >= 0 && config_.reset == config_.rgb.data_gpio_nums[i]) return fail(Status::InvalidArgument);
  if (config_.commandWrite && config_.createIo) return fail(Status::InvalidArgument);
  commandWrite_ = config_.commandWrite; commandContext_ = config_.commandContext;
  if (!profile->noCommand && !commandWrite_) {
    if (!config_.createIo) return fail(Status::InvalidArgument);
    const int commandPins[] = {config_.io3wire.line_config.cs_gpio_num,
      config_.io3wire.line_config.scl_gpio_num, config_.io3wire.line_config.sda_gpio_num};
    for (size_t i = 0; i < 3; ++i) {
      if (commandPins[i] < 0 && i == 0) continue;
      if (commandPins[i] == config_.reset) return fail(Status::InvalidArgument);
      for (size_t j = 0; j < sizeof(controlPins) / sizeof(controlPins[0]); ++j)
        if (commandPins[i] >= 0 && commandPins[i] == controlPins[j]) return fail(Status::InvalidArgument);
      for (size_t j = 0; j < config_.rgb.data_width; ++j)
        if (commandPins[i] >= 0 && commandPins[i] == config_.rgb.data_gpio_nums[j]) return fail(Status::InvalidArgument);
    }
    const Status prepared = prepareCommandSpi(config_.io3wire);
    if (prepared != Status::Ok) return fail(prepared);
  }
  esp_lcd_rgb_panel_config_t rgb=config_.rgb;
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
  if(config_.frameAtomic){
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    if(rgb.flags.refresh_on_demand||!rgb.bounce_buffer_size_px)return fail(Status::Unsupported);
#else
    if(rgb.flags.refresh_on_demand||rgb.bounce_buffer_size_px)return fail(Status::Unsupported);
#endif
    rgb.num_fbs=2;rgb.flags.double_fb=false;
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
    if(ESP_RGB_LCD_PANEL_MAX_FB_NUM>=3)rgb.num_fbs=3;
#endif
  }
#else
  if(config_.frameAtomic)return fail(Status::Unsupported);
#endif
  esp_lcd_panel_handle_t created = nullptr;
  esp_err_t error = esp_lcd_new_rgb_panel(&rgb, &created);
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
  // Allocation failure is the only safe automatic downgrade. Other driver
  // failures retain their original error and must not hide ownership faults.
  if(error==ESP_ERR_NO_MEM&&rgb.num_fbs==3){rgb.num_fbs=2;created=nullptr;error=esp_lcd_new_rgb_panel(&rgb,&created);}
#endif
  if (error != ESP_OK) return fail(error);
  if (!created) return fail(Status::Busy);
  panel_ = created; refreshOnDemand_ = config_.rgb.flags.refresh_on_demand;
#if defined(CONFIG_IDF_TARGET_ESP32S31) || defined(CONFIG_IDF_TARGET_ESP32S3)
  if(config_.frameAtomic){
    frameCompleted_=xSemaphoreCreateBinary();if(!frameCompleted_)return fail(ESP_ERR_NO_MEM);
    void *first=nullptr,*second=nullptr,*third=nullptr;
    const auto acquired=rgb.num_fbs==3?esp_lcd_rgb_panel_get_frame_buffer(panel_,3,&first,&second,&third):
      esp_lcd_rgb_panel_get_frame_buffer(panel_,2,&first,&second);
    if(acquired!=ESP_OK)return fail(acquired);
    if(!first||!second||first==second)return fail(Status::InvalidArgument);
    rgbFrames_[0]=static_cast<uint16_t*>(first);rgbFrames_[1]=static_cast<uint16_t*>(second);
    frontIndex_=0;rgbFramebuffer_=rgbFrames_[1];frameAtomic_=true;staleFull_=true;staleCount_=0;
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
    if(rgb.num_fbs==3){
      if(!third||third==first||third==second)return fail(Status::InvalidArgument);
      pipelineThird_=static_cast<uint16_t*>(third);framePipelined_=true;
    }
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    atomicCore_=xPortGetCoreID();
#endif
  }
#endif
  const Status initialized = initializeHardware(config_.controller, config_.panel, config_.init, config_.reset,
    config_.rgb.timings.h_res, config_.rgb.timings.v_res, 0, config_.displayOnInit,
    config_.rgb.disp_gpio_num >= 0);
  // Keep multi-buffer configurations on the established IDF copy path.
  if (initialized == Status::Ok && !config_.frameAtomic && config_.rgb.num_fbs <= 1 && !config_.rgb.flags.double_fb) {
    void *frame = nullptr;
    if (esp_lcd_rgb_panel_get_frame_buffer(panel_, 1, &frame) == ESP_OK)
      rgbFramebuffer_ = static_cast<uint16_t *>(frame);
  }
  copyStats_=config_.copyStats;
  if(copyStats_){
    *copyStats_=VideoCopyStats{};copyStats_->framebufferCount=uint8_t(rgb.num_fbs?rgb.num_fbs:1);
#if NICE_GFX_S31_PIPELINED_FRAMEBUFFERS_AVAILABLE
    copyStats_->pipelinedFramebuffers=framePipelined_;
#endif
    copyStats_->presentationContext=this;
    copyStats_->presentationSequenceReader=+[](const void *context){return static_cast<const VideoTransport*>(context)->presentationSequence();};
    copyStats_->presentationTimestampReader=+[](const void *context,uint32_t sequence,uint32_t &micros){return static_cast<const VideoTransport*>(context)->presentationTimestamp(sequence,micros);};
  }
  scanStats_=config_.scanStats;if(scanStats_)scanStats_->reset();
#if SOC_PPA_SUPPORTED && defined(CONFIG_IDF_TARGET_ESP32S31)
  if(initialized==Status::Ok&&rgbFramebuffer_&&config_.usePpa){
    ppa_client_config_t client={};client.oper_type=PPA_OPERATION_SRM;client.max_pending_trans_num=1;
    if(ppa_register_client(&client,&ppaCopy_)!=ESP_OK)ppaCopy_=nullptr;
    if(copyStats_)copyStats_->ppaReady=ppaCopy_!=nullptr;
    client.oper_type=PPA_OPERATION_FILL;
    if(ppa_register_client(&client,&ppaFill_)!=ESP_OK)ppaFill_=nullptr;
    if(copyStats_)copyStats_->ppaFillReady=ppaFill_!=nullptr;
  }
#endif
  return initialized;
}
Status Esp32Rgb::setPixelClockHz(uint32_t hz){
  if(!panel_||!rasterReady())return Status::NotInitialized;
  if(hz<100000||hz>80000000)return Status::InvalidArgument;
  const Status s=convert(esp_lcd_rgb_panel_set_pclk(panel_,hz));
  if(s==Status::Ok)config_.rgb.timings.pclk_hz=hz;
  return s;
}
TransportInfo Esp32Rgb::info() const {
  TransportInfo result;
  result.frequency = config_.rgb.timings.pclk_hz; result.clock = config_.rgb.pclk_gpio_num;
  result.lanes = uint8_t(config_.rgb.data_width);
  for (size_t i = 0; i < 16 && i < config_.rgb.data_width; ++i) result.data[i] = config_.rgb.data_gpio_nums[i];
  return result;
}
#endif

#if SOC_MIPI_DSI_SUPPORTED
MipiConfig::MipiConfig() {
  dbi.lcd_cmd_bits = dbi.lcd_param_bits = 8;
#if !defined(ESP_COLOR_FOURCC_RGB16)
  dpi.pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565;
#endif
  dpi.in_color_format = dpi.out_color_format = LCD_COLOR_FMT_RGB565;
  dpi.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;
}
Status Esp32Mipi::begin() {
  if (rasterReady()) return wait();
  const Status cleaned = shutdown();
  if (cleaned != Status::Ok) return cleaned;
  const auto *profile = videoControllerProfile(config_.controller, VideoInterface::MIPI);
  if (!profile || !profile->supportsMipi) return fail(Status::Unsupported);
  if (config_.panel.rgb_ele_order == LCD_RGB_ELEMENT_ORDER_BGR && !profile->colorOrderSupported)
    return fail(Status::Unsupported);
  // ST77922 has one physical data pair. Keep its explicit-one-lane contract;
  // SDK zero would otherwise select two lanes, contrary to that hardware.
  if (config_.controller == VideoController::ST77922 && config_.dsi.num_data_lanes != 1)
    return fail(Status::InvalidArgument);
  const unsigned lanes = config_.dsi.num_data_lanes ? config_.dsi.num_data_lanes : profile->defaultLanes;
  if (config_.reset < -1 || (config_.reset >= 0 && !validOutputPin(config_.reset)) ||
      lanes > 2 || !(profile->laneMask & (1U << lanes)) ||
      config_.dbi.lcd_cmd_bits != 8 || config_.dbi.lcd_param_bits != 8 ||
      (config_.panel.rgb_ele_order != LCD_RGB_ELEMENT_ORDER_RGB && config_.panel.rgb_ele_order != LCD_RGB_ELEMENT_ORDER_BGR) ||
      (!config_.init.commands && config_.init.count)) return fail(Status::InvalidArgument);
  if (config_.dpi.in_color_format && config_.dpi.in_color_format != LCD_COLOR_FMT_RGB565)
    return fail(Status::Unsupported);
#if defined(ESP_COLOR_FOURCC_RGB16)
  // The FourCC SDK removed pixel_format and defaults an unset input to RGB888.
  // This transport's framebuffer is RGB565, including an unset input format.
  config_.dpi.in_color_format = LCD_COLOR_FMT_RGB565;
#else
  if (!config_.dpi.in_color_format && config_.dpi.pixel_format != LCD_COLOR_PIXEL_FORMAT_RGB565)
    return fail(Status::Unsupported);
#endif
  const auto output = config_.dpi.out_color_format ? config_.dpi.out_color_format : LCD_COLOR_FMT_RGB565;
  unsigned bits;
  switch (output) {
    case LCD_COLOR_FMT_RGB565: bits = 16; break;
#if !defined(ESP_COLOR_FOURCC_RGB16)
    case LCD_COLOR_FMT_RGB666: bits = 18; break;
#endif
    case LCD_COLOR_FMT_RGB888: bits = 24; break;
    default: return fail(Status::Unsupported);
  }
  if (config_.panel.bits_per_pixel != bits) return fail(Status::InvalidArgument);
  if(config_.frameAtomic){
#if !defined(CONFIG_IDF_TARGET_ESP32P4) || ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5,5,5)
    // Older refresh/VSYNC callbacks do not provide the required DMA ownership
    // semantics. An atomic request must never fall back to the visible buffer.
    return fail(Status::Unsupported);
#else
    if(bits!=16||config_.panel.rgb_ele_order!=LCD_RGB_ELEMENT_ORDER_RGB)
      return fail(Status::Unsupported);
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
    const bool triple=config_.dpi.video_timing.h_size%32==0&&
      xPortGetCoreID()==1&&xTaskGetCoreID(nullptr)==1;
    config_.dpi.num_fbs=triple?3:2;
    config_.useFramebuffer=true;
#else
    config_.dpi.num_fbs=2;config_.useFramebuffer=true;
#endif
#endif
  }
  if (!(profile->mipiBitsMask & (1UL << bits))) return fail(Status::Unsupported);
#if !defined(CONFIG_ESP_REV_MIN_FULL) || CONFIG_ESP_REV_MIN_FULL < 300
  if (bits != 16) return fail(Status::Unsupported);
#endif
  if (!config_.dpi.video_timing.h_size || !config_.dpi.video_timing.v_size ||
      config_.dpi.video_timing.h_size > INT16_MAX || config_.dpi.video_timing.v_size > INT16_MAX ||
      !(config_.dpi.dpi_clock_freq_mhz > 0 && config_.dpi.dpi_clock_freq_mhz < 1000000.0f) ||
      !(config_.dsi.lane_bit_rate_mbps > 0 && config_.dsi.lane_bit_rate_mbps < 1000000.0f))
    return fail(Status::InvalidArgument);
  config_.dsi.num_data_lanes = lanes;
  esp_err_t error;
  if (config_.acquireLdo) {
    esp_ldo_channel_handle_t handle = nullptr;
    error = esp_ldo_acquire_channel(&config_.ldo, &handle);
    if (error != ESP_OK) return fail(error);
    ldo_ = handle;
  }
  esp_lcd_dsi_bus_handle_t bus = nullptr;
  error = esp_lcd_new_dsi_bus(&config_.dsi, &bus);
  if (error != ESP_OK) return fail(error);
  dsiBus_ = bus;
  esp_lcd_panel_io_handle_t createdIo = nullptr;
  error = esp_lcd_new_panel_io_dbi(bus, &config_.dbi, &createdIo);
  if (error != ESP_OK) return fail(error);
  io_ = createdIo;
  esp_lcd_panel_handle_t created = nullptr;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  mipiInitCore_=xPortGetCoreID();
#endif
  error = esp_lcd_new_panel_dpi(bus, &config_.dpi, &created);
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  // Only constructor NoMemory can retry. No DPI init or user callback has run.
  if(error==ESP_ERR_NO_MEM&&config_.dpi.num_fbs==3){
    config_.dpi.num_fbs=2;created=nullptr;
    error=esp_lcd_new_panel_dpi(bus,&config_.dpi,&created);
  }
#endif
  if (error != ESP_OK) return fail(error);
  if (!created) return fail(Status::Busy);
  panel_ = created;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  mipiDirectSdkDpi_=true; // Created above by esp_lcd_new_panel_dpi, never an external/vendor panel.
#endif
#if SOC_LCD_RGB_SUPPORTED
  copyStats_=config_.copyStats;scanStats_=config_.scanStats;lastVsync_=0;
  if(copyStats_)*copyStats_=VideoCopyStats{};
  if(scanStats_)scanStats_->reset();
#endif
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(config_.frameAtomic){
    void *front=nullptr,*back=nullptr;
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
    void *third=nullptr;
    error=config_.dpi.num_fbs==3?esp_lcd_dpi_panel_get_frame_buffer(panel_,3,&front,&back,&third,static_cast<void**>(nullptr)):
      esp_lcd_dpi_panel_get_frame_buffer(panel_,2,&front,&back,static_cast<void**>(nullptr));
    mipiSdkFramebufferCount_=uint8_t(config_.dpi.num_fbs);
    if(config_.dpi.num_fbs==3&&(!third||third==front||third==back))return fail(Status::InvalidArgument);
    mipiFrames_[2]=static_cast<uint16_t*>(third);
#else
    error=esp_lcd_dpi_panel_get_frame_buffer(panel_,2,&front,&back);
#endif
    if(error!=ESP_OK)return fail(error);
    if(!front||!back||front==back)return fail(Status::InvalidArgument);
    mipiFrameFence_=xSemaphoreCreateBinary();if(!mipiFrameFence_)return fail(Status::NoMemory);
    mipiFrames_[0]=static_cast<uint16_t*>(front);mipiFrames_[1]=static_cast<uint16_t*>(back);
    // The IDF panel starts scanout from its fb0. fb1 is our first draw target.
    resetRetainedRepaintCertificates();mipiFront_=0;rgbFramebuffer_=mipiFrames_[1];mipiAtomic_=true;
    mipiAtomicOpen_=mipiAtomicExplicit_=mipiAtomicPending_=false;
    mipiAtomicFailure_=Status::Ok;mipiFrameSequence_.store(0,std::memory_order_relaxed);
#if SOC_LCD_RGB_SUPPORTED
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
    if(copyStats_)copyStats_->framebufferCount=mipiSdkFramebufferCount_;
#else
    if(copyStats_)copyStats_->framebufferCount=2;
#endif
#endif
  }
#endif
  const Status initialized = initializeHardware(config_.controller, config_.panel, config_.init, config_.reset,
    config_.dpi.video_timing.h_size, config_.dpi.video_timing.v_size, lanes,
    config_.displayOnInit, false);
  if(initialized==Status::Ok && config_.useFramebuffer && bits==16 && config_.dpi.num_fbs<=1){
    void *frame=nullptr;
    if(esp_lcd_dpi_panel_get_frame_buffer(panel_,1,&frame)==ESP_OK)
      rgbFramebuffer_=static_cast<uint16_t*>(frame);
  }
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(initialized==Status::Ok&&rgbFramebuffer_){
#if NICE_GFX_P4_MIPI_NO_PPA_FULL_INHERIT
    mipiPpaConfigured_=config_.usePpa;
#endif
    if(esp_cache_get_alignment(MALLOC_CAP_SPIRAM,&ppaCacheLine_)!=ESP_OK)ppaCacheLine_=0;
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
    if(config_.frameAtomic&&mipiSdkFramebufferCount_==3){
      const size_t bytes=rasterWidth()*rasterHeight()*2;
      for(auto*frame:mipiFrames_){const uintptr_t address=reinterpret_cast<uintptr_t>(frame);
        if(!frame||!bytes||address%64||address>UINTPTR_MAX-(bytes-1)||!esp_ptr_external_ram(frame)||
          !esp_ptr_external_ram(reinterpret_cast<void*>(address+bytes-1)))return fail(Status::InvalidArgument);}
      mipiPresentationOwner_=detail::nextNativePresentationOwner();
      if(!mipiPresentationOwner_||mipiPresentation_.bind(mipiPresentationOwner_,mipiFrames_,rasterWidth(),rasterHeight(),rasterWidth())!=Status::Ok)
        return fail(Status::IoError);
      mipiPresentationTask_=xTaskGetCurrentTaskHandle();mipiPresentationBound_=true;
    }
#endif
    if(config_.batchWrites){
      mipiDirty_=static_cast<MipiDirtySpan*>(heap_caps_malloc(rasterHeight()*sizeof(MipiDirtySpan),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
      if(mipiDirty_)for(size_t row=0;row<rasterHeight();++row)mipiDirty_[row]={uint16_t(rasterWidth()),0};
      mipiDirtyTop_=rasterHeight();mipiDirtyBottom_=0;
    }
#if SOC_PPA_SUPPORTED
    if(config_.usePpa&&ppaCacheLine_){
      ppa_client_config_t client={};client.oper_type=PPA_OPERATION_SRM;client.max_pending_trans_num=1;
      if(ppa_register_client(&client,&ppaCopy_)!=ESP_OK)ppaCopy_=nullptr;
      client.oper_type=PPA_OPERATION_FILL;
      if(ppa_register_client(&client,&ppaFill_)!=ESP_OK)ppaFill_=nullptr;
      if(copyStats_){copyStats_->ppaReady=ppaCopy_!=nullptr;copyStats_->ppaFillReady=ppaFill_!=nullptr;}
#if NICE_GFX_P4_MIPI_PARTIAL_INHERIT_PPA && ESP_IDF_VERSION == ESP_IDF_VERSION_VAL(5,5,5)
      if(mipiAtomic_){
        mipiPartialInheritDone_=xSemaphoreCreateBinary();
        if(mipiPartialInheritDone_){
          ppa_client_config_t inheritance={};inheritance.oper_type=PPA_OPERATION_SRM;inheritance.max_pending_trans_num=1;
          if(ppa_register_client(&inheritance,&mipiPartialInheritPpa_)!=ESP_OK)mipiPartialInheritPpa_=nullptr;
          if(mipiPartialInheritPpa_){
            ppa_event_callbacks_t callbacks={};callbacks.on_trans_done=partialInheritPpaDone;
            if(ppa_client_register_event_callbacks(mipiPartialInheritPpa_,&callbacks)!=ESP_OK){
              const auto removed=ppa_unregister_client(mipiPartialInheritPpa_);
              if(removed!=ESP_OK)return fail(removed);mipiPartialInheritPpa_=nullptr;
            }
          }
          if(!mipiPartialInheritPpa_){vSemaphoreDelete(mipiPartialInheritDone_);mipiPartialInheritDone_=nullptr;}
        }
        if(copyStats_)copyStats_->partialInheritPpaReady=mipiPartialInheritPpa_!=nullptr;
      }
#endif
    }
#endif
    if(copyStats_){copyStats_->fullInheritGdmaReady=false;
      copyStats_->fullInheritGdmaInstallError=ESP_ERR_NOT_SUPPORTED;
      copyStats_->fullInheritGdmaBurstBytes=64;copyStats_->fullInheritGdmaCacheLineBytes=uint16_t(ppaCacheLine_);}
    if(config_.useGdma&&ppaCacheLine_){
      mipiGdmaDone_=xSemaphoreCreateBinary();
      esp_err_t installed=mipiGdmaDone_?ESP_OK:ESP_ERR_NO_MEM;
      if(mipiGdmaDone_){
        // Reuse one engine/channel pair; do not compete for a second AXI pair.
        // 64 is supported by IDF5.5.5 and matches the SDK FB cache alignment.
        async_memcpy_config_t dma={};dma.backlog=1;dma.dma_burst_size=64;
        installed=esp_async_memcpy_install_gdma_axi(&dma,&mipiGdma_);
        if(installed!=ESP_OK){vSemaphoreDelete(mipiGdmaDone_);mipiGdmaDone_=nullptr;mipiGdma_=nullptr;}
      }
      if(copyStats_){copyStats_->gdmaReady=mipiGdma_!=nullptr;
        copyStats_->fullInheritGdmaReady=mipiGdma_&&mipiGdmaDone_;
        copyStats_->fullInheritGdmaInstallError=int32_t(installed);}
    }
  }
#endif
  return initialized;
}
TransportInfo Esp32Mipi::info() const {
  TransportInfo result;
  result.frequency = uint32_t(config_.dpi.dpi_clock_freq_mhz * 1000000.0f);
  const auto *profile = videoControllerProfile(config_.controller, VideoInterface::MIPI);
  result.lanes = config_.dsi.num_data_lanes ? config_.dsi.num_data_lanes : profile ? profile->defaultLanes : 0;
  return result;
}
#endif
} // namespace nice_gfx
#endif

#endif
