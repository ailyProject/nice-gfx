// Included inside View::Impl. A retained terminal page doubles as a sprite
// atlas for isolated, opaque translation-only subtrees. No pixel storage is
// allocated. Every unsupported overlap, clip, state or transform falls back.
struct AnimationLayer { uint16_t root;Rect source,target;uint16_t background,backgroundOwner; };
const Surface *snapshotAnimationSurface=nullptr;
AnimationLayer snapshotAnimationLayers[16];
uint8_t snapshotAnimationLayerCount=0;

void recordAnimationAtlasReject(const Surface &source,uint16_t node,uint8_t reason){
  const auto &p=*source.page;const auto &before=source.key[node],&live=rasterFrame.current.values[node];
  auto &out=stats.animationAtlasLastReject;const uint32_t sequence=out.sequence+1;out=AnimationAtlasRejectInfo{};out.sequence=sequence;
  out.page=uint16_t(source.page-project->pages);out.node=node;out.sourceSlot=uint16_t(&source-surfaces.data());out.id=p.nodes[node].id;out.kind=uint16_t(p.nodes[node].kind);out.reason=reason;
  out.animationEffect=uint8_t(p.nodes[node].animation.effect);out.hasMotion=p.nodes[node].motion?1:0;
  for(uint16_t at=node;at!=NoParent;at=p.nodes[at].parent){
    if(source.key[at].flags&Hidden)out.sourceHidden=1;
    if(rasterFrame.current.values[at].flags&Hidden)out.liveHidden=1;
    if(std::binary_search(systemPanelRoots.begin(),systemPanelRoots.end(),p.nodes[at].id))out.region=2;
    else if(out.region!=2&&std::binary_search(systemBarRoots.begin(),systemBarRoots.end(),p.nodes[at].id))out.region=1;
  }
  if(before.text.compiled!=live.text.compiled||before.text.revision!=live.text.revision)out.diffMask|=1;
  if(before.value!=live.value||before.switchPosition!=live.switchPosition)out.diffMask|=2;
  if(before.flags!=live.flags)out.diffMask|=4;
  if(before.scrollX!=live.scrollX||before.scrollY!=live.scrollY)out.diffMask|=8;
  if(before.animationX!=live.animationX||before.animationY!=live.animationY)out.diffMask|=16;
  if(before.opacity!=live.opacity)out.diffMask|=32;
  if(before.motionRevision!=live.motionRevision)out.diffMask|=64;
  if(before.dynamic!=live.dynamic)out.diffMask|=128;
  if(before.mediaFrame!=live.mediaFrame||before.image.impl_!=live.image.impl_||before.imageOverride!=live.imageOverride)out.diffMask|=256;
  if(before.instrument!=live.instrument){
    bool different=!before.instrument||!live.instrument||before.instrument->visual!=live.instrument->visual||before.instrument->data!=live.instrument->data;
    if(!different)for(unsigned i=0;i<4;++i)if(before.instrument->head[i]!=live.instrument->head[i]||before.instrument->count[i]!=live.instrument->count[i]){different=true;break;}
    if(different)out.diffMask|=512;
  }
  if(before.kind!=live.kind||before.parent!=live.parent||before.styleIndex!=live.styleIndex)out.diffMask|=1024;
  out.sourceX=before.animationX;out.sourceY=before.animationY;out.liveX=live.animationX;out.liveY=live.animationY;
  out.sourceValue=before.value;out.liveValue=live.value;out.sourceFlags=before.flags;out.liveFlags=live.flags;
  out.sourceOpacity=before.opacity;out.liveOpacity=live.opacity;out.sourceTextRevision=before.text.revision;out.liveTextRevision=live.text.revision;
  out.sourceMotionRevision=before.motionRevision;out.liveMotionRevision=live.motionRevision;
}

