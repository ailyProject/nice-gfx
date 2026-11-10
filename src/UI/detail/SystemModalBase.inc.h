#ifndef NICE_GFX_UI_SYSTEM_MODAL_RLE_DIM
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_SYSTEM_MODAL_RLE_DIM 1
#else
#define NICE_GFX_UI_SYSTEM_MODAL_RLE_DIM 0
#endif
#endif
// Included after raw Surface storage inside View::Impl. The lease owns no
// pixels. A live pin protects a closed-host key through the modal lifecycle;
// an independent accepted-frame pin survives later live mutations/navigation.
struct SystemModalBaseLease {
  size_t slot=SIZE_MAX;uint16_t page=NoParent,root=NoParent,bar=NoParent;
  uint64_t owner=0,revision=0;
  bool clearAfterFrame=false;
};
SystemModalBaseLease systemModalBaseLive,systemModalBaseFrame;
std::vector<uint8_t> systemModalBaseLiveKinds,systemModalBaseFrameKinds;
bool systemModalBaseFramePainted=false;
bool systemModalBaseFrameSuppressed=false;
bool systemModalBaseClosingFinal=false;
// A pending repair retains only the lifecycle pin until the real Native target
// has been mapped and independently checked. Accepted readers have drained.
bool systemModalBaseRepairPending=false,systemModalBaseRepairClosingFinal=false;
bool systemModalBaseRepairing=false;

