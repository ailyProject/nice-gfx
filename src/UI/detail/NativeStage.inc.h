// Included inside View::Impl. The original snapshot, painter, dirty declaration
// and driver fences remain in force. Stage is never a retained physical FB.
struct NativeStagePlan {DirtyRect stripes[128]{};size_t count=0,pitch=0;uint64_t snapshot=0;const PageDesc*page=nullptr;const State*values=nullptr;size_t valueCount=0;const MotionRuntime*motions=nullptr;size_t motionCount=0;};
#if NICE_GFX_UI_NATIVE_STAGE && NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
static constexpr size_t NativeStageBytes=96u*1024u,NativeStageGuardWords=32;
struct NativeStageFree {void operator()(uint16_t*p)const noexcept{platform::freeUiNativeStage(p);}};
std::unique_ptr<uint16_t[],NativeStageFree> nativeStageStorage;
bool nativeStageInitialized=false,nativeStagePainting=false,nativeStageFatal=false,nativeStageFrameAccepted=false;
uint64_t nativeStageSourceOwner=nextRetainedChartOwner();
uint64_t nativeStageSnapshotSerial=0;bool nativeStageSnapshotValid=false;
const PageDesc*nativeStageSnapshotPage=nullptr;const State*nativeStageSnapshotValues=nullptr;size_t nativeStageSnapshotValueCount=0;
const MotionRuntime*nativeStageSnapshotMotions=nullptr;size_t nativeStageSnapshotMotionCount=0;
struct NativeStageTimer {Impl&s;uint64_t&counter;uint32_t started=0;NativeStageTimer(Impl&v,uint64_t&c):s(v),counter(c){if(s.profilingClock){++s.stats.nativeStageClockReads;started=s.profilingClock();}}~NativeStageTimer()noexcept{if(s.profilingClock){++s.stats.nativeStageClockReads;counter+=uint32_t(s.profilingClock()-started);}}};
#endif
size_t nativeStageAllocatedBytes()const{
#if NICE_GFX_UI_NATIVE_STAGE && NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
  return nativeStageStorage?NativeStageBytes:0;
#else
  return 0;
#endif
}
Status nativeStageFailureStatus()const{
#if NICE_GFX_UI_NATIVE_STAGE && NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
  return nativeStageFatal?Status::IoError:Status::Ok;
#else
  return Status::Ok;
#endif
}
void nativeStageAbortedFrame()noexcept{
#if NICE_GFX_UI_NATIVE_STAGE && NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
  if(nativeStageFrameAccepted)nativeStageFatal=true;
#endif
}
void nativeStageRefreshResult(Status status,bool final)noexcept{
#if NICE_GFX_UI_NATIVE_STAGE && NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
  if(nativeStageFrameAccepted&&status!=Status::Ok&&(status!=Status::Busy||final))nativeStageFatal=true;
#else
  (void)status;(void)final;
#endif
}
void nativeStageCompletedFrame()noexcept{
#if NICE_GFX_UI_NATIVE_STAGE && NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
  nativeStageFrameAccepted=false;
#endif
}
struct NativeStageTickGuard {
#if NICE_GFX_UI_NATIVE_STAGE && NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
  Impl&s;int exceptions;explicit NativeStageTickGuard(Impl&v)noexcept:s(v),exceptions(std::uncaught_exceptions()){}~NativeStageTickGuard()noexcept{if(std::uncaught_exceptions()>exceptions)s.nativeStageAbortedFrame();}
#else
  explicit NativeStageTickGuard(Impl&)noexcept{}
#endif
};
void nativeStageCaptureSnapshot()noexcept{
#if NICE_GFX_UI_NATIVE_STAGE && NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
  nativeStageSnapshotValid=false;if(nativeStageSnapshotSerial==UINT64_MAX)return;++nativeStageSnapshotSerial;
  nativeStageSnapshotPage=rasterFrame.current.page;nativeStageSnapshotValues=rasterFrame.current.values.data();nativeStageSnapshotValueCount=rasterFrame.current.values.size();
  nativeStageSnapshotMotions=rasterFrame.current.motions.data();nativeStageSnapshotMotionCount=rasterFrame.current.motions.size();nativeStageSnapshotValid=true;
#endif
}
bool nativeStageFrozenSnapshotMatches()const noexcept{
#if NICE_GFX_UI_NATIVE_STAGE && NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
  return nativeStageSnapshotValid&&nativeStageSnapshotSerial&&rasterFrame.current.page==nativeStageSnapshotPage&&rasterFrame.current.values.data()==nativeStageSnapshotValues&&rasterFrame.current.values.size()==nativeStageSnapshotValueCount&&rasterFrame.current.motions.data()==nativeStageSnapshotMotions&&rasterFrame.current.motions.size()==nativeStageSnapshotMotionCount;
#else
  return false;
#endif
}
bool planNativeStage(nice_gfx::Backend&panel,const DirtyRect*damage,size_t n,NativeStagePlan&plan){
  plan=NativeStagePlan{};
#if NICE_GFX_UI_NATIVE_STAGE && NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
  if(!panel.supportsNativeStage())return false;
  NativeStageTimer planTimer(*this,stats.nativeStagePlanMicros);
  const auto rejected=[&](unsigned reason){++stats.nativeStageRejected[reason];return false;};
  if(!panel.supportsFrameBlit()||!n||n>16||retainedActive||renderBudget.maxPixelsPerTick||nativeRequiresLegacySystemPaint()||buildingSurface||foregroundCapture||renderingViewport||
     snapshotTransition||transition.active||rasterFrame.preview.page||snapshotSurface||snapshotAnimationSurface||snapshotKeyboard.visible||snapshotInput||circularKeyboard||
     snapshotMenuEnabled||snapshotMarqueeId||styledFocus()||snapshotPressed>=0||groupMotion.active||groupMotion.prepared||viewportOverlayLevels||nativePrefixLimit>=0||!behaviorVisuals.empty()||behaviorVisualMoving()||
     coldFadeSurface||displayPolicy.monochrome||seedSurface)return rejected(0);
  const auto&p=*rasterFrame.current.page;const auto&values=rasterFrame.current.values;const auto&cache=rasterFrame.current.geometry;
  if(cache.nodes.size()!=p.count||!nativeStageFrozenSnapshotMatches())return rejected(1);
  uint64_t pixels=0;size_t maxWidth=0;unsigned shapes=0,alpha=0;bool affine=false;
  for(size_t i=0;i<n;++i){const auto&r=damage[i];if(r.x<0||r.y<0||r.width<=0||r.height<=0||int64_t(r.x)+r.width>p.width||int64_t(r.y)+r.height>p.height)return false;pixels+=uint64_t(r.width)*r.height;maxWidth=std::max(maxWidth,size_t(r.width));}
  if(pixels<65536)return rejected(2);
  for(uint16_t i=0;i<p.count;++i){const auto&g=cache.nodes[i];const auto&v=values[i];if(!g.visible||!g.alpha)continue;
    if(p.nodes[i].kind==Kind::Media||v.kind==Kind::Media)return rejected(3);
    // The immutable snapshot already owns these animation samples.
    if(v.dynamic)return rejected(4);
    if(p.nodes[i].motion){const auto&pose=motionPose(p,values,i);affine|=pose.scale!=1024||pose.rotation%360||pose.reveal!=1024;}
    bool intersects=false;
#if NICE_GFX_UI_SHAPE_INK_DAMAGE
    const Rect a=intersect(g.clip,g.paintRect);
#else
    const Rect a=intersect(g.clip,g.rect);
#endif
    if(a.width<=0||a.height<=0)continue;
    for(size_t j=0;j<n;++j){const auto&r=damage[j];intersects|=a.x<int64_t(r.x)+r.width&&r.x<int64_t(a.x)+a.width&&a.y<int64_t(r.y)+r.height&&r.y<int64_t(a.y)+a.height;}
    if(intersects){shapes+=p.nodes[i].kind==Kind::Shape;alpha+=g.alpha<255;}
  }
  if(shapes<3&&alpha<2)return rejected(6);
  const size_t pitch=(maxWidth+2u+31u)&~size_t(31u),payload=NativeStageBytes/2-2*NativeStageGuardWords;
  if(!pitch||pitch>payload)return false;const size_t pictureRows=payload/pitch;if(pictureRows<=2||!nativeStageSourceOwner)return false;const size_t rows=std::min<size_t>(75,pictureRows-2);
  for(size_t i=0;i<n;++i)for(int32_t y=damage[i].y;y<damage[i].y+damage[i].height;){if(plan.count==128){plan=NativeStagePlan{};return false;}
    const int32_t h=int32_t(std::min<size_t>(rows,size_t(damage[i].y+damage[i].height-y)));
    // Source height is greater than one: keep both identity MB remainder
    // guards. The single-row integer-repeat guard is not interchangeable.
    const unsigned left=unsigned(damage[i].width)%64u?unsigned(damage[i].width)%64u:64u;
    if(left*(unsigned(h)%16u?unsigned(h)%16u:16u)==96u||left*(unsigned(h)%32u?unsigned(h)%32u:32u)==96u){plan=NativeStagePlan{};return false;}
    plan.stripes[plan.count++]={damage[i].x,y,damage[i].width,h};y+=h;}
  plan.pitch=pitch;plan.snapshot=nativeStageSnapshotSerial;plan.page=&p;plan.values=values.data();plan.valueCount=values.size();plan.motions=rasterFrame.current.motions.data();plan.motionCount=rasterFrame.current.motions.size();
  if(!nativeStageStorage){nativeStageStorage.reset(static_cast<uint16_t*>(platform::allocateUiNativeStage(NativeStageBytes)));}
  auto*memory=nativeStageStorage.get();if(!memory){plan=NativeStagePlan{};return rejected(7);}
  const uintptr_t address=reinterpret_cast<uintptr_t>(memory);
  if((address&63u)||address>UINTPTR_MAX-NativeStageBytes){plan=NativeStagePlan{};return false;}
  // A fresh owned allocation cannot alias another live allocation. Verify the
  // existing borrowed tile and all externally supplied pixel resources too.
  for(auto*tile:tileBuffers)if(nativeMemoryOverlap(address,NativeStageBytes,tile,capacity*2)){plan=NativeStagePlan{};return false;}
  if(nativeMemoryOverlap(address,NativeStageBytes,underlay.data(),underlay.capacity()*2)||nativeMemoryOverlap(address,NativeStageBytes,transitionUnderlay.data(),transitionUnderlay.capacity()*2)||
     nativeMemoryOverlap(address,NativeStageBytes,mediaRow.data(),mediaRow.capacity()*2)||nativeSourceOverlap(p,values,address,NativeStageBytes,true)){plan=NativeStagePlan{};return false;}
  for(const auto&surface:surfaces)if(nativeMemoryOverlap(address,NativeStageBytes,surface.pixels,size_t(p.width)*p.height*2)){plan=NativeStagePlan{};return false;}
  if(nativeMemoryOverlap(address,NativeStageBytes,compressedStorage,compressedCapacity*2)){plan=NativeStagePlan{};return false;}
  ++stats.nativeStagePlans;++stats.nativeStageFrozenSnapshotAdmissions;stats.nativeStageAffinePlans+=affine;return true;
#else
  (void)panel;(void)damage;(void)n;return false;
#endif
}
bool canPaintStagedNative(nice_gfx::Backend&panel,const NativeRenderFrame&candidate,const NativeStagePlan&plan)const{
#if NICE_GFX_UI_NATIVE_STAGE && NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
  return repaintDiscardFrame&&!retainedActive&&plan.count&&plan.pitch&&nativeStageStorage&&candidate.frame.fencedAtomicMipiRmw&&panel.frameOpen()&&
    !nativeMemoryOverlap(reinterpret_cast<uintptr_t>(nativeStageStorage.get()),NativeStageBytes,candidate.frame.pixels,candidate.frame.bytes);
#else
  (void)panel;(void)candidate;(void)plan;return false;
#endif
}
Status paintStagedNativeRender(nice_gfx::Backend&panel,const NativeRenderFrame&candidate,const NativeStagePlan&plan){
#if NICE_GFX_UI_NATIVE_STAGE && NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
  if(!repaintDiscardFrame||retainedActive||!plan.count||!plan.pitch||!nativeStageStorage||!candidate.frame.fencedAtomicMipiRmw||!panel.frameOpen())return Status::InvalidArgument;
  NativeStageTimer totalTimer(*this,stats.nativeStageTotalMicros);
  // Once any stripe may own a hardware lease, an unsuccessful transaction is
  // fatal for this View, even after the real callback finally permits abort.
  // A later ordinary tick must not silently repaint or fall back to CPU.
  struct Transaction {Impl&s;bool accepted=false,success=false;~Transaction()noexcept{if(accepted&&!success)s.nativeStageFatal=true;}} transaction{*this};
  auto*memory=nativeStageStorage.get();auto*pixels=memory+NativeStageGuardWords+plan.pitch+1;
  if(nativeMemoryOverlap(reinterpret_cast<uintptr_t>(memory),NativeStageBytes,candidate.frame.pixels,candidate.frame.bytes))return Status::InvalidArgument;
  struct Bindings {
    Impl&s;bool full,painting;PixelOrder order;const NativeFramebuffer*overlay,*staging;
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR && NICE_GFX_UI_NATIVE_PREFIX_CACHE
    const NativePrefixEntry*prefix;
#endif
    explicit Bindings(Impl&v) noexcept:s(v),full(v.renderingFullFrameSurface),painting(v.nativeStagePainting),order(v.renderOrder),overlay(v.nativeOverlayTarget),staging(v.stagingNativeTarget)
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR && NICE_GFX_UI_NATIVE_PREFIX_CACHE
      ,prefix(v.nativePrefixSource)
#endif
    {s.renderingFullFrameSurface=true;s.renderOrder=PixelOrder::Native;s.nativeOverlayTarget=nullptr;s.stagingNativeTarget=nullptr;s.nativeStagePainting=true;
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR && NICE_GFX_UI_NATIVE_PREFIX_CACHE
      s.nativePrefixSource=nullptr;
#endif
    }
    ~Bindings() noexcept{s.canvas.detach();s.nativeOverlayTarget=overlay;s.stagingNativeTarget=staging;s.nativeStagePainting=painting;s.renderOrder=order;s.renderingFullFrameSurface=full;
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR && NICE_GFX_UI_NATIVE_PREFIX_CACHE
      s.nativePrefixSource=prefix;
#endif
    }
  };
  const auto snapshotMatches=[&](){return nativeStageFrozenSnapshotMatches()&&nativeStageSnapshotSerial==plan.snapshot&&rasterFrame.current.page==plan.page&&rasterFrame.current.values.data()==plan.values&&rasterFrame.current.values.size()==plan.valueCount&&rasterFrame.current.motions.data()==plan.motions&&rasterFrame.current.motions.size()==plan.motionCount;};
  const auto wordPoison=[](size_t w){return uint16_t(0x6b39u^(w*31u));};
  const auto paddingIntact=[&](const DirtyRect&r){
#if !NICE_GFX_NATIVE_STAGE_DIAGNOSTIC_VALIDATION
    (void)r;stats.nativeStageValidationBytes+=NativeStageGuardWords*4;
    for(size_t w=0;w<NativeStageGuardWords;++w)if(memory[w]!=wordPoison(w))return false;
    for(size_t w=NativeStageBytes/2-NativeStageGuardWords;w<NativeStageBytes/2;++w)if(memory[w]!=wordPoison(w))return false;
    return true;
#else
    stats.nativeStageValidationBytes+=NativeStageBytes-size_t(r.pixels())*2;
    for(size_t w=0;w<NativeStageGuardWords+plan.pitch;++w)if(memory[w]!=wordPoison(w))return false;
    for(size_t y=0;y<size_t(r.height);++y){const size_t row=NativeStageGuardWords+(y+1)*plan.pitch;if(memory[row]!=wordPoison(row))return false;for(size_t x=size_t(r.width)+1;x<plan.pitch;++x){const size_t w=row+x;if(memory[w]!=wordPoison(w))return false;}}
    for(size_t w=NativeStageGuardWords+(size_t(r.height)+1)*plan.pitch;w<NativeStageBytes/2;++w)if(memory[w]!=wordPoison(w))return false;
    return true;
#endif
  };
  const auto sourceDigest=[&](){
#if !NICE_GFX_NATIVE_STAGE_DIAGNOSTIC_VALIDATION
    return uint64_t(0);
#else
    stats.nativeStageValidationBytes+=NativeStageBytes;uint64_t value=1469598103934665603ull;for(size_t w=0;w<NativeStageBytes/2;++w)value=(value^memory[w])*1099511628211ull;return value;
#endif
  };
#if !NICE_GFX_NATIVE_STAGE_DIAGNOSTIC_VALIDATION
  // Initialize every picture-padding byte once. Warm paint writes only valid
  // stripe pixels; no full-allocation poison/digest is hidden in production.
  if(!nativeStageInitialized){for(size_t w=0;w<NativeStageBytes/2;++w)memory[w]=wordPoison(w);nativeStageInitialized=true;stats.nativeStageValidationBytes+=NativeStageBytes;++stats.nativeStageInitializations;}
#endif
  ++stats.nativeStageFrames;
  for(size_t i=0;i<plan.count;++i){const auto&r=plan.stripes[i];
    if(!snapshotMatches()){++stats.nativeStageErrors;return Status::IoError;}
#if NICE_GFX_NATIVE_STAGE_DIAGNOSTIC_VALIDATION
    for(size_t w=0;w<NativeStageBytes/2;++w)memory[w]=wordPoison(w);stats.nativeStageValidationBytes+=NativeStageBytes;
#else
    if(!paddingIntact(r)){++stats.nativeStageErrors;return Status::IoError;}
#endif
    {rgb565_ppa::Context::CpuOnlyNativePaint cpu(ppaFade,true);Bindings bindings(*this);
      const uint32_t at=profilingClock?(++stats.nativeStageClockReads,profilingClock()):0;
      const Status painted=render(this,r,pixels,plan.pitch);
      if(profilingClock){++stats.nativeStageClockReads;stats.nativeStageRenderMicros+=uint32_t(profilingClock()-at);}
      if(painted!=Status::Ok){++stats.nativeStageErrors;return painted;}
    }
    if(!paddingIntact(r)){++stats.nativeStageErrors;return Status::IoError;}
    const uint64_t sourceBefore=sourceDigest();
    ++stats.nativeStageStripes;stats.nativeStagePixels+=r.pixels();
    const uint32_t at=profilingClock?(++stats.nativeStageClockReads,profilingClock()):0;
    NativeStageSource source;source.allocation=memory;source.allocationBytes=NativeStageBytes;source.pixels=PixelView(pixels,r.width,r.height,plan.pitch,PixelOrder::Native);source.owner=nativeStageSourceOwner;source.snapshot=plan.snapshot;source.clock=profilingClock;
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
    NativeStageTransfer transfer=nativePipelinePreparing?
      panel.copyNativeDrawingStage(nativePipelineLease,candidate.frame,r,source):panel.copyNativeStage(candidate.frame,r,source);
#else
    NativeStageTransfer transfer=panel.copyNativeStage(candidate.frame,r,source);
#endif
    transaction.accepted|=transfer.accepted;
    nativeStageFrameAccepted|=transfer.accepted;
    bool failed=transfer.status!=Status::Ok&&transfer.status!=Status::Busy;
    if(!transfer.accepted&&!transfer.completed&&transfer.status==Status::Unsupported){
      // Unsupported certifies no submission and no target write. This is the
      // only legal CPU fallback; no accepted error can enter this branch.
      for(int32_t y=0;y<r.height;++y)memcpy(candidate.frame.pixels+size_t(r.y+y)*candidate.frame.stride+r.x,pixels+size_t(y)*plan.pitch,size_t(r.width)*2);
      ++stats.nativeStageCpuCopies;stats.nativeStageCpuCopyPixels+=r.pixels();
    }else{
      if(!transfer.accepted){++stats.nativeStageErrors;return transfer.status==Status::Ok||transfer.status==Status::Unsupported?Status::IoError:transfer.status;}
      while(!transfer.accepted||!transfer.completed){transfer=panel.drainNativeStage();failed|=!transfer.accepted||(transfer.status!=Status::Ok&&transfer.status!=Status::Busy);
        // Nothing, including an SDK error or timeout, releases these leases.
      }
      ++stats.nativeStageTransfers;
      if(failed||transfer.status!=Status::Ok){++stats.nativeStageErrors;return Status::IoError;}
      stats.nativeStageCompletedCopyPixels+=r.pixels();
    }
    // Diagnostic mode verifies the full source. Production checks outer guards;
    // TX source immutability and legal crop geometry follow the frozen SDK contract.
    // Total timing includes poison, guards, digest, render, copy and drain.
    if(!paddingIntact(r)||sourceDigest()!=sourceBefore||!snapshotMatches()){++stats.nativeStageErrors;return Status::IoError;}
    if(profilingClock){++stats.nativeStageClockReads;stats.nativeStageCopyMicros+=uint32_t(profilingClock()-at);}
  }
  transaction.success=true;
  return Status::Ok;
#else
  (void)panel;(void)candidate;(void)plan;return Status::Unsupported;
#endif
}
