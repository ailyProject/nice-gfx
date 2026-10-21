// Included inside View::Impl. An opaque RAM image can replace every earlier
// paint in one complete dirty region. Keep this narrower than ordinary image
// painting: no transformed/rounded/scaled content or partial coverage unions.
static bool opaqueCoverContains(Rect bounds,const DirtyRect &area,int32_t offsetX,int32_t offsetY) {
  return area.width>0&&area.height>0&&bounds.width>0&&bounds.height>0&&
    int64_t(bounds.x)+offsetX<=area.x&&int64_t(bounds.y)+offsetY<=area.y&&
    int64_t(bounds.x)+offsetX+bounds.width>=int64_t(area.x)+area.width&&
    int64_t(bounds.y)+offsetY+bounds.height>=int64_t(area.y)+area.height;
}
bool opaqueCoverTransition() const {
  const auto &v=snapshotMotionRect;const auto effect=transition.config.effect;
  return snapshotTransition&&!previewPage&&transitionFrame.opacity==255&&
    (effect==TransitionEffect::SlideLeft||effect==TransitionEffect::SlideRight||
     effect==TransitionEffect::SlideUp||effect==TransitionEffect::SlideDown||
     (effect==TransitionEffect::Fade&&!transitionFrame.outX&&!transitionFrame.outY&&!transitionFrame.inX&&!transitionFrame.inY))&&
    (snapshotPhase==TransitionPhase::Enter||snapshotPhase==TransitionPhase::Exit)&&
    !v.x&&!v.y&&v.width==snapshotPage->width&&v.height==snapshotPage->height;
}
bool opaqueCoverMedia(const FrameCache &cache,size_t i) const {
  const auto &n=snapshotPage->nodes[i];const auto &g=cache.nodes[i];const auto &value=snapshot[i];
  if(n.kind!=Kind::Media||n.motion||!g.visible||g.alpha!=255||!value.imageOverride||!value.image.impl_)return false;
  const auto &image=*value.image.impl_;const auto &style=styleFor(*snapshotPage,snapshot,int(i));
  return !image.flash&&image.source.format==ImageFormat::RGB565&&image.media.bytesPerPixel==2&&
    !(image.source.strideBytes&1)&&g.rect.width==image.media.width&&g.rect.height==image.media.height&&
    !style.borderWidth&&!style.radius&&!style.paddingTop&&!style.paddingRight&&!style.paddingBottom&&!style.paddingLeft;
}
int opaqueCoverAt(const FrameCache &cache,const DirtyRect &area,int32_t offsetX,int32_t offsetY) const {
  for(size_t end=cache.nodes.size();end;--end){const size_t i=end-1;const auto &g=cache.nodes[i];
    if(opaqueCoverMedia(cache,i)&&opaqueCoverContains(g.rect,area,offsetX,offsetY)&&
       opaqueCoverContains(g.clip,area,offsetX,offsetY))return int(i);
  }
  return -1;
}
int opaqueCover(const DirtyRect &area) const {
  if(renderOrder!=PixelOrder::Native||previewPage||buildingSurface||!paintCache||
     (snapshotTransition&&!opaqueCoverTransition()))return -1;
  return opaqueCoverAt(*paintCache,area,paintOffsetX,paintOffsetY);
}
PixelView opaqueCoverPixels(const FrameCache &cache,int index,const DirtyRect &area,int32_t offsetX,int32_t offsetY) const {
  const auto &g=cache.nodes[index];const auto &value=snapshot[index];const auto &image=*value.image.impl_;
  const size_t sourceX=size_t(int64_t(area.x)-offsetX-g.rect.x),sourceY=size_t(int64_t(area.y)-offsetY-g.rect.y);
  const auto *source=image.media.pixels+size_t(value.mediaFrame)*image.source.strideBytes*image.media.height+
    sourceY*image.source.strideBytes+sourceX*2;
  return PixelView(source,size_t(area.width),size_t(area.height),image.source.strideBytes/2,image.source.order);
}
bool canStageOpaqueCover(const nice_gfx::NativeFramebuffer &target,const FrameCache &cache,int index,
                        const DirtyRect &area,int32_t offsetX,int32_t offsetY)const{
  const auto &g=cache.nodes[index];const auto &value=snapshot[index];const auto &image=*value.image.impl_;
  const PixelView full(image.media.pixels+size_t(value.mediaFrame)*image.source.strideBytes*image.media.height,
    image.media.width,image.media.height,image.source.strideBytes/2,image.source.order);
  return ppaFade.canCopyIntoNative(target,area.x,area.y,full,image.source.byteLength,
    size_t(int64_t(area.x)-offsetX-g.rect.x),size_t(int64_t(area.y)-offsetY-g.rect.y),area.width,area.height);
}
// Preflight the exact cropped image before the first transport write. A
// synchronous copy has completed before CPU overlays touch the same region;
// the normal Native refresh still owns the final framebuffer submission.
Status blitOpaqueCover(nice_gfx::Panel &panel,int index,const DirtyRect &area,bool &painted) {
  painted=false;
  if(stagingNativeTarget){
    if(!canStageOpaqueCover(*stagingNativeTarget,*paintCache,index,area,paintOffsetX,paintOffsetY))return Status::Ok;
    const auto &g=paintCache->nodes[index];const auto &value=snapshot[index];const auto &image=*value.image.impl_;
    const PixelView full(image.media.pixels+size_t(value.mediaFrame)*image.source.strideBytes*image.media.height,
      image.media.width,image.media.height,image.source.strideBytes/2,image.source.order);
    const Status status=ppaFade.copyIntoNative(*stagingNativeTarget,area.x,area.y,full,image.source.byteLength,
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