bool systemModalBasePin(const SystemModalBaseLease &lease,const Surface &surface) const {
  return lease.owner==frameSourceOwner&&lease.slot<surfaces.size()&&&surfaces[lease.slot]==&surface;
}
bool systemModalBasePinned(const Surface &surface) const {
  return systemModalBasePin(systemModalBaseLive,surface)||systemModalBasePin(systemModalBaseFrame,surface)||systemModalPanelPinned(surface);
}
Surface *systemModalBaseSurface(const SystemModalBaseLease &lease) {
  if(!project||lease.owner!=frameSourceOwner||lease.slot>=surfaces.size()||lease.page>=project->count)return nullptr;
  auto &surface=surfaces[lease.slot];
  return surface.ready&&surface.page==&project->pages[lease.page]&&surface.pixelsRevision==lease.revision?&surface:nullptr;
}
void invalidateSystemModalBaseLive() {
  if(systemModalBaseLive.owner){++stats.systemModalBaseInvalidations;surfaceWarmPending=!surfaces.empty();}
  systemModalBaseLive=SystemModalBaseLease{};
}
void releaseSystemModalBaseFrame() {
  releaseSystemModalPanelFrame();
  const PageDesc *clear=nullptr;
  if(systemModalBaseFrame.clearAfterFrame&&project&&systemModalBaseFrame.page<project->count)clear=&project->pages[systemModalBaseFrame.page];
  systemModalBaseFrame=SystemModalBaseLease{};systemModalBaseFramePainted=false;systemModalBaseFrameSuppressed=false;systemModalBaseClosingFinal=false;systemModalBaseRepairPending=systemModalBaseRepairClosingFinal=false;
  if(clear)clearSurfacePage(clear);
}
void clearSystemModalBases() {
  clearSystemModalPanels();
  invalidateSystemModalBaseLive();systemModalBaseFrame=SystemModalBaseLease{};systemModalBaseFramePainted=false;systemModalBaseFrameSuppressed=false;systemModalBaseClosingFinal=false;
  systemModalBaseRepairPending=systemModalBaseRepairClosingFinal=false;
  std::vector<uint8_t>().swap(systemModalBaseLiveKinds);std::vector<uint8_t>().swap(systemModalBaseFrameKinds);
}
bool systemModalBaseKind(uint16_t node,uint8_t kind) const {
  return systemModalBaseFrame.owner&&node<systemModalBaseFrameKinds.size()&&systemModalBaseFrameKinds[node]==kind;
}
bool systemModalBaseHasFrame() const {return systemModalBaseFrame.owner==frameSourceOwner&&systemModalBaseFrame.slot<surfaces.size();}
void suppressSystemModalBaseFrame(){invalidateSystemModalBaseLive();systemModalBaseFrameSuppressed=true;systemModalBaseFrame.clearAfterFrame=true;}
static bool modalBaseSameRect(Rect a,Rect b){return a.x==b.x&&a.y==b.y&&a.width==b.width&&a.height==b.height;}
static bool modalBaseContains(Rect a,Rect b){return b.width>=0&&b.height>=0&&b.x>=a.x&&b.y>=a.y&&int64_t(b.x)+b.width<=int64_t(a.x)+a.width&&int64_t(b.y)+b.height<=int64_t(a.y)+a.height;}
bool modalBaseSameNode(const State &old,const State &live,uint8_t kind,Kind type) const {
  if(old.parent!=live.parent||old.kind!=live.kind||old.styleIndex!=live.styleIndex||old.implicitInput!=live.implicitInput)return false;
  if(sameSurfaceNode(old,live))return true;
  if(kind!=1||(type!=Kind::Text&&type!=Kind::Label&&type!=Kind::Button&&type!=Kind::Progress))return false;
  State normalized=old;normalized.text=live.text;if(type==Kind::Progress)normalized.value=live.value;
  return sameSurfaceNode(normalized,live);
}
bool validSystemModalBase(const Surface &surface,uint16_t root,uint16_t bar,const std::vector<uint8_t> &kinds,bool allowHostTextRepair=false) {
  const auto &p=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;
  if(surface.page!=&p||!surface.ready||surface.key.size()!=p.count||surface.geometry.nodes.size()!=p.count||kinds.size()!=p.count||
     root>=p.count||bar>=p.count||!(surface.key[root].flags&Hidden))return false;
  const Rect screen{0,0,p.width,p.height};
  const auto &background=surface.geometry.nodes[0],&chrome=surface.geometry.nodes[bar];
  if(!background.visible||background.alpha!=255||!modalBaseContains(background.rect,screen)||!modalBaseContains(background.clip,screen)||
     !chrome.visible||chrome.alpha!=255||chrome.rect.x||chrome.rect.y||chrome.rect.width!=p.width||chrome.rect.height<=0||
     !modalBaseContains(chrome.clip,chrome.rect))return false;
  for(uint16_t i=0;i<p.count;++i){
    if(kinds[i]==2){if(surface.geometry.nodes[i].visible)return false;continue;}
    if(!modalBaseSameNode(surface.key[i],values[i],kinds[i],p.nodes[i].kind)){
      // This relaxed comparison is ONLY a repair admission test. It can never
      // borrow old pixels: normal acquisition still requires the exact key.
      if(!allowHostTextRepair||kinds[i]||(p.nodes[i].kind!=Kind::Text&&p.nodes[i].kind!=Kind::Label))return false;
      State textOnly=surface.key[i];textOnly.text=values[i].text;
      if(!modalBaseSameNode(textOnly,values[i],0,p.nodes[i].kind))return false;
    }
    if(kinds[i]==1){const auto &g=surface.geometry.nodes[i];if(g.visible&&g.alpha&&!modalBaseContains(chrome.rect,intersect(g.rect,g.clip)))return false;continue;}
    // Raw caller images have no content revision. Likewise, active host clocks
    // are intentionally kept on the live renderer rather than predicted.
    if(values[i].imageOverride||values[i].animating||values[i].animationPending||focusStyle(i))return false;
    if(componentCount&&p.nodes[i].kind==Kind::Button){
      const auto before=componentPaint(p,surface.key,p.nodes[i].id),current=componentPaint(p,values,p.nodes[i].id);
      if(before.selected!=current.selected||before.expanded!=current.expanded)return false;
    }
    if(p.nodes[i].kind==Kind::Media){const auto *m=media(p.nodes[i],values[i]);if(m&&m->frames>1)return false;}
    if(viewportOverlayRoot(p,i)&&visible(p,values,i))return false;
  }
  return true;
}

