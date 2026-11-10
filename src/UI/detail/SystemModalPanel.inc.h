// Included inside View::Impl. Borrow one existing raw page slot for an opaque
// system panel at its authored pose; no extra framebuffer is allocated.
struct SystemModalPanelLease {
  size_t slot=SIZE_MAX;uint16_t page=NoParent,root=NoParent;
  uint64_t owner=0,revision=0;
  Rect target{};bool clearAfterFrame=false;
  bool sharedSource=false;uint16_t hostPage=NoParent,handle=NoParent;
};
SystemModalPanelLease systemModalPanelLive,systemModalPanelFrame;
bool systemModalPanelBuilding=false;
#ifndef NICE_GFX_UI_SYSTEM_MODAL_DEFER_COLD_CAPTURE
#define NICE_GFX_UI_SYSTEM_MODAL_DEFER_COLD_CAPTURE NICE_GFX_P4_SYSTEM_MODAL_ACCELERATION_DEFAULT
#endif
#if NICE_GFX_UI_SYSTEM_MODAL_DEFER_COLD_CAPTURE && NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE
// One opening may defer one cold capture until after real visible feedback.
// The accepted frame keeps its own epoch: close/reopen while it is pending
// must not consume the next opening's allowance. Cache misses do not rearm it.
struct SystemModalPanelColdFeedback {
  uint64_t owner=0,epoch=0;uint16_t page=NoParent;Id root=0;
};
SystemModalPanelColdFeedback systemModalPanelColdLive,systemModalPanelColdFrame;
uint64_t systemModalPanelColdEpoch=0;
#endif
void beginSystemModalPanelColdFeedback(uint16_t page,Id root){
#if NICE_GFX_UI_SYSTEM_MODAL_DEFER_COLD_CAPTURE && NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE
  if(!++systemModalPanelColdEpoch)++systemModalPanelColdEpoch;
  systemModalPanelColdLive={frameSourceOwner,systemModalPanelColdEpoch,page,root};
#else
  (void)page;(void)root;
#endif
}
void resetSystemModalPanelColdFeedback(uint16_t page,Id root){
#if NICE_GFX_UI_SYSTEM_MODAL_DEFER_COLD_CAPTURE && NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE
  if(systemModalPanelColdLive.page==page&&systemModalPanelColdLive.root==root)systemModalPanelColdLive={};
#else
  (void)page;(void)root;
#endif
}
void clearSystemModalPanelColdFeedback(){
#if NICE_GFX_UI_SYSTEM_MODAL_DEFER_COLD_CAPTURE && NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE
  systemModalPanelColdLive={};systemModalPanelColdFrame={};
#endif
}
void releaseSystemModalPanelColdFeedbackFrame(){
#if NICE_GFX_UI_SYSTEM_MODAL_DEFER_COLD_CAPTURE && NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE
  systemModalPanelColdFrame={};
#endif
}
void markSystemModalPanelColdFeedbackFrame(){
#if NICE_GFX_UI_SYSTEM_MODAL_DEFER_COLD_CAPTURE && NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE
  systemModalPanelColdFrame={};const auto &live=systemModalPanelColdLive;
  if(!live.epoch)return;
  if(!project||live.owner!=frameSourceOwner||live.page>=project->count||history.empty()||live.page!=history.back()||
     rasterFrame.current.page!=&project->pages[live.page]){systemModalPanelColdLive={};return;}
  const auto &p=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;
  for(const auto &visual:behaviorVisuals)if(visual.root>=0&&size_t(visual.root)<p.count&&size_t(visual.root)<values.size()&&
      p.nodes[visual.root].id==live.root&&!(values[visual.root].flags&Hidden)&&
      visual.kind==BehaviorKind::Modal&&visual.motion==DialogMotion::Spring){
    const Rect visible=intersect(visual.visual,{0,0,p.width,p.height});
    if(visible.width>0&&visible.height>0)systemModalPanelColdFrame=live;
    return;
  }
#endif
}
bool deferSystemModalPanelColdCapture(uint16_t root)const{
#if NICE_GFX_UI_SYSTEM_MODAL_DEFER_COLD_CAPTURE && NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE
  const auto &live=systemModalPanelColdLive,&frame=systemModalPanelColdFrame;
  return live.epoch&&frame.epoch==live.epoch&&frame.owner==frameSourceOwner&&frame.owner==live.owner&&
    frame.page==live.page&&frame.root==live.root&&rasterFrame.current.page&&root<rasterFrame.current.page->count&&
    rasterFrame.current.page->nodes[root].id==frame.root;
#else
  (void)root;return false;
#endif
}
void completeSystemModalPanelColdFeedbackFrame(){
#if NICE_GFX_UI_SYSTEM_MODAL_DEFER_COLD_CAPTURE && NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE
  const auto &frame=systemModalPanelColdFrame;const auto &live=systemModalPanelColdLive;
  if(frame.epoch&&frame.epoch==live.epoch&&frame.owner==live.owner&&frame.page==live.page&&frame.root==live.root)
    systemModalPanelColdLive={};
  systemModalPanelColdFrame={};
#endif
}
// A completed early body copy owns no pixels. The existing accepted source
// and target frame remain pinned until the ordinary paint/release sequence.
struct SystemModalPanelOverlap {
  const uint16_t *source=nullptr,*output=nullptr;
  uint64_t owner=0,revision=0;
  Rect area{},pose{},body{},head{};
  uint16_t root=NoParent;
  bool active=false,completed=false;
};
SystemModalPanelOverlap systemModalPanelOverlap;
#ifndef NICE_GFX_UI_SYSTEM_MODAL_COPY_OVERLAP
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_SYSTEM_MODAL_COPY_OVERLAP 1
#else
#define NICE_GFX_UI_SYSTEM_MODAL_COPY_OVERLAP 0
#endif
#endif
#ifndef NICE_GFX_UI_SYSTEM_PANEL_PAGE_SOURCE
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_SYSTEM_PANEL_PAGE_SOURCE 1
#else
#define NICE_GFX_UI_SYSTEM_PANEL_PAGE_SOURCE 0
#endif
#endif
#include "SystemModalPanelSource.inc.h"

