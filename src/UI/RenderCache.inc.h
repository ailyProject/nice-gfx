// Included inside View::Impl. Surfaces contain only page content in wire order;
// fixed navigation chrome and transient focus/press feedback stay separate.
struct Surface {
  uint16_t *pixels=nullptr;
  const PageDesc *page=nullptr;
  std::vector<State> key;
  FrameCache geometry;
  int32_t row=0,column=0;
  uint64_t used=0;
  bool ready=false,paused=false;
  uint32_t pauseEpoch=0,quietAt=0;
};
std::vector<Surface> surfaces;
std::vector<int> surfaceWanted;
std::vector<int> surfacePrewarm; // Explicit route indices, bounded by cache slots.
bool surfaceWarmPending=false;
uint32_t surfaceMutationEpoch=0;
uint16_t *tileBuffers[2]={nullptr,nullptr};
size_t surfacePixels=0;
uint64_t surfaceClock=0;
size_t surfaceWarmNext=0;
RenderCacheStats surfaceStats;
const Surface *snapshotSurface=nullptr,*previewSurface=nullptr;
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
static bool sameSurfaceState(const std::vector<State> &a,const std::vector<State> &b) {
  if(a.size()!=b.size())return false;
  for(size_t i=0;i<a.size();++i)if(!sameSurfaceNode(a[i],b[i]))return false;
  return true;
}
static bool surfaceImageOnlyChange(State &old,const State &live) {
  if(old.image.impl_==live.image.impl_&&old.imageOverride==live.imageOverride)return false;
  Image retained=old.image;const bool overrideSource=old.imageOverride;
  old.image=live.image;old.imageOverride=live.imageOverride;
  const bool equal=sameSurfaceNode(old,live);
  old.image=std::move(retained);old.imageOverride=overrideSource;
  return equal;
}
bool fastSurfaceImageRepair(uint16_t index,const Geometry &geometry) const {
  const auto &node=snapshotPage->nodes[index];const auto &value=snapshot[index];
  if(node.kind!=Kind::Media||node.motion||geometry.alpha!=255||!value.imageOverride||!value.image.impl_)return false;
  const auto &image=*value.image.impl_;const auto &style=styleFor(*snapshotPage,snapshot,index);
  return !image.flash&&image.source.format==ImageFormat::RGB565&&!(image.source.strideBytes&1)&&
    image.media.width==geometry.rect.width&&image.media.height==geometry.rect.height&&
    !style.borderWidth&&!style.radius&&!style.paddingTop&&!style.paddingRight&&!style.paddingBottom&&!style.paddingLeft;
}

