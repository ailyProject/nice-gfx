// Included inside View::Impl. The ordinary Canvas renderer can paint opaque,
// untransformed snapshots directly into a mapped native scanout framebuffer.
// Dirty regions and source leases stay under the normal Refresh lifecycle.
PixelOrder renderOrder=PixelOrder::Wire;
#include "NativeCacheRepair.inc.h"
Status prepareBackendFade(FrameSlice *pieces,size_t &count){
  if(count||renderBudget.maxPixelsPerTick||!snapshotTransition||transition.config.effect!=TransitionEffect::Fade||!snapshotSurface||
     snapshotKeyboard.visible||snapshotInput||(snapshotGroupIndex>=0&&snapshotGroup.indicatorHeight)||
     transitionFrame.outX||transitionFrame.outY||transitionFrame.inX||transitionFrame.inY)return Status::Unsupported;
  const bool paired=snapshotPhase==TransitionPhase::Paired&&rasterFrame.preview.page&&previewSurface;
  const bool solid=!rasterFrame.preview.page&&(snapshotPhase==TransitionPhase::Enter||snapshotPhase==TransitionPhase::Exit);
  if(!paired&&!solid)return Status::Unsupported;
  const int32_t w=rasterFrame.current.page->width,h=rasterFrame.current.page->height;const Rect v=snapshotMotionRect;
  if(v.x<0||v.y<0||v.width<=0||v.height<=0||int64_t(v.x)+v.width>w||int64_t(v.y)+v.height>h||
     (paired&&(rasterFrame.preview.page->width!=w||rasterFrame.preview.page->height!=h)))return Status::Unsupported;
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);if(!panel||!panel->supportsFrameBlit())return Status::Unsupported;
  const int32_t ax=panel->windowAlignmentX(),ay=panel->windowAlignmentY();if(!ax||!ay||v.x%ax||v.y%ay||v.width%ax||v.height%ay)return Status::Unsupported;
  const Surface *fixed=groupMotion.active&&groupMotion.prepared?previewSurface:snapshotSurface;if(!fixed)return Status::Unsupported;
  Rect edges[4];const int edgeCount=raster_occlusion::strips({0,0,w,h},v,edges);FrameSlice candidate[5];size_t n=0;
  for(int i=0;i<edgeCount;++i){const auto &r=edges[i];PixelView source(fixed->pixels+size_t(r.y)*w+r.x,r.width,r.height,w,cacheOrder);
    if(r.x%ax||r.y%ay||r.width%ax||r.height%ay||!panel->canFrameBlit(source))return Status::Unsupported;
    candidate[n].area={r.x,r.y,r.width,r.height};candidate[n++].pixels=source;
  }
  const uint32_t started=profilingClock?profilingClock():0;
  const Status status=panel->blendFrame({v.x,v.y,v.width,v.height},backendFrameSource(*snapshotSurface),paired?backendFrameSource(*previewSurface):FrameSource{},transitionFrame.opacity,snapshotBackground);
  if(status!=Status::Ok)return status;
  candidate[n].area={v.x,v.y,v.width,v.height};candidate[n++].written=true;
  for(size_t i=0;i<n;++i)pieces[i]=candidate[i];count=n;
  ++stats.ppaBlendCalls;stats.ppaBlendPixels+=uint64_t(v.width)*v.height;if(profilingClock)stats.ppaBlendMicros+=profilingClock()-started;
  return Status::Ok;
}
// An exact cached independent fade can use the same bounded full-width
// viewport as a cold Native stage. Keep the stationary authored bar/footer
// slices, and blend only the moving content into the owned mapped frame.
// snapshotSurface is supplied only by the existing exact lookup/repair path;
// body-ready compressed records alone never qualify here.
Status prepareCachedViewportFade(FrameSlice *pieces,size_t &count){
#if NICE_GFX_UI_CACHED_VIEWPORT_FADE
  if(count||renderBudget.maxPixelsPerTick||cacheOrder!=PixelOrder::Wire||!snapshotSurface||
     !snapshotSurface->ready||snapshotSurface->page!=rasterFrame.current.page||
     !snapshotTransition||rasterFrame.preview.page||transition.config.effect!=TransitionEffect::Fade||
     (snapshotPhase!=TransitionPhase::Enter&&snapshotPhase!=TransitionPhase::Exit)||
     snapshotKeyboard.visible||snapshotInput||styledFocus()||snapshotPressed>=0||!behaviorVisuals.empty()||
     (snapshotGroupIndex>=0&&snapshotGroup.indicatorHeight)||
     transitionFrame.outX||transitionFrame.outY||transitionFrame.inX||transitionFrame.inY||
     !ppaBlendEnabled||!rgb565_ppa::Context::available())return Status::Unsupported;
  const auto &p=*rasterFrame.current.page;const int32_t w=p.width,h=p.height;const Rect v=snapshotMotionRect;
  if(v.x||v.width!=w||v.y<0||v.height<=0||int64_t(v.y)+v.height>h||(!v.y&&v.height==h))return Status::Unsupported;
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);if(!panel||!panel->supportsFrameBlit())return Status::Unsupported;
  const int32_t ax=panel->windowAlignmentX(),ay=panel->windowAlignmentY();
  if(!ax||!ay||v.y%ay||v.width%ax||v.height%ay)return Status::Unsupported;
  Rect edges[4];const int edgeCount=raster_occlusion::strips({0,0,w,h},v,edges);FrameSlice candidate[5];size_t n=0;
  for(int i=0;i<edgeCount;++i){const auto &r=edges[i];const PixelView pixels(snapshotSurface->pixels+size_t(r.y)*w+r.x,r.width,r.height,w,PixelOrder::Wire);
    if(r.x%ax||r.y%ay||r.width%ax||r.height%ay||!panel->canFrameBlit(pixels))return Status::Unsupported;
    candidate[n].area={r.x,r.y,r.width,r.height};candidate[n++].pixels=pixels;
  }
  ProfileTimer fadeTimer(*this,&RuntimeStats::cachedViewportFadeMicros);
  auto &content=candidate[n];content.area={v.x,v.y,v.width,v.height};
  if(!transitionFrame.opacity){content.solid=true;content.color=snapshotBackground;}
  else {
    nice_gfx::NativeFramebuffer frame{};const Status mapped=panel->mapNativeFramebuffer(frame);
    if(mapped!=Status::Ok)return mapped;
    if(!frame.pixels||frame.width!=size_t(w)||frame.height!=size_t(h)||frame.stride!=size_t(w)||
       frame.stride>SIZE_MAX/size_t(h)/2||frame.bytes<frame.stride*size_t(h)*2)return Status::Unsupported;
    const uintptr_t address=reinterpret_cast<uintptr_t>(frame.pixels);const size_t bytes=frame.stride*size_t(h)*2;
    if(address>UINTPTR_MAX-bytes||nativeMemoryOverlap(address,bytes,snapshotSurface->pixels,size_t(w)*h*2))return Status::Unsupported;
    const size_t offset=size_t(v.y)*w;
    const PixelView output(frame.pixels+offset,v.width,v.height,w,PixelOrder::Native);
    if((reinterpret_cast<uintptr_t>(frame.pixels+offset)&63)||(uint64_t(v.width)*v.height*2&63)||!panel->canFrameBlit(output))return Status::Unsupported;
    if(transitionFrame.opacity==255){
      const Status copied=detail::copySystemModalNativeRows(ppaFade,frame,PixelView(snapshotSurface->pixels,w,h,w,PixelOrder::Wire),size_t(w)*h*2,v);
      if(copied!=Status::Ok)return copied;
    }else if(!ppaFade.blendSolidIntoNative(frame.pixels+offset,w,frame.bytes-offset*2,
        snapshotSurface->pixels+offset,w,v.width,v.height,snapshotBackground,transitionFrame.opacity)){
      ++stats.ppaBlendFailures;return Status::Unsupported;
    }
    content.pixels=output;++stats.ppaBlendCalls;stats.ppaBlendPixels+=uint64_t(v.width)*v.height;
  }
  ++n;for(size_t i=0;i<n;++i)pieces[i]=candidate[i];count=n;
  ++stats.cachedViewportFadeFrames;stats.cachedViewportFadePixels+=uint64_t(v.width)*v.height;
  return Status::Ok;