bool systemModalPanelPin(const SystemModalPanelLease &lease,const Surface &surface)const{
  return lease.owner==frameSourceOwner&&lease.slot<surfaces.size()&&&surfaces[lease.slot]==&surface;
}
bool systemModalPanelPinned(const Surface &surface)const{
  return systemModalPanelPin(systemModalPanelLive,surface)||systemModalPanelPin(systemModalPanelFrame,surface);
}
Surface *systemModalPanelSurface(const SystemModalPanelLease &lease){
  if(!project||lease.owner!=frameSourceOwner||lease.slot>=surfaces.size()||lease.page>=project->count)return nullptr;
  auto &surface=surfaces[lease.slot];
  return surface.ready&&surface.page==&project->pages[lease.page]&&surface.pixelsRevision==lease.revision?&surface:nullptr;
}
void invalidateSystemModalPanelLive(){
  const bool leaseShared=systemModalPanelLive.sharedSource;
  auto *source=systemModalPanelSurface(systemModalPanelLive);systemModalPanelLive=SystemModalPanelLease{};
  // The normalized key is private panel content, never an ordinary route or
  // compressed cold-page candidate. Release its raw slot after its last reader.
  if(source&&!systemModalPanelBuilding&&!systemModalPanelPin(systemModalPanelFrame,*source)&&!leaseShared)discardWarmSurface(*source);
}
void pruneSystemModalPanelLive(){
  const auto root=systemModalPanelLive.root;
  if(!systemModalPanelSurface(systemModalPanelLive)||history.empty()||(systemModalPanelLive.sharedSource?systemModalPanelLive.hostPage:systemModalPanelLive.page)!=history.back()||
     !rasterFrame.current.page||root>=rasterFrame.current.values.size()||(rasterFrame.current.values[root].flags&Hidden)||
     snapshotTransition||rasterFrame.preview.page){invalidateSystemModalPanelLive();return;}
  bool found=false;for(const auto &visual:behaviorVisuals)if(visual.root==root&&visual.kind==BehaviorKind::Modal&&visual.motion==DialogMotion::Spring){found=true;break;}
  if(!found)invalidateSystemModalPanelLive();
}
void releaseSystemModalPanelFrame(){
  systemModalPanelOverlap=SystemModalPanelOverlap{};
  const bool leaseShared=systemModalPanelFrame.sharedSource;
  auto *source=systemModalPanelSurface(systemModalPanelFrame);
  const PageDesc *clear=nullptr;
  if(systemModalPanelFrame.clearAfterFrame&&project&&systemModalPanelFrame.page<project->count)clear=&project->pages[systemModalPanelFrame.page];
  systemModalPanelFrame=SystemModalPanelLease{};
  if(source&&!systemModalPanelPin(systemModalPanelLive,*source)&&!leaseShared)discardWarmSurface(*source);
  if(clear)clearSurfacePage(clear);
}
void clearSystemModalPanels(){systemModalPanelLive=systemModalPanelFrame=SystemModalPanelLease{};systemModalPanelOverlap=SystemModalPanelOverlap{};systemModalPanelBuilding=false;clearSystemModalPanelColdFeedback();}
bool systemModalPanelFrameNode(uint16_t node)const{
  return systemModalPanelFrame.owner==frameSourceOwner&&systemModalPanelFrame.root!=NoParent&&
    (systemModalPanelFrame.sharedSource?(node>=systemModalPanelFrame.root&&node<systemModalPanelFrame.handle):behaviorSnapshotDescendant(node,systemModalPanelFrame.root));
}
bool sameSystemModalPanelNode(uint16_t index,const State &cached,const State &current,uint16_t root)const{
  if(index!=root)return sameSurfaceNode(cached,current);
  State normalized=current;normalized.animationY=0;return sameSurfaceNode(cached,normalized);
}
void prepareSystemModalPanelFrame(const nice_gfx::NativeFramebuffer *target,bool warmOnly=false){
  releaseSystemModalPanelFrame();
#if NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE
  const auto reject=[&](){if(!warmOnly)invalidateSystemModalPanelLive();};
  const auto *backend=nice_gfx::detail::DisplayAccess::panel(display);
  if(!project||surfaces.size()<2||renderBudget.maxPixelsPerTick||(!warmOnly&&(!target||!target->pixels))||!backend||backend->busy()||
     !backend->supportsFrameBlit()||refresh->busy()||snapshotTransition||rasterFrame.preview.page||snapshotInput||snapshotKeyboard.visible||
     snapshotPressed>=0||softInput.node>=0||suspendedSystemInput||editNode>=0||focusStyleCount||
     buildingSurface||nativeCacheRepairing||foregroundCapture||groupMotion.active||backPrepared||backOffset||menuMoving()||
     behaviorVisuals.size()!=1){reject();return;}
  const auto &p=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;
  const auto &visual=behaviorVisuals.front();const int root=visual.root;
  if(root<=0||size_t(root)>=values.size()||visual.kind!=BehaviorKind::Modal||visual.motion!=DialogMotion::Spring||visual.reveal!=1024||
     p.nodes[root].parent!=0||values[root].parent!=0||p.nodes[root].kind!=Kind::Container||values[root].kind!=Kind::Container||
     !std::binary_search(systemPanelRoots.begin(),systemPanelRoots.end(),p.nodes[root].id)||
     !(values[root].flags&Clip)||values[root].animationX||visual.full.x||visual.full.y||visual.full.width!=p.width||visual.full.height!=p.height||
     p.nodes[root].motion||p.nodes[root].animation.effect!=AnimationEffect::None||p.nodes[root].exitAnimation||
     p.nodes[0].parent!=NoParent||values[0].parent!=NoParent||values[0].animationX||values[0].animationY||p.nodes[0].motion){reject();return;}
  const auto &geometry=rasterFrame.current.geometry;
  if(geometry.nodes.size()!=p.count){reject();return;}
  const auto &panel=geometry.nodes[root];const auto &style=styleFor(p,values,root);
  if(!panel.visible||panel.alpha!=255||panel.rect.x||panel.rect.width!=p.width||panel.rect.height!=p.height||
     panel.rect.y!=values[root].animationY||!modalBaseSameRect(behaviorPaintRect(root,panel.rect),panel.rect)||
     !modalBaseContains(panel.clip,intersect(panel.rect,{0,0,p.width,p.height}))||
     (values[root].flags&(Transparent|Disabled|Hidden))||style.radius||style.gradient||style.borderWidth){reject();return;}
  const size_t bytes=size_t(p.width)*p.height*2;
  if(target&&target->bytes>UINTPTR_MAX-reinterpret_cast<uintptr_t>(target->pixels)){reject();return;}
  const uint16_t layer=viewportOverlayDepth(p,values,root);
  for(uint16_t i=0;i<p.count;++i)if(behaviorSnapshotDescendant(i,root)){
    // Images supplied as raw caller pointers have no content revision; nested
    // viewport overlays can escape the panel's translated opaque rectangle.
    if(values[i].imageOverride||focusStyle(i)||viewportOverlayDepth(p,values,i)!=layer){reject();return;}
    if(p.nodes[i].kind==Kind::Media){const auto *source=media(p.nodes[i],values[i]);if(source&&source->frames>1){reject();return;}}
  }
#if NICE_GFX_UI_SYSTEM_PANEL_PAGE_SOURCE
  if(!warmOnly&&prepareSystemPanelPageSource(*target,uint16_t(root)))return;
#endif
  // A null target is only a non-writing lookup. Shared source-page capture
  // retains its existing target-aware validation on the normal Native path.
  if(warmOnly&&systemModalPanelLive.sharedSource)return;
  Surface *source=systemModalPanelSurface(systemModalPanelLive);
  if(!source||systemModalPanelLive.page!=history.back()||systemModalPanelLive.root!=root||source->key.size()!=values.size()){
    if(warmOnly)return;
    invalidateSystemModalPanelLive();source=nullptr;
  }
  if(source&&target&&nativeMemoryOverlap(reinterpret_cast<uintptr_t>(target->pixels),target->bytes,source->pixels,bytes)){
    reject();return;
  }
  bool changed=!source;
  if(source)for(uint16_t i=0;i<p.count;++i)if(behaviorSnapshotDescendant(i,root)&&!sameSystemModalPanelNode(i,source->key[i],values[i],uint16_t(root))){changed=true;break;}
  if(source&&!changed&&componentCount)for(uint16_t i=0;i<p.count;++i)if(behaviorSnapshotDescendant(i,root)&&p.nodes[i].kind==Kind::Button){
    const auto before=componentPaint(p,source->key,p.nodes[i].id),current=componentPaint(p,values,p.nodes[i].id);
    if(before.selected!=current.selected||before.expanded!=current.expanded){changed=true;break;}
  }
  if(warmOnly&&changed)return;
  // The first visible Native response paints the live clipped panel instead
  // of first rasterizing its entire normalized source. Only completion consumes
  // this opening's allowance; warm lookup and changed ready-source repair do not.
  if(!warmOnly&&!source&&deferSystemModalPanelColdCapture(uint16_t(root)))return;
  if(changed){
    RetainedVector<State> normalized=source?source->key:values;
    for(uint16_t i=0;i<p.count;++i){
      normalized[i]=values[i];
      if(i&&!behaviorSnapshotDescendant(i,root))normalized[i].flags|=Hidden;
    }
    normalized[root].animationY=0;
    if(!source){source=acquireFrameSurface(snapshotSurface,previewSurface,snapshotAnimationSurface,coldFadeSurface,seedSurface);if(!source)return;}
    const uintptr_t address=reinterpret_cast<uintptr_t>(source->pixels);
    if(address>UINTPTR_MAX-bytes||nativeMemoryOverlap(reinterpret_cast<uintptr_t>(target->pixels),target->bytes,source->pixels,bytes)||
       nativeSourceOverlap(p,normalized,address,bytes,true)){invalidateSystemModalPanelLive();return;}
    const bool repair=source->ready;Surface *complete=nullptr;
    {
      struct Scope{
        Impl &s;RetainedVector<State> values;FrameCache geometry;std::vector<BehaviorVisual> visuals;FrameCache *paint;bool building;int group;
        Scope(Impl &owner,RetainedVector<State> &normalized):s(owner),paint(s.paintCache),building(s.systemModalPanelBuilding),group(s.snapshotGroupIndex){
          values.swap(s.rasterFrame.current.values);s.rasterFrame.current.values.swap(normalized);
          std::swap(geometry,s.rasterFrame.current.geometry);visuals.swap(s.behaviorVisuals);s.systemModalPanelBuilding=true;
          // Navigation chrome is painted after the translated panel. Keep it
          // out of the panel-only source, including the portable fallback.
          s.snapshotGroupIndex=-1;
        }
        ~Scope(){values.swap(s.rasterFrame.current.values);std::swap(geometry,s.rasterFrame.current.geometry);visuals.swap(s.behaviorVisuals);s.paintCache=paint;s.systemModalPanelBuilding=building;s.snapshotGroupIndex=group;}
      } scope(*this,normalized);
      invalidateSystemModalPanelLive();
      if(repair){bool prepared=false;complete=patchSurface(prepared,0,source);}
      else{
        prepareCache(p,rasterFrame.current.values,rasterFrame.current.geometry);
        const auto &g=rasterFrame.current.geometry.nodes[root];
        const Rect full{0,0,p.width,p.height};
        if(g.visible&&g.alpha==255&&modalBaseSameRect(g.rect,full)&&modalBaseContains(g.clip,full)){
          const uint64_t tiles=stats.renderedTiles,pixels=stats.renderedPixels;
          const bool building=buildingSurface;buildingSurface=true;
          NativeFramebuffer borrowedFrame{};const NativeFramebuffer *borrowed=nullptr;
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR
          const size_t pitch=(size_t(p.width)+31)&~size_t(31);
          // Retained readers can coexist with capture in different raw slots.
          // Exclude their actual slots instead of globally rejecting the frame.
          if(cacheOrder==PixelOrder::Wire&&surfaces.size()>=3&&
             pitch==p.width&&!(size_t(p.width)*2&63u)&&nativeCacheScratch.bytes<pitch*size_t(p.height)*2){
            if(auto *scratch=acquireFrameSurface(source,snapshotSurface,previewSurface,snapshotAnimationSurface,coldFadeSurface,seedSurface)){
              borrowedFrame={scratch->pixels,p.width,p.height,pitch,size_t(p.width)*p.height*2};
              if(validBorrowedNativeCacheScratch(*source,borrowedFrame,pitch,target))borrowed=&borrowedFrame;
            }
          }
#endif
          // The borrowed slot stays page=null / ready=false. Neither the
          // renderer nor this optional target takes ownership or publishes it.
          const uint32_t nativeStarted=profilingClock?profilingClock():0;
          Status result=renderNativeCachePatch(*source,DirtyRect(0,0,p.width,p.height),borrowed);
          if(result==Status::Ok){
            ++stats.nativeCaptureCalls;stats.nativeCapturePixels+=uint64_t(p.width)*p.height;
            if(profilingClock)stats.nativeCaptureMicros+=profilingClock()-nativeStarted;
          }
          // buildingSurface's portable painter emits Wire565. A Native cache
          // must retain the direct native helper's result or fall back live.
          if(result==Status::Unsupported&&cacheOrder==PixelOrder::Wire)result=render(this,DirtyRect(0,0,p.width,p.height),source->pixels,p.width);
          buildingSurface=building;stats.renderedTiles=tiles;stats.renderedPixels=pixels;
          if(result==Status::Ok){
            source->page=&p;source->key=rasterFrame.current.values;source->geometry=rasterFrame.current.geometry;source->row=p.height;source->column=0;
            source->ready=true;source->used=++surfaceClock;releaseSurfaceText(*source);++surfaceStats.capturedPages;complete=source;
          }else{source->ready=false;source->page=nullptr;source->key.clear();}
        }
      }
    }
    if(!complete){invalidateSystemModalPanelLive();if(source&&!systemModalPanelPinned(*source))discardWarmSurface(*source);return;}
    // Never let a normalized panel-only image replace the host's cold record.
    complete->compressionAttempted=true;
    systemModalPanelLive={size_t(complete-surfaces.data()),uint16_t(history.back()),uint16_t(root),frameSourceOwner,complete->pixelsRevision,panel.rect,false};
  }
  if(systemModalPanelSurface(systemModalPanelLive)){
    systemModalPanelFrame=systemModalPanelLive;systemModalPanelFrame.target=panel.rect;
  }
#else
  (void)target;(void)warmOnly;
#endif
}
// The caller excludes the full raw/encoded banks from the target. This call
// independently proves the panel source, geometry and cropped DMA footprint.
// Host RLE validation finishes before CPU RLE writes. Rejection drains this
// body before any fallback painter; a validated host writes disjoint rows.
Status beginSystemModalPanelOverlap(const DirtyRect &area,Rect host,Rect bar){
#if NICE_GFX_UI_SYSTEM_MODAL_COPY_OVERLAP
#if defined(NICE_GFX_UI_SYSTEM_MODAL_RLE_FILL) && NICE_GFX_UI_SYSTEM_MODAL_RLE_FILL
  // The overlapping phase is CPU-only. An explicit GPU backdrop-fill build
  // keeps the established serial ordering between accelerator operations.
  (void)area;(void)host;(void)bar;return Status::Unsupported;
#else
  if(systemModalPanelOverlap.active||systemModalPanelOverlap.completed||!project||
     !nativeOverlayTarget||!ppaBlendEnabled||!rgb565_ppa::Context::available()||
     renderOrder!=PixelOrder::Native||paintOffsetX||paintOffsetY||stagingNativeTarget||
     !canvas.targetsNativeFramebuffer(*nativeOverlayTarget)||systemModalBaseClosingFinal||
     systemModalPanelFrame.owner!=frameSourceOwner||systemModalPanelFrame.root!=systemModalBaseFrame.root||
     (systemModalPanelFrame.sharedSource?systemModalPanelFrame.hostPage:systemModalPanelFrame.page)!=systemModalBaseFrame.page)return Status::Unsupported;
  auto *source=systemModalPanelSurface(systemModalPanelFrame);
  if(!source||!source->page||!source->pixels||systemModalPanelBuilding||buildingSurface||
     foregroundCapture||nativeCacheRepairing||snapshotTransition||rasterFrame.preview.page)return Status::Unsupported;
  const auto &p=*source->page;const auto pose=systemModalPanelFrame.target;
  const Rect requested{area.x,area.y,area.width,area.height};
  if(!p.width||!p.height||size_t(p.height)>SIZE_MAX/size_t(p.width)/2||
     !modalBaseSameRect(requested,{0,0,p.width,p.height})||
     !modalBaseSameRect(pose,paintCache->nodes[systemModalBaseFrame.root].rect)||
     pose.x||pose.width!=p.width||pose.height!=p.height||pose.y>=0||
     bar.x||bar.y||bar.width!=p.width||bar.height<=0||
     host.x||host.width!=p.width||host.y<=bar.height||host.height<=0||
     int64_t(host.y)+host.height!=p.height||int64_t(pose.y)+pose.height!=host.y)return Status::Unsupported;
  const Rect body{0,bar.height,p.width,host.y-bar.height};
  const size_t bytes=size_t(p.width)*p.height*2,sourceY=size_t(body.y-pose.y);
  const PixelView full(source->pixels,p.width,p.height,p.width,cacheOrder);
  // Whole-picture proof comes first. Cropping is essential: SDK cache sync
  // for the DMA output must never touch the concurrently written host rows.
  if(!ppaFade.canCopyIntoNative(*nativeOverlayTarget,body.x,body.y,full,bytes,
      0,sourceY,size_t(body.width),size_t(body.height)))return Status::Unsupported;
  const size_t inputOffset=sourceY*size_t(p.width)*2;
  if(inputOffset&63u)return Status::Unsupported;
  NativeFramebuffer rows=*nativeOverlayTarget;
  const size_t outputOffset=size_t(body.y)*rows.stride;
  rows.pixels+=outputOffset;rows.height=size_t(body.height);rows.bytes-=outputOffset*2;
  const PixelView input(full.data+inputOffset,p.width,size_t(body.height),p.width,cacheOrder);
  const size_t inputBytes=size_t(p.width)*size_t(body.height)*2;
  // canCopyIntoNative proves the cropped base and final row are cache-line
  // aligned. Entire source and output rows are independently owned here.
  if(!ppaFade.canCopyIntoNative(rows,0,0,input,inputBytes,0,0,size_t(body.width),size_t(body.height)))return Status::Unsupported;
#ifdef NICE_GFX_UI_SYSTEM_MODAL_OVERLAP_TEST_BEGIN
  const Status status=NICE_GFX_UI_SYSTEM_MODAL_OVERLAP_TEST_BEGIN(rows,input,inputBytes);
#else
  const Status status=ppaFade.beginCopyIntoNative(rows,0,0,input,inputBytes,0,0,size_t(body.width),size_t(body.height));
#endif
  if(status!=Status::Ok)return status;
  systemModalPanelOverlap.source=source->pixels;systemModalPanelOverlap.output=nativeOverlayTarget->pixels;
  systemModalPanelOverlap.owner=systemModalPanelFrame.owner;systemModalPanelOverlap.revision=systemModalPanelFrame.revision;
  systemModalPanelOverlap.root=systemModalPanelFrame.root;systemModalPanelOverlap.area=requested;
  systemModalPanelOverlap.pose=pose;systemModalPanelOverlap.body=body;systemModalPanelOverlap.head={0,0,p.width,bar.height};
  systemModalPanelOverlap.active=true;return Status::Ok;
#endif
#else
  (void)area;(void)host;(void)bar;return Status::Unsupported;
#endif
}
Status finishSystemModalPanelOverlap(){
  if(!systemModalPanelOverlap.active)return Status::Ok;
#ifdef NICE_GFX_UI_SYSTEM_MODAL_OVERLAP_TEST_FINISH
  const Status status=NICE_GFX_UI_SYSTEM_MODAL_OVERLAP_TEST_FINISH();
#else
  const Status status=ppaFade.finishCopyIntoNative();
#endif
  if(status==Status::Busy)return status;
  systemModalPanelOverlap.active=false;systemModalPanelOverlap.completed=status==Status::Ok;
  return status;
}
bool completedSystemModalPanelOverlap(const DirtyRect &area,const Surface &source)const{
  const auto &copy=systemModalPanelOverlap;
  return copy.completed&&!copy.active&&nativeOverlayTarget&&copy.owner==frameSourceOwner&&
    copy.owner==systemModalPanelFrame.owner&&copy.revision==systemModalPanelFrame.revision&&
    copy.root==systemModalPanelFrame.root&&copy.source==source.pixels&&copy.output==nativeOverlayTarget->pixels&&
    modalBaseSameRect(copy.area,{area.x,area.y,area.width,area.height})&&
    modalBaseSameRect(copy.pose,systemModalPanelFrame.target);
}

