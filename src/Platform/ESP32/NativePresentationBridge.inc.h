// Included by Esp32Video.cpp inside namespace nice_gfx.
#if defined(CONFIG_IDF_TARGET_ESP32P4)
bool VideoTransport::supportsNativePresentationPipeline()const {
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  return mipiPresentationBound_&&mipiSdkFramebufferCount_==3&&mipiDirectSdkDpi_&&mipiAtomic_&&
  panel_&&mipiFrameFence_&&!swapRedBlue_&&mipiInitCore_==1&&mipiPresentationOwner_;
#else
  return false;
#endif
}

Status VideoTransport::beginNativeDrawing(const DirtyRect *regions,size_t count,bool discard,NativeDrawingLease &lease){
  lease={};
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(!supportsNativePresentationPipeline())return Status::Unsupported;
  if(!mipiPresentationTask())return failMipiAtomic(Status::IoError);
  if(mipiAtomicFailure_!=Status::Ok)return mipiAtomicFailure_;
  if(mipiAtomicOpen_||mipiPresentation_.prepared()!=detail::NativePresentationState::None)return Status::Busy;
  const Status copied=copyOnlyMipiWait();
  if(copied!=Status::Ok)return copied;
  const Status acquired=mipiPresentation_.acquire(regions,count,discard,lease);
  if(acquired!=Status::Ok)return acquired;
  mipiDrawingLease_=lease;
  mipiTypedDrawing_=true;
  rgbFramebuffer_=mipiFrames_[lease.target];
  mipiAtomicOpen_=mipiAtomicExplicit_=mipiFrame_=true;
  mipiBatch_=false;
  DirtyRect needed[64],scratch[64];
  size_t n=0;
  const Status planned=mipiPresentation_.inheritance(lease,needed,scratch,n);
  if(planned!=Status::Ok)return failMipiAtomic(planned);
  const int64_t started=copyStats_?esp_timer_get_time():0;
  for(size_t i=0;i<n;++i){
    const auto&r=needed[i];
    auto*target=rgbFramebuffer_+size_t(r.y)*rasterWidth()+r.x;
    const auto*source=mipiFrames_[lease.source]+size_t(r.y)*rasterWidth()+r.x;
    for(int32_t y=0;y<r.height;++y)memcpy(target+size_t(y)*rasterWidth(),source+size_t(y)*rasterWidth(),size_t(r.width)*2);
    const size_t bytes=(size_t(r.height-1)*rasterWidth()+size_t(r.width))*2;
    const Status synced=convert(esp_cache_msync(target,bytes,ESP_CACHE_MSYNC_FLAG_DIR_C2M|ESP_CACHE_MSYNC_FLAG_UNALIGNED));
    if(copyStats_){
      copyStats_->inheritedPixels+=r.pixels();
      copyStats_->cpuPixels+=r.pixels();
      ++copyStats_->cacheSyncCalls;
      copyStats_->cacheSyncBytes+=bytes;
    }
    if(synced!=Status::Ok)return failMipiAtomic(synced);
  }
  if(copyStats_)copyStats_->inheritMicros+=uint64_t(esp_timer_get_time()-started);
  clearMipiDirty();
  return Status::Ok;
#else
  (void)regions;
  (void)count;
  (void)discard;
  return Status::Unsupported;
#endif
}

Status VideoTransport::mapNativeDrawing(const NativeDrawingLease &lease,NativeFramebuffer &frame){
  frame={};
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(!validMipiDrawing(lease)||copyOnlyMipiWait()!=Status::Ok)return failMipiAtomic(Status::IoError);
  frame.pixels=mipiFrames_[lease.target];
  frame.width=frame.stride=rasterWidth();
  frame.height=rasterHeight();
  frame.bytes=frame.stride*frame.height*2;
  frame.fencedAtomicMipiRmw=true;
  return Status::Ok;
#else
  (void)lease;
  return Status::Unsupported;
#endif
}

