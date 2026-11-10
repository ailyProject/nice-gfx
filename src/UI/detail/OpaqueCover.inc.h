// Included inside View::Impl. An opaque RGB565 image can replace every earlier
// paint in one complete dirty region. Keep this narrower than ordinary image
// painting: no transformed/rounded/scaled content or partial coverage unions.
#ifndef NICE_GFX_UI_JPEG_FRAMEBUFFER_DIRECT
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_UI_JPEG_FRAMEBUFFER_DIRECT 1
#else
#define NICE_GFX_UI_JPEG_FRAMEBUFFER_DIRECT 0
#endif
#endif
// A synchronous decode may populate this start()'s exclusively owned back
// framebuffer before the ordered node painter. The marker is never cached.
int directJpegCover=-1;
const uint16_t *directJpegTarget=nullptr;
const MediaDesc *plannedJpegMedia=nullptr;uint16_t plannedJpegFrame=0;
mutable const void *jpegAheadResource=nullptr;mutable uint16_t jpegAheadFrame=0;mutable int jpegAheadDirection=1;
void requestJpegAhead(const MediaDesc &source,uint16_t frame)const{
#if NICE_GFX_UI_HAS_JPEG_PREFETCH
  if(jpegAheadResource==source.pixels&&jpegAheadFrame==frame)return;
  const int direction=jpegAheadResource==source.pixels&&jpegAheadFrame!=frame?(frame>jpegAheadFrame?1:-1):jpegAheadDirection;
  detail::JpegMediaCache::FramebufferPlan plans[4];size_t count=0;
  const int32_t requested[4]={int32_t(frame),
    int32_t(frame)+direction,int32_t(frame)-direction,int32_t(frame)+2*direction};
  // Start after the owner has decoded/copied this frame. Neighbours have
  // priority; retain current last for repeated UI samples of the same video frame.
  const uint8_t order[4]={1,2,3,0};
  for(uint8_t at:order){const int32_t next=requested[at];if(next<0||next>=source.frames)continue;
    if(jpegCache.planFramebuffer(source,uint16_t(next),plans[count]))++count;}
  if(count&&jpegPrefetch.request(plans,count)){jpegAheadResource=source.pixels;jpegAheadFrame=frame;jpegAheadDirection=direction;}
#else
  (void)source;(void)frame;
#endif
}
int planDirectJpegFrame(detail::JpegMediaCache::FramebufferPlan &plan) {
  plannedJpegMedia=nullptr;
#if NICE_GFX_UI_JPEG_FRAMEBUFFER_DIRECT
  if(renderBudget.maxPixelsPerTick||buildingSurface||snapshotTransition||rasterFrame.preview.page||
     snapshotKeyboard.visible||snapshotInput||circularKeyboard||
     (!animationsRunning()&&mediaTweens.empty()))return -1;
#if NICE_GFX_UI_MEDIA_OCCLUSION
  // Partial damage must stay partial: the independent background worker can
  // also lend its exact output to cropped ordinary JPEG painters.
  DirtyRect pending;
  if(refresh->pendingRegions()!=1||!refresh->pendingRegion(0,pending)||pending.x||pending.y||
     pending.width!=rasterFrame.current.page->width||pending.height!=rasterFrame.current.page->height)return -1;
#endif
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);
  if(!panel||!panel->supportsFrameBlit())return -1;
  prepareFrameGeometry(*rasterFrame.current.page,rasterFrame.current.values,rasterFrame.current.geometry);
  const auto &p=*rasterFrame.current.page;const DirtyRect full(0,0,p.width,p.height);
  const int index=opaqueCoverAt(rasterFrame.current.geometry,full,0,0);
  if(index<0)return -1;
  const auto &g=rasterFrame.current.geometry.nodes[index];
  const auto &n=p.nodes[index];const auto &v=rasterFrame.current.values[index];const auto *source=media(n,v);
  if(!source||v.imageOverride||!mediaJpeg(*source)||g.rect.x||g.rect.y||g.rect.width!=p.width||g.rect.height!=p.height)return -1;
  if(!jpegCache.planFramebuffer(*source,v.mediaFrame,plan))return -1;
  plannedJpegMedia=source;plannedJpegFrame=v.mediaFrame;return index;