#else
  (void)pieces;(void)count;return Status::Unsupported;
#endif
}
// Cached content fades keep navigation chrome stationary. Full-width aligned
// viewports can blend directly into a mapped native framebuffer; other layouts
// and unmapped displays (for example S31 QSPI) retain an independent scratch
// surface. Refresh borrows either output and immutable Wire565 chrome until
// the complete frame has drained, exactly as it borrows cached slide slices.
bool prepareViewportFade(FrameSlice *pieces,size_t &count) {
  if(cacheOrder!=PixelOrder::Wire)return false;
  const Rect v=snapshotMotionRect;const auto &t=transitionFrame;
  const int32_t w=rasterFrame.current.page->width,h=rasterFrame.current.page->height;
  if(renderBudget.maxPixelsPerTick||count||!snapshotTransition||snapshotPhase!=TransitionPhase::Paired||
     transition.config.effect!=TransitionEffect::Fade||!rasterFrame.preview.page||!snapshotSurface||!previewSurface||
     rasterFrame.preview.page->width!=w||rasterFrame.preview.page->height!=h||
     snapshotKeyboard.visible||snapshotInput||(snapshotGroupIndex>=0&&snapshotGroup.indicatorHeight)||
     t.outX||t.outY||t.inX||t.inY||v.x<0||v.y<0||v.width<=0||v.height<=0||
     int64_t(v.x)+v.width>w||int64_t(v.y)+v.height>h||
     (!v.x&&!v.y&&v.width==w&&v.height==h))return false;
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);
  if(!panel||!panel->supportsFrameBlit())return false;
  const int32_t ax=panel->windowAlignmentX(),ay=panel->windowAlignmentY();
  if(!ax||!ay||v.x%ax||v.y%ay||v.width%ax||v.height%ay)return false;
  const Surface *fixedSurface=groupMotion.active&&groupMotion.prepared?previewSurface:snapshotSurface;
  Rect fixed[4];const int fixedCount=raster_occlusion::strips({0,0,w,h},v,fixed);
  FrameSlice candidate[5];size_t candidateCount=0;
  for(int i=0;i<fixedCount;++i){const Rect r=fixed[i];
    const PixelView pixels(fixedSurface->pixels+size_t(r.y)*w+r.x,r.width,r.height,w,PixelOrder::Wire);
    if(r.x%ax||r.y%ay||r.width%ax||r.height%ay||!panel->canFrameBlit(pixels))return false;
    auto &slice=candidate[candidateCount++];slice.area=DirtyRect(r.x,r.y,r.width,r.height);slice.pixels=pixels;
  }
  const size_t offset=size_t(v.y)*w+v.x;
  PixelView moving((t.opacity?snapshotSurface:previewSurface)->pixels+offset,v.width,v.height,w,PixelOrder::Wire);
  if(t.opacity&&t.opacity<255){
    if(!ppaBlendEnabled||!rgb565_ppa::Context::available())return false;
    // Preflight native/packed output before launching the blocking PPA job.
    if(!panel->canFrameBlit(PixelView(snapshotSurface->pixels,v.width,v.height,v.width,PixelOrder::Native)))return false;
    RenderTimer timer(*this);const uint32_t started=profilingClock?profilingClock():0;
    const uint16_t *blended=nullptr;
    // The existing PPA target API accepts complete packed rows only. Keeping
    // this fast path full-width prevents its output-cache invalidation from
    // discarding CPU-owned side chrome. beginFrame() already owns the mapped
    // target; its exact rectangle is later committed without another copy.
    if(!v.x&&v.width==w){
      nice_gfx::NativeFramebuffer frame{};
      if(panel->mapNativeFramebuffer(frame)==Status::Ok&&frame.pixels&&
         frame.width==size_t(w)&&frame.height==size_t(h)&&frame.stride==size_t(w)&&
         frame.stride<=SIZE_MAX/frame.height/2&&frame.bytes>=frame.stride*frame.height*2){
        uint16_t *target=frame.pixels+offset;
        const size_t bytes=size_t(v.width)*v.height*2;
        const uintptr_t address=reinterpret_cast<uintptr_t>(target);
        if(!(address&63)&&!(bytes&63)&&
           !nativeMemoryOverlap(address,bytes,snapshotSurface->pixels,size_t(w)*h*2)&&
           !nativeMemoryOverlap(address,bytes,previewSurface->pixels,size_t(w)*h*2)&&
           panel->canFrameBlit(PixelView(target,v.width,v.height,w,PixelOrder::Native))&&
           ppaFade.blendSourcesIntoNative(target,w,frame.bytes-offset*2,
             snapshotSurface->pixels+offset,w,previewSurface->pixels+offset,w,v.width,v.height,t.opacity))blended=target;
      }
    }
    if(!blended)blended=ppaFade.blendSourceRegionsToNative(snapshotSurface->pixels,w,
      previewSurface->pixels,w,h,v.x,v.y,v.width,v.height,t.opacity);
    if(!blended){++stats.ppaBlendFailures;return false;}
    moving=PixelView(blended,v.width,v.height,v.width,PixelOrder::Native);
    ++stats.ppaBlendCalls;stats.ppaBlendPixels+=uint64_t(v.width)*v.height;
    if(profilingClock)stats.ppaBlendMicros+=profilingClock()-started;
  }
  if(!panel->canFrameBlit(moving))return false;
  auto &slice=candidate[candidateCount++];slice.area=DirtyRect(v.x,v.y,v.width,v.height);slice.pixels=moving;
  for(size_t i=0;i<candidateCount;++i)pieces[i]=candidate[i];count=candidateCount;return true;
}
const CompressedSurface *exactCompressedFadeSource(const PageDesc *p,const RetainedVector<State> &values) {
  auto *source=p?compressedSurfaceFor(*p):nullptr;
  return source&&sameSurfaceState(source->key,values)?source:nullptr;
}
bool compressedFadeKeysReady() {
  return (!ppaBlendEnabled||!rgb565_ppa::Context::available())&&snapshotTransition&&transition.config.effect==TransitionEffect::Fade&&
    !snapshotKeyboard.visible&&!snapshotInput&&!styledFocus()&&!behaviorVisualMoving()&&
    (snapshotGroupIndex<0||!snapshotGroup.indicatorHeight)&&
    !transitionFrame.outX&&!transitionFrame.outY&&!transitionFrame.inX&&!transitionFrame.inY&&
    exactCompressedFadeSource(rasterFrame.current.page,rasterFrame.current.values)&&
    (!rasterFrame.preview.page||exactCompressedFadeSource(rasterFrame.preview.page,rasterFrame.preview.values));
}
// A stationary cached page fade can blend straight into RGB scanout on CPU
// backends as well. Preserve authored viewport and stationary navigation;
// only submitted dirty rectangles are written and all sources stay leased.
Status prepareCpuCachedFade(PixelView &surface) {
  if(cacheOrder!=PixelOrder::Wire)return Status::Unsupported;
  const bool runSources=compressedFadeKeysReady();
  if(!snapshotTransition||(!snapshotSurface&&!runSources)||transition.config.effect!=TransitionEffect::Fade||
     snapshotKeyboard.visible||snapshotInput||(snapshotGroupIndex>=0&&snapshotGroup.indicatorHeight)||
     transitionFrame.outX||transitionFrame.outY||transitionFrame.inX||transitionFrame.inY||
     (rasterFrame.preview.page&&!previewSurface&&!runSources)||
     (snapshotPhase!=TransitionPhase::Paired&&snapshotPhase!=TransitionPhase::Enter&&snapshotPhase!=TransitionPhase::Exit))return Status::Unsupported;
  const size_t w=rasterFrame.current.page->width,h=rasterFrame.current.page->height;
  if(rasterFrame.preview.page&&(rasterFrame.preview.page->width!=w||rasterFrame.preview.page->height!=h))return Status::Unsupported;
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);
  if(!panel||!panel->supportsFrameBlit()||(ppaBlendEnabled&&rgb565_ppa::Context::available()))return Status::Unsupported;
  nice_gfx::NativeFramebuffer frame{};const Status mapped=panel->mapNativeFramebuffer(frame);
  if(mapped!=Status::Ok)return mapped;
  if(frame.width!=w||frame.height!=h||frame.stride<w||frame.stride>SIZE_MAX/2/h||frame.bytes<frame.stride*h*2)return Status::Unsupported;
  const uintptr_t output=reinterpret_cast<uintptr_t>(frame.pixels);const size_t bytes=frame.stride*h*2;
  if(output>UINTPTR_MAX-bytes||(snapshotSurface&&nativeMemoryOverlap(output,bytes,snapshotSurface->pixels,w*h*2))||
     (previewSurface&&nativeMemoryOverlap(output,bytes,previewSurface->pixels,w*h*2)))return Status::Unsupported;
  const PixelView candidate(frame.pixels,w,h,frame.stride,PixelOrder::Native);
  if(!panel->canFrameBlit(candidate))return Status::Unsupported;
  rle565::Reader frontRuns,backRuns;
  if(runSources){
    const auto *f=exactCompressedFadeSource(rasterFrame.current.page,rasterFrame.current.values),*b=rasterFrame.preview.page?exactCompressedFadeSource(rasterFrame.preview.page,rasterFrame.preview.values):nullptr;
    for(const auto *record:{f,b})if(record){
      if(record->offset>compressedCapacity||record->words>compressedCapacity-record->offset||
         nativeMemoryOverlap(output,bytes,compressedStorage+record->offset,record->words*2))return Status::Unsupported;
      if(record->verifiedPixels!=w*h){
        if(!rle565::validate(compressedStorage+record->offset,record->words,w*h))return Status::Unsupported;
        record->verifiedPixels=w*h;
      }
    }
    frontRuns.begin(compressedStorage+f->offset,f->words);
    if(b)backRuns.begin(compressedStorage+b->offset,b->words);
  }
  RenderTimer timer(*this);
  const Rect viewport=snapshotMotionRect;
  for(size_t i=0;i<refresh->pendingRegions();++i){DirtyRect dirty;if(!refresh->pendingRegion(i,dirty))return Status::InvalidArgument;
    for(int32_t row=dirty.y;row<dirty.y+dirty.height;++row){
      // Short glyph runs otherwise issue costly halfword PSRAM stores. Merge
      // a row in the already-owned internal tile, then stream it out once.
      // Stream PSRAM output once per row. Interleaving scanout stores with
      // two cached PSRAM reads can thrash the S3 cache on photo page fades.
      // The existing internal tile also bounds scratch use for raw sources.
      uint16_t *rowTarget=capacity>=size_t(dirty.width)?tileBuffers[0]:nullptr;
      const auto blend=[&](int32_t x,int32_t width,bool moving){if(width<=0)return;
        const size_t offset=size_t(row)*w+size_t(x);
        if(runSources){rgb565_blend::nativeRunSpan(rowTarget?rowTarget+x-dirty.x:frame.pixels+size_t(row)*frame.stride+x,size_t(width),frontRuns,
            moving&&rasterFrame.preview.page?&backRuns:nullptr,offset,moving?transitionFrame.opacity:255,snapshotBackground);return;}
        rgb565_blend::nativeRowsFromWire(rowTarget?rowTarget+x-dirty.x:frame.pixels+size_t(row)*frame.stride+x,frame.stride,
          snapshotSurface->pixels+offset,w,moving&&previewSurface?previewSurface->pixels+offset:nullptr,w,
          size_t(width),1,moving?transitionFrame.opacity:255,snapshotBackground);};
      if(row<viewport.y||row>=viewport.y+viewport.height){blend(dirty.x,dirty.width,false);if(rowTarget)memcpy(frame.pixels+size_t(row)*frame.stride+dirty.x,rowTarget,size_t(dirty.width)*2);continue;}
      const int32_t left=std::max(dirty.x,viewport.x),right=std::min(dirty.x+dirty.width,viewport.x+viewport.width);
      if(right<=left){blend(dirty.x,dirty.width,false);if(rowTarget)memcpy(frame.pixels+size_t(row)*frame.stride+dirty.x,rowTarget,size_t(dirty.width)*2);continue;}
      blend(dirty.x,left-dirty.x,false);blend(left,right-left,true);blend(right,dirty.x+dirty.width-right,false);
      if(rowTarget)memcpy(frame.pixels+size_t(row)*frame.stride+dirty.x,rowTarget,size_t(dirty.width)*2);
    }
  }
  surfaceStats.hits+=rasterFrame.preview.page?2:1;surface=candidate;return Status::Ok;
}
const nice_gfx::NativeFramebuffer *stagingNativeTarget=nullptr;
bool buildingColdFadeStage=false;
// Non-owning destination available only while painting one mapped native
// frame. Overlay helpers share this scope instead of mapping for every node.
const nice_gfx::NativeFramebuffer *nativeOverlayTarget=nullptr;
#if (NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL && NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE) || (defined(CONFIG_IDF_TARGET_ESP32S31) && NICE_GFX_S31_PPA_BLEND_AVAILABLE)
Status nativeOpaqueFillFailure=Status::Ok;
#endif
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL && NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
static bool consumeNativeOpaqueFill(const NativeStageTransfer&transfer,Status&failure){
  if(transfer.status==Status::Unsupported&&!transfer.accepted&&!transfer.completed)return false;
  if(transfer.status==Status::Ok&&transfer.accepted&&transfer.completed)return true;
  failure=Status::IoError;return true;
}
#endif
static bool fillNativeCanvas(void *context,const DirtyRect &r,uint16_t *first,size_t stride,uint16_t color){
#if (NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL && NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE) || defined(CONFIG_IDF_TARGET_ESP32P4) || (defined(CONFIG_IDF_TARGET_ESP32S31) && NICE_GFX_S31_PPA_BLEND_AVAILABLE)
  auto &s=*static_cast<Impl*>(context);
#endif
#if NICE_GFX_P4_NATIVE_PRESENTATION_OPAQUE_FILL && NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
  if(s.nativeOpaqueFillFailure!=Status::Ok)return true;
  if(s.nativePipelinePreparing&&s.nativeOverlayTarget&&s.ppaBlendEnabled&&r.pixels()>=4096){
    const auto*target=s.nativeOverlayTarget;
    NativeStageTransfer transfer{Status::IoError,false,false};
    if(r.x>=0&&r.y>=0&&stride==target->stride&&first==target->pixels+size_t(r.y)*stride+r.x){
      if(auto*panel=nice_gfx::detail::DisplayAccess::panel(s.display))
        transfer=panel->fillNativeDrawing(s.nativePipelineLease,*target,r,color);
    }
    const bool consumed=consumeNativeOpaqueFill(transfer,s.nativeOpaqueFillFailure);
    if(s.nativeOpaqueFillFailure!=Status::Ok){s.nativePipelineFatal=true;++s.stats.ppaFillFailures;}
    else if(consumed){++s.stats.ppaFillCalls;s.stats.ppaFillPixels+=r.pixels();}
    return consumed;
  }
#endif
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(!s.nativeGpuPaintAllowed())return false;const auto *target=s.nativeOverlayTarget;
  if(!target||!s.ppaBlendEnabled||r.pixels()<4096||stride!=target->stride||first!=target->pixels+size_t(r.y)*stride+r.x)return false;
  const uint32_t started=s.profilingClock?s.profilingClock():0;const Status result=s.ppaFade.fillIntoNative(*target,r,color);
  if(result==Status::Ok){++s.stats.ppaFillCalls;s.stats.ppaFillPixels+=r.pixels();if(s.profilingClock)s.stats.ppaFillMicros+=s.profilingClock()-started;return true;}
  if(result!=Status::Unsupported)++s.stats.ppaFillFailures;return false;
#elif defined(CONFIG_IDF_TARGET_ESP32S31) && NICE_GFX_S31_PPA_BLEND_AVAILABLE
  if(s.nativeOpaqueFillFailure!=Status::Ok)return true;
  const auto *target=s.nativeOverlayTarget;
  if(!s.nativeGpuPaintAllowed()||!s.ppaBlendEnabled||s.renderOrder!=PixelOrder::Native||s.buildingSurface||s.stagingNativeTarget||
     !target||!target->pixels||!target->width||!target->height||target->stride<target->width||
     target->height>SIZE_MAX/target->stride/2||target->bytes<target->stride*target->height*2||
     r.x<0||r.y<0||r.width<=0||r.height<=0||r.pixels()<4096||
     int64_t(r.x)+r.width>target->width||int64_t(r.y)+r.height>target->height||
     stride!=target->stride||first!=target->pixels+size_t(r.y)*stride+size_t(r.x)||
     xPortInIsrContext()||!xPortCanYield())return false;
  const uint32_t started=s.profilingClock?s.profilingClock():0;
  // Context validates the complete allocation/cache lines and the SDK RGB565
  // DMA2D window before submitting this blocking fill. Only Unsupported is
  // guaranteed untouched and may continue through the ordinary CPU painter.
  const Status result=s.ppaFade.fillIntoNative(*target,r,color);
  if(result==Status::Unsupported)return false;
  if(result!=Status::Ok){s.nativeOpaqueFillFailure=Status::IoError;++s.stats.ppaFillFailures;return true;}
  ++s.stats.ppaFillCalls;s.stats.ppaFillPixels+=r.pixels();
  if(s.profilingClock)s.stats.ppaFillMicros+=s.profilingClock()-started;
  return true;
#else
  (void)context;(void)r;(void)first;(void)stride;(void)color;return false;
#endif
}
// A cold fade may sample the same immutable page many times. Borrow one raw
// cache slot for its Native565 raster; never mark those pixels as a Wire565
// retained page. Any slot writer advances pixelsRevision and rejects reuse.
Surface *coldFadeSurface=nullptr;
const PageDesc *coldFadePage=nullptr;
uint64_t coldFadeRevision=0;
uint32_t coldFadeEpoch=0;
void releaseColdFadeSurface(){
  if(coldFadeSurface&&coldFadeSurface->pixelsRevision==coldFadeRevision&&!coldFadeSurface->page&&!coldFadeSurface->ready)
    coldFadeSurface->key.clear();
  coldFadeSurface=nullptr;coldFadePage=nullptr;
}
struct NativeRenderFrame {nice_gfx::NativeFramebuffer frame{};DirtyRect regions[16];uint8_t count=0;};
#ifndef NICE_GFX_UI_NATIVE_AFFINE
#if defined(CONFIG_IDF_TARGET_ESP32P4) || defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_NATIVE_AFFINE 1
#else
#define NICE_GFX_UI_NATIVE_AFFINE 0
#endif
#endif
bool nativeOpaquePage(const PageDesc &p,const RetainedVector<State> &values) const {
#if NICE_GFX_UI_NATIVE_AFFINE
  // paintTransformedNode already preserves native destinations: opaque vector
  // spans write Native565 directly, and other leaves swap only their clipped
  // ROI around the existing Wire565 kernel. Keeping a second retained page
  // merely to execute those same kernels adds repair/copy/rotation work.
  (void)p;(void)values;return true;
#else
  for(uint16_t i=0;i<p.count;++i)if(p.nodes[i].motion&&visible(p,values,i)){
    // Only the sampled affine pose requires the Wire565 transform kernel.
    // Completed keyframes, translations, rectangular reveals and opacity use
    // ordinary native geometry/underlays even if a motion descriptor remains.
    if(p.nodes[i].motion){const auto &pose=motionPose(p,values,i);
      if(pose.scale!=1024||pose.rotation%360!=0)return false;
    }
  }
  return true;
#endif
}
static bool nativeMemoryOverlap(uintptr_t destination,size_t destinationBytes,const void *source,size_t bytes) {
  if(!source||!bytes)return false;const uintptr_t begin=reinterpret_cast<uintptr_t>(source);
  return begin>UINTPTR_MAX-bytes||(begin<destination+destinationBytes&&destination<begin+bytes);
}
bool nativeSourceOverlap(const PageDesc &p,const RetainedVector<State> &values,uintptr_t output,size_t bytes,bool includeHidden=false) const {
  for(uint16_t i=0;i<p.count;++i){const auto &n=p.nodes[i];if(n.kind!=Kind::Media&&n.kind!=Kind::Icon)continue;
    if(!includeHidden&&!visible(p,values,i))continue;const auto &v=values[i];
    if(n.kind==Kind::Media){const MediaDesc *source=media(n,v);if(source){
      size_t sourceBytes=0;
      if(mediaJpeg(*source)){
        const auto *jpeg=detail::jpegResource(*source);
        if(!jpeg||nativeMemoryOverlap(output,bytes,jpeg->data,jpeg->bytes)||jpegCache.overlaps(output,bytes)||jpegPrefetch.overlaps(output,bytes))return true;
        continue;
      }
      if(v.imageOverride&&v.image.impl_)sourceBytes=v.image.impl_->source.byteLength;
      else {
        const size_t frameBytes=mediaFrameBytes(*source),prefix=mediaColorKeyed(*source)?2:0;
        if(!source->width||!source->frames||!frameBytes||frameBytes>(SIZE_MAX-prefix)/source->frames)return true;
        sourceBytes=frameBytes*source->frames+prefix;
      }
      if(nativeMemoryOverlap(output,bytes,source->pixels,sourceBytes))return true;
    }}
    if(n.kind==Kind::Icon&&n.icon&&nativeMemoryOverlap(output,bytes,n.icon->bitmap,n.icon->bytes))return true;
  }
  return false;
}
bool nativeNodeScratchReady(uintptr_t target,size_t targetBytes) const {
  bool needed=false;
  for(const auto *values:{&rasterFrame.current.values,&rasterFrame.preview.values})for(const auto &value:*values)
    if(value.opacity&&value.opacity!=255){needed=true;break;}
  if(!needed)return true;
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);
#if NICE_GFX_P4_NATIVE_PRESENTATION_PIPELINE
  const bool leasePermission=nativePipelineScratchPermission(target,targetBytes);
  if((refresh->busy()&&!leasePermission)||!panel||(panel->busy()&&!leasePermission)||
     !tileBuffers[0]||!capacity||capacity>SIZE_MAX/2)return false;