Status paintSystemModalPanel(const DirtyRect &area,bool &painted){
  painted=false;
#if NICE_GFX_UI_SYSTEM_MODAL_PANEL_CACHE
  auto *surface=systemModalPanelSurface(systemModalPanelFrame);
  if(!surface||systemModalPanelBuilding||buildingSurface||nativeCacheRepairing||snapshotTransition||rasterFrame.preview.page||
     renderOrder!=PixelOrder::Native||!nativeOverlayTarget||stagingNativeTarget||
     !canvas.targetsNativeFramebuffer(*nativeOverlayTarget)||paintOffsetX||paintOffsetY)return Status::Ok;
  const auto &p=*surface->page;const size_t bytes=size_t(p.width)*p.height*2;
  if(nativeOverlayTarget->bytes>UINTPTR_MAX-reinterpret_cast<uintptr_t>(nativeOverlayTarget->pixels)||
     nativeMemoryOverlap(reinterpret_cast<uintptr_t>(nativeOverlayTarget->pixels),nativeOverlayTarget->bytes,surface->pixels,bytes)){
    invalidateSystemModalPanelLive();systemModalPanelFrame=SystemModalPanelLease{};return Status::Ok;
  }
  const auto target=systemModalPanelFrame.target;const Rect clipped=intersect(target,{area.x,area.y,area.width,area.height});
  if(clipped.width<=0||clipped.height<=0){painted=true;return Status::Ok;}
  canvas.setViewport(clipped.x,clipped.y,clipped.width,clipped.height,false);
  // Preserve the original bar-then-panel paint order: early DMA copied
  // only the body. This root painter still overwrites the complete head.
  const Rect copyRect=completedSystemModalPanelOverlap(area,*surface)?systemModalPanelOverlap.head:clipped;
  const PixelView input(surface->pixels,p.width,p.height,p.width,cacheOrder);
  Status result=copySystemModalMappedRows(input,bytes,copyRect,size_t(copyRect.x-target.x),size_t(copyRect.y-target.y));
  if(result==Status::Unsupported&&renderOrder==PixelOrder::Native&&ppaBlendEnabled&&rgb565_ppa::Context::available())
#if NICE_GFX_UI_SYSTEM_MODAL_ROW_COPY
    result=detail::copySystemModalTranslatedNativeRows(ppaFade,*nativeOverlayTarget,input,bytes,copyRect,
      size_t(copyRect.x-target.x),size_t(copyRect.y-target.y));
#else
    result=ppaFade.copyIntoNative(*nativeOverlayTarget,copyRect.x,copyRect.y,input,bytes,
      size_t(copyRect.x-target.x),size_t(copyRect.y-target.y),size_t(copyRect.width),size_t(copyRect.height));
#endif
  if(result==Status::Unsupported)result=canvas.blit(target.x,target.y,input);
  if(result!=Status::Ok)return result;
  ++surfaceStats.hits;painted=true;
#else
  (void)area;
#endif
  return Status::Ok;
}
