// Included inside View::Impl. An opaque RGB565 image can replace every earlier
// paint in one complete dirty region. Keep this narrower than ordinary image
// painting: no transformed/rounded/scaled content or partial coverage unions.
#ifndef NICE_GFX_UI_JPEG_FRAMEBUFFER_DIRECT
#define NICE_GFX_UI_JPEG_FRAMEBUFFER_DIRECT 0
#endif
// A synchronous decode may populate this start()'s exclusively owned back
// framebuffer before the ordered node painter. The marker is never cached.
int directJpegCover=-1;
const uint16_t *directJpegTarget=nullptr;
const MediaDesc *plannedJpegMedia=nullptr;uint16_t plannedJpegFrame=0;
const void *jpegAheadResource=nullptr;uint16_t jpegAheadFrame=0;int jpegAheadDirection=1;
void requestJpegAhead(const MediaDesc &source,uint16_t frame){
#if NICE_GFX_UI_HAS_JPEG_PREFETCH
  if(jpegAheadResource==source.pixels&&jpegAheadFrame!=frame)jpegAheadDirection=frame>jpegAheadFrame?1:-1;
  jpegAheadResource=source.pixels;jpegAheadFrame=frame;
  detail::JpegMediaCache::FramebufferPlan plans[4];size_t count=0;
  const int32_t requested[4]={int32_t(frame),
    int32_t(frame)+jpegAheadDirection,int32_t(frame)-jpegAheadDirection,int32_t(frame)+2*jpegAheadDirection};
  // Start after the owner has decoded/copied this frame. Neighbours have
  // priority; retain current last for repeated UI samples of the same video frame.
  const uint8_t order[4]={1,2,3,0};
  for(uint8_t at:order){const int32_t next=requested[at];if(next<0||next>=source.frames)continue;
    if(jpegCache.planFramebuffer(source,uint16_t(next),plans[count]))++count;}
  if(count)jpegPrefetch.request(plans,count);
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
  if(pageRotated(*rasterFrame.current.page))return false;
  const auto &n=rasterFrame.current.page->nodes[i];const auto &g=cache.nodes[i];const auto &value=rasterFrame.current.values[i];
  if(n.kind!=Kind::Media||n.motion||!g.visible||g.alpha!=255)return false;
  const auto &style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,int(i));
  if(style.borderWidth||maxCornerRadius(style)||style.paddingTop||style.paddingRight||style.paddingBottom||style.paddingLeft)return false;
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
  const auto &g=cache.nodes[index];PixelView full(nullptr,0);size_t bytes=0;
  if(!opaqueCoverSource(index,full,bytes))return false;
  return ppaFade.canCopyIntoNative(target,area.x,area.y,full,bytes,
    size_t(int64_t(area.x)-offsetX-g.rect.x),size_t(int64_t(area.y)-offsetY-g.rect.y),area.width,area.height);
}
// Preflight the exact cropped image before the first transport write. A
// synchronous copy has completed before CPU overlays touch the same region;
// the normal Native refresh still owns the final framebuffer submission.
Status blitOpaqueCover(nice_gfx::Backend &panel,int index,const DirtyRect &area,bool &painted) {
  painted=false;
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