#else
  (void)plan;return -1;
#endif
}
static bool opaqueCoverContains(Rect bounds,const DirtyRect &area,int32_t offsetX,int32_t offsetY) {
  return area.width>0&&area.height>0&&bounds.width>0&&bounds.height>0&&
    int64_t(bounds.x)+offsetX<=area.x&&int64_t(bounds.y)+offsetY<=area.y&&
    int64_t(bounds.x)+offsetX+bounds.width>=int64_t(area.x)+area.width&&
    int64_t(bounds.y)+offsetY+bounds.height>=int64_t(area.y)+area.height;
}
#ifndef NICE_GFX_UI_OPAQUE_CONTAINER_COVER
#define NICE_GFX_UI_OPAQUE_CONTAINER_COVER 1
#endif
#ifndef NICE_GFX_UI_SYSTEM_MODAL_CULL
#define NICE_GFX_UI_SYSTEM_MODAL_CULL 1
#endif
#ifndef NICE_GFX_UI_SYSTEM_MODAL_CLIP
#define NICE_GFX_UI_SYSTEM_MODAL_CLIP 1
#endif
struct SystemModalCover {int root=-1;uint16_t layer=0;Rect rect{};};
SystemModalCover systemModalCover()const{
  SystemModalCover result;
#if NICE_GFX_UI_SYSTEM_MODAL_CULL
  // Only a live ordinary page may borrow this ordering fact. Independent
  // snapshots/prefixes/repairs may omit the later modal painter entirely.
  if(!paintCache||buildingSurface||foregroundCapture||nativeCacheRepairing||nativePrefixLimit>=0||
     snapshotTransition||rasterFrame.preview.page||snapshotInput||snapshotKeyboard.visible||
     paintOffsetX||paintOffsetY||snapshotBackOffset||rasterFrame.current.page!=&page())return result;
  const auto &p=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;
  for(const auto &b:behaviorVisuals){const int root=b.root;
    if(root<0||size_t(root)>=values.size()||b.kind!=BehaviorKind::Modal||b.reveal!=1024||
       !std::binary_search(systemPanelRoots.begin(),systemPanelRoots.end(),p.nodes[root].id))continue;
    const auto &n=p.nodes[root];const auto &v=values[root];const auto &g=paintCache->nodes[root];
    if(n.kind!=Kind::Container||v.kind!=Kind::Container||n.instrument||v.instrument||!g.visible||g.alpha!=255)continue;
    bool disabled=false,affine=false;
    for(int at=root;at>=0;at=values[at].parent==NoParent?-1:values[at].parent){
      disabled|=(values[at].flags&Disabled)!=0;
      if(p.nodes[at].motion){const auto &pose=motionPose(p,values,at);if(pose.scale!=1024||pose.rotation%360||pose.reveal!=1024){affine=true;break;}}
    }
    if(affine)continue;
    Style style=styleFor(p,values,root);uint16_t flags=v.flags;applyFocusStyle(focusStyle(root),style,flags);
    if(style.radius||(disabled?(flags&DisabledTransparent)!=0:root==snapshotPressed?(flags&ActiveTransparent)!=0:(flags&Transparent)!=0))continue;
    const Rect covered=intersect(g.rect,g.clip);if(covered.width<=0||covered.height<=0)continue;
    const auto layer=viewportOverlayDepth(p,values,root);
    if(layer>result.layer||(layer==result.layer&&root>result.root))result={root,layer,covered};
  }
#endif
  return result;
}
bool systemModalCovers(const SystemModalCover &cover,int node,uint16_t layer,Rect clipped)const{
#if NICE_GFX_UI_SYSTEM_MODAL_CULL
  // Keep all viewport chrome, modal descendants and toast trees on their
  // original painter. This first optimization only removes ordinary host
  // nodes that are wholly overwritten by the later opaque system panel.
  if(cover.root<0||layer||!(layer<cover.layer||(layer==cover.layer&&node<cover.root))||
     behaviorSnapshotDescendant(node,cover.root)||!opaqueCoverContains(cover.rect,{clipped.x,clipped.y,clipped.width,clipped.height},0,0))return false;
  for(const auto &b:behaviorVisuals)if(b.kind==BehaviorKind::Toast&&behaviorSnapshotDescendant(node,b.root))return false;
  return true;
#else
  (void)cover;(void)node;(void)layer;(void)clipped;return false;
#endif
}
bool systemModalTopCover(const SystemModalCover &cover)const{
#if NICE_GFX_UI_SYSTEM_MODAL_CLIP
  if(cover.root<0||cover.rect.x||cover.rect.y||cover.rect.width!=rasterFrame.current.page->width||
     !paintCache||paintCache->nodes.empty()||rasterFrame.current.values.empty())return false;
  const auto &p=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;
  const auto &n=p.nodes[0];const auto &v=values[0];const auto &g=paintCache->nodes[0];
  // The host's opaque page root paints the uncovered bottom, and the later
  // opaque system panel paints the top. Root fades/transparent/disabled or
  // focused roots retain the old full-area clear and node painter.
  if(n.parent!=NoParent||v.parent!=NoParent||n.kind!=Kind::Container||v.kind!=Kind::Container||
     n.instrument||v.instrument||!g.visible||g.alpha!=255||(v.flags&(Hidden|Disabled|Transparent))||snapshotPressed==0||focusStyle(0))return false;
  if(n.motion){const auto &pose=motionPose(p,values,0);if(pose.scale!=1024||pose.rotation%360||pose.reveal!=1024)return false;}
  const auto &style=styleFor(p,values,0);if(style.radius)return false;
  const DirtyRect full(0,0,p.width,p.height);
  return opaqueCoverContains(g.rect,full,0,0)&&opaqueCoverContains(g.clip,full,0,0);
#else
  (void)cover;return false;
#endif
}
bool clipSystemModalHost(const SystemModalCover &cover,int node,uint16_t layer,Rect &clipped)const{
#if NICE_GFX_UI_SYSTEM_MODAL_CLIP
  if(cover.root<0||layer||!(layer<cover.layer||(layer==cover.layer&&node<cover.root))||behaviorSnapshotDescendant(node,cover.root))return false;
  for(const auto &b:behaviorVisuals)if(b.kind==BehaviorKind::Toast&&behaviorSnapshotDescendant(node,b.root))return false;
  const int32_t bottom=cover.rect.y+cover.rect.height;
  if(clipped.y>=bottom)return false;
  const int32_t end=clipped.y+clipped.height;clipped.y=std::min(bottom,end);clipped.height=end-clipped.y;return true;
#else
  (void)cover;(void)node;(void)layer;(void)clipped;return false;
#endif
}
#ifndef NICE_GFX_UI_OPAQUE_CONTAINER_BANDS
#define NICE_GFX_UI_OPAQUE_CONTAINER_BANDS 0
#endif
#define NICE_GFX_UI_OPAQUE_CONTAINER_BANDS_STATS_VERSION 1