#else
  if(refresh->busy()||!panel||panel->busy()||!tileBuffers[0]||!capacity||capacity>SIZE_MAX/2)return false;
#endif
  const size_t bytes=capacity*2;const uintptr_t scratch=reinterpret_cast<uintptr_t>(tileBuffers[0]);
  if(scratch>UINTPTR_MAX-bytes||nativeMemoryOverlap(target,targetBytes,tileBuffers[0],bytes)||
     nativeSourceOverlap(*rasterFrame.current.page,rasterFrame.current.values,scratch,bytes,true)||
     (rasterFrame.preview.page&&nativeSourceOverlap(*rasterFrame.preview.page,rasterFrame.preview.values,scratch,bytes,true)))return false;
  const size_t rawBytes=size_t(rasterFrame.current.page->width)*rasterFrame.current.page->height*2;
  for(const auto &surface:surfaces)if(nativeMemoryOverlap(scratch,bytes,surface.pixels,rawBytes))return false;
  if(nativeMemoryOverlap(scratch,bytes,compressedStorage,compressedCapacity*2))return false;
  return true;
}
Status prepareNativeRender(NativeRenderFrame &candidate) {
  candidate=NativeRenderFrame{};
  if(buildingSurface||snapshotSurface||snapshotAnimationSurface||circularKeyboard||snapshotKeyboard.visible||
     compressedFadeKeysReady()||
     (snapshotTransition&&transitionFrame.opacity!=255)||!nativeOpaquePage(*rasterFrame.current.page,rasterFrame.current.values)||
     (rasterFrame.preview.page&&!nativeOpaquePage(*rasterFrame.preview.page,rasterFrame.preview.values)))return Status::Unsupported;
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);
  if(!panel||!panel->supportsFrameBlit()||!refresh->pendingRegions()||refresh->pendingRegions()>16)return Status::Unsupported;
  nice_gfx::NativeFramebuffer frame{};const Status mapped=retainedActive?panel->mapRetainedRepaintFramebuffer(retainedReceipts,retainedCount,frame):panel->mapNativeFramebuffer(frame);
  if(mapped!=Status::Ok)return mapped;
  if(!frame.pixels||frame.width!=rasterFrame.current.page->width||frame.height!=rasterFrame.current.page->height||frame.stride<frame.width||
     !frame.width||!frame.height||frame.stride>SIZE_MAX/2/frame.height||frame.bytes<frame.stride*frame.height*2)return Status::Unsupported;
  const uintptr_t output=reinterpret_cast<uintptr_t>(frame.pixels);const size_t bytes=frame.stride*frame.height*2;
  if(output>UINTPTR_MAX-bytes||nativeSourceOverlap(*rasterFrame.current.page,rasterFrame.current.values,output,bytes)||
     (rasterFrame.preview.page&&nativeSourceOverlap(*rasterFrame.preview.page,rasterFrame.preview.values,output,bytes))||
     !nativeNodeScratchReady(output,bytes))return Status::Unsupported;
  for(const Surface *surface:{snapshotSurface,previewSurface,snapshotAnimationSurface})if(surface&&
      nativeMemoryOverlap(output,bytes,surface->pixels,size_t(surface->page->width)*surface->page->height*2))return Status::Unsupported;
  if(auto *base=systemModalBaseSurface(systemModalBaseFrame))if(nativeMemoryOverlap(output,bytes,base->pixels,size_t(base->page->width)*base->page->height*2)){
    suppressSystemModalBaseFrame();return Status::Unsupported;
  }
  const int32_t ax=panel->windowAlignmentX(),ay=panel->windowAlignmentY();if(!ax||!ay)return Status::Unsupported;
  for(size_t i=0;i<refresh->pendingRegions();++i){DirtyRect area;
    if(!refresh->pendingRegion(i,area)||area.x<0||area.y<0||area.width<=0||area.height<=0||
       int64_t(area.x)+area.width>frame.width||int64_t(area.y)+area.height>frame.height||
       area.x%ax||area.y%ay||area.width%ax||area.height%ay)return Status::Unsupported;
    const PixelView view(frame.pixels+size_t(area.y)*frame.stride+area.x,area.width,area.height,frame.stride,PixelOrder::Native);
    if(!panel->canFrameBlit(view))return Status::Unsupported;
    candidate.regions[candidate.count++]=area;
  }
  candidate.frame=frame;return Status::Ok;
}
// Geometry survives dynamic image publications independently of raw page slots.
// Isolate the visible opaque image in a single-page slide. Outside pieces
// retain ordinary background/sidebar painting; the covered piece can use a
// direct source copy and omit every overwritten background. Commit the split
// only after every region/source is eligible, with no writes or allocation.
void partitionNativeRegions(NativeRenderFrame &candidate,bool identity=false) {
  if(!snapshotTransition)identity=true;
  if(rasterFrame.preview.page||(identity?snapshotTransition:!opaqueCoverTransition())||buildingSurface||rasterFrame.current.geometry.nodes.empty())return;
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);if(!panel)return;
  const bool entering=snapshotPhase==TransitionPhase::Enter;
  const int32_t offsetX=identity?0:entering?transitionFrame.inX:transitionFrame.outX,offsetY=identity?0:entering?transitionFrame.inY:transitionFrame.outY;
  int image=-1;Rect cover{};
  for(size_t end=rasterFrame.current.geometry.nodes.size();end;--end){const size_t i=end-1;if(!opaqueCoverMedia(rasterFrame.current.geometry,i))continue;
    Rect visible=intersect(rasterFrame.current.geometry.nodes[i].rect,rasterFrame.current.geometry.nodes[i].clip);visible.x+=offsetX;visible.y+=offsetY;
    visible=intersect(visible,{0,0,rasterFrame.current.page->width,rasterFrame.current.page->height});if(visible.width<=0||visible.height<=0)continue;
    image=int(i);cover=visible;break;
  }
  if(image<0)return;
  const int32_t ax=panel->windowAlignmentX(),ay=panel->windowAlignmentY();if(!ax||!ay)return;
  DirtyRect regions[16];uint8_t count=0;
  const auto add=[&](Rect r){if(r.width<=0||r.height<=0)return true;if(count==16||r.x%ax||r.y%ay||r.width%ax||r.height%ay)return false;
    const PixelView native(candidate.frame.pixels+size_t(r.y)*candidate.frame.stride+r.x,r.width,r.height,candidate.frame.stride,PixelOrder::Native);
    if(!stagingNativeTarget&&!panel->canFrameBlit(native))return false;regions[count++]=DirtyRect(r.x,r.y,r.width,r.height);return true;};
  for(uint8_t i=0;i<candidate.count;++i){const auto &dirty=candidate.regions[i];const Rect r{dirty.x,dirty.y,dirty.width,dirty.height},inside=intersect(r,cover);
    if(inside.width<=0||inside.height<=0){if(!add(r))return;continue;}
    const DirtyRect copied(inside.x,inside.y,inside.width,inside.height);
    if(stagingNativeTarget){if(!canStageOpaqueCover(candidate.frame,rasterFrame.current.geometry,image,copied,offsetX,offsetY))return;}
    else if(!panel->canFrameBlit(opaqueCoverPixels(rasterFrame.current.geometry,image,copied,offsetX,offsetY)))return;
    if(!add({r.x,r.y,r.width,inside.y-r.y})||
       !add({r.x,inside.y+inside.height,r.width,r.y+r.height-inside.y-inside.height})||
       !add({r.x,inside.y,inside.x-r.x,inside.height})||
       !add({inside.x+inside.width,inside.y,r.x+r.width-inside.x-inside.width,inside.height})||!add(inside))return;
  }
  candidate.count=count;for(uint8_t i=0;i<count;++i)candidate.regions[i]=regions[i];
}
// Build an uncached single-page fade in one existing raw pixel slot. The
// explicit staging target prevents OpaqueCover from flushing an unblended
// image to scanout. Only the final blend writes the mapped display target.
Status prepareColdFade(PixelView &output) {
  output=PixelView(nullptr,0);
  if(directJpegCover>=0)return Status::Unsupported;
  const Rect viewport=snapshotMotionRect;
  const bool tail=!snapshotTransition&&!transition.active&&coldFadeSurface&&coldFadePage==rasterFrame.current.page&&coldFadeEpoch==transitionEpoch;
  const uint8_t alpha=tail?255:transitionFrame.opacity;
  if(!ppaBlendEnabled||!rgb565_ppa::Context::available()||buildingSurface||renderingViewport||rasterFrame.preview.page||(!snapshotTransition&&!tail)||
     snapshotSurface||
#if !NICE_GFX_UI_VIEWPORT_ANIMATION_ATLAS
     snapshotAnimationSurface||
#endif
     snapshotInput||styledFocus()||snapshotKeyboard.visible||snapshotPressed>=0||down||
      (snapshotGroupIndex>=0&&snapshotGroup.indicatorHeight)||transition.config.effect!=TransitionEffect::Fade||
     (!tail&&snapshotPhase!=TransitionPhase::Enter&&snapshotPhase!=TransitionPhase::Exit)||
      !alpha||viewport.x||viewport.y<0||viewport.height<=0||
      viewport.width!=rasterFrame.current.page->width||
      int64_t(viewport.y)+viewport.height>rasterFrame.current.page->height||
     transitionFrame.outX||transitionFrame.outY||transitionFrame.inX||transitionFrame.inY||
     !nativeOpaquePage(*rasterFrame.current.page,rasterFrame.current.values))return Status::Unsupported;
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);
  if(!panel||!panel->supportsFrameBlit())return Status::Unsupported;
  struct Timer {
    Impl &self;uint64_t before;uint32_t maximum,started;
    explicit Timer(Impl &s):self(s),before(s.stats.renderMicros),maximum(s.stats.maxRenderMicros),started(s.profilingClock?s.profilingClock():0){}
    ~Timer(){if(self.profilingClock){const uint32_t elapsed=self.profilingClock()-started;self.stats.renderMicros=before+elapsed;self.stats.maxRenderMicros=std::max(maximum,elapsed);}}
  } timer(*this);
  const size_t w=rasterFrame.current.page->width,h=rasterFrame.current.page->height,bytes=w*h*2;
  nice_gfx::NativeFramebuffer target{};const Status mapped=panel->mapNativeFramebuffer(target);
  if(mapped!=Status::Ok)return mapped;
  if(!target.pixels||target.width!=w||target.height!=h||target.stride!=w||target.bytes<bytes||
     (reinterpret_cast<uintptr_t>(target.pixels)&63)||(bytes&63)||
     !panel->canFrameBlit(PixelView(target.pixels,w,h,w,PixelOrder::Native))||
     nativeSourceOverlap(*rasterFrame.current.page,rasterFrame.current.values,reinterpret_cast<uintptr_t>(target.pixels),bytes))return Status::Unsupported;
  const uint32_t epoch=tail?transitionEpoch:snapshotTransitionEpoch;
  const bool candidate=coldFadeSurface&&coldFadeEpoch==epoch&&coldFadePage==rasterFrame.current.page&&
    coldFadeSurface->pixelsRevision==coldFadeRevision&&!coldFadeSurface->page&&!coldFadeSurface->ready;
  const bool reuse=candidate&&sameSurfaceState(coldFadeSurface->key,rasterFrame.current.values);
  const bool atlas=snapshotAnimationSurface!=nullptr;
  DirtyRegions<16> repairs(0);bool partial=false;
  if(candidate&&!reuse&&!atlas&&coldFadeSurface->key.size()==rasterFrame.current.values.size()&&
     coldFadeSurface->geometry.nodes.size()==rasterFrame.current.values.size()){
    const auto &page=*rasterFrame.current.page;
    prepareFrameGeometry(page,rasterFrame.current.values,rasterFrame.current.geometry);
    for(uint16_t i=0;i<page.count;++i)if(!sameSurfaceNode(coldFadeSurface->key[i],rasterFrame.current.values[i]))
      for(uint16_t k=i;k<page.count;++k){
        int ancestor=k;while(ancestor>=0&&ancestor!=i)ancestor=page.nodes[ancestor].parent==NoParent?-1:page.nodes[ancestor].parent;
        if(ancestor!=i)continue;
        for(const auto *geometry:{&coldFadeSurface->geometry,&rasterFrame.current.geometry}){
          const auto &g=geometry->nodes[k];const Rect changed=intersect(g.rect,g.clip);
          repairs.add(DirtyRect(changed.x,changed.y,changed.width,changed.height),w,h);
        }
      }
    uint64_t changed=0;for(size_t i=0;i<repairs.size();++i)changed+=repairs[i].pixels();
    // Reuse the unblended stage for bounded scene repairs. A large changed
    // map or parent still takes a complete fresh raster; the pixels and
    // clocks are identical whichever work policy is selected.
    uint64_t limit=uint64_t(capacity)*4;
#if NICE_GFX_UI_VIEWPORT_ANIMATION_ATLAS
    limit=std::max<uint64_t>(limit,uint64_t(viewport.width)*viewport.height/2);
#endif
    partial=changed<=limit;
  }
  auto *slot=(reuse||partial||(atlas&&candidate))?coldFadeSurface:acquireFrameSurface(snapshotSurface,previewSurface,snapshotAnimationSurface);
  if(!slot)return Status::Unsupported;
  nice_gfx::NativeFramebuffer stage{slot->pixels,w,h,w,bytes};
  if(!stage.pixels||(reinterpret_cast<uintptr_t>(stage.pixels)&63)||
     nativeMemoryOverlap(reinterpret_cast<uintptr_t>(target.pixels),bytes,stage.pixels,bytes)||
     nativeSourceOverlap(*rasterFrame.current.page,rasterFrame.current.values,reinterpret_cast<uintptr_t>(stage.pixels),bytes)||
     !nativeNodeScratchReady(reinterpret_cast<uintptr_t>(target.pixels),bytes)||
     !nativeNodeScratchReady(reinterpret_cast<uintptr_t>(stage.pixels),bytes))return Status::Unsupported;
  struct Scope {
    Impl &self;bool transition,full,coldStage;PixelOrder order;const nice_gfx::NativeFramebuffer *staging;FrameCache *paint;int32_t x,y;
    Scope(Impl &owner,const nice_gfx::NativeFramebuffer &frame):self(owner),transition(owner.snapshotTransition),full(owner.renderingFullFrameSurface),coldStage(owner.buildingColdFadeStage),order(owner.renderOrder),staging(owner.stagingNativeTarget),paint(owner.paintCache),x(owner.paintOffsetX),y(owner.paintOffsetY){
      self.snapshotTransition=false;self.renderingFullFrameSurface=true;self.buildingColdFadeStage=true;self.renderOrder=PixelOrder::Native;self.stagingNativeTarget=&frame;self.paintOffsetX=self.paintOffsetY=0;
    }
    ~Scope(){self.snapshotTransition=transition;self.renderingFullFrameSurface=full;self.buildingColdFadeStage=coldStage;self.renderOrder=order;self.stagingNativeTarget=staging;self.paintCache=paint;self.paintOffsetX=x;self.paintOffsetY=y;}
  };
  if(!reuse){
    coldFadeSurface=nullptr;coldFadePage=nullptr;
    ++slot->pixelsRevision;
    Scope scope(*this,stage);
    prepareFrameGeometry(*rasterFrame.current.page,rasterFrame.current.values,rasterFrame.current.geometry);
    const auto composed=atlas?composeAnimationNativeStage(stage):Status::Unsupported;
    if(composed!=Status::Ok&&composed!=Status::Unsupported)return composed;
    if(composed==Status::Unsupported){
    ++stats.coldFadeStageRasters;if(partial)++stats.coldFadeStagePartialRasters;
    NativeRenderFrame pieces;pieces.frame=stage;
    if(partial){pieces.count=uint8_t(repairs.size());for(uint8_t i=0;i<pieces.count;++i)pieces.regions[i]=repairs[i];}
    else {pieces.regions[0]=DirtyRect(0,0,w,h);pieces.count=1;}
    partitionNativeRegions(pieces,true);
    for(uint8_t i=0;i<pieces.count;++i){const auto &area=pieces.regions[i];
      stats.coldFadeStageRasterPixels+=area.pixels();
      if(partial)stats.coldFadeStagePartialPixels+=area.pixels();
      const Status painted=render(this,area,stage.pixels+size_t(area.y)*w+area.x,w);if(painted!=Status::Ok)return painted;
    }
    }
    slot->key=rasterFrame.current.values;
    slot->geometry=rasterFrame.current.geometry;
    coldFadeSurface=slot;coldFadePage=rasterFrame.current.page;
    coldFadeRevision=slot->pixelsRevision;coldFadeEpoch=epoch;
  }else ++stats.coldFadeStageReuses;
  const uint32_t started=profilingClock?profilingClock():0;
  Status blended=Status::Unsupported;bool copied=false;
  if(alpha==255){
    blended=ppaFade.copyIntoNative(target,0,0,PixelView(stage.pixels,w,h,w,PixelOrder::Native),bytes,0,0,w,h);
    copied=blended==Status::Ok;
    if(blended!=Status::Ok&&blended!=Status::Unsupported)return blended;
  }
  if(!copied){
    // Fixed chrome keeps the incoming/current page's unblended pixels. Restrict
    // DMA output to full aligned rows so it cannot invalidate side padding.
    Rect fixed[4];const int count=raster_occlusion::strips({0,0,int32_t(w),int32_t(h)},viewport,fixed);
    for(int i=0;i<count;++i){const Rect r=fixed[i];
      blended=ppaFade.copyIntoNative(target,r.x,r.y,PixelView(stage.pixels,w,h,w,PixelOrder::Native),
        bytes,r.x,r.y,r.width,r.height);
      if(blended!=Status::Ok)return blended;
    }
    const size_t offset=size_t(viewport.y)*w;
    blended=ppaFade.blendSolidNativeIntoNative(target.pixels+offset,target.stride,target.bytes-offset*2,
      stage.pixels+offset,w,w,viewport.height,snapshotBackground,alpha);
  }
  if(blended!=Status::Ok){if(blended!=Status::Unsupported)++stats.ppaBlendFailures;return blended;}
  if(!copied){++stats.ppaBlendCalls;stats.ppaBlendPixels+=uint64_t(viewport.width)*viewport.height;if(profilingClock)stats.ppaBlendMicros+=profilingClock()-started;}
  if(tail&&!animationsRunning())releaseColdFadeSurface();
  output=PixelView(target.pixels,w,h,w,PixelOrder::Native);return Status::Ok;
}
#include "NativeStage.inc.h"
Status paintNativeRender(const NativeRenderFrame &candidate) {
#if NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
  rgb565_ppa::Context::CpuOnlyNativePaint cpuPaint(ppaFade,retainedActive);
  const uint64_t suppressedBefore=ppaFade.suppressedNativeOperations();
  if(retainedActive)++stats.retainedCpuOnlyPaintFrames;
#endif
  struct NativePaintBindings {
    Impl &self;bool full;PixelOrder order;const nice_gfx::NativeFramebuffer *overlay;
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR && NICE_GFX_UI_NATIVE_PREFIX_CACHE
    const NativePrefixEntry *prefix;
#endif
    NativePaintBindings(Impl&s,const NativeRenderFrame&frame) noexcept :self(s),full(s.renderingFullFrameSurface),order(s.renderOrder),overlay(s.nativeOverlayTarget)
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR && NICE_GFX_UI_NATIVE_PREFIX_CACHE
      ,prefix(s.nativePrefixSource)
#endif
    {self.renderingFullFrameSurface=true;self.renderOrder=PixelOrder::Native;self.nativeOverlayTarget=&frame.frame;}
    ~NativePaintBindings() noexcept {self.nativeOverlayTarget=overlay;self.renderOrder=order;self.renderingFullFrameSurface=full;
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR && NICE_GFX_UI_NATIVE_PREFIX_CACHE
      self.nativePrefixSource=prefix;
#endif
    }
    NativePaintBindings(const NativePaintBindings&)=delete;NativePaintBindings&operator=(const NativePaintBindings&)=delete;
  } bindings(*this,candidate);
  Status status=Status::Ok;
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR && NICE_GFX_UI_NATIVE_PREFIX_CACHE
  // Prefix ownership is restored by NativePaintBindings, including exceptions.
#if NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP
  // Each call owns one immutable native raster snapshot; public profiler
  // counter resets cannot alias this private validation epoch.
  ++stats.roiMemoFrames;
  if(!++roiPrefixSnapshotSequence){++roiPrefixSnapshotSequence;for(auto &entry:nativePrefixes)entry.roiValidatedSequence=0;
#if NICE_GFX_UI_ROI_PREFIX_FRAME_MEMO
    clearRoiPrefixFrameMemo();
#endif
  }
#endif
  const Surface *previous=nullptr;
  if(!snapshotTransition&&!rasterFrame.preview.page)for(const auto &surface:surfaces)
    if(surface.ready&&surface.page==rasterFrame.current.page&&surface.key.size()==rasterFrame.current.values.size()){previous=&surface;break;}
#endif
  for(uint8_t i=0;i<candidate.count;++i){const auto &area=candidate.regions[i];
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR && NICE_GFX_UI_NATIVE_PREFIX_CACHE
    nativePrefixSource=previous?resolveNativePrefix(*previous,area):nullptr;
#if NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP
    if(!nativePrefixSource)nativePrefixSource=resolveNativeRoiPrefix(area);
#endif
#endif
    status=render(this,area,candidate.frame.pixels+size_t(area.y)*candidate.frame.stride+area.x,candidate.frame.stride);
    if(status!=Status::Ok)break;
  }
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR && NICE_GFX_UI_NATIVE_PREFIX_CACHE
  // NativePaintBindings restores the prefix together with the native target.
#endif
#if NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
  stats.retainedGpuSuppressedOperations+=ppaFade.suppressedNativeOperations()-suppressedBefore;
#endif
  return status;
}