NativeStageTransfer VideoTransport::copyNativeDrawingStage(const NativeDrawingLease &lease,const NativeFramebuffer &frame,const DirtyRect &area,const NativeStageSource &source)noexcept{
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE && NICE_GFX_NATIVE_STAGE_ENGINE_AVAILABLE
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL
  if(mipiFillPreparing_||(nativeFillEngine_&&(nativeFillEngine_->preparing||nativeFillEngine_->pending||nativeFillEngine_->ownerTask!=xTaskGetCurrentTaskHandle()))){
    if(nativeFillEngine_){nativeFillEngine_->disabled=true;nativeFillEngine_->stats.disabled=true;}
    return {failMipiAtomic(Status::IoError),false,false};
  }
#endif
  if(nativeStageEngine_&&(nativeStageEngine_->pending||nativeStageEngine_->ownerTask!=xTaskGetCurrentTaskHandle())){
    const auto poison=+[](void*context,Status status){
      static_cast<VideoTransport*>(context)->failMipiAtomic(status);
    };
    nativeStageEngine_->fail(poison,this);
    return {
      mipiAtomicFailure_,false,false
    };
  }
  if(!validMipiDrawing(lease)||frame.pixels!=mipiFrames_[lease.target]||frame.width!=rasterWidth()||frame.height!=rasterHeight()||
  frame.stride!=rasterWidth()||frame.bytes!=rasterWidth()*rasterHeight()*2||!frame.fencedAtomicMipiRmw||!mipiDrawingCovers(area)||
  !detail::NativeStageEngine::validSource(source,area))return {
    failMipiAtomic(Status::IoError),false,false
  };
#if defined(CONFIG_SECURE_FLASH_ENC_ENABLED) && CONFIG_SECURE_FLASH_ENC_ENABLED
  return {};
#endif
  if(!supportsNativeStage()||!detail::NativeStageEngine::geometry(frame,area,source))return {};
  if(!nativeStageEngine_){
    const Status created=detail::NativeStageEngine::create(nativeStageEngine_,frame);
    if(created!=Status::Ok)return {
      created==Status::Unsupported?created:failMipiAtomic(created),false,false
    };
  }
  const auto poison=+[](void*context,Status status){
    static_cast<VideoTransport*>(context)->failMipiAtomic(status);
  };
  auto copied=nativeStageEngine_->run(frame,area,source,poison,this);
  if(copied.status!=Status::Ok&&copied.status!=Status::Unsupported)failMipiAtomic(copied.status);
  return copied;
#else
  (void)lease;
  (void)frame;
  (void)area;
  (void)source;
  return {};
#endif
}