int opaqueContainerCover(const DirtyRect &area)const{
#if NICE_GFX_UI_OPAQUE_CONTAINER_COVER
  if(!paintCache||area.pixels()<1024||foregroundCapture||behaviorVisualMoving()||viewportOverlayLevels)return -1;
  const auto &p=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;int last=-1;
  const bool profile=profilingClock!=nullptr;
  if(profile){++stats.opaqueContainerQueries;stats.opaqueContainerFullNodeCount+=p.count;}
  // Keep the exact original predicate, including focus/disabled ancestry and
  // last-in-source-order selection. Bands only omit impossible candidates.
  const auto visit=[&](uint16_t i){const auto &n=p.nodes[i];const auto &g=paintCache->nodes[i];const auto &v=values[i];
    if(n.kind!=Kind::Container||v.kind!=Kind::Container||!g.visible||g.alpha!=255||!opaqueCoverContains(g.rect,area,paintOffsetX,paintOffsetY)||!opaqueCoverContains(g.clip,area,paintOffsetX,paintOffsetY))return;
    if(n.motion){const auto &m=motionPose(p,values,i);if(m.scale!=1024||m.rotation%360)return;}
    Style style=styleFor(p,values,i);uint16_t flags=v.flags;applyFocusStyle(focusStyle(i),style,flags);if(style.radius)return;
    bool disabled=(v.flags&Disabled)!=0;for(uint16_t parent=v.parent;parent!=NoParent&&!disabled;parent=values[parent].parent)disabled=(values[parent].flags&Disabled)!=0;
    if(disabled?(flags&DisabledTransparent)!=0:i==snapshotPressed?(flags&ActiveTransparent)!=0:(flags&Transparent)!=0)return;

    last=i;if(profile)++stats.opaqueContainerMatches;
  };
#if NICE_GFX_UI_OPAQUE_CONTAINER_BANDS
  const FrameCache &cache=*paintCache;
  const int64_t top=int64_t(area.y)-paintOffsetY,bottom=top+int64_t(area.height)-1;
  // Full ordinary metadata includes ALL visible/nonzero-alpha nodes, not just
  // leaves. Unsupported/incomplete metadata retains the original linear loop.
  const bool ready=p.height&&area.width>0&&area.height>0&&cache.nodes.size()==p.count&&values.size()>=p.count&&
    cache.visibleHeight<0&&cache.words==(size_t(p.count)+31)/32&&cache.bandHeight==(int32_t(p.height)+31)/32&&
    cache.bands.size()==cache.words*32&&top>=INT32_MIN&&top<=INT32_MAX&&bottom>=INT32_MIN&&bottom<=INT32_MAX;
  if(ready){
    if(profile)++stats.opaqueContainerBandQueries;
    const int first=clamp32(int32_t(top)/cache.bandHeight,0,31),end=clamp32(int32_t(bottom)/cache.bandHeight,0,31);
    for(size_t word=0;word<cache.words;++word){uint32_t candidates=0;
      for(int band=first;band<=end;++band)candidates|=cache.bands[size_t(band)*cache.words+word];
      while(candidates){unsigned bit=0;
#if defined(__GNUC__) || defined(__clang__)
        bit=unsigned(__builtin_ctz(candidates));
#else
        while(!(candidates&(uint32_t(1)<<bit)))++bit;
#endif
        candidates&=candidates-1;const size_t i=word*32+bit;if(i>=p.count)continue;
        if(profile)++stats.opaqueContainerBandCandidates;visit(uint16_t(i));
      }
    }
    if(profile&&last>=0)++stats.opaqueContainerSelections;return last;
  }
  if(profile)++stats.opaqueContainerBandFallbacks;
#endif
  if(profile)stats.opaqueContainerLinearCandidates+=p.count;
  for(uint16_t i=0;i<p.count;++i)visit(i);
  if(profile&&last>=0)++stats.opaqueContainerSelections;return last;
#else
  (void)area;return -1;
#endif
}
bool opaqueCoverTransition() const {
  const auto &v=snapshotMotionRect;const auto effect=transition.config.effect;
  return snapshotTransition&&!rasterFrame.preview.page&&transitionFrame.opacity==255&&
    (effect==TransitionEffect::SlideLeft||effect==TransitionEffect::SlideRight||
     effect==TransitionEffect::SlideUp||effect==TransitionEffect::SlideDown||
     (effect==TransitionEffect::Fade&&!transitionFrame.outX&&!transitionFrame.outY&&!transitionFrame.inX&&!transitionFrame.inY))&&
    (snapshotPhase==TransitionPhase::Enter||snapshotPhase==TransitionPhase::Exit)&&
    !v.x&&!v.y&&v.width==rasterFrame.current.page->width&&v.height==rasterFrame.current.page->height;
}
bool opaqueCoverMedia(const FrameCache &cache,size_t i) const {
  const auto &n=rasterFrame.current.page->nodes[i];const auto &g=cache.nodes[i];const auto &value=rasterFrame.current.values[i];
  if(n.kind!=Kind::Media||n.motion||!g.visible||g.alpha!=255)return false;
  const auto &style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,int(i));
  if(style.borderWidth||style.radius||style.paddingTop||style.paddingRight||style.paddingBottom||style.paddingLeft)return false;
  const auto *source=media(n,value);
  if(!source||g.rect.width!=source->width||g.rect.height!=source->height)return false;
  // JPEG output is an opaque native RGB565 RAM image too. Eligibility itself
  // does not decode or allocate; the synchronous copy below resolves the exact
  // snapshot frame through the ordinary shared-instance JPEG cache.
  if(mediaJpeg(*source))return jpegCache.available();
  if(!value.imageOverride||!value.image.impl_)return false;
  const auto &image=*value.image.impl_;
  return !image.flash&&image.source.format==ImageFormat::RGB565&&image.media.bytesPerPixel==2&&
    !(image.source.strideBytes&1);
}
bool opaqueCoverSource(int index,PixelView &pixels,size_t &bytes) const {
  pixels=PixelView(nullptr,0);bytes=0;
  const auto &n=rasterFrame.current.page->nodes[index];const auto &value=rasterFrame.current.values[index];
  const auto *source=media(n,value);if(!source)return false;
  if(mediaJpeg(*source)){
    JpegDecodeTarget decoded;if(!decodeJpeg(*source,value.mediaFrame,decoded,n.id))return false;
    pixels=PixelView(decoded.pixels,decoded.width,decoded.height,decoded.strideBytes/2,PixelOrder::Native);
    bytes=decoded.capacityBytes;return true;
  }
  if(!value.imageOverride||!value.image.impl_)return false;
  const auto &image=*value.image.impl_;
  pixels=PixelView(image.media.pixels+size_t(value.mediaFrame)*image.source.strideBytes*image.media.height,
    image.media.width,image.media.height,image.source.strideBytes/2,image.source.order);
  bytes=image.source.byteLength;return true;
}
int opaqueCoverAt(const FrameCache &cache,const DirtyRect &area,int32_t offsetX,int32_t offsetY) const {
  for(size_t end=cache.mediaEnd;end>cache.mediaFirst;--end){const size_t i=end-1;const auto &g=cache.nodes[i];
    if(opaqueCoverMedia(cache,i)&&opaqueCoverContains(g.rect,area,offsetX,offsetY)&&
       opaqueCoverContains(g.clip,area,offsetX,offsetY))return int(i);
  }
  return -1;
}
int opaqueCover(const DirtyRect &area) const {
  // Coverage also removes overwritten backgrounds from ordinary Wire565
  // tiles. Only Native targets may use the panel-copy path below; Wire tiles
  // must still paint the image into their own buffer before later overlays.
  if(rasterFrame.preview.page||buildingSurface||!paintCache||
     (snapshotTransition&&!opaqueCoverTransition()))return -1;
  return opaqueCoverAt(*paintCache,area,paintOffsetX,paintOffsetY);
}
PixelView opaqueCoverPixels(const FrameCache &cache,int index,const DirtyRect &area,int32_t offsetX,int32_t offsetY) const {
  const auto &g=cache.nodes[index];PixelView full(nullptr,0);size_t bytes=0;
  if(!opaqueCoverSource(index,full,bytes))return PixelView(nullptr,0);
  const size_t sourceX=size_t(int64_t(area.x)-offsetX-g.rect.x),sourceY=size_t(int64_t(area.y)-offsetY-g.rect.y);
  const auto *source=full.data+(sourceY*full.stride+sourceX)*2;
  return PixelView(source,size_t(area.width),size_t(area.height),full.stride,full.order);
}
bool canStageOpaqueCover(const nice_gfx::NativeFramebuffer &target,const FrameCache &cache,int index,
                        const DirtyRect &area,int32_t offsetX,int32_t offsetY)const{
  if(!nativeGpuPaintAllowed())return false;
  const auto &g=cache.nodes[index];PixelView full(nullptr,0);size_t bytes=0;
  if(!opaqueCoverSource(index,full,bytes))return false;
  return ppaFade.canCopyIntoNative(target,area.x,area.y,full,bytes,
    size_t(int64_t(area.x)-offsetX-g.rect.x),size_t(int64_t(area.y)-offsetY-g.rect.y),area.width,area.height);
}
// Preflight the exact cropped image before the first transport write. A
// synchronous copy has completed before CPU overlays touch the same region;
// the normal Native refresh still owns the final framebuffer submission.
Status blitOpaqueCover(nice_gfx::Backend &panel,int index,const DirtyRect &area,bool &painted) {
  painted=false;if(!nativeGpuPaintAllowed())return Status::Ok;
  if(nativeCacheRepairing){
    // The mapped renderer may be repairing an independent retained page.
    // A panel flush here would publish to the LCD and leave that page's image
    // pixels unwritten. Copy into its actual target or use its ordinary painter.
    if(!nativeOverlayTarget||!canStageOpaqueCover(*nativeOverlayTarget,*paintCache,index,area,paintOffsetX,paintOffsetY))return Status::Ok;
    const auto &g=paintCache->nodes[index];PixelView full(nullptr,0);size_t bytes=0;
    if(!opaqueCoverSource(index,full,bytes))return Status::Ok;
    const Status status=ppaFade.copyIntoNative(*nativeOverlayTarget,area.x,area.y,full,bytes,
      size_t(int64_t(area.x)-paintOffsetX-g.rect.x),size_t(int64_t(area.y)-paintOffsetY-g.rect.y),area.width,area.height);
    painted=status==Status::Ok;return status==Status::Unsupported?Status::Ok:status;
  }
  if(index==directJpegCover&&directJpegTarget&&nativeOverlayTarget&&nativeOverlayTarget->pixels==directJpegTarget&&
     !stagingNativeTarget&&!paintOffsetX&&!paintOffsetY&&renderOrder==PixelOrder::Native&&!area.x&&!area.y&&
     area.width==rasterFrame.current.page->width&&area.height==rasterFrame.current.page->height){painted=true;return Status::Ok;}
  if(stagingNativeTarget){
    if(!canStageOpaqueCover(*stagingNativeTarget,*paintCache,index,area,paintOffsetX,paintOffsetY))return Status::Ok;
    const auto &g=paintCache->nodes[index];PixelView full(nullptr,0);size_t bytes=0;
    if(!opaqueCoverSource(index,full,bytes))return Status::Ok;
    const Status status=ppaFade.copyIntoNative(*stagingNativeTarget,area.x,area.y,full,bytes,
      size_t(int64_t(area.x)-paintOffsetX-g.rect.x),size_t(int64_t(area.y)-paintOffsetY-g.rect.y),area.width,area.height);
    painted=status==Status::Ok;return status;
  }
  const PixelView cropped=opaqueCoverPixels(*paintCache,index,area,paintOffsetX,paintOffsetY);
  const int32_t alignX=panel.windowAlignmentX(),alignY=panel.windowAlignmentY();
  if(!panel.supportsFrameBlit()||!alignX||!alignY||area.x%alignX||area.y%alignY||
     area.width%alignX||area.height%alignY||!panel.canFrameBlit(cropped))return Status::Ok;
  Status status=panel.flush(area.x,area.y,cropped,false);
  // Some asynchronous transports treat false as a hint. Drain their source
  // lease before CPU overlays; S31's blocking PPA is already idle here.
  if(status==Status::Ok&&panel.busy())status=panel.finish();
  if(status!=Status::Ok){panel.finish();return status==Status::Unsupported?Status::IoError:status;}
  painted=true;return Status::Ok;
}
