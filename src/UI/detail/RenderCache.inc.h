// Included inside View::Impl. Surfaces contain only page content in wire order;
// fixed navigation chrome and transient focus/press feedback stay separate.
struct Surface {
  uint16_t *pixels=nullptr;
  const PageDesc *page=nullptr;
  RetainedVector<State> key;
  FrameCache geometry;
  int32_t row=0,column=0;
  uint64_t used=0;
  uint64_t pixelsRevision=0;
  uint64_t presentedAtFrame=UINT64_MAX;
  bool ready=false,paused=false,compressionAttempted=false;
  uint32_t pauseEpoch=0,quietAt=0;
};
std::vector<Surface> surfaces;
std::vector<int> surfaceWanted;
std::vector<int> surfacePrewarm; // One-shot route requests, bounded by project pages.
bool surfaceWarmPending=false;
uint32_t surfaceMutationEpoch=0;
uint16_t *tileBuffers[2]={nullptr,nullptr};
size_t surfacePixels=0;
uint64_t surfaceClock=0;
size_t surfaceWarmNext=0;
RenderCacheStats surfaceStats;
const Surface *snapshotSurface=nullptr,*previewSurface=nullptr;
Surface *seedSurface=nullptr;
uint64_t seedPixels=0;
uint64_t seedExpectedPixels=0;
bool seedFullPage=false;
bool buildingSurface=false;

static bool sameSurfaceNode(const State &x,const State &y) {
    if(x.text.compiled!=y.text.compiled||x.text.revision!=y.text.revision||
       x.value!=y.value||x.scrollX!=y.scrollX||x.scrollY!=y.scrollY||x.flags!=y.flags||
       x.mediaFrame!=y.mediaFrame||x.image.impl_!=y.image.impl_||x.imageOverride!=y.imageOverride||x.animationX!=y.animationX||x.animationY!=y.animationY||
       x.opacity!=y.opacity||x.switchPosition!=y.switchPosition||x.motionRevision!=y.motionRevision||x.dynamic!=y.dynamic)return false;
    // Route entry resets animation clocks and may COW-clone an instrument
    // without changing any pixels. Compare its visible samples/number, while
    // retaining the old shared pointer so actual writers still copy safely.
    if(x.instrument!=y.instrument){
      if(!x.instrument||!y.instrument||x.instrument->visual!=y.instrument->visual||x.instrument->data!=y.instrument->data)return false;
      for(int s=0;s<4;++s)if(x.instrument->head[s]!=y.instrument->head[s]||x.instrument->count[s]!=y.instrument->count[s])return false;
    }
  return true;
}
static bool sameSurfaceGeometry(const State &x,const State &y) {
  return x.scrollX==y.scrollX&&x.scrollY==y.scrollY&&x.flags==y.flags&&
    x.animationX==y.animationX&&x.animationY==y.animationY&&x.opacity==y.opacity&&x.motionRevision==y.motionRevision&&x.dynamic==y.dynamic;
}
static void releaseSurfaceText(Surface &surface) {
  // Finished pixels and text layout offsets are independent of source text.
  // Keep the exact revision, but let ordinary synchronous setters reuse their
  // string buffer. Partial warm keys and live DMA snapshots keep their owners.
  for(auto &state:surface.key)state.text.owned.reset();
}
static bool sameSurfaceState(const RetainedVector<State> &a,const RetainedVector<State> &b) {
  if(a.size()!=b.size())return false;
  for(size_t i=0;i<a.size();++i)if(!sameSurfaceNode(a[i],b[i]))return false;
  return true;
}
#include "RenderCacheCompressed.inc.h"
static bool surfaceImageOnlyChange(State &old,const State &live) {
  if(old.image.impl_==live.image.impl_&&old.imageOverride==live.imageOverride)return false;
  Image retained=old.image;const bool overrideSource=old.imageOverride;
  old.image=live.image;old.imageOverride=live.imageOverride;
  const bool equal=sameSurfaceNode(old,live);
  old.image=std::move(retained);old.imageOverride=overrideSource;
  return equal;
}
bool fastSurfaceImageRepair(uint16_t index,const Geometry &geometry) const {
  const auto &node=rasterFrame.current.page->nodes[index];const auto &value=rasterFrame.current.values[index];
  if(node.kind!=Kind::Media||node.motion||geometry.alpha!=255||!value.imageOverride||!value.image.impl_)return false;
  const auto &image=*value.image.impl_;const auto &style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,index);
  return !image.flash&&image.source.format==ImageFormat::RGB565&&!(image.source.strideBytes&1)&&
    image.media.width==geometry.rect.width&&image.media.height==geometry.rect.height&&
    !style.borderWidth&&!style.radius&&!style.paddingTop&&!style.paddingRight&&!style.paddingBottom&&!style.paddingLeft;
}