#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL
NativeStageTransfer VideoTransport::fillNativeDrawing(const NativeDrawingLease&lease,const NativeFramebuffer&frame,
    const DirtyRect&area,uint16_t color)noexcept{
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE && NICE_GFX_NATIVE_STAGE_ENGINE_AVAILABLE
  const auto poison=+[](void*context,Status status){static_cast<VideoTransport*>(context)->failMipiAtomic(status);};
  if(mipiFillPreparing_)return {failMipiAtomic(Status::IoError),false,false};
  if(nativeFillEngine_&&(nativeFillEngine_->preparing||nativeFillEngine_->pending||nativeFillEngine_->ownerTask!=xTaskGetCurrentTaskHandle())){
    nativeFillEngine_->fail(poison,this);return {mipiAtomicFailure_,false,false};
  }
  if(nativeStageEngine_&&(nativeStageEngine_->pending||nativeStageEngine_->ownerTask!=xTaskGetCurrentTaskHandle())){
    nativeStageEngine_->fail(poison,this);return {mipiAtomicFailure_,false,false};
  }
  if(!validMipiDrawing(lease)||frame.pixels!=mipiFrames_[lease.target]||frame.width!=rasterWidth()||
     frame.height!=rasterHeight()||frame.stride!=rasterWidth()||frame.bytes!=rasterWidth()*rasterHeight()*2||
     !frame.fencedAtomicMipiRmw||!mipiDrawingCovers(area)||!ppaCacheLine_||ppaCacheLine_>64||64%ppaCacheLine_||
     xPortGetCoreID()!=1||xTaskGetCoreID(nullptr)!=1)return {failMipiAtomic(Status::IoError),false,false};
  const Status copied=copyOnlyMipiWait();
  if(copied!=Status::Ok)return {copied,false,false};
#if defined(CONFIG_SECURE_FLASH_ENC_ENABLED) && CONFIG_SECURE_FLASH_ENC_ENABLED
  return {};
#endif
  if(!supportsNativeStage()||!detail::NativeStageEngine::fillGeometry(frame,area)||area.pixels()<4096)return {};
  mipiFillPreparing_=true;
  struct Preparation{VideoTransport&transport;~Preparation()noexcept{transport.mipiFillPreparing_=false;}} preparation{*this};
  if(!nativeFillEngine_){
    const Status created=detail::NativeStageEngine::createFill(nativeFillEngine_,frame);
    if(created!=Status::Ok)return {created==Status::Unsupported?created:failMipiAtomic(created),false,false};
  }
  if(!validMipiDrawing(lease))return {failMipiAtomic(Status::IoError),false,false};
  auto result=nativeFillEngine_->runFill(frame,area,color,poison,this);
  if(result.status!=Status::Ok&&result.status!=Status::Unsupported)failMipiAtomic(result.status);
  return result;
#else
  (void)lease;(void)frame;(void)area;(void)color;return {};
#endif
}
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
Status VideoTransport::drainNativeDrawingFill()noexcept{
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE && NICE_GFX_NATIVE_STAGE_ENGINE_AVAILABLE
  if(mipiFillPreparing_){
    if(nativeFillEngine_){nativeFillEngine_->disabled=true;nativeFillEngine_->stats.disabled=true;}
    return failMipiAtomic(Status::IoError);
  }
  if(nativeFillEngine_){
    const auto poison=+[](void*context,Status status){static_cast<VideoTransport*>(context)->failMipiAtomic(status);};
    return nativeFillEngine_->drainFill(poison,this);
  }
#endif
  return Status::Ok;
}
#endif
#endif

Status VideoTransport::syncNativeDrawing(const NativeDrawingLease &lease)noexcept{
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(!validMipiDrawing(lease))return failMipiAtomic(Status::IoError);
  return copyOnlyMipiWait();
#else
  (void)lease;
  return Status::Unsupported;
#endif
}

Status VideoTransport::sealNativeDrawing(const NativeDrawingLease &lease)noexcept{
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  const Status copied=syncNativeDrawing(lease);
  if(copied!=Status::Ok)return copied;
  const Status sealed=mipiPresentation_.ready(lease);
  if(sealed!=Status::Ok)return failMipiAtomic(sealed);
  mipiReadyLease_=lease;
  mipiDrawingLease_={};
  mipiTypedReady_=mipiTypedDrawing_;
  mipiTypedDrawing_=false;
  mipiAtomicOpen_=mipiAtomicExplicit_=mipiFrame_=mipiBatch_=false;
  return Status::Ok;
#else
  (void)lease;
  return Status::Unsupported;
#endif
}

Status VideoTransport::abortNativeDrawing(const NativeDrawingLease &lease)noexcept{
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(!mipiPresentationTask())return failMipiAtomic(Status::IoError);
  const Status copied=copyOnlyMipiWait();
  if(copied!=Status::Ok)return copied;
  const Status aborted=mipiPresentation_.abort(lease);
  if(aborted!=Status::Ok)return failMipiAtomic(aborted);
  mipiDrawingLease_=mipiReadyLease_={};
  mipiTypedDrawing_=mipiTypedReady_=false;
  mipiAtomicOpen_=mipiAtomicExplicit_=mipiFrame_=mipiBatch_=false;
  clearMipiDirty();
  if(copyStats_)++copyStats_->frameAborts;
  return Status::Ok;
#else
  (void)lease;
  return Status::Unsupported;
#endif
}