// Repair visual changes without discarding a retained page's static content.
// Ordinary changes have a four-tile budget; full-size opaque RAM image updates
// may repair at most one page through the same renderer, preserving overlays.
Surface *patchSurface(bool &geometryPrepared) {
  if(down||snapshotInput||snapshotKeyboard.visible)return nullptr;
  Surface *slot=nullptr;
  for(auto &entry:surfaces)if(entry.page==snapshotPage&&entry.ready&&entry.key.size()==snapshot.size()){slot=&entry;break;}
  if(!slot||slot==previewSurface||slot->geometry.nodes.size()!=snapshot.size())return nullptr;
  bool visualOnly=true;
  for(size_t i=0;i<snapshot.size();++i)if(!sameSurfaceGeometry(slot->key[i],snapshot[i])){visualOnly=false;break;}
  if(!visualOnly){prepareCache(*snapshotPage,snapshot,snapshotCache);geometryPrepared=true;}
  DirtyRegions<16> damage(0),ordinaryDamage(0);
  bool fastImage=false;
  for(uint16_t i=0;i<snapshot.size();++i)if(!sameSurfaceNode(slot->key[i],snapshot[i])) {
    if(visualOnly){const auto &g=slot->geometry.nodes[i];
      if(g.visible&&g.alpha){const Rect r=intersect(g.rect,g.clip);const DirtyRect changed(r.x,r.y,r.width,r.height);
        damage.add(changed,snapshotPage->width,snapshotPage->height);
        if(fastSurfaceImageRepair(i,g)&&surfaceImageOnlyChange(slot->key[i],snapshot[i]))fastImage=true;
        else ordinaryDamage.add(changed,snapshotPage->width,snapshotPage->height);
      }continue;
    }
    for(uint16_t k=i;k<snapshot.size();++k){
      int a=k;while(a>=0&&a!=i)a=snapshotPage->nodes[a].parent==NoParent?-1:snapshotPage->nodes[a].parent;
      if(a!=i)continue;
      for(const auto *geometry:{&slot->geometry,&snapshotCache}){
        const auto &g=geometry->nodes[k];const Rect r=intersect(g.rect,g.clip);
        damage.add(DirtyRect(r.x,r.y,r.width,r.height),snapshotPage->width,snapshotPage->height);
      }
    }
  }
  // Selection and disclosure arrows borrow state from a component root,
  // pager or panel. Their buttons can lie outside that owner's rectangle.
  // Compare those visual dependencies before repairing the retained pixels.
  if(componentCount)for(uint16_t i=0;i<snapshot.size();++i)if(snapshotPage->nodes[i].kind==Kind::Button){
    const Id id=snapshotPage->nodes[i].id;
    const auto previous=componentPaint(*snapshotPage,slot->key,id),current=componentPaint(*snapshotPage,snapshot,id);
    if(previous.selected==current.selected&&previous.expanded==current.expanded)continue;
    for(const auto *geometry:{&slot->geometry,visualOnly?&slot->geometry:&snapshotCache}){
      const auto &g=geometry->nodes[i];if(!g.visible||!g.alpha)continue;
      const Rect r=intersect(g.rect,g.clip);damage.add(DirtyRect(r.x,r.y,r.width,r.height),snapshotPage->width,snapshotPage->height);
      ordinaryDamage.add(DirtyRect(r.x,r.y,r.width,r.height),snapshotPage->width,snapshotPage->height);
    }
  }
  uint64_t pixels=0;for(size_t i=0;i<damage.size();++i)pixels+=damage[i].pixels();
  surfaceStats.lastRepairPixels=pixels;
  const uint64_t smallBudget=uint64_t(capacity)*4;
  if(pixels>smallBudget){
    uint64_t ordinaryPixels=0;for(size_t i=0;i<ordinaryDamage.size();++i)ordinaryPixels+=ordinaryDamage[i].pixels();
    // Full-size opaque RAM images use clipped row copies. Repair their changed
    // rectangle once in the retained page, including unchanged overlay chrome,
    // instead of discarding a complete surface for every incoming map frame.
    // Other visual changes retain the old four-tile limit. Work never exceeds
    // one page, and geometry/alpha/scale/flash-image changes use normal fallback.
    if(!visualOnly||!fastImage||ordinaryPixels>smallBudget||pixels>uint64_t(snapshotPage->width)*snapshotPage->height)return nullptr;
  }
  if(visualOnly){prepareVisualCache(*snapshotPage,snapshot,slot->key,slot->geometry,snapshotCache);geometryPrepared=true;}
  const uint64_t savedTiles=stats.renderedTiles,savedPixels=stats.renderedPixels;
  FrameCache *savedPaint=paintCache;const bool savedBuilding=buildingSurface;
  const bool savedTransition=snapshotTransition;const PageDesc *savedPreview=previewPage;
  snapshotTransition=false;previewPage=nullptr;
  buildingSurface=true;
  for(size_t i=0;i<damage.size();++i){const auto &area=damage[i];
    if(render(this,area,slot->pixels+size_t(area.y)*snapshotPage->width+area.x,snapshotPage->width)!=Status::Ok){slot->ready=false;slot=nullptr;break;}
  }
  buildingSurface=savedBuilding;paintCache=savedPaint;snapshotTransition=savedTransition;previewPage=savedPreview;
  stats.renderedTiles=savedTiles;stats.renderedPixels=savedPixels;
  if(slot){slot->key=snapshot;releaseSurfaceText(*slot);slot->geometry=snapshotCache;slot->used=++surfaceClock;++surfaceStats.repairedPages;surfaceStats.repairPixels+=pixels;}
  return slot;
}
Surface *findSurface(const PageDesc &p,const std::vector<State> &s) {
  for(auto &entry:surfaces)if(entry.page==&p&&entry.ready&&sameSurfaceState(entry.key,s)){
    entry.used=++surfaceClock;return &entry;
  }
  return nullptr;
}
bool surfaceReady(int index) const {
  if(index<0||pages[size_t(index)].empty())return false;
  const auto &p=project->pages[index];
  for(const auto &entry:surfaces)if(entry.page==&p&&entry.ready&&sameSurfaceState(entry.key,pages[index]))return true;
  return false;
}
void clearSurfacePage(const PageDesc *p) {
  surfaceWarmPending=!surfaces.empty();
  for(auto &entry:surfaces)if(entry.page==p){entry.page=nullptr;entry.key.clear();entry.geometry=FrameCache{};entry.ready=false;entry.row=entry.column=0;}
}
void clearSurfaces() {
  surfaces.clear();surfacePixels=0;surfaceClock=0;surfaceWarmNext=0;surfaceStats=RenderCacheStats{};
  std::vector<int>().swap(surfaceWanted);std::vector<int>().swap(surfacePrewarm);surfaceWarmPending=false;
  snapshotSurface=previewSurface=nullptr;
}
bool configureSurfaces(uint16_t *storage,size_t pixels) {
  if(processing||refresh->busy()||groupMotion.active||transition.active||backAnimation.active)return false;
  if(!storage&&!pixels){clearSurfaces();return true;}
  if(!project||!storage||!pixels||reinterpret_cast<uintptr_t>(storage)%alignof(uint16_t)||
     pixels>SIZE_MAX/sizeof(uint16_t)||reinterpret_cast<uintptr_t>(storage)>UINTPTR_MAX-pixels*sizeof(uint16_t))return false;
  const size_t perPage=size_t(page().width)*page().height;
  if(!perPage||pixels<perPage)return false;
  for(uint16_t i=0;i<project->count;++i)if(project->pages[i].width!=page().width||project->pages[i].height!=page().height)return false;
  const uintptr_t begin=reinterpret_cast<uintptr_t>(storage),end=begin+pixels*sizeof(uint16_t);
  for(auto *buffer:tileBuffers)if(buffer){const uintptr_t other=reinterpret_cast<uintptr_t>(buffer);
    if(begin<other+capacity*sizeof(uint16_t)&&other<end)return false;
  }
  clearSurfaces();
  const size_t count=std::min<size_t>(project->count,pixels/perPage);
  surfaces.resize(count);surfacePixels=count*perPage;surfaceStats.capacityPages=count;
  surfaceWanted.reserve(count);surfacePrewarm.reserve(count);surfaceWarmPending=true;
  for(size_t i=0;i<count;++i)surfaces[i].pixels=storage+i*perPage;
  return true;
}
bool prewarmSurface(const char *path) {
  if(!project||surfaces.empty())return false;
  const int index=routeIndex(path);if(index<0)return false;
  if(std::find(surfacePrewarm.begin(),surfacePrewarm.end(),index)!=surfacePrewarm.end())return true;
  if(surfacePrewarm.size()==surfaces.size())return false;
  surfacePrewarm.push_back(index);surfaceWarmPending=true;return true;
}
// A media frame can be built once while playback continues. Every other
// visual sample must match so live transform poses still describe the key;
// the changed-media union must fit the existing bounded repair budget.
bool canCompleteMediaWarm(Surface &slot,const PageDesc &p,const std::vector<State> &live){
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
// One page/one immutable key is built to complete coverage. If a setter or an
// animation changes that key between tiles, none of the partial image is used.
void warmSurfaceTile(uint32_t now) {
  if(!surfaceWarmPending||surfaces.empty()||!project||refresh->busy()||refresh->dirty()||processing||updateDepth||
     down||groupMotion.active||transition.active||backAnimation.active||animation.active||inertia.active)return;
  // The list is bounded by cache slots and allocated once when configured.
  // A completed, unchanged cache never enters the key-comparison loop.
  auto &wanted=surfaceWanted;wanted.clear();
  const auto add=[&](int index){if(index>=0&&wanted.size()<surfaces.size()&&std::find(wanted.begin(),wanted.end(),index)==wanted.end())wanted.push_back(index);};
  // Explicit callers can warm never-visited routes without changing history or
  // synthesizing entry/route events. With no requests, preserve lazy warming.
  for(int index:surfacePrewarm)add(index);
  add(history.back());
  const auto *preferred=navigationGroup(page().path);
  if(preferred)for(uint16_t i=0;i<preferred->count;++i)add(routeIndex(preferred->paths[i]));
  if(navigation)for(uint16_t g=0;g<navigation->groupCount;++g)for(uint16_t i=0;i<navigation->groups[g].count;++i)add(routeIndex(navigation->groups[g].paths[i]));
  // Without route groups, retain recently visited pages rather than eagerly
  // initializing unrelated routes. Group warming is explicitly opted into.
  for(uint16_t i=0;i<project->count;++i)if(!pages[i].empty())add(i);
  // A continuously updated first page must not monopolize the idle budget.
  // Rotate after each successful tile; exact immutable keys still decide hits.
  const size_t count=wanted.size(),first=count?surfaceWarmNext%count:0;
  for(size_t offset=0;offset<count;++offset){
    const size_t wantedAt=(first+offset)%count;const int index=wanted[wantedAt];
    initializePage(uint16_t(index));const PageDesc &p=project->pages[index];auto &state=pages[index];
    if(surfaceReady(index))continue;
    Surface *slot=nullptr;
    for(auto &entry:surfaces)if(entry.page==&p){slot=&entry;break;}
    if(!slot)for(auto &entry:surfaces)if(!entry.page){slot=&entry;break;}
    if(!slot)for(auto &entry:surfaces){const int old=entry.page?int(entry.page-project->pages):-1;
      if(std::find(wanted.begin(),wanted.end(),old)==wanted.end()&&(!slot||entry.used<slot->used))slot=&entry;
    }
    if(!slot)return;
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
      slot->page=&p;slot->key=state;slot->row=slot->column=0;slot->ready=slot->paused=false;
      const RuntimeStats saved=stats;prepareCache(p,slot->key,slot->geometry);stats=saved;
    }
    const int32_t width=int32_t(std::min<size_t>(size_t(p.width-slot->column),capacity));
    const int32_t height=int32_t(std::min<size_t>(p.height-slot->row,size_t(p.width)<=capacity?capacity/width:1));
    const DirtyRect area(slot->column,slot->row,width,height);
    const PageDesc *savedPage=snapshotPage,*savedPreview=previewPage;
    const bool savedTransition=snapshotTransition,savedInput=snapshotInput;
    const int savedPressed=snapshotPressed,savedFocus=snapshotFocus,savedGroup=snapshotGroupIndex;
    const int32_t savedBack=snapshotBackOffset;
    FrameCache *savedPaint=paintCache;
    const RuntimeStats savedStats=stats;
    snapshot.swap(slot->key);snapshotMotions.swap(pageMotions[size_t(index)]);std::swap(snapshotCache,slot->geometry);
    snapshotPage=&p;previewPage=nullptr;snapshotTransition=false;snapshotInput=false;
    snapshotPressed=snapshotFocus=snapshotGroupIndex=-1;snapshotBackOffset=0;buildingSurface=true;
    const Status result=render(this,area,slot->pixels+size_t(area.y)*p.width+area.x,p.width);
    buildingSurface=false;
    snapshot.swap(slot->key);snapshotMotions.swap(pageMotions[size_t(index)]);std::swap(snapshotCache,slot->geometry);
    snapshotPage=savedPage;previewPage=savedPreview;snapshotTransition=savedTransition;snapshotInput=savedInput;
    snapshotPressed=savedPressed;snapshotFocus=savedFocus;snapshotGroupIndex=savedGroup;snapshotBackOffset=savedBack;paintCache=savedPaint;stats=savedStats;
    if(result!=Status::Ok){slot->page=nullptr;slot->key.clear();slot->ready=false;return;}
    ++surfaceStats.warmedTiles;slot->used=++surfaceClock;
    slot->column+=width;if(slot->column==p.width){slot->column=0;slot->row+=height;}
    if(slot->row==p.height){slot->ready=true;releaseSurfaceText(*slot);}
    surfaceWarmNext=(wantedAt+1)%count;
    return;
  }
  surfaceWarmPending=false;
  for(const auto &entry:surfaces)if(entry.paused)surfaceWarmPending=true;
}