// Repair visual changes without discarding a retained page's static content.
// Ordinary changes have a four-tile budget; full-size opaque RAM image updates
// may repair at most one page through the same renderer, preserving overlays.
Surface *patchSurface(bool &geometryPrepared,uint64_t repairBudget=0) {
  ProfileTimer repairTimer(*this,&RuntimeStats::repairMicros);
  // A claimed follow gesture owns immutable incoming/outgoing snapshots and
  // cancels pressed-control feedback. Repair those keys once even while the
  // physical pointer remains down; ordinary pressed/input frames stay excluded.
  const bool pairedGesture=groupMotion.active&&groupMotion.prepared&&snapshotTransition&&snapshotPhase==TransitionPhase::Paired;
  if((down&&!pairedGesture)||snapshotInput||snapshotKeyboard.visible)return nullptr;
  Surface *slot=nullptr;
  for(auto &entry:surfaces)if(entry.page==rasterFrame.current.page&&entry.ready&&entry.key.size()==rasterFrame.current.values.size()){slot=&entry;break;}
  if(!slot||slot==previewSurface||slot->geometry.nodes.size()!=rasterFrame.current.values.size())return nullptr;
  bool visualOnly=true;
  for(size_t i=0;i<rasterFrame.current.values.size();++i)if(!sameSurfaceGeometry(slot->key[i],rasterFrame.current.values[i])){visualOnly=false;break;}
  if(!visualOnly){prepareCache(*rasterFrame.current.page,rasterFrame.current.values,rasterFrame.current.geometry);geometryPrepared=true;}
  DirtyRegions<16> damage(0),ordinaryDamage(0);
  bool fastImage=false;Rect fastImageCoverage{0,0,0,0};
  const auto sameRect=[](Rect a,Rect b){return a.x==b.x&&a.y==b.y&&a.width==b.width&&a.height==b.height;};
  for(uint16_t i=0;i<rasterFrame.current.values.size();++i) {
    const auto &old=slot->geometry.nodes[i];
    const auto &current=visualOnly?old:rasterFrame.current.geometry.nodes[i];
    const bool unchanged=sameSurfaceNode(slot->key[i],rasterFrame.current.values[i]);
    // A small overlay can change geometry while the camera's resolved bounds
    // remain identical. Classify this image separately instead of making its
    // entire map ordinary damage merely because another node moved. Compare
    // resolved geometry: ancestor movement, clipping and opacity must never
    // qualify through the Media node's unchanged local State alone.
    if(old.visible&&current.visible&&old.alpha==current.alpha&&sameRect(old.rect,current.rect)&&sameRect(old.clip,current.clip)&&
       fastSurfaceImageRepair(i,current)&&(unchanged||surfaceImageOnlyChange(slot->key[i],rasterFrame.current.values[i]))){
      const Rect r=intersect(current.rect,current.clip);
      if(!unchanged)damage.add(DirtyRect(r.x,r.y,r.width,r.height),rasterFrame.current.page->width,rasterFrame.current.page->height);
      // Recognition may move an overlay between two camera frames. The same
      // immutable opaque RAM map still restores those pixels through clipped
      // row copies; requiring a new Image in this tick would reject the repair
      // and let idle warming replace this useful completed key with a partial.
      // Keep one exact opaque rectangle, never the bounding box of separated
      // images. Subtraction below may conservatively miss other coverage but
      // must not mistake an uncovered hole for already-repainted image pixels.
      if(uint64_t(r.width)*r.height>uint64_t(fastImageCoverage.width)*fastImageCoverage.height)fastImageCoverage=r;
      fastImage=true;continue;
    }
    if(unchanged)continue;
    if(visualOnly){const auto &g=slot->geometry.nodes[i];
      if(g.visible&&g.alpha){const Rect r=intersect(g.rect,g.clip);const DirtyRect changed(r.x,r.y,r.width,r.height);
        damage.add(changed,rasterFrame.current.page->width,rasterFrame.current.page->height);
        ordinaryDamage.add(changed,rasterFrame.current.page->width,rasterFrame.current.page->height);
      }continue;
    }
    for(uint16_t k=i;k<rasterFrame.current.values.size();++k){
      int a=k;while(a>=0&&a!=i)a=rasterFrame.current.page->nodes[a].parent==NoParent?-1:rasterFrame.current.page->nodes[a].parent;
      if(a!=i)continue;
      for(const auto *geometry:{&slot->geometry,&rasterFrame.current.geometry}){
        const auto &g=geometry->nodes[k];const Rect r=intersect(g.rect,g.clip);
        damage.add(DirtyRect(r.x,r.y,r.width,r.height),rasterFrame.current.page->width,rasterFrame.current.page->height);
        ordinaryDamage.add(DirtyRect(r.x,r.y,r.width,r.height),rasterFrame.current.page->width,rasterFrame.current.page->height);
      }
    }
  }
  // Selection and disclosure arrows borrow state from a component root,
  // pager or panel. Their buttons can lie outside that owner's rectangle.
  // Compare those visual dependencies before repairing the retained pixels.
  if(componentCount)for(uint16_t i=0;i<rasterFrame.current.values.size();++i)if(rasterFrame.current.page->nodes[i].kind==Kind::Button){
    const Id id=rasterFrame.current.page->nodes[i].id;
    const auto previous=componentPaint(*rasterFrame.current.page,slot->key,id),current=componentPaint(*rasterFrame.current.page,rasterFrame.current.values,id);
    if(previous.selected==current.selected&&previous.expanded==current.expanded)continue;
    for(const auto *geometry:{&slot->geometry,visualOnly?&slot->geometry:&rasterFrame.current.geometry}){
      const auto &g=geometry->nodes[i];if(!g.visible||!g.alpha)continue;
      const Rect r=intersect(g.rect,g.clip);damage.add(DirtyRect(r.x,r.y,r.width,r.height),rasterFrame.current.page->width,rasterFrame.current.page->height);
      ordinaryDamage.add(DirtyRect(r.x,r.y,r.width,r.height),rasterFrame.current.page->width,rasterFrame.current.page->height);
    }
  }
  uint64_t pixels=0;for(size_t i=0;i<damage.size();++i)pixels+=damage[i].pixels();
  surfaceStats.lastRepairPixels=pixels;
  const uint64_t smallBudget=repairBudget?repairBudget:uint64_t(capacity)*4;
  if(pixels>smallBudget){
    uint64_t ordinaryPixels=0;for(size_t i=0;i<ordinaryDamage.size();++i){const auto &r=ordinaryDamage[i];
      const Rect covered=intersect({r.x,r.y,r.width,r.height},fastImageCoverage);
      ordinaryPixels+=r.pixels()-uint64_t(covered.width)*covered.height;
    }
    // Full-size opaque RAM images use clipped row copies. Repair their changed
    // rectangle once in the retained page, including unchanged overlay chrome,
    // instead of discarding a complete surface for every incoming map frame.
    // Other visual changes retain the old four-tile limit. Work never exceeds
    // one page. Resolved image geometry stays unchanged; unrelated ordinary
    // old/new subtree damage outside that one already-repainted image retains
    // the original four-tile budget. A large person box inside a camera image
    // adds no pixels: the same full image region is repainted in paint order.
    if(!fastImage||ordinaryPixels>smallBudget||pixels>uint64_t(rasterFrame.current.page->width)*rasterFrame.current.page->height)return nullptr;
  }
  if(visualOnly){prepareVisualCache(*rasterFrame.current.page,rasterFrame.current.values,slot->key,slot->geometry,rasterFrame.current.geometry);geometryPrepared=true;}
  const uint64_t savedTiles=stats.renderedTiles,savedPixels=stats.renderedPixels;
  FrameCache *savedPaint=paintCache;const bool savedBuilding=buildingSurface;
  const bool savedTransition=snapshotTransition;const PageDesc *savedPreview=rasterFrame.preview.page;
  snapshotTransition=false;rasterFrame.preview.page=nullptr;
  buildingSurface=true;
  ++slot->pixelsRevision;
  slot->presentedAtFrame=UINT64_MAX;
  for(size_t i=0;i<damage.size();++i){const auto &area=damage[i];
    if(render(this,area,slot->pixels+size_t(area.y)*rasterFrame.current.page->width+area.x,rasterFrame.current.page->width)!=Status::Ok){slot->ready=false;slot=nullptr;break;}
  }
  buildingSurface=savedBuilding;paintCache=savedPaint;snapshotTransition=savedTransition;rasterFrame.preview.page=savedPreview;
  stats.renderedTiles=savedTiles;stats.renderedPixels=savedPixels;
  if(slot){slot->key=rasterFrame.current.values;releaseSurfaceText(*slot);slot->geometry=rasterFrame.current.geometry;slot->used=++surfaceClock;slot->compressionAttempted=false;surfaceWarmPending=true;++surfaceStats.repairedPages;surfaceStats.repairPixels+=pixels;}
  return slot;
}
Surface *findSurface(const PageDesc &p,const RetainedVector<State> &s) {
  for(auto &entry:surfaces)if(entry.page==&p&&entry.ready&&sameSurfaceState(entry.key,s)){
    entry.used=++surfaceClock;return &entry;
  }
  return nullptr;
}
// Borrow an existing pixel slot for an immutable refresh frame. The caller
// excludes every source retained by that refresh; warming cannot run while
// Refresh owns it. No pixel allocation or cache-budget expansion is required.
Surface *acquireFrameSurface(const Surface *a,const Surface *b,const Surface *c) {
  Surface *slot=nullptr;
  for(auto &entry:surfaces)if(&entry!=a && &entry!=b && &entry!=c){
    if(!entry.page){slot=&entry;break;}
    if(!slot||entry.used<slot->used)slot=&entry;
  }
  if(!slot)return nullptr;
  ++slot->pixelsRevision;
  slot->presentedAtFrame=UINT64_MAX;
  slot->page=nullptr;slot->key.clear();slot->geometry=FrameCache{};
  slot->ready=slot->paused=slot->compressionAttempted=false;slot->row=slot->column=0;
  slot->used=++surfaceClock;surfaceWarmPending=true;
  return slot;
}
// Cold paired snapshots are rasterized into an existing borrowed slot. Both
// interactive slides and ordinary fades need immutable incoming/outgoing keys;
// waiting for idle warming would rerasterize them at every transition sample.
// Live incoming animations still advance and repair/capture their new key on
// subsequent frames. No additional surface allocation or clock freezing occurs.
Surface *captureGestureSurface(bool &geometryPrepared) {
  ProfileTimer captureTimer(*this,&RuntimeStats::captureMicros);
  const bool pairedFade=snapshotTransition&&snapshotPhase==TransitionPhase::Paired&&transition.config.effect==TransitionEffect::Fade;
  if((!pairedFade&&(!groupMotion.active||!groupMotion.prepared))||!snapshotTransition||snapshotPhase!=TransitionPhase::Paired||
     surfaces.size()<2||snapshotPressed>=0||snapshotInput||snapshotKeyboard.visible||snapshotAnimationSurface)return nullptr;
  Surface *slot=nullptr;
  for(auto &candidate:surfaces)if(candidate.page==rasterFrame.current.page && &candidate!=snapshotSurface && &candidate!=previewSurface){slot=&candidate;break;}
  if(!slot)slot=acquireFrameSurface(snapshotSurface,previewSurface,nullptr);
  if(!slot)return nullptr;
  prepareFrameGeometry(*rasterFrame.current.page,rasterFrame.current.values,rasterFrame.current.geometry);geometryPrepared=true;
  const bool savedTransition=snapshotTransition;const auto *savedPreview=rasterFrame.preview.page;
  const bool savedBuilding=buildingSurface;const int savedGroup=snapshotGroupIndex;FrameCache *savedPaint=paintCache;
  const uint64_t tiles=stats.renderedTiles,pixels=stats.renderedPixels;
  slot->ready=false;slot->paused=false;slot->compressionAttempted=false;slot->presentedAtFrame=UINT64_MAX;++slot->pixelsRevision;
  snapshotTransition=false;rasterFrame.preview.page=nullptr;buildingSurface=true;snapshotGroupIndex=-1;
  const auto &p=*rasterFrame.current.page;
  const Status result=render(this,DirtyRect(0,0,p.width,p.height),slot->pixels,p.width);
  snapshotTransition=savedTransition;rasterFrame.preview.page=savedPreview;buildingSurface=savedBuilding;snapshotGroupIndex=savedGroup;paintCache=savedPaint;
  stats.renderedTiles=tiles;stats.renderedPixels=pixels;
  if(result!=Status::Ok){slot->page=nullptr;slot->key.clear();return nullptr;}
  slot->page=&p;slot->key=rasterFrame.current.values;slot->geometry=rasterFrame.current.geometry;slot->row=p.height;slot->column=0;
  slot->ready=true;slot->used=++surfaceClock;releaseSurfaceText(*slot);++surfaceStats.capturedPages;surfaceWarmPending=true;
  return slot;
}
bool surfaceReady(int index) const {
  if(index<0||pages[size_t(index)].empty())return false;
  const auto &p=project->pages[index];
  for(const auto &entry:surfaces)if(entry.page==&p&&entry.ready&&sameSurfaceState(entry.key,pages[index]))return true;
  return compressedSurfaceReady(index);
}
void clearSurfacePage(const PageDesc *p) {
  surfaceWarmPending=!surfaces.empty();
  clearCompressedPage(p);
  for(auto &entry:surfaces)if(entry.page==p){++entry.pixelsRevision;entry.page=nullptr;entry.key.clear();entry.geometry=FrameCache{};entry.ready=entry.compressionAttempted=false;entry.row=entry.column=0;}
}
void clearSurfaces() {
  seedSurface=nullptr;seedPixels=seedExpectedPixels=0;
  coldFadeSurface=nullptr;coldFadePage=nullptr;
  clearCompressedSurfaces();
  surfaces.clear();surfacePixels=0;surfaceClock=0;surfaceWarmNext=0;surfaceStats=RenderCacheStats{};
  std::vector<int>().swap(surfaceWanted);std::vector<int>().swap(surfacePrewarm);surfaceWarmPending=false;
  snapshotSurface=previewSurface=snapshotAnimationSurface=nullptr;snapshotAnimationLayerCount=0;
}
bool configureSurfaces(uint16_t *storage,size_t pixels,size_t compressedPixels) {
  if(processing||refresh->busy()||groupMotion.active||transition.active||backAnimation.active)return false;
  if(!storage&&!pixels&&!compressedPixels){clearSurfaces();return true;}
  if(!project||!storage||!pixels||reinterpret_cast<uintptr_t>(storage)%alignof(uint16_t)||
     pixels>SIZE_MAX/sizeof(uint16_t)||reinterpret_cast<uintptr_t>(storage)>UINTPTR_MAX-pixels*sizeof(uint16_t))return false;
  const size_t perPage=size_t(page().width)*page().height;
  if(!perPage||compressedPixels>pixels||pixels-compressedPixels<perPage||
     (compressedPixels&&((compressedPixels&1)||(pixels-compressedPixels)/perPage<2)))return false;
  for(uint16_t i=0;i<project->count;++i)if(project->pages[i].width!=page().width||project->pages[i].height!=page().height)return false;
  const uintptr_t begin=reinterpret_cast<uintptr_t>(storage),end=begin+pixels*sizeof(uint16_t);
  for(auto *buffer:tileBuffers)if(buffer){const uintptr_t other=reinterpret_cast<uintptr_t>(buffer);
    if(begin<other+capacity*sizeof(uint16_t)&&other<end)return false;
  }
  clearSurfaces();
  const size_t count=std::min<size_t>(project->count,(pixels-compressedPixels)/perPage);
  surfaces.resize(count);surfacePixels=count*perPage+compressedPixels;surfaceStats.capacityPages=count;
  surfaceWanted.reserve(count);surfacePrewarm.reserve(project->count);surfaceWarmPending=true;
  for(size_t i=0;i<count;++i)surfaces[i].pixels=storage+i*perPage;
  if(compressedPixels){compressedStorage=storage+count*perPage;compressedCapacity=compressedPixels;compressedSurfaces.resize(project->count);updateCompressedStats();}
  return true;
}
bool prewarmSurface(const char *path) {
  if(!project||surfaces.empty())return false;
  const int index=routeIndex(path);if(index<0)return false;
  // A completed request is a cache touch, not a permanent reservation. Keep
  // storage available for the current route and for later explicit requests.
  if(surfaceReady(index)){
    for(auto &entry:surfaces)if(entry.page==&project->pages[index])entry.used=++surfaceClock;
    if(auto *entry=compressedSurfaceFor(project->pages[index]))entry->used=++surfaceClock;
    return true;
  }
  if(std::find(surfacePrewarm.begin(),surfacePrewarm.end(),index)!=surfacePrewarm.end())return true;
  if(surfacePrewarm.size()==project->count)return false;
  surfacePrewarm.push_back(index);surfaceWarmPending=true;return true;
}
// A media frame can be built once while playback continues. Every other
// visual sample must match so live transform poses still describe the key;
// the changed-media union must fit the existing bounded repair budget.
bool canCompleteMediaWarm(Surface &slot,const PageDesc &p,const RetainedVector<State> &live){
  if(slot.key.size()!=live.size()||slot.geometry.nodes.size()!=live.size())return false;
  DirtyRegions<16> damage(0);bool changed=false;
  for(uint16_t i=0;i<live.size();++i){
    if(sameSurfaceNode(slot.key[i],live[i]))continue;
    if(p.nodes[i].kind!=Kind::Media||slot.key[i].mediaFrame==live[i].mediaFrame)return false;
    auto &old=slot.key[i];const auto frame=old.mediaFrame;
    old.mediaFrame=live[i].mediaFrame;const bool equal=sameSurfaceNode(old,live[i]);old.mediaFrame=frame;
    if(!equal)return false;changed=true;
    const auto &g=slot.geometry.nodes[i];
    if(g.visible&&g.alpha){const Rect r=intersect(g.rect,g.clip);damage.add(DirtyRect(r.x,r.y,r.width,r.height),p.width,p.height);}
  }
  uint64_t pixels=0;for(size_t i=0;i<damage.size();++i)pixels+=damage[i].pixels();
  return changed&&pixels<=uint64_t(capacity)*4;
}
bool activeWarmProducer(int index,const Surface *slot=nullptr) const {
  if(index!=history.back())return false;
  if(animationsRunning())return true;
  ensureTickNodes();
  for(uint16_t i:mediaNodes){
    if(slot&&slot->key.size()==state().size()&&
       (slot->key[i].image.impl_!=state()[i].image.impl_||slot->key[i].imageOverride!=state()[i].imageOverride)&&
       visible(page(),state(),i))return true;
    const auto *source=media(page().nodes[i],state()[i]);
    if(source&&source->frames>1&&visible(page(),state(),i))return true;
  }
  return false;
}
bool borrowedWarmSurface(const Surface &surface) const {
  return &surface==snapshotSurface||&surface==previewSurface||
    &surface==snapshotAnimationSurface||&surface==coldFadeSurface;
}
void discardWarmSurface(Surface &surface) {
  ++surface.pixelsRevision;surface.presentedAtFrame=UINT64_MAX;
  surface.page=nullptr;surface.key.clear();surface.geometry=FrameCache{};
  surface.ready=surface.paused=surface.compressionAttempted=false;surface.row=surface.column=0;
}
// One page/one immutable key is built to complete coverage. If a setter or an
// animation changes that key between tiles, none of the partial image is used.
void warmSurfaceTile(uint32_t now,bool allowDirty=false) {
  if(!surfaceWarmPending||surfaces.empty()||!project||refresh->busy()||(refresh->dirty()&&!allowDirty)||processing||updateDepth||
     down||groupMotion.active||transition.active||backAnimation.active||animation.active||inertia.active)return;
  // A page fade can finish before its staggered controls. Their bounded
  // Native565 repairs still own a raw slot; warming must not overwrite it
  // with Wire565 tiles between two visible animation frames.
  if(coldFadeSurface&&animationsRunning())return;
  // Counting and encoding share at most 8192 input pixels per tick, reduced
  // further by a configured raster budget.
  // Completed keys are published only after both resumable passes finish.
  if(stepCompressedSurfaces())return;
  // The list is bounded by cache slots and allocated once when configured.
  // A completed, unchanged cache never enters the key-comparison loop.
  auto &wanted=surfaceWanted;wanted.clear();
  const auto add=[&](int index){if(index>=0&&wanted.size()<surfaces.size()&&std::find(wanted.begin(),wanted.end(),index)==wanted.end())wanted.push_back(index);};
  // Requests complete once exact pixels are available. Keeping every old
  // request in wanted would pin all slots forever and exclude the live page
  // as soon as an application requested more routes than fit in its cache.
  surfacePrewarm.erase(std::remove_if(surfacePrewarm.begin(),surfacePrewarm.end(),
    [&](int index){return surfaceReady(index);}),surfacePrewarm.end());
  // A one-slot cache cannot retain the live page and warm another route at
  // once. Let an already-started explicit build finish before restoring the
  // live page, rather than discarding that partial key on every idle tick.
  if(surfaces.size()==1&&!surfacePrewarm.empty()){
    const auto &entry=surfaces.front();
    const int requested=surfacePrewarm.front();
    if(surfaceReady(history.back())||(entry.page==&project->pages[requested]&&!entry.ready))add(requested);
  }
  add(history.back());
  // Explicit callers may warm never-visited routes without navigation/events.
  // The current route always owns the first opportunity; pending requests use
  // remaining slots in FIFO order and rotate fairly with a changing live page.
  for(int index:surfacePrewarm)add(index);
  const auto *preferred=navigationGroup(page().path);
  // Completed explicit requests remain useful residents. Filling a group in
  // descriptor order first would immediately evict its later warmed tabs in
  // favor of never-visited early routes, even with no intervening mutation.
  // Current/pending requests above retain priority; preserve recent group
  // residents next, then use descriptor order only to fill genuinely free
  // opportunities. A new explicit request can still replace the LRU resident.
  const auto retainGroup=[&](const NavigationGroupDesc *group){
    if(!group)return;
    while(wanted.size()<surfaces.size()){
      const Surface *recent=nullptr;int recentIndex=-1;
      for(const auto &entry:surfaces)if(entry.page&&navigationGroup(entry.page->path)==group){
        const int index=int(entry.page-project->pages);
        if(std::find(wanted.begin(),wanted.end(),index)==wanted.end()&&(!recent||entry.used>recent->used)){recent=&entry;recentIndex=index;}
      }
      if(!recent)break;add(recentIndex);
    }
  };
  retainGroup(preferred);
  if(navigation)for(uint16_t g=0;g<navigation->groupCount;++g)if(&navigation->groups[g]!=preferred)retainGroup(&navigation->groups[g]);
  if(preferred)for(uint16_t i=0;i<preferred->count;++i)add(routeIndex(preferred->paths[i]));
  if(navigation)for(uint16_t g=0;g<navigation->groupCount;++g)for(uint16_t i=0;i<navigation->groups[g].count;++i)add(routeIndex(navigation->groups[g].paths[i]));
  // Preserve the most recently used retained pages. Do not automatically add
  // every initialized page: when there are more pages than slots that would
  // repeatedly rebuild and evict the same fixed first routes after navigation.
  while(wanted.size()<surfaces.size()){
    const Surface *recent=nullptr;int recentIndex=-1;
    for(const auto &entry:surfaces)if(entry.page){const int index=int(entry.page-project->pages);
      if(std::find(wanted.begin(),wanted.end(),index)==wanted.end()&&(!recent||entry.used>recent->used)){recent=&entry;recentIndex=index;}}
    if(!recent)break;add(recentIndex);
  }
  // A continuously updated first page must not monopolize the idle budget.
  // Rotate after each successful tile; exact immutable keys still decide hits.
  const size_t count=wanted.size(),first=count?surfaceWarmNext%count:0;
  for(size_t offset=0;offset<count;++offset){
    const size_t wantedAt=(first+offset)%count;const int index=wanted[wantedAt];
    // Explicit idle maintenance may prewarm inactive pages while the live page
    // already has next-sample damage. Never replace that page's last presented
    // cache underneath its pending ordinary refresh.
    if(allowDirty&&refresh->dirty()&&index==history.back())continue;
    initializePage(uint16_t(index));const PageDesc &p=project->pages[index];auto &state=pages[index];
    if(surfaceReady(index)){
      // A normal frame may repair the retained key while its replacement is
      // still being built. The incomplete duplicate no longer has any work.
      for(auto &entry:surfaces)if(entry.page==&p&&!entry.ready&&!borrowedWarmSurface(entry))discardWarmSurface(entry);
      continue;
    }
    Surface *slot=nullptr;
    Surface *retained=nullptr;
    for(auto &entry:surfaces)if(entry.page==&p){
      if(entry.ready){if(!retained||entry.used>retained->used)retained=&entry;}
      else if(!slot||entry.used>slot->used)slot=&entry;
    }
    if(!slot)slot=retained;
    if(!slot)for(auto &entry:surfaces)if(!entry.page){slot=&entry;break;}
    if(!slot)for(auto &entry:surfaces){const int old=entry.page?int(entry.page-project->pages):-1;
      if(std::find(wanted.begin(),wanted.end(),old)==wanted.end()&&(!slot||entry.used<slot->used))slot=&entry;
    }
    // With two slots the live page may temporarily own a complete source and
    // its builder. An unserviceable prewarm request must not block that builder.
    if(!slot)continue;
    bool resumed=false;
    if(slot->paused){
      if(!activeWarmProducer(index,slot)){slot->paused=false;resumed=true;}
      else {
        if(slot->pauseEpoch!=surfaceMutationEpoch){slot->pauseEpoch=surfaceMutationEpoch;slot->quietAt=now;}
        if(uint32_t(now-slot->quietAt)<100)continue;
        slot->paused=false;resumed=true;
      }
    }
    const bool same=slot->page==&p&&sameSurfaceState(slot->key,state);
    const bool carry=!same&&slot->page==&p&&!slot->ready&&canCompleteMediaWarm(*slot,p,state);
    if(!same&&!carry){
      // Continuous changes cannot finish a page-sized warm. Keep a completed
      // old key available for exact hits/repairs; retry once state is quiet.
      if(!resumed&&slot->page==&p&&(slot->ready||slot->row||slot->column)&&activeWarmProducer(index,slot)){
        slot->paused=true;slot->pauseEpoch=surfaceMutationEpoch;slot->quietAt=now;continue;
      }
      if(index==history.back()&&slot->ready&&surfaces.size()>1){
        // Keep the last complete live-page key available for a departure or
        // bounded repair. Build its replacement in a different existing slot;
        // never expand the caller's pixel budget or overwrite a borrowed source.
        Surface *replacement=nullptr;
        for(auto &entry:surfaces)if(&entry!=slot&&!borrowedWarmSurface(entry)){
          if(!entry.page){replacement=&entry;break;}
          if(!replacement||entry.used<replacement->used)replacement=&entry;
        }
        if(!replacement)continue;
        slot=replacement;
      }
      ++slot->pixelsRevision;slot->presentedAtFrame=UINT64_MAX;slot->page=&p;slot->key=state;slot->row=slot->column=0;slot->ready=slot->paused=slot->compressionAttempted=false;
      const RuntimeStats saved=stats;prepareCache(p,slot->key,slot->geometry);stats=saved;
    }
    const size_t workCapacity=rasterCapacity();
    const int32_t width=int32_t(std::min<size_t>(size_t(p.width-slot->column),workCapacity));
    const int32_t height=int32_t(std::min<size_t>(p.height-slot->row,size_t(p.width)<=workCapacity?workCapacity/width:1));
    const DirtyRect area(slot->column,slot->row,width,height);
    const PageDesc *savedPage=rasterFrame.current.page,*savedPreview=rasterFrame.preview.page;
    const bool savedTransition=snapshotTransition,savedInput=snapshotInput;
    const int savedPressed=snapshotPressed,savedFocus=snapshotFocus,savedGroup=snapshotGroupIndex;
    const int32_t savedBack=snapshotBackOffset;
    FrameCache *savedPaint=paintCache;
    const RuntimeStats savedStats=stats;
    rasterFrame.current.values.swap(slot->key);rasterFrame.current.motions.swap(pageMotions[size_t(index)]);std::swap(rasterFrame.current.geometry,slot->geometry);
    rasterFrame.current.page=&p;rasterFrame.preview.page=nullptr;snapshotTransition=false;snapshotInput=false;
    snapshotPressed=snapshotFocus=snapshotGroupIndex=-1;snapshotBackOffset=0;buildingSurface=true;
    ++slot->pixelsRevision;
    const Status result=render(this,area,slot->pixels+size_t(area.y)*p.width+area.x,p.width);
    buildingSurface=false;
    rasterFrame.current.values.swap(slot->key);rasterFrame.current.motions.swap(pageMotions[size_t(index)]);std::swap(rasterFrame.current.geometry,slot->geometry);
    rasterFrame.current.page=savedPage;rasterFrame.preview.page=savedPreview;snapshotTransition=savedTransition;snapshotInput=savedInput;
    snapshotPressed=savedPressed;snapshotFocus=savedFocus;snapshotGroupIndex=savedGroup;snapshotBackOffset=savedBack;paintCache=savedPaint;stats=savedStats;
    if(result!=Status::Ok){slot->page=nullptr;slot->key.clear();slot->ready=false;return;}
    ++surfaceStats.warmedTiles;slot->used=++surfaceClock;
    slot->column+=width;if(slot->column==p.width){slot->column=0;slot->row+=height;}
    if(slot->row==p.height){
      slot->ready=true;releaseSurfaceText(*slot);
      // Publish complete coverage before releasing the previous source. No
      // refresh can borrow either slot while this bounded idle work runs.
      for(auto &entry:surfaces)if(&entry!=slot&&entry.page==&p&&!borrowedWarmSurface(entry))discardWarmSurface(entry);
    }
    surfaceWarmNext=(wantedAt+1)%count;
    return;
  }
  surfaceWarmPending=false;
  for(const auto &entry:surfaces)if(entry.paused)surfaceWarmPending=true;
}