NativePresentationTransfer VideoTransport::submitNativePresentation(const NativeDrawingLease &lease)noexcept{
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(!supportsNativePresentationPipeline()||!mipiPresentationTask()||mipiAtomicFailure_!=Status::Ok)return {
    Status::IoError
  };
  if(mipiPresentation_.pending()!=detail::NativePresentationState::None||mipiPresentation_.retirementPending())return {
    Status::Busy
  };
  if(lease.owner!=mipiReadyLease_.owner||lease.frame!=mipiReadyLease_.frame||lease.epoch!=mipiReadyLease_.epoch||
  lease.target!=mipiReadyLease_.target||lease.source!=mipiReadyLease_.source||lease.sourceVersion!=mipiReadyLease_.sourceVersion||
  mipiPresentation_.prepared()!=lease.target||!lease.owner)return {
    failMipiAtomic(Status::IoError)
  };
  const Status copied=copyOnlyMipiWait();
  if(copied!=Status::Ok)return {
    copied
  };
  rgbFramebuffer_=mipiFrames_[lease.target];
  const size_t bytes=rasterWidth()*rasterHeight()*2;
  const int64_t validationAt=copyStats_?esp_timer_get_time():0;
  const bool delegated=delegateMipiPresentationFlush(bytes);
  if(copyStats_)copyStats_->sdkFlushValidationMicros+=uint64_t(esp_timer_get_time()-validationAt);
  if(!delegated){
    const Status synced=convert(esp_cache_msync(rgbFramebuffer_,bytes,ESP_CACHE_MSYNC_FLAG_DIR_C2M|ESP_CACHE_MSYNC_FLAG_UNALIGNED));
    if(synced!=Status::Ok)return {
      failMipiAtomic(synced)
    };
  }
  NativePresentationReceipt receipt;
  const Status accepted=mipiPresentation_.accept(lease,receipt);
  if(accepted!=Status::Ok)return {
    accepted
  };
  mipiPresentationReceipt_=receipt;
  mipiReadyLease_={};
  mipiTypedPending_=mipiTypedReady_;
  mipiTypedReady_=false;
  mipiAtomicPending_=true;
  clearMipiDirty();
  const int pinnedCore=xTaskGetCoreID(nullptr);
  const int interruptCore=mipiInterruptCore_.load(std::memory_order_acquire);
  const esp_err_t submitted=esp_lcd_panel_draw_bitmap(panel_,0,0,int(rasterWidth()),int(rasterHeight()),rgbFramebuffer_);
  // Acceptance precedes invocation. An SDK error leaves this unarmed receipt
  // permanently held until an independent qualified quiescence proof exists.
  if(submitted!=ESP_OK)return {
    failMipiAtomic(convert(submitted)),receipt,false,true,false
  };
  uint8_t required=2;
#if !defined(NICE_GFX_P4_MIPI_FORCE_TWO_FRAME_FENCE) || !NICE_GFX_P4_MIPI_FORCE_TWO_FRAME_FENCE
  if(pinnedCore>=0&&pinnedCore==xPortGetCoreID()&&pinnedCore==xTaskGetCoreID(nullptr)&&
  interruptCore==pinnedCore&&mipiInterruptCore_.load(std::memory_order_acquire)==interruptCore)required=1;
#else
  (void)pinnedCore;
  (void)interruptCore;
#endif
  const uint32_t baseline=mipiFrameSequence_.load(std::memory_order_acquire);
  const Status armed=mipiPresentation_.arm(receipt,baseline,required);
  mipiPresentationReceipt_=receipt;
  mipiFenceBaseline_=baseline;
  mipiFenceCompletions_=required;
  if(copyStats_){
    ++copyStats_->frameCommits;
    if(delegated){
      ++copyStats_->sdkFlushDelegatedCommits;
      copyStats_->sdkFlushDelegatedBytes+=bytes;
    }
  }
  return {
    armed==Status::Ok?Status::Ok:failMipiAtomic(armed),receipt,false,true,false
  };
#else
  (void)lease;
  return {};
#endif
}

