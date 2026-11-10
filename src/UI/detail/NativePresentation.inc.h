// Included inside View::Impl. Display receipts own only immutable SDK pixels.
// Raster State/Motion and the current stage acceptance ledger have one owner.
static_assert(NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT&&NICE_GFX_UI_NATIVE_STAGE,
  "Native presentation requires the CPU facade and frozen snapshot identity");
struct NativePipelineTicket {
  nice_gfx::NativeDrawingLease lease{};
  uint64_t snapshot=0;
  bool stageAccepted=false;
};
NativePipelineTicket nativePipelineReady{},nativePipelinePending{};
nice_gfx::NativeDrawingLease nativePipelineLease{};
nice_gfx::NativeFramebuffer nativePipelineTarget{};
nice_gfx::NativePresentationStats nativePipelineStats{};
uint64_t nativePipelineReservation=0;
bool nativePipelinePreparing=false,nativePipelineFatal=false;
bool nativePipelineDrawingAcquired=false;
bool nativePipelineUncertainAccepted=false;
bool nativePipelineInPoll=false,nativePipelinePublishedInPoll=false;
nice_gfx::NativePresentationTransfer nativePipelineAcceptedTransfer{};

bool nativePipelineEnding()const {
  if(!project||history.empty())return true;
  for(const auto &motion:pageMotions[history.back()])if(motion.ending)return true;
  return false;
}
bool nativePipelineLiveEligible()const {
  if(!project||processing||updateDepth||renderBudget.maxPixelsPerTick||nativeRequiresLegacySystemPaint()||
     transition.active||backPrepared||backOffset||groupMotion.active||groupMotion.prepared||
     buildingSurface||foregroundCapture||renderingViewport||viewportOverlayLevels||nativePrefixLimit>=0||
     !behaviorVisuals.empty()||behaviorVisualMoving()||coldFadeSurface||displayPolicy.monochrome||
     down||softInput.visible||snapshotInput||circularKeyboard||styledFocus()||nativePipelineEnding())return false;
  for(const auto &value:state())if(value.dynamic)return false;
  return true;
}
bool nativePipelineSnapshotEligible(bool directJpegPlanned)const {
  if(!nativePipelineLiveEligible()||directJpegPlanned||retainedActive||snapshotTransition||
     rasterFrame.preview.page||seedSurface||snapshotSurface||previewSurface||snapshotAnimationSurface||
     snapshotKeyboard.visible||snapshotInput||snapshotMenuEnabled||snapshotMarqueeId||snapshotPressed>=0||
     !nativeStageFrozenSnapshotMatches())return false;
  for(const auto &motion:rasterFrame.current.motions)if(motion.ending)return false;
  return true;
}
bool nativePipelineScratchPermission(uintptr_t target,size_t bytes)const {
  return nativePipelinePreparing&&nativePipelineLease.owner&&nativePipelineLease.frame&&nativePipelineLease.epoch&&
    nativePipelineTarget.fencedAtomicMipiRmw&&target==reinterpret_cast<uintptr_t>(nativePipelineTarget.pixels)&&
    bytes==nativePipelineTarget.bytes;
}
Status prepareNativePipelineRender(NativeRenderFrame &candidate){
  candidate=NativeRenderFrame{};
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);
  const size_t count=refresh->preparedNativeRegions(nativePipelineReservation);
  if(!nativePipelinePreparing||!panel||!count||count>16||
     !nativeOpaquePage(*rasterFrame.current.page,rasterFrame.current.values))return Status::Unsupported;
  nice_gfx::NativeFramebuffer frame{};
  const Status mapped=panel->mapNativeDrawing(nativePipelineLease,frame);
  if(mapped!=Status::Ok)return mapped;
  if(!frame.pixels||!frame.fencedAtomicMipiRmw||frame.width!=rasterFrame.current.page->width||
     frame.height!=rasterFrame.current.page->height||!frame.width||!frame.height||frame.stride<frame.width||
     frame.stride>SIZE_MAX/2/frame.height||frame.bytes!=frame.stride*frame.height*2)return Status::IoError;
  nativePipelineTarget=frame;
  const uintptr_t output=reinterpret_cast<uintptr_t>(frame.pixels);
  const size_t bytes=frame.stride*frame.height*2;
  if(output>UINTPTR_MAX-bytes||nativeSourceOverlap(*rasterFrame.current.page,rasterFrame.current.values,output,bytes)||
     !nativeNodeScratchReady(output,bytes))return Status::Unsupported;
  const int32_t ax=panel->windowAlignmentX(),ay=panel->windowAlignmentY();
  if(!ax||!ay)return Status::Unsupported;
  for(size_t i=0;i<count;++i){DirtyRect area;
    if(!refresh->preparedNativeRegion(nativePipelineReservation,i,area)||area.x<0||area.y<0||
       area.width<=0||area.height<=0||int64_t(area.x)+area.width>frame.width||
       int64_t(area.y)+area.height>frame.height||area.x%ax||area.y%ay||area.width%ax||area.height%ay)
      return Status::InvalidArgument;
    const PixelView view(frame.pixels+size_t(area.y)*frame.stride+area.x,
      size_t(area.width),size_t(area.height),frame.stride,PixelOrder::Native);
    if(!panel->canFrameBlit(view))return Status::Unsupported;
    candidate.regions[candidate.count++]=area;
  }
  candidate.frame=frame;return Status::Ok;
}
void releaseNativePipelineRaster()noexcept {
  retainedForgetFrame();repaintDiscardFrame=false;rasterFrame.release();
  snapshotSurface=previewSurface=nullptr;
  nativeStageSnapshotValid=false;
  nativePipelineTarget=nice_gfx::NativeFramebuffer{};
}
Status completeNativePipelineTicket(){
  const auto &receipt=refresh->lastNativeRetirement();
  if(!nativePipelinePending.snapshot||refresh->lastNativeRetirementStatus()!=Status::Ok||
     receipt.owner!=nativePipelinePending.lease.owner||receipt.frame!=nativePipelinePending.lease.frame||
     receipt.epoch!=nativePipelinePending.lease.epoch||receipt.target!=nativePipelinePending.lease.target){
    nativePipelineFatal=true;++nativePipelineStats.errors;return Status::IoError;
  }
  ++stats.completedFrames;++nativePipelineStats.publishedFrames;
  if(nativePipelineInPoll)nativePipelinePublishedInPoll=true;
  nativePipelinePending=NativePipelineTicket{};
  nativePipelineAcceptedTransfer=nice_gfx::NativePresentationTransfer{};
  return Status::Ok;
}
Status retireNativePipelinePending(bool wait){
  if(!refresh->nativePresentationPending())return nativePipelinePending.snapshot?Status::IoError:Status::Ok;
  const bool failed=nativePipelineFatal||nativeStageFailureStatus()!=Status::Ok;
  const uint64_t before=refresh->stats().submitMicros;
  bool retired=false;
  const Status status=refresh->pollNativePresentation(wait,retired);
  stats.submitMicros+=refresh->stats().submitMicros-before;
  if(retired){
    ++nativePipelineStats.retiredFences;
    if(status==Status::Ok&&!failed){
      if(completeNativePipelineTicket()!=Status::Ok)return Status::IoError;
    }else{
      const auto &receipt=refresh->lastNativeRetirement();
      if(receipt.owner==nativePipelinePending.lease.owner&&receipt.frame==nativePipelinePending.lease.frame&&
         receipt.epoch==nativePipelinePending.lease.epoch&&receipt.target==nativePipelinePending.lease.target){
        nativePipelinePending=NativePipelineTicket{};
        nativePipelineAcceptedTransfer=nice_gfx::NativePresentationTransfer{};
      }
    }
  }
  if(status!=Status::Ok&&status!=Status::Busy){
    nativePipelineFatal=true;++nativePipelineStats.errors;return Status::IoError;
  }
  return failed?Status::IoError:status;
}
Status discardNativePipelineReady(){
  if(!nativePipelineReady.snapshot)return Status::Ok;
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);
  ++nativePipelineStats.typedAborts;
  if(!panel||panel->abortNativeDrawing(nativePipelineReady.lease)!=Status::Ok||
     refresh->restoreNativeDamage(nativePipelineReady.snapshot)!=Status::Ok){
    nativePipelineFatal=true;++nativePipelineStats.errors;return Status::IoError;
  }
  nativePipelineReady=NativePipelineTicket{};
  return Status::Ok;
}
bool nativePipelineOwnsWork()const {
  return nativePipelinePending.snapshot||nativePipelineReady.snapshot||nativePipelineUncertainAccepted||
    refresh->nativePresentationPending();
}
Status cancelNativePipelineWork(bool cancelLegacy=true){
  const Status retired=retireNativePipelinePending(true);
  if(retired!=Status::Ok)return retired;
  const Status discarded=discardNativePipelineReady();
  if(discarded!=Status::Ok)return discarded;
  if(nativePipelineFatal||nativeStageFailureStatus()!=Status::Ok)return Status::IoError;
  return cancelLegacy?refresh->cancel():Status::Ok;
}
bool nativePipelineConfigurationReady(){
  if(nativePipelinePreparing||nativePipelineFatal||nativeStageFailureStatus()!=Status::Ok||nativePipelineUncertainAccepted)return false;
  if(!nativePipelineOwnsWork())return true;
  if(nativePipelineInPoll&&nativePipelinePublishedInPoll)return false;
  return cancelNativePipelineWork(false)==Status::Ok;
}
Status submitNativePipelineReady(){
  if(nativePipelineFatal||nativeStageFailureStatus()!=Status::Ok||nativePipelineUncertainAccepted)return Status::IoError;
  if(!nativePipelineReady.snapshot||refresh->nativePresentationPending())return Status::Busy;
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);
  const auto submitted=panel->submitNativePresentation(nativePipelineReady.lease);
  if(submitted.status==Status::Busy&&!submitted.accepted)return Status::Busy;
  if(!submitted.accepted){
    nativePipelineFatal=true;++nativePipelineStats.errors;return Status::IoError;
  }
  // The SDK may hold this image even when its reply is malformed or non-OK.
  // Transfer the UI ledger before interpreting that reply, never leave it ready.
  nativePipelinePending=nativePipelineReady;nativePipelineReady=NativePipelineTicket{};
  nativePipelineAcceptedTransfer=submitted;
  ++nativePipelineStats.acceptedFrames;
  const Status accepted=refresh->acceptNativePresentation(nativePipelinePending.snapshot,submitted);
  nativePipelineUncertainAccepted=accepted!=Status::Ok||refresh->nativePresentationUncertain();
  if(accepted!=Status::Ok){nativePipelineFatal=true;++nativePipelineStats.errors;return Status::IoError;}
  if(submitted.status!=Status::Ok){nativePipelineFatal=true;++nativePipelineStats.errors;return Status::IoError;}
  return Status::Ok;
}
Status finishNativePipelineWork(){
  while(refresh->nativePresentationPending()||nativePipelineReady.snapshot){
    if(nativePipelineInPoll&&nativePipelinePublishedInPoll)return Status::Busy;
    const Status status=refresh->nativePresentationPending()?retireNativePipelinePending(true):submitNativePipelineReady();
    if(status!=Status::Ok)return status;
  }
  return nativePipelineFatal||nativePipelinePending.snapshot||nativePipelineUncertainAccepted||
    nativeStageFailureStatus()!=Status::Ok?Status::IoError:Status::Ok;
}
struct NativePipelineTickResult {bool handled=false;Status status=Status::Ok;};
NativePipelineTickResult nativePipelineTick(uint32_t now,bool wait=false){
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);
  if(!panel||!panel->supportsNativePresentationPipeline())return {};
  if(nativePipelinePublishedInPoll)return {true,Status::Ok};
  const bool failed=nativePipelineFatal||nativeStageFailureStatus()!=Status::Ok;
  if(failed&&!refresh->nativePresentationPending())return {true,Status::IoError};
  if(refresh->busy()&&!refresh->nativePresentationPending()&&!nativePipelineReady.snapshot)return {};
  const bool ending=nativePipelineEnding();
  const bool ordinary=nativePipelineLiveEligible();
  bool published=false;
  if(refresh->nativePresentationPending()){
    const uint64_t before=nativePipelineStats.publishedFrames;
    const Status status=retireNativePipelinePending(wait||!ordinary||failed);
    published=nativePipelineStats.publishedFrames!=before;
    if(status!=Status::Ok&&status!=Status::Busy)return {true,Status::IoError};
    if(!ordinary){if(ending)++nativePipelineStats.endingDrains;else ++nativePipelineStats.specialDrains;}
  }
  if(nativePipelineFatal||nativeStageFailureStatus()!=Status::Ok||nativePipelineUncertainAccepted)
    return {true,Status::IoError};
  if(!ordinary&&nativePipelineReady.snapshot&&discardNativePipelineReady()!=Status::Ok)
    return {true,Status::IoError};
  if(nativePipelineReady.snapshot){
    if(!refresh->nativePresentationPending())return {true,submitNativePipelineReady()};
    return {true,Status::Busy};
  }
  if(!ordinary){
    if(published){++nativePipelineStats.onePerPollDeferrals;return {true,Status::Ok};}
    return {refresh->nativePresentationPending(),Status::Ok};
  }
  if(wait)return {refresh->nativePresentationPending()||published,Status::Ok};
  const bool movingFrame=menuMoving()||behaviorMoving()||animationsRunning()||transition.active;
  const uint32_t motionInterval=menuMotionConfigured&&movingFrame?(1000u+menuMotionConfig.maxFps-1)/menuMotionConfig.maxFps:0;
  const uint32_t frameInterval=std::max(displayPolicy.frameIntervalMs,motionInterval);
  const bool cadenceReady=!displayFrameStarted||!frameInterval||uint32_t(now-lastDisplayFrameAt)>=frameInterval;
  if(cadenceReady&&refresh->canPrepareNative()){
    nativePipelinePreparing=true;
    nativePipelineDrawingAcquired=false;
    struct Preparation {
      Impl &self;
      ~Preparation()noexcept {
        if(self.nativePipelineReservation&&!self.nativePipelineReady.snapshot&&
           self.refresh->preparedNativeRegions(self.nativePipelineReservation))
          self.refresh->restoreNativeDamage(self.nativePipelineReservation);
        self.nativePipelinePreparing=false;self.nativePipelineReservation=0;
        self.releaseNativePipelineRaster();
      }
    } preparation{*this};
    const Status status=start(true);
    if(status==Status::Busy&&!nativePipelineDrawingAcquired&&!nativeStageFrameAccepted)
      return {true,Status::Busy};
    if(status!=Status::Ok&&status!=Status::Unsupported){nativePipelineFatal=true;++nativePipelineStats.errors;return {true,status==Status::Busy?Status::IoError:status};}
    if(nativePipelineReady.snapshot){
      lastDisplayFrameAt=now;displayFrameStarted=true;
      if(!refresh->nativePresentationPending())return {true,submitNativePipelineReady()};
      return {true,Status::Busy};
    }
  }
  if(published){++nativePipelineStats.onePerPollDeferrals;return {true,Status::Ok};}
  return {refresh->nativePresentationPending(),Status::Ok};
}
