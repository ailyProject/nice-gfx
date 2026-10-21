// Included inside View::Impl. The ordinary Canvas renderer can paint opaque,
// untransformed snapshots directly into a mapped native scanout framebuffer.
// Dirty regions and source leases stay under the normal Refresh lifecycle.
PixelOrder renderOrder=PixelOrder::Wire;
const CompressedSurface *exactCompressedFadeSource(const PageDesc *p,const RetainedVector<State> &values) {
  auto *source=p?compressedSurfaceFor(*p):nullptr;
  return source&&sameSurfaceState(source->key,values)?source:nullptr;
}
bool compressedFadeKeysReady() {
  return !rgb565_ppa::Context::available()&&snapshotTransition&&transition.config.effect==TransitionEffect::Fade&&
    !snapshotKeyboard.visible&&!snapshotInput&&
    (snapshotGroupIndex<0||!snapshotGroup.indicatorHeight)&&
    !transitionFrame.outX&&!transitionFrame.outY&&!transitionFrame.inX&&!transitionFrame.inY&&
    exactCompressedFadeSource(snapshotPage,snapshot)&&
    (!previewPage||exactCompressedFadeSource(previewPage,previewSnapshot));
}
// A stationary cached page fade can blend straight into RGB scanout on CPU
// backends as well. Preserve authored viewport and stationary navigation;
// only submitted dirty rectangles are written and all sources stay leased.
Status prepareCpuCachedFade(PixelView &surface) {
  const bool runSources=compressedFadeKeysReady();
  if(!snapshotTransition||(!snapshotSurface&&!runSources)||transition.config.effect!=TransitionEffect::Fade||
     snapshotKeyboard.visible||snapshotInput||(snapshotGroupIndex>=0&&snapshotGroup.indicatorHeight)||
     transitionFrame.outX||transitionFrame.outY||transitionFrame.inX||transitionFrame.inY||
     (previewPage&&!previewSurface&&!runSources)||
     (snapshotPhase!=TransitionPhase::Paired&&snapshotPhase!=TransitionPhase::Enter&&snapshotPhase!=TransitionPhase::Exit))return Status::Unsupported;
  const size_t w=snapshotPage->width,h=snapshotPage->height;
  if(previewPage&&(previewPage->width!=w||previewPage->height!=h))return Status::Unsupported;
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);
  if(!panel||!panel->supportsFrameBlit()||rgb565_ppa::Context::available())return Status::Unsupported;
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
    const auto *f=exactCompressedFadeSource(snapshotPage,snapshot),*b=previewPage?exactCompressedFadeSource(previewPage,previewSnapshot):nullptr;
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
            moving&&previewPage?&backRuns:nullptr,offset,moving?transitionFrame.opacity:255,snapshotBackground);return;}
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
  surfaceStats.hits+=previewPage?2:1;surface=candidate;return Status::Ok;
}
const nice_gfx::NativeFramebuffer *stagingNativeTarget=nullptr;
struct NativeRenderFrame {nice_gfx::NativeFramebuffer frame{};DirtyRect regions[16];uint8_t count=0;};
bool nativeOpaquePage(const PageDesc &p,const RetainedVector<State> &values) const {
  for(uint16_t i=0;i<p.count;++i)if(visible(p,values,i)){
    // Affine/reveal motion kernels still use Wire565. Ordinary node opacity
    // uses a bounded, order-aware underlay; legacy slides remain supported.
    if(p.nodes[i].motion)return false;
  }
  return true;
}
static bool nativeMemoryOverlap(uintptr_t destination,size_t destinationBytes,const void *source,size_t bytes) {
  if(!source||!bytes)return false;const uintptr_t begin=reinterpret_cast<uintptr_t>(source);
  return begin>UINTPTR_MAX-bytes||(begin<destination+destinationBytes&&destination<begin+bytes);
}
bool nativeSourceOverlap(const PageDesc &p,const RetainedVector<State> &values,uintptr_t output,size_t bytes,bool includeHidden=false) const {
  for(uint16_t i=0;i<p.count;++i){if(!includeHidden&&!visible(p,values,i))continue;const auto &n=p.nodes[i];const auto &v=values[i];
    if(n.kind==Kind::Media){const MediaDesc *source=media(n,v);if(source){
      size_t sourceBytes=0;
      if(v.imageOverride&&v.image.impl_)sourceBytes=v.image.impl_->source.byteLength;
      else {
        if(!source->width||!source->frames||!source->bytesPerPixel||size_t(source->height)>SIZE_MAX/source->width||
           size_t(source->width)*source->height>SIZE_MAX/source->frames||
           size_t(source->width)*source->height*source->frames>SIZE_MAX/source->bytesPerPixel)return true;
        sourceBytes=size_t(source->width)*source->height*source->frames*source->bytesPerPixel;
      }
      if(nativeMemoryOverlap(output,bytes,source->pixels,sourceBytes))return true;
    }}
    if(n.kind==Kind::Icon&&n.icon&&nativeMemoryOverlap(output,bytes,n.icon->bitmap,n.icon->bytes))return true;
  }
  return false;
}
bool nativeNodeScratchReady(uintptr_t target,size_t targetBytes) const {
  bool needed=false;
  for(const auto *values:{&snapshot,&previewSnapshot})for(const auto &value:*values)
    if(value.opacity&&value.opacity!=255){needed=true;break;}
  if(!needed)return true;
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);
  if(refresh->busy()||!panel||panel->busy()||!tileBuffers[0]||!capacity||capacity>SIZE_MAX/2)return false;
  const size_t bytes=capacity*2;const uintptr_t scratch=reinterpret_cast<uintptr_t>(tileBuffers[0]);
  if(scratch>UINTPTR_MAX-bytes||nativeMemoryOverlap(target,targetBytes,tileBuffers[0],bytes)||
     nativeSourceOverlap(*snapshotPage,snapshot,scratch,bytes,true)||
     (previewPage&&nativeSourceOverlap(*previewPage,previewSnapshot,scratch,bytes,true)))return false;
  const size_t rawBytes=size_t(snapshotPage->width)*snapshotPage->height*2;
  for(const auto &surface:surfaces)if(nativeMemoryOverlap(scratch,bytes,surface.pixels,rawBytes))return false;
  if(nativeMemoryOverlap(scratch,bytes,compressedStorage,compressedCapacity*2))return false;
  return true;
}
Status prepareNativeRender(NativeRenderFrame &candidate) {
  candidate=NativeRenderFrame{};
  if(buildingSurface||snapshotSurface||snapshotAnimationSurface||circularKeyboard||snapshotKeyboard.visible||
     compressedFadeKeysReady()||
     (snapshotTransition&&transitionFrame.opacity!=255)||!nativeOpaquePage(*snapshotPage,snapshot)||
     (previewPage&&!nativeOpaquePage(*previewPage,previewSnapshot)))return Status::Unsupported;
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);
  if(!panel||!panel->supportsFrameBlit()||!refresh->pendingRegions()||refresh->pendingRegions()>16)return Status::Unsupported;
  nice_gfx::NativeFramebuffer frame{};const Status mapped=panel->mapNativeFramebuffer(frame);
  if(mapped!=Status::Ok)return mapped;
  if(!frame.pixels||frame.width!=snapshotPage->width||frame.height!=snapshotPage->height||frame.stride<frame.width||
     !frame.width||!frame.height||frame.stride>SIZE_MAX/2/frame.height||frame.bytes<frame.stride*frame.height*2)return Status::Unsupported;
  const uintptr_t output=reinterpret_cast<uintptr_t>(frame.pixels);const size_t bytes=frame.stride*frame.height*2;
  if(output>UINTPTR_MAX-bytes||nativeSourceOverlap(*snapshotPage,snapshot,output,bytes)||
     (previewPage&&nativeSourceOverlap(*previewPage,previewSnapshot,output,bytes))||
     !nativeNodeScratchReady(output,bytes))return Status::Unsupported;
  for(const Surface *surface:{snapshotSurface,previewSurface,snapshotAnimationSurface})if(surface&&
      nativeMemoryOverlap(output,bytes,surface->pixels,size_t(surface->page->width)*surface->page->height*2))return Status::Unsupported;
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
// These keys deliberately own no Image, instrument or text string. Retain only
// immutable DynamicNode identity, preserving Surface's COW/ABA protection.
// Keep the scalar geometry fields in sync with sameSurfaceGeometry().
struct NativeGeometryKey {
  struct Text {const char *compiled=nullptr;uint64_t revision=0;} text;
  int32_t scrollX=0,scrollY=0,animationX=0,animationY=0;
  std::shared_ptr<const DynamicNode> dynamic;
  uint32_t motionRevision=0;
  uint16_t flags=0;
  uint8_t opacity=255;
  bool matches(const State &value) const {
    return scrollX==value.scrollX&&scrollY==value.scrollY&&flags==value.flags&&
      animationX==value.animationX&&animationY==value.animationY&&opacity==value.opacity&&
      motionRevision==value.motionRevision&&dynamic==value.dynamic;
  }
  void assign(const State &value) {
    text.compiled=value.text.compiled;text.revision=value.text.revision;
    scrollX=value.scrollX;scrollY=value.scrollY;flags=value.flags;
    animationX=value.animationX;animationY=value.animationY;opacity=value.opacity;
    motionRevision=value.motionRevision;dynamic=value.dynamic;
  }
};
const PageDesc *nativeGeometryPage=nullptr;
RetainedVector<NativeGeometryKey> nativeGeometryKey;
FrameCache nativeGeometryCache;
void clearNativeGeometryMemo() {
  nativeGeometryPage=nullptr;RetainedVector<NativeGeometryKey>().swap(nativeGeometryKey);nativeGeometryCache=FrameCache{};
}
void retainNativeGeometry(const PageDesc &p,const RetainedVector<State> &values,const FrameCache &cache) {
  nativeGeometryKey.resize(values.size());
  for(size_t i=0;i<values.size();++i)nativeGeometryKey[i].assign(values[i]);
  nativeGeometryCache=cache;nativeGeometryPage=&p;
}
void prepareNativeGeometry(const PageDesc &p,const RetainedVector<State> &values,FrameCache &cache) {
  if(nativeGeometryPage==&p&&nativeGeometryKey.size()==values.size()&&nativeGeometryCache.nodes.size()==values.size()){
    bool same=true;for(size_t i=0;i<values.size();++i)if(!nativeGeometryKey[i].matches(values[i])){same=false;break;}
    if(same){prepareVisualCache(p,values,nativeGeometryKey,nativeGeometryCache,cache);retainNativeGeometry(p,values,cache);return;}
  }
  for(const auto &entry:surfaces)if(entry.page==&p&&entry.ready&&entry.key.size()==values.size()&&entry.geometry.nodes.size()==values.size()){
    bool same=true;for(size_t i=0;i<values.size();++i)if(!sameSurfaceGeometry(entry.key[i],values[i])){same=false;break;}
    if(same){prepareVisualCache(p,values,entry.key,entry.geometry,cache);retainNativeGeometry(p,values,cache);return;}
  }
  prepareCache(p,values,cache);retainNativeGeometry(p,values,cache);
}
// Isolate the visible opaque image in a single-page slide. Outside pieces
// retain ordinary background/sidebar painting; the covered piece can use a
// direct source copy and omit every overwritten background. Commit the split
// only after every region/source is eligible, with no writes or allocation.
void partitionNativeRegions(NativeRenderFrame &candidate,bool identity=false) {
  if(!snapshotTransition)identity=true;
  if(previewPage||(identity?snapshotTransition:!opaqueCoverTransition())||buildingSurface||snapshotCache.nodes.empty())return;
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);if(!panel)return;
  const bool entering=snapshotPhase==TransitionPhase::Enter;
  const int32_t offsetX=identity?0:entering?transitionFrame.inX:transitionFrame.outX,offsetY=identity?0:entering?transitionFrame.inY:transitionFrame.outY;
  int image=-1;Rect cover{};
  for(size_t end=snapshotCache.nodes.size();end;--end){const size_t i=end-1;if(!opaqueCoverMedia(snapshotCache,i))continue;
    Rect visible=intersect(snapshotCache.nodes[i].rect,snapshotCache.nodes[i].clip);visible.x+=offsetX;visible.y+=offsetY;
    visible=intersect(visible,{0,0,snapshotPage->width,snapshotPage->height});if(visible.width<=0||visible.height<=0)continue;
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
    if(stagingNativeTarget){if(!canStageOpaqueCover(candidate.frame,snapshotCache,image,copied,offsetX,offsetY))return;}
    else if(!panel->canFrameBlit(opaqueCoverPixels(snapshotCache,image,copied,offsetX,offsetY)))return;
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
  const Rect viewport=snapshotMotionRect;
  if(!rgb565_ppa::Context::available()||buildingSurface||renderingViewport||previewPage||!snapshotTransition||
     snapshotSurface||snapshotAnimationSurface||snapshotInput||snapshotKeyboard.visible||snapshotPressed>=0||down||
     snapshotGroupIndex>=0||transition.config.effect!=TransitionEffect::Fade||
     (snapshotPhase!=TransitionPhase::Enter&&snapshotPhase!=TransitionPhase::Exit)||
     !transitionFrame.opacity||transitionFrame.opacity==255||viewport.x||viewport.y||
     viewport.width!=snapshotPage->width||viewport.height!=snapshotPage->height||
     transitionFrame.outX||transitionFrame.outY||transitionFrame.inX||transitionFrame.inY||
     !nativeOpaquePage(*snapshotPage,snapshot))return Status::Unsupported;
  auto *panel=nice_gfx::detail::DisplayAccess::panel(display);
  if(!panel||!panel->supportsFrameBlit())return Status::Unsupported;
  struct Timer {
    Impl &self;uint64_t before;uint32_t maximum,started;
    explicit Timer(Impl &s):self(s),before(s.stats.renderMicros),maximum(s.stats.maxRenderMicros),started(s.profilingClock?s.profilingClock():0){}
    ~Timer(){if(self.profilingClock){const uint32_t elapsed=self.profilingClock()-started;self.stats.renderMicros=before+elapsed;self.stats.maxRenderMicros=std::max(maximum,elapsed);}}
  } timer(*this);
  const size_t w=snapshotPage->width,h=snapshotPage->height,bytes=w*h*2;
  nice_gfx::NativeFramebuffer target{};const Status mapped=panel->mapNativeFramebuffer(target);
  if(mapped!=Status::Ok)return mapped;
  if(!target.pixels||target.width!=w||target.height!=h||target.stride!=w||target.bytes<bytes||
     (reinterpret_cast<uintptr_t>(target.pixels)&63)||(bytes&63)||
     !panel->canFrameBlit(PixelView(target.pixels,w,h,w,PixelOrder::Native))||
     nativeSourceOverlap(*snapshotPage,snapshot,reinterpret_cast<uintptr_t>(target.pixels),bytes))return Status::Unsupported;
  auto *slot=acquireFrameSurface(snapshotSurface,previewSurface,snapshotAnimationSurface);if(!slot)return Status::Unsupported;
  nice_gfx::NativeFramebuffer stage{slot->pixels,w,h,w,bytes};
  if(!stage.pixels||(reinterpret_cast<uintptr_t>(stage.pixels)&63)||
     nativeMemoryOverlap(reinterpret_cast<uintptr_t>(target.pixels),bytes,stage.pixels,bytes)||
     nativeSourceOverlap(*snapshotPage,snapshot,reinterpret_cast<uintptr_t>(stage.pixels),bytes)||
     !nativeNodeScratchReady(reinterpret_cast<uintptr_t>(target.pixels),bytes)||
     !nativeNodeScratchReady(reinterpret_cast<uintptr_t>(stage.pixels),bytes))return Status::Unsupported;
  struct Scope {
    Impl &self;bool transition,full;PixelOrder order;const nice_gfx::NativeFramebuffer *staging;FrameCache *paint;int32_t x,y;
    Scope(Impl &owner,const nice_gfx::NativeFramebuffer &frame):self(owner),transition(owner.snapshotTransition),full(owner.renderingFullFrameSurface),order(owner.renderOrder),staging(owner.stagingNativeTarget),paint(owner.paintCache),x(owner.paintOffsetX),y(owner.paintOffsetY){
      self.snapshotTransition=false;self.renderingFullFrameSurface=true;self.renderOrder=PixelOrder::Native;self.stagingNativeTarget=&frame;self.paintOffsetX=self.paintOffsetY=0;
    }
    ~Scope(){self.snapshotTransition=transition;self.renderingFullFrameSurface=full;self.renderOrder=order;self.stagingNativeTarget=staging;self.paintCache=paint;self.paintOffsetX=x;self.paintOffsetY=y;}
  };
  {
    Scope scope(*this,stage);
    prepareNativeGeometry(*snapshotPage,snapshot,snapshotCache);
    NativeRenderFrame pieces;pieces.frame=stage;pieces.regions[0]=DirtyRect(0,0,w,h);pieces.count=1;partitionNativeRegions(pieces,true);
    for(uint8_t i=0;i<pieces.count;++i){const auto &area=pieces.regions[i];
      const Status painted=render(this,area,stage.pixels+size_t(area.y)*w+area.x,w);if(painted!=Status::Ok)return painted;
    }
  }
  const uint32_t started=profilingClock?profilingClock():0;
  const Status blended=ppaFade.blendSolidNativeIntoNative(target.pixels,target.stride,target.bytes,
    stage.pixels,w,w,h,snapshotBackground,transitionFrame.opacity);
  if(blended!=Status::Ok){if(blended!=Status::Unsupported)++stats.ppaBlendFailures;return blended;}
  ++stats.ppaBlendCalls;stats.ppaBlendPixels+=w*h;if(profilingClock)stats.ppaBlendMicros+=profilingClock()-started;
  output=PixelView(target.pixels,w,h,w,PixelOrder::Native);return Status::Ok;
}
Status paintNativeRender(const NativeRenderFrame &candidate) {
  const bool savedFull=renderingFullFrameSurface;const PixelOrder savedOrder=renderOrder;
  renderingFullFrameSurface=true;renderOrder=PixelOrder::Native;
  Status status=Status::Ok;
  for(uint8_t i=0;i<candidate.count;++i){const auto &area=candidate.regions[i];
    status=render(this,area,candidate.frame.pixels+size_t(area.y)*candidate.frame.stride+area.x,candidate.frame.stride);
    if(status!=Status::Ok)break;
  }
  renderOrder=savedOrder;renderingFullFrameSurface=savedFull;return status;
}