NativePresentationTransfer VideoTransport::pollNativePresentation(const NativePresentationReceipt &receipt,bool wait)noexcept{
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
  if(!mipiPresentationTask())return {
    failMipiAtomic(Status::IoError)
  };
  const auto same=[](const NativePresentationReceipt&a,const NativePresentationReceipt&b){
    return a.owner&&a.owner==b.owner&&a.frame==b.frame&&a.epoch==b.epoch&&a.target==b.target&&a.baseline==b.baseline&&a.required==b.required;
  };
  if(!same(receipt,mipiPresentation_.retirementPending()?mipiRetiredReceipt_:mipiPresentationReceipt_))return {
    failMipiAtomic(Status::IoError)
  };
  bool retired=false;
  Status status=Status::Ok;
  if(!mipiPresentation_.retirementPending()){
    status=observeMipiPresentation(wait,retired);
    if(!retired)return {
      status,receipt,false,true,false
    };
  }
  status=claimMipiPresentation(retired);
  return {
    status,receipt,false,true,retired
  };
#else
  (void)receipt;
  (void)wait;
  return {};
#endif
}
#endif
#if NICE_GFX_P4_NATIVE_PRESENTATION_AVAILABLE
bool VideoTransport::mipiPresentationTask()const{
  return mipiPresentationBound_&&!xPortInIsrContext()&&xPortCanYield()&&mipiPresentationTask_==xTaskGetCurrentTaskHandle();
}

bool VideoTransport::validMipiDrawing(const NativeDrawingLease&lease)const{
  return mipiPresentationTask()&&mipiAtomicFailure_==Status::Ok&&mipiAtomicOpen_&&lease.owner&&lease.target<3&&
  mipiPresentation_.drawing()==lease.target&&lease.owner==mipiDrawingLease_.owner&&lease.frame==mipiDrawingLease_.frame&&
  lease.epoch==mipiDrawingLease_.epoch&&lease.target==mipiDrawingLease_.target&&lease.source==mipiDrawingLease_.source&&
  lease.sourceVersion==mipiDrawingLease_.sourceVersion&&rgbFramebuffer_==mipiFrames_[lease.target];
}

bool VideoTransport::mipiDrawingCovers(const DirtyRect&r)const{
  if(r.x<0||r.y<0||r.width<=0||r.height<=0)return false;
  for(size_t i=0;i<mipiPresentation_.damageCount();++i){
    const auto&d=mipiPresentation_.damage(i);
    if(r.x>=d.x&&r.y>=d.y&&int64_t(r.x)+r.width<=int64_t(d.x)+d.width&&int64_t(r.y)+r.height<=int64_t(d.y)+d.height)return true;
  }
  return false;
}

Status VideoTransport::copyOnlyMipiWait(){
  Status consumed=Status::Ok;
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL
  const Status filled=drainNativeDrawingFill();
  if(filled!=Status::Ok)consumed=failMipiAtomic(filled);
#endif
  if(pending_){
    if(!completed_||xSemaphoreTake(completed_,portMAX_DELAY)!=pdTRUE)return failMipiAtomic(Status::IoError);
    pending_=false;
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL
    if(consumed==Status::Ok)consumed=result_;
#else
    consumed=result_;
#endif
  }
  return mipiAtomicFailure_==Status::Ok?consumed:mipiAtomicFailure_;
}