bool queueSystemModalBaseTextRepair(Surface &surface,uint16_t root,uint16_t bar,bool closingFinal){
  if(renderBudget.maxPixelsPerTick||systemModalBaseFrame.owner||refresh->busy()||
     &surface==snapshotSurface||&surface==previewSurface||&surface==snapshotAnimationSurface||&surface==coldFadeSurface||
     !validSystemModalBase(surface,root,bar,systemModalBaseLiveKinds,true))return false;
  systemModalBaseRepairPending=true;systemModalBaseRepairClosingFinal=closingFinal;return true;
}
void repairSystemModalBaseFrame(const nice_gfx::NativeFramebuffer *target){
  if(!systemModalBaseRepairPending)return;
  systemModalBaseRepairPending=false;const bool closingFinal=systemModalBaseRepairClosingFinal;systemModalBaseRepairClosingFinal=false;
  const auto lease=systemModalBaseLive;auto *surface=systemModalBaseSurface(lease);
  auto *backend=nice_gfx::detail::DisplayAccess::panel(display);
  const auto &p=*rasterFrame.current.page;const size_t bytes=size_t(p.width)*p.height*2;
  if(!surface||!target||!target->pixels||!backend||backend->busy()||systemModalBaseFrame.owner||refresh->busy()||
     target->bytes>UINTPTR_MAX-reinterpret_cast<uintptr_t>(target->pixels)||
     nativeMemoryOverlap(reinterpret_cast<uintptr_t>(target->pixels),target->bytes,surface->pixels,bytes)||
     &*surface==snapshotSurface||&*surface==previewSurface||&*surface==snapshotAnimationSurface||&*surface==coldFadeSurface||
     !validSystemModalBase(*surface,lease.root,lease.bar,systemModalBaseLiveKinds,true)){
    invalidateSystemModalBaseLive();return;
  }
  // Preserve the accepted live frame by moving it aside, never the public
  // state, input, animation clock or event queue. The existing repair sees a
  // closed host key; panel/bar remain the retained source's exact subtrees.
  RetainedVector<State> closed=rasterFrame.current.values;
  for(size_t i=0;i<closed.size();++i)if(systemModalBaseLiveKinds[i])closed[i]=surface->key[i];
  const uintptr_t cacheAddress=reinterpret_cast<uintptr_t>(surface->pixels);
  if(cacheAddress>UINTPTR_MAX-bytes||nativeSourceOverlap(p,closed,cacheAddress,bytes,true)){
    invalidateSystemModalBaseLive();return;
  }
  Surface *repaired=nullptr;
  {
    struct Scope{
      Impl &s;RetainedVector<State> values;FrameCache geometry;std::vector<BehaviorVisual> visuals;FrameCache *paint;bool repairing;
      Scope(Impl &owner,RetainedVector<State> &closed):s(owner),paint(s.paintCache),repairing(s.systemModalBaseRepairing){
        values.swap(s.rasterFrame.current.values);s.rasterFrame.current.values.swap(closed);
        std::swap(geometry,s.rasterFrame.current.geometry);visuals.swap(s.behaviorVisuals);s.systemModalBaseRepairing=true;
      }
      ~Scope(){values.swap(s.rasterFrame.current.values);std::swap(geometry,s.rasterFrame.current.geometry);
        visuals.swap(s.behaviorVisuals);s.paintCache=paint;s.systemModalBaseRepairing=repairing;}
    } scope(*this,closed);
    // No accepted-frame lease remains; now release the sole lifecycle writer
    // exclusion before patchSurface checks its exact preferred source slot.
    systemModalBaseLive=SystemModalBaseLease{};bool geometryPrepared=false;
    repaired=patchSurface(geometryPrepared,0,surface);
  }
  if(repaired==surface&&validSystemModalBase(*surface,lease.root,lease.bar,systemModalBaseLiveKinds)){
    systemModalBaseLive=lease;systemModalBaseLive.revision=surface->pixelsRevision;++stats.systemModalBaseAcquisitions;
    systemModalBaseFrame=systemModalBaseLive;systemModalBaseFrameKinds=systemModalBaseLiveKinds;systemModalBaseClosingFinal=closingFinal;
    if(closingFinal)invalidateSystemModalBaseLive();
  }else{
    systemModalBaseLive=lease;invalidateSystemModalBaseLive();
  }
}
void prepareSystemModalBaseFrame() {
  // start() is reached only after the preceding immutable refresh drained.
  releaseSystemModalBaseFrame();
  pruneSystemModalPanelLive();
#if NICE_GFX_UI_SYSTEM_MODAL_BASE_LEASE
  const auto *backend=nice_gfx::detail::DisplayAccess::panel(display);
  if(!project||surfaces.empty()||snapshotTransition||rasterFrame.preview.page||snapshotInput||snapshotKeyboard.visible||
     softInput.node>=0||suspendedSystemInput||editNode>=0||focusStyleCount||
     buildingSurface||nativeCacheRepairing||foregroundCapture||groupMotion.active||backPrepared||backOffset||menuMoving()||
     !backend||!backend->supportsFrameBlit()){
    invalidateSystemModalBaseLive();return;
  }
  const auto &p=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;
  uint16_t root=NoParent,bar=NoParent;bool closingFinal=false;
  for(const auto &pair:systemChromePairs)if(pair.page==history.back()){
    for(const auto &visual:behaviorVisuals)if(visual.root==pair.panel&&visual.kind==BehaviorKind::Modal&&visual.motion==DialogMotion::Spring){root=pair.panel;bar=pair.bar;break;}
    if(root!=NoParent)break;
  }
  // completeBehaviorClose hides the root and restores host input/focus before
  // requesting its terminal frame. Transfer the existing closed-host pin to
  // that accepted frame before dropping the lifecycle pin; never acquire a
  // new source for this exception or bypass the common restored-input guards.
  if(root==NoParent&&behaviorVisuals.empty()&&systemModalBaseSurface(systemModalBaseLive)&&systemModalBaseLive.page==history.back())
    for(const auto &pair:systemChromePairs)if(pair.page==history.back()&&pair.panel==systemModalBaseLive.root&&pair.bar==systemModalBaseLive.bar&&
        (values[pair.panel].flags&Hidden)){root=pair.panel;bar=pair.bar;closingFinal=true;break;}
  if(root==NoParent||(!closingFinal&&behaviorVisuals.size()!=1)||p.nodes[root].parent!=0||p.nodes[bar].parent!=0||p.nodes[0].parent!=NoParent||
     p.nodes[0].kind!=Kind::Container||p.nodes[root].kind!=Kind::Container||p.nodes[bar].kind!=Kind::Container){invalidateSystemModalBaseLive();return;}
  for(uint16_t i:{uint16_t(0),root,bar}){
    const auto &v=values[i];const auto &n=p.nodes[i];const auto &style=styleFor(p,values,i);
    const uint16_t unavailable=Transparent|Disabled|((closingFinal&&i==root)?0:Hidden);
    if(v.kind!=Kind::Container||(v.flags&unavailable)||v.opacity!=255||style.radius||style.gradient||style.borderWidth||
       n.motion||focusStyle(i)||(i==root&&(n.animation.effect!=AnimationEffect::None||n.exitAnimation))){invalidateSystemModalBaseLive();return;}
  }
  if(snapshotPressed>=0){int at=snapshotPressed;for(;at>=0&&at!=root&&at!=bar;at=values[at].parent==NoParent?-1:values[at].parent){}if(at<0){invalidateSystemModalBaseLive();return;}}
  auto *surface=systemModalBaseSurface(systemModalBaseLive);
  if(closingFinal){
    if(surface&&validSystemModalBase(*surface,root,bar,systemModalBaseLiveKinds)){
      systemModalBaseFrame=systemModalBaseLive;systemModalBaseFrameKinds=systemModalBaseLiveKinds;systemModalBaseClosingFinal=true;
    }
    else if(surface&&queueSystemModalBaseTextRepair(*surface,root,bar,true))return;
    invalidateSystemModalBaseLive();return;
  }
  const bool matchesLive=surface&&systemModalBaseLive.root==root&&systemModalBaseLive.bar==bar&&systemModalBaseLive.page==history.back();
  const bool validLive=matchesLive&&validSystemModalBase(*surface,root,bar,systemModalBaseLiveKinds);
  if(matchesLive&&behaviorVisualMoving()&&!validLive&&
     queueSystemModalBaseTextRepair(*surface,root,bar,false))return;
  if(!validLive){
    invalidateSystemModalBaseLive();systemModalBaseLiveKinds.resize(p.count);
    for(uint16_t i=0;i<p.count;++i)systemModalBaseLiveKinds[i]=i==root?2:i==bar?1:values[i].parent==NoParent?0:systemModalBaseLiveKinds[values[i].parent];
    surface=nullptr;
    for(size_t slot=0;slot<surfaces.size();++slot)if(validSystemModalBase(surfaces[slot],root,bar,systemModalBaseLiveKinds)){
      systemModalBaseLive={slot,uint16_t(history.back()),root,bar,frameSourceOwner,surfaces[slot].pixelsRevision,false};surface=&surfaces[slot];++stats.systemModalBaseAcquisitions;break;
    }
    // A cold retained host may differ only in live text. Reuse the same
    // full-node text-repair admission and preferred-slot strict revalidation
    // as an existing live lease, before any cached pixels become drawable.
    if(!surface&&behaviorVisualMoving())
      for(size_t slot=0;slot<surfaces.size();++slot)if(queueSystemModalBaseTextRepair(surfaces[slot],root,bar,false)){
        systemModalBaseLive={slot,uint16_t(history.back()),root,bar,frameSourceOwner,surfaces[slot].pixelsRevision,false};return;
      }
  }
  if(surface&&behaviorVisualMoving()){
    systemModalBaseFrame=systemModalBaseLive;systemModalBaseFrameKinds=systemModalBaseLiveKinds;
  }
#endif
}
Status paintSystemModalBase(const DirtyRect &area,bool &painted,bool &backdropFused,Rect &backdropRemainder) {
  painted=false;backdropFused=false;
#if NICE_GFX_UI_SYSTEM_MODAL_BASE_LEASE
  auto *surface=systemModalBaseSurface(systemModalBaseFrame);
  if(!surface||systemModalBaseFrameSuppressed||buildingSurface||foregroundCapture||nativeCacheRepairing||snapshotTransition||rasterFrame.preview.page||
     rasterFrame.current.page!=surface->page||!paintCache||systemModalBaseFrame.root>=paintCache->nodes.size())return Status::Ok;
  if(renderOrder==PixelOrder::Native){
    if(!nativeOverlayTarget||stagingNativeTarget||!canvas.targetsNativeFramebuffer(*nativeOverlayTarget))return Status::Ok;
    const uintptr_t address=reinterpret_cast<uintptr_t>(nativeOverlayTarget->pixels);
    if(address>UINTPTR_MAX-nativeOverlayTarget->bytes||nativeMemoryOverlap(address,nativeOverlayTarget->bytes,surface->pixels,size_t(surface->page->width)*surface->page->height*2)){
      suppressSystemModalBaseFrame();return Status::Ok;
    }
  }
  const auto &panel=paintCache->nodes[systemModalBaseFrame.root];const auto &bar=surface->geometry.nodes[systemModalBaseFrame.bar];
  Rect box{};
  if(systemModalBaseClosingFinal){
    if(panel.visible||!(rasterFrame.current.values[systemModalBaseFrame.root].flags&Hidden))return Status::Ok;
  }else{
    if(!panel.visible||panel.alpha!=255||panel.rect.x||panel.rect.width!=surface->page->width||panel.rect.height!=surface->page->height||
       !modalBaseSameRect(behaviorPaintRect(systemModalBaseFrame.root,panel.rect),panel.rect))return Status::Ok;
    box=intersect(panel.rect,panel.clip);
  }
  // strips() requires its cover to be contained in the area being split.
  // In particular, a top bar outside an exposed bottom strip must not expand
  // that strip back to the full host below the bar.
  const Rect requested{area.x,area.y,area.width,area.height};
  Rect strips[4];const int count=raster_occlusion::strips(requested,intersect(box,requested),strips);
  const auto width=surface->page->width,height=surface->page->height;
  bool fuseDim=false;uint8_t fusedDim=0;
#if NICE_GFX_UI_SYSTEM_MODAL_FUSED_DIM
  // Only one opaque system panel survives this path. The bar is deliberately
  // painted later from current state; leave that rectangle for the finisher.
  // Focus/footer/keyboard and nested overlays keep the ordinary final dim.
  if(renderOrder==PixelOrder::Native&&!systemModalBaseClosingFinal&&!snapshotInput&&
     (snapshotGroupIndex<0||!snapshotGroup.indicatorHeight)&&behaviorVisuals.size()==1&&
     modalBaseContains({0,0,width,height},requested)&&
     modalBaseSameRect(intersect(panel.rect,panel.clip),intersect(panel.rect,{0,0,width,height}))){
    const auto &visual=behaviorVisuals[0];size_t targetWords=0,sourceWords=0;
    if(visual.root==systemModalBaseFrame.root&&visual.dim&&modalBaseSameRect(visual.visual,panel.rect)&&
       detail::nativeWireCopyFootprint(nativeOverlayTarget->pixels,nativeOverlayTarget->bytes/2,nativeOverlayTarget->stride,width,height,targetWords)&&
       detail::nativeWireCopyFootprint(surface->pixels,size_t(width)*height,width,width,height,sourceWords)){
      fuseDim=true;fusedDim=visual.dim;
    }
  }
#endif
  const uint32_t started=profilingClock?profilingClock():0;
  // The panel body has its own complete source/target admission. Its DMA
  // may overlap host RLE validation, which still finishes before CPU RLE writes.
  struct PanelOverlapScope {
    Impl &self;bool active=false;
    explicit PanelOverlapScope(Impl &owner):self(owner){}
    Status finish(){
      if(!active)return Status::Ok;
      const Status status=self.finishSystemModalPanelOverlap();
      if(status!=Status::Busy)active=false;
      return status;
    }
    ~PanelOverlapScope(){finish();}
  } overlap(*this);
#if NICE_GFX_UI_SYSTEM_MODAL_RLE_DIM
  Rect parallelRoi{};const Rect *parallelRoiHint=nullptr;
  // Exclude the whole compressed bank before any speculative DMA write,
  // including cases whose later record/geometry/tail validation will reject.
  if(fuseDim&&count==1&&compressedStorage&&compressedCapacity&&compressedCapacity<=SIZE_MAX/2){
    const uintptr_t bank=reinterpret_cast<uintptr_t>(compressedStorage),output=reinterpret_cast<uintptr_t>(nativeOverlayTarget->pixels);
    const size_t bankBytes=compressedCapacity*2;
    if(!(bank%alignof(uint16_t))&&bank<=UINTPTR_MAX-bankBytes&&output<=UINTPTR_MAX-nativeOverlayTarget->bytes&&
       !nativeMemoryOverlap(output,nativeOverlayTarget->bytes,compressedStorage,bankBytes)){
      Rect pieces[4];const int pieceCount=raster_occlusion::strips(strips[0],intersect(bar.rect,strips[0]),pieces);
      if(pieceCount==1){
        // This is the actual host ROI after panel and covered-bar exclusion.
        // Partial paint calls retain the original single-seed path.
        if(modalBaseSameRect(requested,{0,0,width,height})&&pieces[0].x==0&&pieces[0].width==width&&pieces[0].height>=128){
          parallelRoi=pieces[0];parallelRoiHint=&parallelRoi;
        }
        const Status submitted=beginSystemModalPanelOverlap(area,pieces[0],bar.rect);
        if(submitted==Status::Ok)overlap.active=true;
        else if(submitted!=Status::Unsupported)return submitted;
      }
    }
  }
#endif
  SystemModalBaseRlePlan rlePlan;bool rleReady=false;
#if NICE_GFX_UI_SYSTEM_MODAL_RLE_DIM
  if(fuseDim)rleReady=prepareSystemModalBaseRlePlan(*surface,*nativeOverlayTarget,rlePlan,parallelRoiHint);
#endif
  if(!rleReady){
    const Status overlapStatus=overlap.finish();if(overlapStatus!=Status::Ok)return overlapStatus;
  }
  for(int part=0;part<count;++part){Rect pieces[4];const int piecesCount=raster_occlusion::strips(strips[part],intersect(bar.rect,strips[part]),pieces);
    for(int piece=0;piece<piecesCount;++piece){const auto &r=pieces[piece];Status status=Status::Unsupported;
      if(fuseDim){
        const size_t outputOffset=size_t(r.y)*nativeOverlayTarget->stride+size_t(r.x),inputOffset=size_t(r.y)*width+size_t(r.x);
        // Unsupported is established before any failed GPU write. Driver
        // errors abort this frame and must never be hidden by a CPU rewrite.
        const Status rleStatus=rleReady?copyDimSystemModalRleRows(*surface,*nativeOverlayTarget,r,rlePlan,fusedDim):Status::Unsupported;
        if(rleStatus!=Status::Ok){
          const Status overlapStatus=overlap.finish();if(overlapStatus!=Status::Ok)return overlapStatus;
          if(rleStatus!=Status::Unsupported)return rleStatus;
          if(!detail::copyDimSystemModalRows(nativeOverlayTarget->pixels+outputOffset,nativeOverlayTarget->bytes/2-outputOffset,nativeOverlayTarget->stride,
               surface->pixels+inputOffset,size_t(width)*height-inputOffset,width,size_t(r.width),size_t(r.height),cacheOrder,fusedDim))return Status::InvalidArgument;
        }
        status=Status::Ok;stats.behaviorBackdropPixels+=uint64_t(r.width)*r.height;stats.behaviorBackdropSpans+=r.height;
      }
      if(status==Status::Unsupported)status=copySystemModalMappedRows(
        PixelView(surface->pixels,width,height,width,cacheOrder),size_t(width)*height*2,r,size_t(r.x),size_t(r.y));
      if(status==Status::Unsupported&&renderOrder==PixelOrder::Native&&ppaBlendEnabled&&rgb565_ppa::Context::available())
#if NICE_GFX_UI_SYSTEM_MODAL_ROW_COPY
        status=detail::copySystemModalNativeRows(ppaFade,*nativeOverlayTarget,PixelView(surface->pixels,width,height,width,cacheOrder),size_t(width)*height*2,r);
#else
        status=ppaFade.copyIntoNative(*nativeOverlayTarget,r.x,r.y,PixelView(surface->pixels,width,height,width,cacheOrder),size_t(width)*height*2,r.x,r.y,r.width,r.height);
#endif
      if(status==Status::Unsupported)status=canvas.blit(r.x,r.y,PixelView(surface->pixels+size_t(r.y)*width+r.x,r.width,r.height,width,cacheOrder));
      if(status!=Status::Ok)return status;
      ++stats.systemModalBaseCopyCalls;stats.systemModalBaseCopyPixels+=uint64_t(r.width)*r.height;
    }
  }
  const Status overlapStatus=overlap.finish();if(overlapStatus!=Status::Ok)return overlapStatus;
  if(!systemModalBaseFramePainted){systemModalBaseFramePainted=true;++stats.systemModalBaseFrames;if(systemModalBaseClosingFinal)++stats.systemModalBaseClosingFrames;}
  if(profilingClock)stats.systemModalBaseCopyMicros+=profilingClock()-started;
  painted=true;backdropFused=fuseDim;backdropRemainder=bar.rect;
#else
  (void)area;
#endif
  return Status::Ok;
}