static bool animationContains(Rect outer,Rect inner) {
  return inner.width>=0&&inner.height>=0&&inner.x>=outer.x&&inner.y>=outer.y&&
    int64_t(inner.x)+inner.width<=int64_t(outer.x)+outer.width&&
    int64_t(inner.y)+inner.height<=int64_t(outer.y)+outer.height;
}
static bool animationOverlaps(Rect a,Rect b) {
  return a.width>0&&a.height>0&&b.width>0&&b.height>0&&
    int64_t(a.x)<int64_t(b.x)+b.width&&int64_t(b.x)<int64_t(a.x)+a.width&&
    int64_t(a.y)<int64_t(b.y)+b.height&&int64_t(b.y)<int64_t(a.y)+a.height;
}
bool animationAncestor(const PageDesc &p,int ancestor,int child) const {
  for(int at=child;at>=0;at=p.nodes[at].parent==NoParent?-1:p.nodes[at].parent)
    if(at==ancestor)return true;
  return false;
}
bool animationBackground(const Surface &surface,uint16_t root,Rect swept,uint16_t &color,uint16_t &owner) const {
  const PageDesc &p=*surface.page;color=p.background;owner=NoParent;bool found=false;
  for(uint16_t at=p.nodes[root].parent;at!=NoParent;at=p.nodes[at].parent){
    const auto &node=p.nodes[at];const auto &state=surface.key[at];
    const Rect bounds=poseRect(p,surface.key,at);
    // Moving outside an ancestor's clip would change the sprite's coverage.
    if(((state.flags&Clip)||scrolling(node.kind))&&
       !animationContains(childBox(p,surface.key,at,bounds),swept))return false;
    if(state.opacity!=255||node.motion||state.animationX||state.animationY)return false;
    const bool disabled=(state.flags&Disabled)!=0;
    const bool transparent=disabled?(state.flags&DisabledTransparent)!=0:(state.flags&Transparent)!=0;
    if(transparent||found)continue;
    const auto &style=styleFor(p,surface.key,at);
    if(node.kind!=Kind::Container||style.gradient||style.borderWidth||style.radius||
       !animationContains(bounds,swept))return false;
    color=disabled?style.disabledColor:style.background;found=true;
  }
  // A flat, earlier sibling can be the actual backdrop (for example a dark
  // sidebar over a light page). Sharing the parent preserves the ancestor
  // clip/transform proof above. Its own rectangle and clip must cover the
  // complete translation; its children still undergo ordinary overlap guards.
  for(uint16_t at=0;at<root;++at){const auto &node=p.nodes[at];const auto &state=surface.key[at];
    if(node.parent!=p.nodes[root].parent||node.kind!=Kind::Container||node.motion||node.exitAnimation||
       node.animation.effect!=AnimationEffect::None||state.animationX||state.animationY||state.opacity!=255)continue;
    const auto &g=surface.geometry.nodes[at];
    if(!g.visible||g.alpha!=255||!animationContains(g.rect,swept)||!animationContains(g.clip,swept))continue;
    const bool disabled=(state.flags&Disabled)!=0;
    if(disabled?(state.flags&DisabledTransparent)!=0:(state.flags&Transparent)!=0)continue;
    const auto &style=styleFor(p,surface.key,at);
    if(style.gradient||style.borderWidth||style.radius)continue;
    color=disabled?style.disabledColor:style.background;owner=at;
  }
  return true;
}
const Surface *findAnimationSurface() {
  ++stats.animationAtlasAttempts;
  snapshotAnimationSurface=nullptr;snapshotAnimationLayerCount=0;
#if NICE_GFX_UI_VIEWPORT_ANIMATION_ATLAS
  // Surface pixels omit transient behavior frames/backdrops. Their visual
  // clocks and decoration order stay with the ordinary scene painter.
  if(!behaviorVisuals.empty()){++stats.animationAtlasBehaviorRejects;return nullptr;}
#endif
  if(buildingSurface||snapshotSurface||rasterFrame.preview.page||snapshotPressed>=0||snapshotInput||snapshotKeyboard.visible||styledFocus()||behaviorVisualMoving()||
     (snapshotGroupIndex>=0&&snapshotGroup.indicatorHeight)||
     (snapshotTransition&&snapshotPhase!=TransitionPhase::Enter)){++stats.animationAtlasContextRejects;return nullptr;}
  const PageDesc &p=*rasterFrame.current.page;const Rect pageRect{0,0,p.width,p.height};
  const bool fixedViewport=snapshotTransition&&(snapshotMotionRect.x||snapshotMotionRect.y||
     snapshotMotionRect.width!=p.width||snapshotMotionRect.height!=p.height);
  if(fixedViewport){
#if NICE_GFX_UI_VIEWPORT_ANIMATION_ATLAS
    if(snapshotMotionRect.x||snapshotMotionRect.width!=p.width||snapshotMotionRect.y<0||snapshotMotionRect.height<=0||
       int64_t(snapshotMotionRect.y)+snapshotMotionRect.height>p.height){++stats.animationAtlasViewportRejects;return nullptr;}
#else
    ++stats.animationAtlasViewportRejects;return nullptr;
#endif
  }
  bool eligible=false;for(auto &entry:surfaces){
    if(!entry.ready||entry.page!=&p||entry.key.size()!=rasterFrame.current.values.size()||entry.geometry.nodes.size()!=rasterFrame.current.values.size())continue;
    eligible=true;++stats.animationAtlasEligibleSources;
    uint8_t count=0;bool valid=true;
    for(uint16_t i=0;i<p.count&&valid;++i){
      if(sameSurfaceNode(entry.key[i],rasterFrame.current.values[i]))continue;
      const auto &node=p.nodes[i];const auto &before=entry.key[i];const auto &live=rasterFrame.current.values[i];
      if(count==16||(node.kind!=Kind::Container&&node.kind!=Kind::Button)||node.motion||
         before.opacity!=255||live.opacity!=255||
         node.animation.effect<AnimationEffect::SlideLeft||node.animation.effect>AnimationEffect::SlideDown){recordAnimationAtlasReject(entry,i,1);valid=false;break;}
      State normalized=live;normalized.animationX=before.animationX;normalized.animationY=before.animationY;
      if(!sameSurfaceNode(before,normalized)){recordAnimationAtlasReject(entry,i,2);valid=false;break;}
      const auto &g=entry.geometry.nodes[i];
      const Rect source=poseRect(p,entry.key,i),target=poseRect(p,rasterFrame.current.values,i);
      if(!g.visible||g.alpha!=255||!animationContains(pageRect,source)||!animationContains(pageRect,target)||
         !animationContains(g.clip,source)){recordAnimationAtlasReject(entry,i,3);valid=false;break;}
      const int32_t left=std::min(source.x,target.x),top=std::min(source.y,target.y);
      const Rect swept{left,top,int32_t(std::max(int64_t(source.x)+source.width,int64_t(target.x)+target.width)-left),
        int32_t(std::max(int64_t(source.y)+source.height,int64_t(target.y)+target.height)-top)};
      // Fixed system/navigation strips must never become part of a moving
      // sprite, including its erased terminal position and complete sweep.
      if(fixedViewport&&!animationContains(snapshotMotionRect,swept)){recordAnimationAtlasReject(entry,i,4);valid=false;break;}
      uint16_t background,backgroundOwner;
      if(!animationBackground(entry,i,swept,background,backgroundOwner)){recordAnimationAtlasReject(entry,i,5);valid=false;break;}
      snapshotAnimationLayers[count++]={i,source,target,background,backgroundOwner};
    }
    if(!valid){++stats.animationAtlasStateRejects;continue;}
    if(!count){++stats.animationAtlasZeroMotion;continue;}
    // A source sprite includes its antialiasing and rounded-corner backdrop.
    // Require the whole subtree to stay inside that rectangle, and reject
    // every external paint intersecting either its old or translated area.
    for(uint8_t layer=0;layer<count&&valid;++layer){
      const auto &sprite=snapshotAnimationLayers[layer];
      for(uint16_t i=0;i<p.count;++i){const auto &g=entry.geometry.nodes[i];
        if(!g.visible||!g.alpha||i==sprite.backgroundOwner)continue;
        const Rect painted=intersect(g.rect,g.clip);
        if(animationAncestor(p,sprite.root,i)){
          if(!animationContains(sprite.source,painted)){valid=false;break;}
        }else if(!animationAncestor(p,i,sprite.root)){
          bool moving=false;for(uint8_t other=0;other<count;++other)
            if(animationAncestor(p,snapshotAnimationLayers[other].root,i)){moving=true;break;}
          if(!moving&&(animationOverlaps(painted,sprite.source)||animationOverlaps(painted,sprite.target))){valid=false;break;}
        }
      }
      for(uint8_t other=0;other<layer&&valid;++other){const auto &previous=snapshotAnimationLayers[other];
        // Every old rectangle is erased before any target is drawn. Crossing
        // another moving sprite's old rectangle is safe; source/source and
        // target/target overlaps would contaminate a sprite or its backdrop.
        if(animationOverlaps(sprite.source,previous.source)||animationOverlaps(sprite.target,previous.target))valid=false;
      }
    }
    if(!valid){++stats.animationAtlasProofRejects;continue;}
    entry.used=++surfaceClock;snapshotAnimationLayerCount=count;snapshotAnimationSurface=&entry;++stats.animationAtlasAcceptedFrames;return &entry;
  }
  if(!eligible)++stats.animationAtlasSourceMisses;
  return nullptr;
}
// RouteChange handlers may update static data after a page was cached. Repair
// those pixels at the actual cached sprite pose, which need not be terminal.
// Keeping its real pose avoids repainting all translated subtrees for a small
// label update. Only this render snapshot changes; live clocks/state stay intact.
bool repairTerminalAnimationSurface(bool &geometryPrepared,uint64_t repairBudget=0) {
  if((snapshotPhase!=TransitionPhase::Enter
#if NICE_GFX_UI_VIEWPORT_ANIMATION_ATLAS
       &&snapshotTransition
#endif
      )||rasterFrame.preview.page||snapshotSurface||snapshotAnimationSurface||
     down||snapshotInput||snapshotKeyboard.visible||styledFocus()||behaviorVisualMoving())return false;
  Surface *source=nullptr;
  for(auto &entry:surfaces)if(entry.ready&&entry.page==rasterFrame.current.page&&entry.key.size()==rasterFrame.current.values.size()&&
      &entry!=previewSurface&&!systemModalBasePinned(entry)){source=&entry;break;}
  if(!source||source->geometry.nodes.size()!=rasterFrame.current.values.size())return false;
  struct Pose {uint16_t index;int32_t x,y;};Pose poses[16];uint8_t count=0;
  const auto &p=*rasterFrame.current.page;
  for(uint16_t i=0;i<p.count;++i){const auto &live=rasterFrame.current.values[i],&terminal=source->key[i];
    // Preserve the actual cached coordinates, including equal nonzero samples.
    // Static text/value changes must still pass the ordinary real raster repair;
    // no old image is relabelled and no state-comparison guard is bypassed.
    if(!live.animationX&&!live.animationY&&!terminal.animationX&&!terminal.animationY)continue;
    const auto &n=p.nodes[i];
    if(count==16||(n.kind!=Kind::Container&&n.kind!=Kind::Button)||n.motion||
       terminal.opacity!=255||live.opacity!=255||source->geometry.nodes[i].alpha!=255||opacity(p,rasterFrame.current.values,i)!=255||
       n.animation.effect<AnimationEffect::SlideLeft||n.animation.effect>AnimationEffect::SlideDown){recordAnimationAtlasReject(*source,i,6);return false;}
    poses[count++]={i,live.animationX,live.animationY};
  }
  if(!count)return false;
  for(uint8_t i=0;i<count;++i){const uint16_t at=poses[i].index;
    rasterFrame.current.values[at].animationX=source->key[at].animationX;
    rasterFrame.current.values[at].animationY=source->key[at].animationY;
  }
  bool terminalGeometry=false;const bool repaired=patchSurface(terminalGeometry,repairBudget,source)!=nullptr;
  for(uint8_t i=0;i<count;++i){auto &value=rasterFrame.current.values[poses[i].index];value.animationX=poses[i].x;value.animationY=poses[i].y;}
  // Even a rejected repair can prepare terminal geometry before its damage
  // budget check. A portable fallback must rebuild the restored live poses.
  geometryPrepared=false;
  return repaired;
}
Status paintAnimationSurface(const DirtyRect &area) {
  const auto &atlas=*snapshotAnimationSurface;const size_t width=rasterFrame.current.page->width;
  canvas.setViewport(area.x,area.y,area.width,area.height,false);
  Status status=canvas.blit(paintOffsetX,paintOffsetY,
    PixelView(atlas.pixels,rasterFrame.current.page->width,rasterFrame.current.page->height,width,cacheOrder));
  if(status!=Status::Ok)return status;
  for(uint8_t i=0;i<snapshotAnimationLayerCount;++i){const auto &sprite=snapshotAnimationLayers[i];
    canvas.fillRect(sprite.source.x+paintOffsetX,sprite.source.y+paintOffsetY,sprite.source.width,sprite.source.height,sprite.background);
  }
  for(uint8_t i=0;i<snapshotAnimationLayerCount;++i){const auto &sprite=snapshotAnimationLayers[i];
    status=canvas.blit(sprite.target.x+paintOffsetX,sprite.target.y+paintOffsetY,
      PixelView(atlas.pixels+size_t(sprite.source.y)*width+sprite.source.x,sprite.source.width,sprite.source.height,width,cacheOrder));
    if(status!=Status::Ok)return status;
  }
  ++surfaceStats.hits;return Status::Ok;
}
// Compose the exact same opaque sprites into the existing unblended Native
// fade stage. Copying is a byte-order conversion, never an alpha flattening:
// component opacity/reveal/affine cases remain on the ordered node painter.
Status composeAnimationNativeStage(const nice_gfx::NativeFramebuffer &target) {
#if NICE_GFX_UI_VIEWPORT_ANIMATION_ATLAS
  if(!snapshotAnimationSurface||!rasterFrame.current.page||!snapshotAnimationLayerCount||
     snapshotPhase!=TransitionPhase::Enter||rasterFrame.preview.page||
     transition.config.effect!=TransitionEffect::Fade)return Status::Unsupported;
  const auto &atlas=*snapshotAnimationSurface;const size_t width=rasterFrame.current.page->width,height=rasterFrame.current.page->height;
  if(!target.pixels||target.width!=width||target.height!=height||target.stride<width||!width||!height||
     height>SIZE_MAX/target.stride/2||target.bytes<target.stride*height*2||atlas.page!=rasterFrame.current.page||
     nativeMemoryOverlap(reinterpret_cast<uintptr_t>(target.pixels),target.bytes,atlas.pixels,width*height*2))return Status::Unsupported;
  const Rect screen{0,0,int32_t(width),int32_t(height)};
  for(uint8_t i=0;i<snapshotAnimationLayerCount;++i){const auto &sprite=snapshotAnimationLayers[i];
    if(!animationContains(screen,sprite.source)||!animationContains(screen,sprite.target))return Status::Unsupported;
  }
  const uint32_t started=profilingClock?profilingClock():0;
  const PixelView source(atlas.pixels,width,height,width,cacheOrder);const size_t bytes=width*height*2;
  const auto copy=[&](Rect from,int32_t x,int32_t y)->Status{
    if(ppaBlendEnabled&&rgb565_ppa::Context::available()){
      const auto status=ppaFade.copyIntoNative(target,x,y,source,bytes,from.x,from.y,from.width,from.height);
      if(status!=Status::Unsupported)return status;
    }
    auto *output=target.pixels+size_t(y)*target.stride+x;const auto *input=atlas.pixels+size_t(from.y)*width+from.x;
    if(cacheOrder==PixelOrder::Wire)rgb565_blend::nativeRowsFromWire(output,target.stride,input,width,nullptr,0,from.width,from.height,255);
    else for(int32_t row=0;row<from.height;++row)memcpy(output+size_t(row)*target.stride,input+size_t(row)*width,size_t(from.width)*2);
    return Status::Ok;
  };
  Status status=copy(screen,0,0);if(status!=Status::Ok)return status;
  for(uint8_t i=0;i<snapshotAnimationLayerCount;++i){const auto &sprite=snapshotAnimationLayers[i];const auto &r=sprite.source;
    auto *out=target.pixels+size_t(r.y)*target.stride+r.x;const size_t capacity=target.bytes/2-size_t(out-target.pixels);
    if(!nice_gfx::detail::fillWordRect16(out,capacity,r.width,r.height,target.stride,sprite.background))
      for(int32_t row=0;row<r.height;++row)std::fill(out+size_t(row)*target.stride,out+size_t(row)*target.stride+r.width,sprite.background);
  }
  for(uint8_t i=0;i<snapshotAnimationLayerCount;++i){const auto &sprite=snapshotAnimationLayers[i];status=copy(sprite.source,sprite.target.x,sprite.target.y);if(status!=Status::Ok)return status;}
  ++surfaceStats.hits;++stats.animationAtlasStageFrames;stats.animationAtlasStagePixels+=width*height;
  if(profilingClock)stats.animationAtlasStageMicros+=profilingClock()-started;
  return Status::Ok;
#else
  (void)target;return Status::Unsupported;
#endif
}
// Optional native framebuffer composition. All sources remain the immutable
// atlas; no in-place framebuffer move, scratch bitmap or source lease escapes.
// Unsupported is returned only before the first write. A driver error after a
// write must propagate to the caller, rather than starting a partial fallback.
Status composeAnimationNative(nice_gfx::Backend &panel,nice_gfx::NativeFramebuffer &output) {
  output=nice_gfx::NativeFramebuffer{};
  if(!snapshotAnimationSurface||!panel.supportsFrameBlit()||!rasterFrame.current.page||
     (snapshotTransition&&(snapshotPhase!=TransitionPhase::Enter||transitionFrame.opacity!=255)))return Status::Unsupported;
  const int32_t w=rasterFrame.current.page->width,h=rasterFrame.current.page->height;const Rect screen{0,0,w,h};
  const Rect viewport=snapshotTransition?snapshotMotionRect:screen;
  if(!animationContains(screen,viewport)||viewport.width<=0||viewport.height<=0)return Status::Unsupported;
  nice_gfx::NativeFramebuffer frame{};const Status mapped=panel.mapNativeFramebuffer(frame);
  if(mapped!=Status::Ok)return mapped;
  if(!frame.pixels||frame.width!=size_t(w)||frame.height!=size_t(h)||frame.stride<size_t(w)||
     frame.stride>SIZE_MAX/2||size_t(h-1)>(SIZE_MAX/2-size_t(w))/frame.stride||
     ((size_t(h-1)*frame.stride+size_t(w))*2)>frame.bytes)return Status::Unsupported;
  const uintptr_t destination=reinterpret_cast<uintptr_t>(frame.pixels),source=reinterpret_cast<uintptr_t>(snapshotAnimationSurface->pixels);
  const size_t destinationBytes=(size_t(h-1)*frame.stride+size_t(w))*2,sourceBytes=size_t(w)*h*2;
  if(destination>UINTPTR_MAX-destinationBytes||source>UINTPTR_MAX-sourceBytes||
     (destination<source+sourceBytes&&source<destination+destinationBytes)||
     !panel.canFrameBlit(PixelView(frame.pixels,w,h,frame.stride,PixelOrder::Native)))return Status::Unsupported;
  struct NativeOp {DirtyRect area;PixelView pixels{nullptr,0};bool solid=false;uint16_t color=0;};
  NativeOp operations[41];uint8_t count=0;
  const int32_t offsetX=snapshotTransition?transitionFrame.inX:0,offsetY=snapshotTransition?transitionFrame.inY:0;
  const auto fill=[&](Rect rect,uint16_t color){rect=intersect(rect,viewport);if(rect.width<=0||rect.height<=0)return;
    auto &op=operations[count++];op.area=DirtyRect(rect.x,rect.y,rect.width,rect.height);op.solid=true;op.color=color;};
  const auto copy=[&](Rect original,int32_t x,int32_t y,Rect clip){const Rect target{x,y,original.width,original.height};const Rect clipped=intersect(target,clip);
    if(clipped.width<=0||clipped.height<=0)return;auto &op=operations[count++];op.area=DirtyRect(clipped.x,clipped.y,clipped.width,clipped.height);
    const size_t sourceX=size_t(original.x+clipped.x-target.x),sourceY=size_t(original.y+clipped.y-target.y);
    op.pixels=PixelView(snapshotAnimationSurface->pixels+sourceY*size_t(w)+sourceX,clipped.width,clipped.height,w,cacheOrder);};
  Rect fixed[4];const int fixedCount=raster_occlusion::strips(screen,viewport,fixed);
  for(int i=0;i<fixedCount;++i)copy(fixed[i],fixed[i].x,fixed[i].y,screen);
  const Rect visible=intersect(viewport,{viewport.x+offsetX,viewport.y+offsetY,viewport.width,viewport.height});
  if(visible.width<=0||visible.height<=0)fill(viewport,snapshotBackground);
  else {
    Rect uncovered[4];const int uncoveredCount=raster_occlusion::strips(viewport,visible,uncovered);
    for(int i=0;i<uncoveredCount;++i)fill(uncovered[i],snapshotBackground);
    copy(viewport,viewport.x+offsetX,viewport.y+offsetY,viewport);
    for(uint8_t i=0;i<snapshotAnimationLayerCount;++i){const auto &sprite=snapshotAnimationLayers[i];fill({sprite.source.x+offsetX,sprite.source.y+offsetY,sprite.source.width,sprite.source.height},sprite.background);}
    for(uint8_t i=0;i<snapshotAnimationLayerCount;++i){const auto &sprite=snapshotAnimationLayers[i];copy(sprite.source,sprite.target.x+offsetX,sprite.target.y+offsetY,viewport);}
  }
  const int32_t alignX=panel.windowAlignmentX(),alignY=panel.windowAlignmentY();
  if(!alignX||!alignY)return Status::Unsupported;
  for(uint8_t i=0;i<count;++i){const auto &op=operations[i];const auto &r=op.area;
    if(r.x%alignX||r.y%alignY||r.width%alignX||r.height%alignY||(!op.solid&&!panel.canFrameBlit(op.pixels)))return Status::Unsupported;
  }
  for(uint8_t i=0;i<count;++i){const auto &op=operations[i];Status status;
    if(op.solid){status=panel.window(op.area.x,op.area.y,op.area.x+op.area.width-1,op.area.y+op.area.height-1);
      if(status==Status::Ok)status=panel.fill(op.color,uint32_t(op.area.pixels()));}
    else status=panel.flush(op.area.x,op.area.y,op.pixels,false);
    if(status!=Status::Ok){panel.finish();return status==Status::Unsupported?Status::IoError:status;}
  }
  const Status completed=panel.finish();if(completed!=Status::Ok)return completed==Status::Unsupported?Status::IoError:completed;
  output=frame;++surfaceStats.hits;return Status::Ok;
}