Status VideoTransport::observeMipiPresentation(bool block,bool&retired){
  retired=false;
  if(mipiPresentation_.pending()==detail::NativePresentationState::None)return mipiPresentation_.faulted()?Status::IoError:Status::Ok;
  if(!mipiPresentationTask()||!mipiFrameFence_)return failMipiAtomic(Status::IoError);
  if(!mipiPresentationReceipt_.required)return failMipiAtomic(Status::IoError);
  const int64_t started=(block||copyStats_)?esp_timer_get_time():0;
  for(;;){
    const Status observed=mipiPresentation_.observe(mipiPresentationReceipt_,mipiFrameSequence_.load(std::memory_order_acquire),retired);
    if(retired){
      mipiRetiredReceipt_=mipiPresentationReceipt_;
      mipiPresentationReceipt_={};
      mipiTypedRetired_=mipiTypedPending_;
      mipiTypedPending_=false;
      mipiFront_=mipiPresentation_.front();
      mipiAtomicPending_=false;
      ++mipiOwnershipEpoch_;
      if(copyStats_){
        ++copyStats_->frameSwitches;
        copyStats_->linkWaitMicros+=uint64_t(esp_timer_get_time()-started);
      }
      return observed;
    }
    if(!block)return observed;
    const int64_t remaining=250000-(esp_timer_get_time()-started);
    if(remaining<=0){
      if(copyStats_)++copyStats_->linkTimeouts;
      return failMipiAtomic(Status::IoError);
    }
    TickType_t ticks=pdMS_TO_TICKS(uint32_t((remaining+999)/1000));
    if(!ticks)ticks=1;
    xSemaphoreTake(mipiFrameFence_,ticks);
  }
}

Status VideoTransport::claimMipiPresentation(bool&retired){
  retired=false;
  if(!mipiPresentation_.retirementPending())return Status::Ok;
  const Status status=mipiPresentation_.claim(mipiRetiredReceipt_,retired);
  if(retired){
    mipiRetiredReceipt_={};
    mipiTypedRetired_=false;
  }
  return status;
}

Status VideoTransport::beginMipiPresentationLegacy(bool explicitFrame,const DirtyRect*regions,size_t count,bool discard){
  if(mipiPresentation_.drawing()!=detail::NativePresentationState::None){
    if(mipiTypedDrawing_)return Status::Busy;
    if(count)for(size_t i=0;i<count;++i)if(!regions||!mipiDrawingCovers(regions[i]))return failMipiAtomic(Status::IoError);
    if(explicitFrame)mipiAtomicExplicit_=true;
    return Status::Ok;
  }
  if(mipiPresentation_.prepared()!=detail::NativePresentationState::None)return Status::Busy;
  bool retired=false;
  const Status drained=observeMipiPresentation(true,retired);
  if(drained!=Status::Ok)return drained;
  if(mipiPresentation_.retirementPending()&&mipiTypedRetired_)return Status::Busy;
  if(mipiPresentation_.retirementPending()){
    const Status claimed=claimMipiPresentation(retired);
    if(claimed!=Status::Ok)return claimed;
  }
  NativeDrawingLease lease;
  const Status acquired=beginNativeDrawing(regions,count,discard,lease);
  if(acquired==Status::Ok){
    mipiTypedDrawing_=false;
    mipiAtomicExplicit_=explicitFrame;
  }
  return acquired;
}

Status VideoTransport::mapMipiPresentationLegacy(NativeFramebuffer&frame){
  frame={};
  if(mipiTypedDrawing_)return failMipiAtomic(Status::IoError);
  const Status begun=beginMipiPresentationLegacy(false,nullptr,0,false);
  if(begun!=Status::Ok)return begun;
  return mapNativeDrawing(mipiDrawingLease_,frame);
}

Status VideoTransport::commitMipiPresentationLegacy(){
  if(mipiTypedDrawing_||mipiTypedReady_||mipiTypedPending_||mipiTypedRetired_)return Status::Busy;
  if(mipiPresentation_.drawing()!=detail::NativePresentationState::None){
    const auto lease=mipiDrawingLease_;
    const Status sealed=sealNativeDrawing(lease);
    if(sealed!=Status::Ok)return sealed;
  }
  if(mipiPresentation_.prepared()!=detail::NativePresentationState::None){
    const auto submitted=submitNativePresentation(mipiReadyLease_);
    if(submitted.status!=Status::Ok)return submitted.status;
  }
  bool retired=false;
  const Status observed=observeMipiPresentation(true,retired);
  if(observed!=Status::Ok)return observed;
  return claimMipiPresentation(retired);
}

