// Included inside View::Impl. Repair Wire cache pixels through independent
// native scratch, enabling the same overlays as a normal mapped frame.
// It never publishes, ticks or retimes the UI.
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR
struct NativeCacheScratch{
  uint16_t *pixels=nullptr;size_t bytes=0;
  ~NativeCacheScratch(){release();}
  void release(){if(pixels){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    heap_caps_free(pixels);
#else
    detail::freeAlignedMemory<64>(pixels);
#endif
  }pixels=nullptr;bytes=0;}
  bool reserve(size_t required){if(bytes>=required)return true;release();
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    if(heap_caps_get_free_size(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT)<required+2u*1024u*1024u)return false;
    pixels=static_cast<uint16_t*>(heap_caps_aligned_calloc(64,1,required,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
#else
    pixels=static_cast<uint16_t*>(detail::allocateAlignedMemory<64>(required));if(pixels)memset(pixels,0,required);
#endif
    if(!pixels)return false;bytes=required;return true;
  }
} nativeCacheScratch;
#endif
#ifndef NICE_GFX_UI_ROI_PREFIX_FRAME_MEMO
#define NICE_GFX_UI_ROI_PREFIX_FRAME_MEMO 1
#endif
#ifndef NICE_GFX_UI_ROI_PREFIX_DIRTY_REGION
#define NICE_GFX_UI_ROI_PREFIX_DIRTY_REGION 1
#endif
#ifndef NICE_GFX_UI_ROI_PREFIX_MEDIA_WORLD
#define NICE_GFX_UI_ROI_PREFIX_MEDIA_WORLD 1
#endif
#ifndef NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP
#define NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP 1
#endif
#if NICE_GFX_UI_ROI_PREFIX_DIRTY_REGION && !NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP
#error Dirty-region prefix requires the base ROI-prefix feature
#endif
#ifndef NICE_GFX_UI_NATIVE_PREFIX_CACHE
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_UI_NATIVE_PREFIX_CACHE 1
#else
#define NICE_GFX_UI_NATIVE_PREFIX_CACHE 0
#endif
#endif
// A repair may redraw hundreds of unchanged leaves below its first changed
// painter. Retain that exact ordered prefix, independently of the live page.
// A changed key, geometry or component selection always rebuilds the prefix.
int nativePrefixLimit=-1;
bool nativeCacheRepairing=false;
bool nativeCacheCapturing=false;
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR && NICE_GFX_UI_NATIVE_PREFIX_CACHE
#if NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP
struct RoiPrefixMetadata {State key;Geometry geometry;Style style;uint8_t component;bool context=false;};
#endif
struct NativePrefixEntry {
  NativeCacheScratch storage;
  const PageDesc *page=nullptr;uint64_t owner=0,used=0;size_t limit=0,pitch=0;
  RetainedVector<State> key;
  RetainedVector<Geometry> geometry;
  RetainedVector<uint8_t> components;
#if NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP
  Rect roi{};size_t roiGroup=0,roiMetaCount=0;RoiPrefixMetadata *roiMeta=nullptr;uint64_t roiValidatedSequence=0;uint8_t roiScopeKind=0;
  void releaseRoi(){if(roiMeta){for(size_t i=0;i<roiMetaCount;++i)roiMeta[i].~RoiPrefixMetadata();platform::freeUiMemory(roiMeta);}roiMeta=nullptr;roiMetaCount=0;roi={0,0,0,0};roiGroup=0;roiValidatedSequence=0;roiScopeKind=0;}
  ~NativePrefixEntry(){releaseRoi();}
#endif
};
NativePrefixEntry nativePrefixes[4];uint64_t nativePrefixClock=0;
const NativePrefixEntry *nativePrefixSource=nullptr;
#if NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP
uint64_t roiPrefixSnapshotSequence=0;
#endif
static bool prefixGeometryEqual(const Geometry &a,const Geometry &b){
  return a.visible==b.visible&&a.alpha==b.alpha&&a.rect.x==b.rect.x&&a.rect.y==b.rect.y&&a.rect.width==b.rect.width&&a.rect.height==b.rect.height&&
    a.clip.x==b.clip.x&&a.clip.y==b.clip.y&&a.clip.width==b.clip.width&&a.clip.height==b.clip.height;
}
uint8_t prefixComponent(const RetainedVector<State> &values,size_t i)const{
  if(!componentCount||rasterFrame.current.page->nodes[i].kind!=Kind::Button)return 0;
  const auto paint=componentPaint(*rasterFrame.current.page,values,rasterFrame.current.page->nodes[i].id);
  return uint8_t((paint.selected?1:0)|(paint.expanded?2:0));
}
size_t nativePrefixBytes()const{size_t bytes=0;for(const auto &e:nativePrefixes)bytes+=e.storage.bytes+e.key.capacity()*sizeof(State)+e.geometry.capacity()*sizeof(Geometry)+e.components.capacity();
#if NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP
  for(const auto &e:nativePrefixes)bytes+=e.roiMetaCount*sizeof(RoiPrefixMetadata);
#endif
  return bytes;}
static bool prefixTouches(const Geometry &g,const DirtyRect &area){
  if(!g.visible||!g.alpha)return false;const auto r=intersect(g.rect,g.clip);
  return r.width>0&&r.height>0&&int64_t(r.x)<int64_t(area.x)+area.width&&int64_t(area.x)<int64_t(r.x)+r.width&&
    int64_t(r.y)<int64_t(area.y)+area.height&&int64_t(area.y)<int64_t(r.y)+r.height;
}
const NativePrefixEntry *resolveNativePrefix(const Surface &previous,const DirtyRect &area){
// BEGIN ROI_PREFIX_TIMING scope-LegacyResolve
#if NICE_GFX_UI_ROI_PREFIX_TIMING
  RoiPrefixTimer roiTiming(*this,RoiPrefixTimingPhase::LegacyResolve);
#endif
// END ROI_PREFIX_TIMING scope-LegacyResolve

  if(nativePrefixLimit>=0||foregroundCapture||behaviorVisualMoving()||viewportOverlayLevels||displayPolicy.monochrome||
     snapshotPressed>=0||styledFocus()||snapshotInput||snapshotKeyboard.visible)return nullptr;
  const auto &page=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;const auto &geometry=rasterFrame.current.geometry.nodes;
  if(previous.key.size()!=values.size()||previous.geometry.nodes.size()!=geometry.size())return nullptr;
  size_t limit=0;
  for(;limit<values.size();++limit){
    // Mutable caller images have no content revision. Never retain their data.
    if(values[limit].imageOverride)return nullptr;
    // A blinking status light in another region cannot invalidate the static
    // underlay beneath a rotor. Validate both old and new bounds so movement
    // into/out of this region still invalidates every affected cached pixel.
    if((prefixTouches(previous.geometry.nodes[limit],area)||prefixTouches(geometry[limit],area))&&
       (!sameSurfaceNode(previous.key[limit],values[limit])||!prefixGeometryEqual(previous.geometry.nodes[limit],geometry[limit])||prefixComponent(previous.key,limit)!=prefixComponent(values,limit)))break;
  }
  if(limit<8||limit==values.size())return nullptr;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  // Native spans already make sparse chrome cheap. A large PPA background
  // copy only pays off when it replaces substantial painting in THIS region.
  // Keep cheap control/rotor underlays on the ordinary painter instead.
  size_t shapes=0;bool largeImage=false;
  for(size_t i=0;i<limit;++i)if(prefixTouches(geometry[i],area)){
    shapes+=page.nodes[i].kind==Kind::Shape;
    if(page.nodes[i].kind==Kind::Media){const auto r=intersect(geometry[i].rect,geometry[i].clip);const auto visible=intersect(r,{area.x,area.y,area.width,area.height});largeImage|=uint64_t(visible.width)*visible.height>=16384;}
  }
  if(shapes<40&&!largeImage)return nullptr;
#endif
  for(auto &e:nativePrefixes)if(e.page==&page&&e.owner==frameSourceOwner&&e.limit==limit){
#if NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP
    if(e.roi.width)continue;
#endif
    bool same=e.key.size()==limit&&e.geometry.size()==limit&&e.components.size()==limit;
    for(size_t i=0;same&&i<limit;++i)if(prefixTouches(e.geometry[i],area)||prefixTouches(geometry[i],area))
      same=sameSurfaceNode(e.key[i],values[i])&&prefixGeometryEqual(e.geometry[i],geometry[i])&&e.components[i]==prefixComponent(values,i);
    if(same){e.used=++nativePrefixClock;++stats.nativePrefixHits;return &e;}
  }
  const size_t pitch=(size_t(page.width)+31)&~size_t(31);
  if(!pitch||!page.height||size_t(page.height)>SIZE_MAX/pitch/2)return nullptr;
  const size_t required=pitch*page.height*2;if(required>3u*1024u*1024u)return nullptr;
  NativePrefixEntry *entry=nullptr;
  for(auto &e:nativePrefixes)if(!e.page){entry=&e;break;}
  if(!entry)for(auto &e:nativePrefixes)if(!entry||e.used<entry->used)entry=&e;
  entry->page=nullptr;entry->key.clear();entry->geometry.clear();entry->components.clear();
#if NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP
  entry->releaseRoi();
#endif
  size_t other=0;for(auto &e:nativePrefixes)if(&e!=entry)other+=e.storage.bytes;
  while(other>3u*1024u*1024u-required){NativePrefixEntry *oldest=nullptr;for(auto &e:nativePrefixes)if(&e!=entry&&e.storage.bytes&&(!oldest||e.used<oldest->used))oldest=&e;
    if(!oldest)return nullptr;other-=oldest->storage.bytes;oldest->storage.release();oldest->page=nullptr;oldest->key.clear();oldest->geometry.clear();oldest->components.clear();}
  if(!entry->storage.reserve(required))return nullptr;
  NativeFramebuffer target;target.pixels=entry->storage.pixels;target.width=page.width;target.height=page.height;target.stride=pitch;target.bytes=entry->storage.bytes;
  const uint32_t started=profilingClock?profilingClock():0;
  const auto *saved=nativeOverlayTarget;const bool repairing=nativeCacheRepairing;
  nativeOverlayTarget=&target;nativePrefixLimit=int(limit);nativeCacheRepairing=true;
  const Status result=render(this,DirtyRect(0,0,page.width,page.height),target.pixels,pitch);
  nativePrefixLimit=-1;nativeOverlayTarget=saved;nativeCacheRepairing=repairing;if(result!=Status::Ok)return nullptr;
  ++stats.nativePrefixBuilds;if(profilingClock)stats.nativePrefixBuildMicros+=profilingClock()-started;
  entry->key.assign(values.begin(),values.begin()+limit);entry->geometry.assign(geometry.begin(),geometry.begin()+limit);entry->components.resize(limit);
  for(size_t i=0;i<limit;++i){entry->components[i]=prefixComponent(values,i);entry->key[i].text.owned.reset();}
  entry->page=&page;entry->owner=frameSourceOwner;entry->limit=limit;entry->pitch=pitch;entry->used=++nativePrefixClock;return entry;
}
#if NICE_GFX_UI_ROI_PREFIX_BOOTSTRAP
// Experimental/default-OFF static subtree underlay. This never captures an LCD
// framebuffer: the bounded ROI is rasterized once into independent Native565
// RAM from the current immutable snapshot, with the exact ordered prefix only.
static bool roiPrefixContains(Rect outer,const DirtyRect &inner){
  return outer.width>0&&outer.height>0&&inner.width>0&&inner.height>0&&
    inner.x>=outer.x&&inner.y>=outer.y&&int64_t(inner.x)+inner.width<=int64_t(outer.x)+outer.width&&
    int64_t(inner.y)+inner.height<=int64_t(outer.y)+outer.height;
}
static bool roiPrefixRectEqual(Rect a,Rect b){return a.x==b.x&&a.y==b.y&&a.width==b.width&&a.height==b.height;}
static bool roiPrefixStyleEqual(const Style &a,const Style &b){
  return a.background==b.background&&a.color==b.color&&a.borderColor==b.borderColor&&a.trackColor==b.trackColor&&
    a.fillColor==b.fillColor&&a.thumbColor==b.thumbColor&&a.pressedColor==b.pressedColor&&a.disabledColor==b.disabledColor&&
    a.borderWidth==b.borderWidth&&a.radius==b.radius&&a.fontSize==b.fontSize&&a.textAlign==b.textAlign&&
    a.paddingTop==b.paddingTop&&a.paddingRight==b.paddingRight&&a.paddingBottom==b.paddingBottom&&a.paddingLeft==b.paddingLeft&&
    a.trackSize==b.trackSize&&a.thumbWidth==b.thumbWidth&&a.thumbHeight==b.thumbHeight&&a.thumbRadius==b.thumbRadius&&
    a.font==b.font&&a.gradient==b.gradient&&a.trackRadius==b.trackRadius;
}
static bool roiPrefixContinuous(const NodeDesc &node){
  return node.animation.duration&&node.animation.repeat==0&&
    (node.animation.effect!=AnimationEffect::None||(node.motion&&node.motion->count));
}
bool roiPrefixDescendant(size_t child,size_t ancestor)const{
  const auto &values=rasterFrame.current.values;
  for(size_t count=0;child<values.size()&&count<values.size();++count){
    if(child==ancestor)return true;const auto parent=values[child].parent;if(parent==NoParent)return false;child=parent;
  }return false;
}
bool roiPrefixOrdinary()const{
  return !nativeCacheRepairing&&nativePrefixLimit<0&&renderOrder==PixelOrder::Native&&nativeOverlayTarget&&!stagingNativeTarget&&
    !displayPolicy.monochrome&&!buildingSurface&&!foregroundCapture&&!renderingViewport&&!snapshotTransition&&
    !rasterFrame.preview.page&&!snapshotSurface&&!snapshotAnimationSurface&&!snapshotKeyboard.visible&&!snapshotInput&&
    !snapshotMenuEnabled&&!snapshotMenuVisible&&!snapshotMarqueeId&&!styledFocus()&&snapshotPressed<0&&!viewportOverlayLevels&&
    !behaviorVisualMoving()&&!groupMotion.active&&!groupMotion.prepared&&!coldFadeSurface&&!renderBudget.maxPixelsPerTick&&
    !canvas.attached()&&!paintOffsetX&&!paintOffsetY;
}
struct RoiPrefixPlan {Rect roi{};size_t group=0,limit=0,containers=0,shapes=0;};
// Pure planning only: one immutable paintNativeRender snapshot. No pixels,
// SDK operation, entry pointer, shared owner or payload is retained here.
#if NICE_GFX_UI_ROI_PREFIX_FRAME_MEMO
struct RoiFramePlanMemo {
  RoiPrefixPlan plan{};Rect roi{};const PageDesc *page=nullptr;
  uint64_t sequence=0,owner=0;size_t group=0;uint8_t kind=0;bool accepted=false;
};
mutable RoiFramePlanMemo roiFramePlanMemo[64];
struct RoiFrameWorld {Rect world{};RoiPrefixPlan plan{};};
mutable RoiFrameWorld roiMemoWorld[16];
mutable size_t roiMemoWorldCount=0;
mutable uint64_t roiMemoWorldSequence=0,roiMemoWorldOwner=0;
mutable const PageDesc *roiMemoWorldPage=nullptr;
void clearRoiPrefixFrameMemo()const{
  for(auto &slot:roiFramePlanMemo)slot.sequence=0;
  roiMemoWorldSequence=0;roiMemoWorldPage=nullptr;roiMemoWorldCount=0;
  ++stats.roiMemoResets;
}
RoiFramePlanMemo &roiFramePlanSlot(uint8_t kind,size_t group,Rect roi)const{
  // Collisions replace one result, never weaken key equality. The fallback
  // is always the original full proof; the bounded cache cannot reject work.
  uint32_t hash=kind==1?uint32_t(group):
    uint32_t(roi.x)*73856093u^uint32_t(roi.y)*19349663u^
    uint32_t(roi.width)*83492791u^uint32_t(roi.height)*2654435761u;
  // Rounded ROI coordinates have six zero low bits; fold upper bits before
  // indexing instead of sending every 64-pixel grid rectangle to slot zero.
  if(kind!=1){hash^=hash>>16;hash^=hash>>8;}
  return roiFramePlanMemo[hash&63u];
}
bool roiFramePlanLookup(const RoiFramePlanMemo &slot,uint8_t kind,size_t group,Rect roi)const{
  return roiPrefixSnapshotSequence&&slot.sequence==roiPrefixSnapshotSequence&&
    slot.page==rasterFrame.current.page&&slot.owner==frameSourceOwner&&
    slot.kind==kind&&slot.group==group&&roiPrefixRectEqual(slot.roi,roi);
}
void roiFramePlanStore(RoiFramePlanMemo &slot,uint8_t kind,size_t group,Rect roi,bool accepted,const RoiPrefixPlan &plan)const{
  slot.plan=plan;slot.roi=roi;slot.page=rasterFrame.current.page;slot.owner=frameSourceOwner;
  slot.group=group;slot.kind=kind;slot.accepted=accepted;slot.sequence=roiPrefixSnapshotSequence;
}
#endif
bool roiPrefixPlan(size_t group,RoiPrefixPlan &out)const{
// BEGIN ROI_PREFIX_TIMING scope-ContainerPlan
#if NICE_GFX_UI_ROI_PREFIX_TIMING
  RoiPrefixTimer roiTiming(*this,RoiPrefixTimingPhase::ContainerPlan);
#endif
// END ROI_PREFIX_TIMING scope-ContainerPlan

  ++stats.roiMemoContainerRequests;
#if NICE_GFX_UI_ROI_PREFIX_FRAME_MEMO
  if(roiPrefixSnapshotSequence){
    auto &slot=roiFramePlanSlot(1,group,{});
    if(roiFramePlanLookup(slot,1,group,{})){++stats.roiMemoPlanHits;if(slot.accepted)out=slot.plan;return slot.accepted;}
    RoiPrefixPlan plan{};const bool accepted=roiPrefixPlanUncached(group,plan);
    roiFramePlanStore(slot,1,group,{},accepted,plan);if(accepted)out=plan;return accepted;
  }
#endif
  return roiPrefixPlanUncached(group,out);
}
#if NICE_GFX_UI_ROI_PREFIX_DIRTY_REGION
bool roiDirtyPrefixPlan(Rect roi,RoiPrefixPlan &out)const{
// BEGIN ROI_PREFIX_TIMING scope-DirtyPlan
#if NICE_GFX_UI_ROI_PREFIX_TIMING
  RoiPrefixTimer roiTiming(*this,RoiPrefixTimingPhase::DirtyPlan);
#endif
// END ROI_PREFIX_TIMING scope-DirtyPlan

  ++stats.roiMemoDirtyRequests;
#if NICE_GFX_UI_ROI_PREFIX_FRAME_MEMO
  if(roiPrefixSnapshotSequence){
    auto &slot=roiFramePlanSlot(2,NoParent,roi);
    if(roiFramePlanLookup(slot,2,NoParent,roi)){++stats.roiMemoPlanHits;if(slot.accepted)out=slot.plan;return slot.accepted;}
    RoiPrefixPlan plan{};const bool accepted=roiDirtyPrefixPlanUncached(roi,plan);
    roiFramePlanStore(slot,2,NoParent,roi,accepted,plan);if(accepted)out=plan;return accepted;
  }
#endif
  return roiDirtyPrefixPlanUncached(roi,out);
}
#endif

bool roiPrefixPlanUncached(size_t group,RoiPrefixPlan &out)const{
  ++stats.roiMemoContainerEvaluations;
  const auto &page=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;const auto &geometry=rasterFrame.current.geometry.nodes;
  if(group>=page.count||values.size()!=page.count||geometry.size()!=page.count)return false;
  const auto &node=page.nodes[group];const auto &state=values[group];const auto &g=geometry[group];
  if(node.kind!=Kind::Container||state.kind!=Kind::Container||!g.visible||g.alpha!=255||state.imageOverride||state.dynamic||
     state.animating||state.animationPending||roiPrefixContinuous(node))return false;
  const Rect roi=intersect(g.rect,g.clip);
  if(!roiPrefixRectEqual(roi,g.rect)||roi.x<0||roi.y<0||roi.width<=0||roi.height<=0||
     int64_t(roi.x)+roi.width>page.width||int64_t(roi.y)+roi.height>page.height||uint64_t(roi.width)*roi.height<8192)return false;
  for(size_t at=group,count=0;at<values.size()&&count<values.size();++count){
    const auto &v=values[at];
    if(v.parent!=page.nodes[at].parent||v.kind!=Kind::Container||page.nodes[at].kind!=Kind::Container||v.dynamic||v.animating||v.animationPending||v.imageOverride||
       roiPrefixContinuous(page.nodes[at]))return false;
    if(v.parent==NoParent)break;if(v.parent>=at)return false;at=v.parent;
  }
  size_t limit=page.count;
  for(size_t i=group+1;i<page.count;++i)if(roiPrefixDescendant(i,group)&&geometry[i].visible&&roiPrefixContinuous(page.nodes[i])){limit=i;break;}
  if(limit<=group+1||limit==page.count||limit>1024)return false;
  size_t containers=0,shapes=0;bool opaqueRoot=false;
  const DirtyRect area(roi.x,roi.y,roi.width,roi.height);
  for(size_t i=0;i<limit;++i){
    const auto &v=values[i];const auto &n=page.nodes[i];const auto &q=geometry[i];
    if(!prefixTouches(q,area))continue;
    // Unknown mutable inputs, caller images, charts and live/ancestor movement
    // remain on the ordinary painter. Static resource tables stay immutable.
    if(q.alpha!=255||v.kind!=n.kind||v.parent!=n.parent||v.dynamic||v.imageOverride||v.instrument||n.instrument||n.motion||v.animating||v.animationPending||
       roiPrefixContinuous(n)||(n.kind!=Kind::Container&&n.kind!=Kind::Shape&&n.kind!=Kind::Text&&n.kind!=Kind::Label))return false;
    // An overflowing child's disabled/selection/style context can live outside
    // its own paint ROI. Initial scope permits only settled Container parents.
    for(uint16_t parent=v.parent;parent!=NoParent;parent=values[parent].parent){
      if(parent>=i)return false;const auto &a=values[parent];const auto &an=page.nodes[parent];
      if(a.parent!=an.parent||a.kind!=Kind::Container||an.kind!=Kind::Container||a.dynamic||a.imageOverride||a.instrument||an.instrument||an.motion||
         a.animating||a.animationPending||roiPrefixContinuous(an))return false;
    }
    const Style style=styleFor(page,values,int(i));
    bool disabled=(v.flags&Disabled)!=0;
    for(uint16_t parent=v.parent;parent!=NoParent&&!disabled;parent=values[parent].parent)disabled=(values[parent].flags&Disabled)!=0;
    if(n.kind==Kind::Container&&q.alpha==255&&!style.radius&&!emptyContainerPaint(int(i))&&
       !(v.flags&(disabled?DisabledTransparent:Transparent))&&roiPrefixContains(q.rect,area)&&roiPrefixContains(q.clip,area))opaqueRoot=true;
    if(i>group&&roiPrefixDescendant(i,group)){
      if(n.kind==Kind::Container&&!emptyContainerPaint(int(i)))++containers;
      if(n.kind==Kind::Shape)++shapes;
    }
  }
  // A transparent group is safe only over an independently certified opaque
  // earlier root. Count actual nonempty container painters: grid lines matter.
  if(!opaqueRoot||(containers<24&&shapes<40))return false;
  out={roi,group,limit,containers,shapes};return true;
}
#if NICE_GFX_UI_ROI_PREFIX_DIRTY_REGION
static bool roiDirtyPrefixRounded(const DirtyRect &damage,const PageDesc &page,Rect &roi){
  if(damage.x<0||damage.y<0||damage.width<=0||damage.height<=0||int64_t(damage.x)+damage.width>page.width||int64_t(damage.y)+damage.height>page.height)return false;
  const int64_t x=int64_t(damage.x)/64*64,y=int64_t(damage.y)/64*64;
  const int64_t right=std::min<int64_t>(page.width,(int64_t(damage.x)+damage.width+63)/64*64);
  const int64_t bottom=std::min<int64_t>(page.height,(int64_t(damage.y)+damage.height+63)/64*64);
  if(right<=x||bottom<=y||uint64_t(right-x)*uint64_t(bottom-y)>256u*1024u)return false;
  roi={int32_t(x),int32_t(y),int32_t(right-x),int32_t(bottom-y)};return true;
}
bool roiDirtyPrefixPlanUncached(Rect roi,RoiPrefixPlan &out)const{
  ++stats.roiMemoDirtyEvaluations;
  const auto &page=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;const auto &geometry=rasterFrame.current.geometry.nodes;
  if(values.size()!=page.count||geometry.size()!=page.count||roi.x<0||roi.y<0||roi.width<=0||roi.height<=0||
     int64_t(roi.x)+roi.width>page.width||int64_t(roi.y)+roi.height>page.height||uint64_t(roi.width)*roi.height<8192||uint64_t(roi.width)*roi.height>256u*1024u)return false;
  size_t limit=page.count;const size_t group=NoParent;
  for(size_t i=0;i<page.count;++i){const auto &g=geometry[i];if(!g.visible||!roiPrefixContinuous(page.nodes[i]))continue;
    const Rect overlap=intersect(intersect(g.rect,g.clip),roi);if(overlap.width>0&&overlap.height>0){limit=i;break;}}
  if(limit<8||limit==page.count||limit>1024)return false;
  size_t containers=0,shapes=0;bool opaqueRoot=false;
  const DirtyRect area(roi.x,roi.y,roi.width,roi.height);
  for(size_t i=0;i<limit;++i){
    const auto &v=values[i];const auto &n=page.nodes[i];const auto &q=geometry[i];
    if(!prefixTouches(q,area))continue;
    // Unknown mutable inputs, caller images, charts and live/ancestor movement
    // remain on the ordinary painter. Static resource tables stay immutable.
    if(q.alpha!=255||v.kind!=n.kind||v.parent!=n.parent||v.dynamic||v.imageOverride||v.instrument||n.instrument||n.motion||v.animating||v.animationPending||
       roiPrefixContinuous(n)||(n.kind!=Kind::Container&&n.kind!=Kind::Shape&&n.kind!=Kind::Text&&n.kind!=Kind::Label))return false;
    // An overflowing child's disabled/selection/style context can live outside
    // its own paint ROI. Initial scope permits only settled Container parents.
    for(uint16_t parent=v.parent;parent!=NoParent;parent=values[parent].parent){
      if(parent>=i)return false;const auto &a=values[parent];const auto &an=page.nodes[parent];
      if(a.parent!=an.parent||a.kind!=Kind::Container||an.kind!=Kind::Container||a.dynamic||a.imageOverride||a.instrument||an.instrument||an.motion||
         a.animating||a.animationPending||roiPrefixContinuous(an))return false;
    }
    const Style style=styleFor(page,values,int(i));
    bool disabled=(v.flags&Disabled)!=0;
    for(uint16_t parent=v.parent;parent!=NoParent&&!disabled;parent=values[parent].parent)disabled=(values[parent].flags&Disabled)!=0;
    if(n.kind==Kind::Container&&q.alpha==255&&!style.radius&&!emptyContainerPaint(int(i))&&
       !(v.flags&(disabled?DisabledTransparent:Transparent))&&roiPrefixContains(q.rect,area)&&roiPrefixContains(q.clip,area))opaqueRoot=true;
    {
      if(n.kind==Kind::Container&&!emptyContainerPaint(int(i)))++containers;
      if(n.kind==Kind::Shape)++shapes;
    }
  }
  // A transparent group is safe only over an independently certified opaque
  // earlier root. Count actual nonempty container painters: grid lines matter.
  if(!opaqueRoot||containers*2+shapes*3<64)return false;
  out={roi,group,limit,containers,shapes};return true;
}
#endif
#if NICE_GFX_UI_ROI_PREFIX_DIRTY_REGION && NICE_GFX_UI_ROI_PREFIX_MEDIA_WORLD
bool roiDirtyMediaWorldPlan(const DirtyRect &damage,RoiPrefixPlan &out)const{
// BEGIN ROI_PREFIX_TIMING scope-MediaWorld
#if NICE_GFX_UI_ROI_PREFIX_TIMING
  RoiPrefixTimer roiTiming(*this,RoiPrefixTimingPhase::MediaWorld);
#endif
// END ROI_PREFIX_TIMING scope-MediaWorld

  ++stats.roiMemoWorldRequests;
#if NICE_GFX_UI_ROI_PREFIX_FRAME_MEMO
  if(roiPrefixSnapshotSequence){
    if(roiMemoWorldSequence!=roiPrefixSnapshotSequence||roiMemoWorldPage!=rasterFrame.current.page||roiMemoWorldOwner!=frameSourceOwner){
      ++stats.roiMemoWorldBuilds;
  const auto &page=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;const auto &geometry=rasterFrame.current.geometry.nodes;
  uint16_t parents[16];size_t parentCount=0;roiMemoWorldCount=0;
  for(size_t first=0;first<page.count;++first){
    const auto &node=page.nodes[first];const auto &value=values[first];
    if(node.kind!=Kind::Media||value.kind!=Kind::Media||!node.media||value.imageOverride||value.dynamic||!geometry[first].visible||!roiPrefixContinuous(node))continue;
    bool seen=false;for(size_t p=0;p<parentCount;++p)seen|=parents[p]==value.parent;if(seen)continue;
    if(parentCount==16)break;parents[parentCount++]=value.parent;
    Rect world{};size_t count=0;
    for(size_t i=first;i<page.count;++i){const auto &n=page.nodes[i];const auto &v=values[i];const auto &g=geometry[i];
      if(n.kind!=Kind::Media||v.kind!=Kind::Media||!n.media||v.parent!=value.parent||v.imageOverride||v.dynamic||!g.visible||!roiPrefixContinuous(n))continue;
      const Rect r=intersect(g.rect,g.clip);if(r.width<=0||r.height<=0)continue;
      if(!count)world=r;else{const int64_t x=std::min<int64_t>(world.x,r.x),y=std::min<int64_t>(world.y,r.y),right=std::max(int64_t(world.x)+world.width,int64_t(r.x)+r.width),bottom=std::max(int64_t(world.y)+world.height,int64_t(r.y)+r.height);world={int32_t(x),int32_t(y),int32_t(right-x),int32_t(bottom-y)};}++count;
    }
    if(count<2)continue;
    // Include intersecting current continuous actors as candidate geometry,
    // never as cached pixels. No phase is changed and no envelope is guessed.
    for(size_t i=0;i<page.count;++i){const auto &g=geometry[i];if(!g.visible||!roiPrefixContinuous(page.nodes[i]))continue;
      const Rect r=intersect(g.rect,g.clip),overlap=intersect(r,world);if(overlap.width<=0||overlap.height<=0)continue;
      const int64_t x=std::min<int64_t>(world.x,r.x),y=std::min<int64_t>(world.y,r.y),right=std::max(int64_t(world.x)+world.width,int64_t(r.x)+r.width),bottom=std::max(int64_t(world.y)+world.height,int64_t(r.y)+r.height);world={int32_t(x),int32_t(y),int32_t(right-x),int32_t(bottom-y)};
    }
    Rect rounded;RoiPrefixPlan plan;
    if(!roiDirtyPrefixRounded(DirtyRect(world.x,world.y,world.width,world.height),page,rounded)||!roiDirtyPrefixPlan(rounded,plan))continue;
    roiMemoWorld[roiMemoWorldCount++]={world,plan};
  }
      roiMemoWorldSequence=roiPrefixSnapshotSequence;roiMemoWorldPage=rasterFrame.current.page;roiMemoWorldOwner=frameSourceOwner;
    }else ++stats.roiMemoWorldHits;
    bool found=false;size_t bestCost=0;
    for(size_t i=0;i<roiMemoWorldCount;++i){const auto &candidate=roiMemoWorld[i];const auto &plan=candidate.plan;
      if(!roiPrefixContains(candidate.world,damage)){const Rect overlap=intersect(candidate.world,{damage.x,damage.y,damage.width,damage.height});if(overlap.width<=0||overlap.height<=0)continue;}
      if(!roiPrefixContains(plan.roi,damage))continue;
      const size_t cost=plan.containers*2+plan.shapes*3;if(!found||cost>bestCost){out=plan;bestCost=cost;found=true;}
    }return found;
  }
#endif
  ++stats.roiMemoWorldBuilds;
  const auto &page=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;const auto &geometry=rasterFrame.current.geometry.nodes;
  uint16_t parents[16];size_t parentCount=0;bool found=false;size_t bestCost=0;
  for(size_t first=0;first<page.count;++first){
    const auto &node=page.nodes[first];const auto &value=values[first];
    if(node.kind!=Kind::Media||value.kind!=Kind::Media||!node.media||value.imageOverride||value.dynamic||!geometry[first].visible||!roiPrefixContinuous(node))continue;
    bool seen=false;for(size_t p=0;p<parentCount;++p)seen|=parents[p]==value.parent;if(seen)continue;
    if(parentCount==16)break;parents[parentCount++]=value.parent;
    Rect world{};size_t count=0;
    for(size_t i=first;i<page.count;++i){const auto &n=page.nodes[i];const auto &v=values[i];const auto &g=geometry[i];
      if(n.kind!=Kind::Media||v.kind!=Kind::Media||!n.media||v.parent!=value.parent||v.imageOverride||v.dynamic||!g.visible||!roiPrefixContinuous(n))continue;
      const Rect r=intersect(g.rect,g.clip);if(r.width<=0||r.height<=0)continue;
      if(!count)world=r;else{const int64_t x=std::min<int64_t>(world.x,r.x),y=std::min<int64_t>(world.y,r.y),right=std::max(int64_t(world.x)+world.width,int64_t(r.x)+r.width),bottom=std::max(int64_t(world.y)+world.height,int64_t(r.y)+r.height);world={int32_t(x),int32_t(y),int32_t(right-x),int32_t(bottom-y)};}++count;
    }
    if(count<2)continue;
    // Include intersecting current continuous actors as candidate geometry,
    // never as cached pixels. No phase is changed and no envelope is guessed.
    for(size_t i=0;i<page.count;++i){const auto &g=geometry[i];if(!g.visible||!roiPrefixContinuous(page.nodes[i]))continue;
      const Rect r=intersect(g.rect,g.clip),overlap=intersect(r,world);if(overlap.width<=0||overlap.height<=0)continue;
      const int64_t x=std::min<int64_t>(world.x,r.x),y=std::min<int64_t>(world.y,r.y),right=std::max(int64_t(world.x)+world.width,int64_t(r.x)+r.width),bottom=std::max(int64_t(world.y)+world.height,int64_t(r.y)+r.height);world={int32_t(x),int32_t(y),int32_t(right-x),int32_t(bottom-y)};
    }
    if(!roiPrefixContains(world,damage)){const Rect overlap=intersect(world,{damage.x,damage.y,damage.width,damage.height});if(overlap.width<=0||overlap.height<=0)continue;}
    Rect rounded;RoiPrefixPlan plan;
    if(!roiDirtyPrefixRounded(DirtyRect(world.x,world.y,world.width,world.height),page,rounded)||!roiPrefixContains(rounded,damage)||!roiDirtyPrefixPlan(rounded,plan))continue;
    const size_t cost=plan.containers*2+plan.shapes*3;
    if(!found||cost>bestCost){out=plan;bestCost=cost;found=true;}
  }return found;
}
#endif

bool roiPrefixMatches(NativePrefixEntry &entry,const RoiPrefixPlan &plan){
// BEGIN ROI_PREFIX_TIMING scope-SignatureMatch
#if NICE_GFX_UI_ROI_PREFIX_TIMING
  RoiPrefixTimer roiTiming(*this,RoiPrefixTimingPhase::SignatureMatch);
#endif
// END ROI_PREFIX_TIMING scope-SignatureMatch

  ++stats.roiMemoMatchEvaluations;
  const auto &page=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;const auto &geometry=rasterFrame.current.geometry.nodes;
  if(entry.page!=&page||entry.owner!=frameSourceOwner||entry.limit!=plan.limit||entry.roiGroup!=plan.group||
     !roiPrefixRectEqual(entry.roi,plan.roi)||entry.roiMetaCount!=plan.limit||!entry.roiMeta)return false;
  const DirtyRect area(plan.roi.x,plan.roi.y,plan.roi.width,plan.roi.height);
  for(size_t i=0;i<plan.limit;++i){const auto &old=entry.roiMeta[i];
    // Movement/clip/visibility into the ROI is validated as well as old ink.
    if(!old.context&&!prefixTouches(old.geometry,area)&&!prefixTouches(geometry[i],area))continue;
    if(!sameSurfaceNode(old.key,values[i])||old.key.kind!=values[i].kind||old.key.parent!=values[i].parent||old.key.styleIndex!=values[i].styleIndex||
       !prefixGeometryEqual(old.geometry,geometry[i])||old.component!=prefixComponent(values,i)||
       !roiPrefixStyleEqual(old.style,styleFor(page,values,int(i))))return false;
  }return true;
}
void clearNativeRoiPrefixes(const PageDesc *page=nullptr){
#if NICE_GFX_UI_ROI_PREFIX_FRAME_MEMO
  clearRoiPrefixFrameMemo();
#endif
  for(auto &entry:nativePrefixes)if(entry.roi.width&&(!page||entry.page==page)){
    entry.page=nullptr;entry.releaseRoi();entry.storage.release();
  }
}
const NativePrefixEntry *resolveNativeRoiPrefix(const DirtyRect &area){
// BEGIN ROI_PREFIX_TIMING scope-Resolve
#if NICE_GFX_UI_ROI_PREFIX_TIMING
  RoiPrefixTimer roiTiming(*this,RoiPrefixTimingPhase::Resolve);
#endif
// END ROI_PREFIX_TIMING scope-Resolve

  ++stats.roiMemoResolveCalls;
  if(!roiPrefixOrdinary())return nullptr;
  const auto &page=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;const auto &geometry=rasterFrame.current.geometry.nodes;
  if(!page.count||page.count>4096||values.size()!=page.count||geometry.size()!=page.count)return nullptr;
  // Check existing entries before scanning new groups. A plan is recomputed
  // each snapshot, so a formerly outside moving contributor cannot slip in.
  for(auto &entry:nativePrefixes)if(entry.roi.width&&entry.page==&page&&entry.owner==frameSourceOwner){
    const Rect overlap=intersect(entry.roi,{area.x,area.y,area.width,area.height});
    if(overlap.width<=0||overlap.height<=0)continue;
    if(entry.roiValidatedSequence!=roiPrefixSnapshotSequence){RoiPrefixPlan plan;
      ++stats.roiMemoEntryValidations;
      bool validPlan=false;
#if NICE_GFX_UI_ROI_PREFIX_DIRTY_REGION
      if(entry.roiGroup==NoParent)validPlan=roiDirtyPrefixPlan(entry.roi,plan);
      else
#endif
      validPlan=roiPrefixPlan(entry.roiGroup,plan);
      if(!validPlan||!roiPrefixMatches(entry,plan)){
#if NICE_GFX_UI_ROI_PREFIX_DIRTY_REGION
        if(entry.roiGroup==NoParent)++stats.roiDirtyPrefixInvalidations;
#endif
        entry.page=nullptr;entry.releaseRoi();++stats.roiPrefixInvalidations;
#if NICE_GFX_UI_ROI_PREFIX_FRAME_MEMO
        clearRoiPrefixFrameMemo();
#endif
        continue;
      }
      entry.roiValidatedSequence=roiPrefixSnapshotSequence;
    }
    if(roiPrefixContains(entry.roi,area)){entry.used=++nativePrefixClock;++stats.nativePrefixHits;++stats.roiPrefixHits;
#if NICE_GFX_UI_ROI_PREFIX_DIRTY_REGION
      if(entry.roiGroup==NoParent){++stats.roiDirtyPrefixHits;if(entry.roiScopeKind==2)++stats.roiDirtyPrefixWorldHits;}
#endif
// BEGIN ROI_PREFIX_TIMING resolve-hit
#if NICE_GFX_UI_ROI_PREFIX_TIMING
      roiTiming.accepted=true;
#endif
// END ROI_PREFIX_TIMING resolve-hit
      return &entry;}
    // Container caches keep v1 crossing fallback. A changing generic region
    // may select a distinct larger bounded grid scope without clipping damage.
#if NICE_GFX_UI_ROI_PREFIX_DIRTY_REGION
    if(entry.roiGroup==NoParent)continue;
#endif
    return nullptr;
  }
  RoiPrefixPlan best;bool found=false;size_t bestCost=0;
  for(size_t group=0;group<page.count;++group){
    if(page.nodes[group].kind!=Kind::Container||!geometry[group].visible)continue;
    const Rect visible=intersect(geometry[group].rect,geometry[group].clip),dirty=intersect(visible,{area.x,area.y,area.width,area.height});
    if(dirty.width<=0||dirty.height<=0)continue;
    RoiPrefixPlan plan;if(!roiPrefixPlan(group,plan))continue;
    const Rect overlap=intersect(plan.roi,{area.x,area.y,area.width,area.height});if(overlap.width<=0||overlap.height<=0)continue;
    const size_t cost=plan.containers+4*plan.shapes;
    if(!found||cost>bestCost||(cost==bestCost&&plan.group>best.group)){best=plan;bestCost=cost;found=true;}
  }
#if NICE_GFX_UI_ROI_PREFIX_DIRTY_REGION
  uint8_t scopeKind=0;
  if(!found){Rect rounded;bool genericFound=false;
#if NICE_GFX_UI_ROI_PREFIX_MEDIA_WORLD
    if(roiDirtyMediaWorldPlan(area,best)){rounded=best.roi;genericFound=true;scopeKind=2;}
#endif
    if(!genericFound&&roiDirtyPrefixRounded(area,page,rounded)&&roiDirtyPrefixPlan(rounded,best)){genericFound=true;scopeKind=1;}
    if(genericFound){
      found=true;++stats.roiDirtyPrefixPlans;
      if(scopeKind==2)++stats.roiDirtyPrefixWorldPlans;
      stats.roiDirtyPrefixLastX=rounded.x;stats.roiDirtyPrefixLastY=rounded.y;stats.roiDirtyPrefixLastWidth=rounded.width;stats.roiDirtyPrefixLastHeight=rounded.height;
      stats.roiDirtyPrefixLastDamagePixels=uint32_t(area.pixels());stats.roiDirtyPrefixLastScopePixels=uint32_t(uint64_t(rounded.width)*rounded.height);stats.roiDirtyPrefixLastLimit=uint32_t(best.limit);stats.roiDirtyPrefixLastScopeKind=scopeKind;
    }else ++stats.roiDirtyPrefixRejects;
  }
#endif
  if(!found)return nullptr;
  struct ColdTimer{Impl &s;uint32_t started;bool dirty;ColdTimer(Impl &v,bool d):s(v),started(v.profilingClock?v.profilingClock():0),dirty(d){}
    ~ColdTimer(){if(s.profilingClock){const auto elapsed=uint32_t(s.profilingClock()-started);s.stats.roiPrefixColdBuildMicros+=elapsed;
#if NICE_GFX_UI_ROI_PREFIX_DIRTY_REGION
      if(dirty)s.stats.roiDirtyPrefixColdBuildMicros+=elapsed;
#endif
    }}} coldTimer(*this,best.group==NoParent);
// BEGIN ROI_PREFIX_TIMING cold-meta-open
#if NICE_GFX_UI_ROI_PREFIX_TIMING
  RoiPrefixTimer roiColdMetadataTimer(*this,RoiPrefixTimingPhase::ColdMetadata);
#endif
// END ROI_PREFIX_TIMING cold-meta-open
  ++stats.roiPrefixPlans;
  const size_t pitch=(size_t(best.roi.width)+31)&~size_t(31),height=size_t(best.roi.height);
  if(!pitch||height>SIZE_MAX/pitch/2)return nullptr;const size_t required=pitch*height*2;
  constexpr size_t Budget=3u*1024u*1024u;if(required>Budget||best.limit>SIZE_MAX/sizeof(RoiPrefixMetadata))return nullptr;
  NativePrefixEntry *entry=nullptr;for(auto &e:nativePrefixes)if(!e.page){entry=&e;break;}
  if(!entry)for(auto &e:nativePrefixes)if(!entry||e.used<entry->used)entry=&e;
  size_t other=0;for(const auto &e:nativePrefixes)if(&e!=entry)other+=e.storage.bytes;
  while(other>Budget-required){NativePrefixEntry *oldest=nullptr;
    for(auto &e:nativePrefixes)if(&e!=entry&&e.storage.bytes&&(!oldest||e.used<oldest->used))oldest=&e;
    if(!oldest)return nullptr;other-=oldest->storage.bytes;oldest->page=nullptr;oldest->key.clear();oldest->geometry.clear();oldest->components.clear();oldest->releaseRoi();oldest->storage.release();
  }
  entry->page=nullptr;entry->key.clear();entry->geometry.clear();entry->components.clear();entry->releaseRoi();
  if(entry->storage.bytes!=required)entry->storage.release();
  if(!entry->storage.reserve(required)){++stats.roiPrefixOom;return nullptr;}
  auto *metadata=static_cast<RoiPrefixMetadata*>(platform::allocateRetained(best.limit*sizeof(RoiPrefixMetadata)));
  if(!metadata){entry->storage.release();++stats.roiPrefixOom;return nullptr;}
  // Copy constructors only retain existing shared owners; no vector/string
  // allocation is hidden in this checked optional cache allocation path.
  for(size_t i=0;i<best.limit;++i){new (metadata+i) RoiPrefixMetadata{values[i],geometry[i],styleFor(page,values,int(i)),prefixComponent(values,i)};metadata[i].key.text.owned.reset();}
  const DirtyRect coverage(best.roi.x,best.roi.y,best.roi.width,best.roi.height);
  for(size_t i=0;i<best.limit;++i)if(prefixTouches(geometry[i],coverage)){
    metadata[i].context=true;
    for(uint16_t parent=values[i].parent;parent!=NoParent;parent=values[parent].parent)metadata[parent].context=true;
  }
  entry->roiMeta=metadata;entry->roiMetaCount=best.limit;
  struct Scope{
    Impl &s;const NativeFramebuffer *overlay,*staging;const NativePrefixEntry *prefix;FrameCache *paint;
    int limit;bool repair,full,discard,aa,datum;PixelOrder order;const MonoFont *font;
    int32_t x,y,vx,vy,vw,vh;uint8_t size,textDatum,decoderState;uint32_t color,bg,bitmapFg,bitmapBg,decoderBuffer;
    Scope(Impl &owner,size_t stop):s(owner),overlay(s.nativeOverlayTarget),staging(s.stagingNativeTarget),prefix(s.nativePrefixSource),paint(s.paintCache),
      limit(s.nativePrefixLimit),repair(s.nativeCacheRepairing),full(s.renderingFullFrameSurface),discard(s.repaintDiscardFrame),aa(s.canvas.getAntialiasing()),datum(s.canvas.getViewportDatum()),order(s.renderOrder),font(s.canvas.getFont()),
      x(s.canvas.getCursorX()),y(s.canvas.getCursorY()),vx(s.canvas.getViewportX()),vy(s.canvas.getViewportY()),vw(s.canvas.getViewportWidth()),vh(s.canvas.getViewportHeight()),size(s.canvas.textsize),textDatum(s.canvas.textdatum),decoderState(s.canvas.decoderState),
      color(s.canvas.textcolor),bg(s.canvas.textbgcolor),bitmapFg(s.canvas.bitmap_fg),bitmapBg(s.canvas.bitmap_bg),decoderBuffer(s.canvas.decoderBuffer){
      s.nativeOverlayTarget=s.stagingNativeTarget=nullptr;s.nativePrefixSource=nullptr;s.nativePrefixLimit=int(stop);s.nativeCacheRepairing=true;
      s.renderingFullFrameSurface=true;s.repaintDiscardFrame=false;s.renderOrder=PixelOrder::Native;
    }
    ~Scope(){s.canvas.detach();s.nativeOverlayTarget=overlay;s.stagingNativeTarget=staging;s.nativePrefixSource=prefix;s.paintCache=paint;
      s.nativePrefixLimit=limit;s.nativeCacheRepairing=repair;s.renderingFullFrameSurface=full;s.repaintDiscardFrame=discard;s.renderOrder=order;
      s.canvas.setAntialiasing(aa);s.canvas.setFont(font);s.canvas.setTextSize(size);s.canvas.setCursor(x,y);s.canvas.textdatum=textDatum;s.canvas.textcolor=color;s.canvas.textbgcolor=bg;
      s.canvas.bitmap_fg=bitmapFg;s.canvas.bitmap_bg=bitmapBg;s.canvas.decoderBuffer=decoderBuffer;s.canvas.decoderState=decoderState;s.canvas.setViewport(vx,vy,vw,vh,datum);
    }
  };
  Status result;
// BEGIN ROI_PREFIX_TIMING cold-meta-close
#if NICE_GFX_UI_ROI_PREFIX_TIMING
  roiColdMetadataTimer.stop();
#endif
// END ROI_PREFIX_TIMING cold-meta-close
  {
// BEGIN ROI_PREFIX_TIMING cold-raster
#if NICE_GFX_UI_ROI_PREFIX_TIMING
  RoiPrefixTimer roiColdRasterTimer(*this,RoiPrefixTimingPhase::ColdRaster);
#endif
// END ROI_PREFIX_TIMING cold-raster
  Scope scope(*this,best.limit);result=render(this,DirtyRect(best.roi.x,best.roi.y,best.roi.width,best.roi.height),entry->storage.pixels,pitch);}

  stats.roiPrefixColdPixels+=uint64_t(best.roi.width)*best.roi.height;
#if NICE_GFX_UI_ROI_PREFIX_DIRTY_REGION
  if(best.group==NoParent)stats.roiDirtyPrefixColdPixels+=uint64_t(best.roi.width)*best.roi.height;
#endif
  if(result!=Status::Ok){entry->releaseRoi();entry->storage.release();++stats.roiPrefixRejects;return nullptr;}
  entry->roi=best.roi;entry->roiGroup=best.group;entry->page=&page;entry->owner=frameSourceOwner;entry->limit=best.limit;entry->pitch=pitch;entry->used=++nativePrefixClock;entry->roiValidatedSequence=roiPrefixSnapshotSequence;
#if NICE_GFX_UI_ROI_PREFIX_DIRTY_REGION
  entry->roiScopeKind=scopeKind;
#endif
  ++stats.roiPrefixColdBuilds;++stats.nativePrefixBuilds;
#if NICE_GFX_UI_ROI_PREFIX_FRAME_MEMO
  // Publishing a replacement entry invalidates all optional planning memo.
  clearRoiPrefixFrameMemo();
#endif
#if NICE_GFX_UI_ROI_PREFIX_DIRTY_REGION
  if(best.group==NoParent)++stats.roiDirtyPrefixColdBuilds;
  if(best.group==NoParent&&scopeKind==2)++stats.roiDirtyPrefixWorldColdBuilds;
#endif
  if(roiPrefixContains(entry->roi,area)){
// BEGIN ROI_PREFIX_TIMING resolve-cold-hit
#if NICE_GFX_UI_ROI_PREFIX_TIMING
    roiTiming.accepted=true;
#endif
// END ROI_PREFIX_TIMING resolve-cold-hit
    return entry;}
  return nullptr; // Crossing/full-frame damage retains the original painter.
}

#endif
#endif
// Included inside View::Impl. A supplied Native565 target is synchronous
// borrowed scratch, never owned by NativeCacheScratch and never published.
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR
bool validBorrowedNativeCacheScratch(const Surface &destination,const NativeFramebuffer &scratch,
                                    size_t pitch,const NativeFramebuffer *actualTarget=nullptr)const{
  if(!project||!rasterFrame.current.page||cacheOrder!=PixelOrder::Wire||!systemModalPanelBuilding||
     capacity>SIZE_MAX/2||compressedCapacity>SIZE_MAX/2)return false;
  const auto &p=*rasterFrame.current.page;const size_t width=p.width,height=p.height;
  if(!width||!height||!pitch||height>SIZE_MAX/pitch/2||height>SIZE_MAX/width/2)return false;
  const size_t required=pitch*height*2,rawBytes=width*height*2;
  const uintptr_t address=reinterpret_cast<uintptr_t>(scratch.pixels),output=reinterpret_cast<uintptr_t>(destination.pixels);
  if(!scratch.pixels||!destination.pixels||scratch.width!=width||scratch.height!=height||scratch.stride!=pitch||
     scratch.bytes!=rawBytes||scratch.bytes<required||(address&63u)||(pitch*2&63u)||(required&63u)||
     address>UINTPTR_MAX-scratch.bytes||output>UINTPTR_MAX-rawBytes||
     nativeMemoryOverlap(address,scratch.bytes,destination.pixels,rawBytes)||
     nativeSourceOverlap(p,rasterFrame.current.values,address,scratch.bytes,true)||
     (rasterFrame.preview.page&&nativeSourceOverlap(*rasterFrame.preview.page,rasterFrame.preview.values,address,scratch.bytes,true)))return false;
  if(actualTarget){
    const uintptr_t target=reinterpret_cast<uintptr_t>(actualTarget->pixels);
    if(!actualTarget->pixels||!actualTarget->bytes||target>UINTPTR_MAX-actualTarget->bytes||
       nativeMemoryOverlap(target,actualTarget->bytes,scratch.pixels,scratch.bytes))return false;
  }
  for(const auto *buffer:tileBuffers)if(nativeMemoryOverlap(address,scratch.bytes,buffer,capacity*2))return false;
  if(nativeMemoryOverlap(address,scratch.bytes,compressedStorage,compressedCapacity*2)||
     nativeMemoryOverlap(address,scratch.bytes,nativeCacheScratch.pixels,nativeCacheScratch.bytes))return false;
  bool found=false;
  for(const auto &entry:surfaces){
    if(!nativeMemoryOverlap(address,scratch.bytes,entry.pixels,rawBytes))continue;
    // Only the exact unpublished existing slot can back this frame. Every
    // other retained source and every lifecycle/accepted-frame pin is excluded.
    if(entry.pixels!=scratch.pixels||&entry==&destination||entry.page||entry.ready||
       &entry==snapshotSurface||&entry==previewSurface||&entry==snapshotAnimationSurface||&entry==coldFadeSurface||
       &entry==seedSurface||systemModalBasePinned(entry))return false;
    if(found)return false;found=true;
  }
  return found;
}
#endif

Status renderNativeCachePatch(Surface &surface,const DirtyRect &area,const NativeFramebuffer *suppliedScratch=nullptr){
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR
  if(displayPolicy.monochrome||area.width<=0||area.height<=0)return Status::Unsupported;
  const size_t w=rasterFrame.current.page->width,h=rasterFrame.current.page->height;
  const bool direct=cacheOrder==PixelOrder::Native;const size_t pitch=direct?w:(w+31)&~size_t(31);
  if(!w||!h||h>SIZE_MAX/pitch/2)return Status::Unsupported;
  NativeFramebuffer frame;
  if(suppliedScratch){
    if(direct||!validBorrowedNativeCacheScratch(surface,*suppliedScratch,pitch,nativeOverlayTarget))return Status::Unsupported;
    frame=*suppliedScratch;
  }else{
    // The private panel path may reuse existing owned scratch but cannot grow
    // it. Ordinary two-argument repair calls retain their allocation policy.
    if(systemModalPanelBuilding&&!direct&&nativeCacheScratch.bytes<pitch*h*2)return Status::Unsupported;
    if(!direct&&!nativeCacheScratch.reserve(pitch*h*2))return Status::Unsupported;
    frame.pixels=direct?surface.pixels:nativeCacheScratch.pixels;frame.width=w;frame.stride=pitch;frame.height=h;frame.bytes=direct?w*h*2:nativeCacheScratch.bytes;
  }
  struct Scope{
    Impl &s;const Surface *surface,*preview,*animation;Surface *seed;const PageDesc *previewPage;
    const NativeFramebuffer *overlay,*staging;PixelOrder order;
    bool building,full,viewport,transition,input,keyboard,repair,capturing;int pressed,focus,group,back;Id focusId;size_t layers;
    Scope(Impl &owner,const NativeFramebuffer &target):s(owner),surface(s.snapshotSurface),preview(s.previewSurface),animation(s.snapshotAnimationSurface),seed(s.seedSurface),previewPage(s.rasterFrame.preview.page),overlay(s.nativeOverlayTarget),staging(s.stagingNativeTarget),order(s.renderOrder),building(s.buildingSurface),full(s.renderingFullFrameSurface),viewport(s.renderingViewport),transition(s.snapshotTransition),input(s.snapshotInput),keyboard(s.snapshotKeyboard.visible),repair(s.nativeCacheRepairing),capturing(s.nativeCacheCapturing),pressed(s.snapshotPressed),focus(s.snapshotFocus),group(s.snapshotGroupIndex),back(s.snapshotBackOffset),focusId(s.snapshotFocusId),layers(s.snapshotAnimationLayerCount){
      s.snapshotSurface=s.previewSurface=s.snapshotAnimationSurface=nullptr;s.snapshotAnimationLayerCount=0;s.seedSurface=nullptr;s.rasterFrame.preview.page=nullptr;
      // Retained surfaces contain body pixels. Backdrops, circular frames and
      // other transient finishers belong to presentation, even for a small
      // status-only repair beneath an already-open ordinary modal.
      s.nativeOverlayTarget=&target;s.stagingNativeTarget=nullptr;s.nativeCacheRepairing=true;s.nativeCacheCapturing=true;s.renderOrder=PixelOrder::Native;s.buildingSurface=false;s.renderingFullFrameSurface=true;s.renderingViewport=false;
      s.snapshotTransition=s.snapshotInput=s.snapshotKeyboard.visible=false;s.snapshotPressed=s.snapshotFocus=s.snapshotGroupIndex=-1;s.snapshotFocusId=0;s.snapshotBackOffset=0;
    }
    ~Scope(){s.snapshotSurface=surface;s.previewSurface=preview;s.snapshotAnimationSurface=animation;s.snapshotAnimationLayerCount=layers;s.seedSurface=seed;s.rasterFrame.preview.page=previewPage;s.nativeOverlayTarget=overlay;s.stagingNativeTarget=staging;s.nativeCacheRepairing=repair;s.nativeCacheCapturing=capturing;s.renderOrder=order;s.buildingSurface=building;s.renderingFullFrameSurface=full;s.renderingViewport=viewport;s.snapshotTransition=transition;s.snapshotInput=input;s.snapshotKeyboard.visible=keyboard;s.snapshotPressed=pressed;s.snapshotFocus=focus;s.snapshotGroupIndex=group;s.snapshotFocusId=focusId;s.snapshotBackOffset=back;}
  } scope(*this,frame);
#if NICE_GFX_UI_NATIVE_PREFIX_CACHE
  const auto *savedPrefix=nativePrefixSource;nativePrefixSource=resolveNativePrefix(surface,area);
#endif
  const Status result=render(this,area,frame.pixels+size_t(area.y)*pitch+area.x,pitch);
#if NICE_GFX_UI_NATIVE_PREFIX_CACHE
  nativePrefixSource=savedPrefix;
#endif
  if(result!=Status::Ok)return result;
  if(direct)return Status::Ok;
#if NICE_GFX_UI_NATIVE_CACHE_WIRE_COPY
  const NativeFramebuffer output{surface.pixels,w,h,w,w*h*2};
  return detail::copyNativeRgb565ToWire(ppaFade,output,frame,area,ppaBlendEnabled);
#else
  for(int32_t y=0;y<area.height;++y){const auto *src=frame.pixels+size_t(area.y+y)*pitch+area.x;auto *dst=surface.pixels+size_t(area.y+y)*w+area.x;size_t count=area.width;
    while(count>=2){uint32_t pair;memcpy(&pair,src,4);pair=rgb565_blend::swapPairBytes(pair);memcpy(dst,&pair,4);src+=2;dst+=2;count-=2;}if(count)*dst=rgb565_blend::toWire(*src);
  }return Status::Ok;
#endif
#else
  (void)surface;(void)area;(void)suppliedScratch;return Status::Unsupported;
#endif
}