Status VideoTransport::abortMipiPresentationLegacy(){
  if(mipiTypedDrawing_||mipiTypedReady_||mipiTypedPending_||mipiTypedRetired_)return Status::Busy;
  const Status copied=copyOnlyMipiWait();
  if(copied!=Status::Ok)return copied;
  if(mipiPresentation_.drawing()!=detail::NativePresentationState::None)return abortNativeDrawing(mipiDrawingLease_);
  if(mipiPresentation_.prepared()!=detail::NativePresentationState::None)return abortNativeDrawing(mipiReadyLease_);
  bool retired=false;
  const Status observed=observeMipiPresentation(true,retired);
  if(observed!=Status::Ok)return observed;
  return claimMipiPresentation(retired);
}

Status VideoTransport::presentMipiPresentation(){
  if(mipiPresentation_.drawing()!=detail::NativePresentationState::None&&!mipiAtomicExplicit_&&!mipiTypedDrawing_)return commitMipiPresentationLegacy();
  const Status copied=copyOnlyMipiWait();
  bool retired=false;
  const Status observed=observeMipiPresentation(true,retired);
  return copied==Status::Ok?observed:copied;
}

Status VideoTransport::quiesceMipiPresentation(){
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL
  const Status filled=drainNativeDrawingFill();
#if NICE_GFX_NATIVE_STAGE_ENGINE_AVAILABLE
  if(filled!=Status::Ok&&nativeFillEngine_&&(nativeFillEngine_->preparing||nativeFillEngine_->pending))return filled;
#else
  (void)filled;
#endif
#endif
  // An unarmed accepted SDK call has no qualified completion proof. In this
  // prototype end()/destruction keeps this callback context and pool alive.
  if(mipiPresentation_.pending()!=detail::NativePresentationState::None&&!mipiPresentationReceipt_.required)return Status::IoError;
  bool retired=false;
  const Status observed=observeMipiPresentation(true,retired);
  if(mipiPresentation_.pending()!=detail::NativePresentationState::None)return observed==Status::Ok?Status::IoError:observed;
  if(mipiPresentation_.retirementPending())claimMipiPresentation(retired);
  return Status::Ok;
}

bool VideoTransport::delegateMipiPresentationFlush(size_t bytes)const{
#if !NICE_GFX_P4_MIPI_DELEGATE_SDK_FLUSH || NICE_GFX_P4_MIPI_SDK_FLUSH_AUDIT_ID != 0xb774170f || !CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_ENABLE || !defined(CONFIG_ESP_REV_MIN_FULL) || CONFIG_ESP_REV_MIN_FULL < 300
  (void)bytes;
  return false;
#else
  if(!supportsNativePresentationPipeline()||!mipiPresentationTask()||mipiPresentation_.prepared()!=mipiReadyLease_.target||mipiReadyLease_.target>=3||
  mipiPresentation_.pending()!=detail::NativePresentationState::None||mipiPresentation_.retirementPending()||pending_||
  mipiFullInheritPending_||mipiPartialInheritPending_||mipiAtomicFailure_!=Status::Ok||xPortGetCoreID()!=1||xTaskGetCoreID(nullptr)!=1)return false;
  const uintptr_t address=reinterpret_cast<uintptr_t>(mipiFrames_[mipiReadyLease_.target]);
  if(!bytes||bytes!=rasterWidth()*rasterHeight()*2||address>UINT32_MAX-bytes||address%64||bytes%64||
  !esp_ptr_external_ram(reinterpret_cast<void*>(address))||!esp_ptr_external_ram(reinterpret_cast<void*>(address+bytes-1)))return false;
  uint32_t level=0,id=0;
  return cache_hal_vaddr_to_cache_level_id(uint32_t(address),uint32_t(bytes),&level,&id)&&level==2&&cache_hal_get_cache_line_size(level,CACHE_TYPE_DATA)==64;
#endif
}
#endif
