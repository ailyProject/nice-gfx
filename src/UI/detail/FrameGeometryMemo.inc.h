// ISOLATED CANDIDATE: not part of the shipping runtime until independently
// compared against prepareCache with identical states, clocks and pixels.
// Included inside View::Impl after retained surfaces.
#ifndef NICE_GFX_UI_SYSTEM_PANEL_TRANSLATION_GEOMETRY
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_SYSTEM_PANEL_TRANSLATION_GEOMETRY 1
#else
#define NICE_GFX_UI_SYSTEM_PANEL_TRANSLATION_GEOMETRY 0
#endif
#endif
struct FrameGeometryKey {
  struct Text {const char *compiled=nullptr;uint64_t revision=0;} text;
  int32_t scrollX=0,scrollY=0,animationX=0,animationY=0;
  std::shared_ptr<const DynamicNode> dynamic;
  uint32_t motionRevision=0;
  uint16_t flags=0,parent=NoParent,styleIndex=NoParent;
  uint8_t opacity=255;
  Kind kind=Kind::Container;
  bool modalMember=false;
  bool matches(const State &value) const {
    return scrollX==value.scrollX&&scrollY==value.scrollY&&flags==value.flags&&
      animationX==value.animationX&&animationY==value.animationY&&opacity==value.opacity&&
      motionRevision==value.motionRevision&&dynamic==value.dynamic;
  }
  bool sameTranslationContext(const State &value,bool movingRoot)const {
    return scrollX==value.scrollX&&scrollY==value.scrollY&&flags==value.flags&&
      animationX==value.animationX&&(movingRoot||animationY==value.animationY)&&opacity==value.opacity&&
      motionRevision==value.motionRevision&&dynamic==value.dynamic&&parent==value.parent&&
      styleIndex==value.styleIndex&&kind==value.kind&&text.compiled==value.text.compiled&&text.revision==value.text.revision;
  }
  void assign(const State &value) {
    text.compiled=value.text.compiled;text.revision=value.text.revision;
    scrollX=value.scrollX;scrollY=value.scrollY;flags=value.flags;
    animationX=value.animationX;animationY=value.animationY;opacity=value.opacity;
    motionRevision=value.motionRevision;dynamic=value.dynamic;
    parent=value.parent;styleIndex=value.styleIndex;kind=value.kind;
  }
};
const PageDesc *geometryMemoPage=nullptr;
RetainedVector<FrameGeometryKey> geometryMemoKey;
FrameCache geometryMemoCache;
uint16_t geometryMemoPanelRoot=NoParent;
Id geometryMemoMarqueeId=0;
bool geometryMemoMenuMotion=false,geometryMemoEllipsis=false;
#if NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE
// Borrow only the stable ordinary current-frame cache. Direct cache writers
// invalidate this marker before changing its contents; no pointer dereference
// occurs when the next context/page/cache identity does not match.
const FrameCache *geometryMemoFrame=nullptr;
void invalidateBorrowedFrameGeometry(const FrameCache &cache){
  if(geometryMemoFrame==&cache){geometryMemoFrame=nullptr;geometryMemoPage=nullptr;
    #if NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY
    geometryMemoIncrementalReady=false;
    #endif
    ++stats.frameGeometryInPlaceInvalidations;}
}
#endif
#if NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY
bool geometryMemoIncrementalReady=false;
// Only the ordinary immutable current snapshot may seed this memo. Native cold
// repairs, surfaces, transitions, inputs and overlays retain full preparation.
bool incrementalGeometryContext(const PageDesc &p,const RetainedVector<State> &values,bool allowed) const {
  if(!allowed||!p.count||p.count>1024||values.size()!=p.count||
     rasterFrame.current.page!=&p||&values!=&rasterFrame.current.values||
     displayPolicy.monochrome||transition.active||snapshotTransition||rasterFrame.preview.page||
     snapshotSurface||snapshotAnimationSurface||snapshotKeyboard.visible||snapshotInput||
     snapshotMenuEnabled||snapshotMarqueeId||snapshotPressed>=0||styledFocus()||buildingSurface||foregroundCapture||
     renderingViewport||viewportOverlayLevels||groupMotion.active||groupMotion.prepared||
     coldFadeSurface||stagingNativeTarget||nativePrefixLimit>=0||renderBudget.maxPixelsPerTick)return false;
#if NICE_GFX_UI_NATIVE_CACHE_REPAIR
  if(nativeCacheRepairing)return false;
#endif
  for(uint16_t i=0;i<p.count;++i){const auto &s=values[i];
    if(s.dynamic||(s.kind==Kind::Shape&&s.instrument)||s.parent!=p.nodes[i].parent||s.kind!=p.nodes[i].kind||
       (s.parent!=NoParent&&s.parent>=i))return false;
  }
  return true;
}
NICE_GFX_UI_GEOMETRY_IRAM_ATTR bool prepareIncrementalFrameGeometry(const PageDesc &p,const RetainedVector<State> &values,FrameCache &cache,int32_t visibleHeight,bool inPlace=false) {
#if NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE
  const FrameCache &previous=inPlace?cache:geometryMemoCache;
#else
  (void)inPlace;const FrameCache &previous=geometryMemoCache;
#endif
  ++stats.incrementalGeometryAttempts;
  if(!geometryMemoIncrementalReady||geometryMemoPage!=&p||geometryMemoKey.size()!=values.size()||
     previous.nodes.size()!=values.size()||previous.visibleHeight!=visibleHeight||
     previous.words!=(size_t(p.count)+31)/32||
     previous.bandHeight!=(int32_t(p.height)+31)/32||
     previous.bands.size()!=previous.words*32||&cache==&geometryMemoCache){
    ++stats.incrementalGeometryFallbacks;return false;
  }
  uint32_t changed[32]={};
  for(uint16_t i=0;i<p.count;++i){const auto &key=geometryMemoKey[i];const auto &s=values[i];const auto &old=previous.nodes[i];
    if(key.dynamic||key.parent!=s.parent||key.kind!=s.kind||key.styleIndex!=s.styleIndex||
       (old.lineCount&&(old.lineFirst>previous.lines.size()||old.lineCount>previous.lines.size()-old.lineFirst))){
      ++stats.incrementalGeometryFallbacks;return false;
    }
    if(!key.matches(s)||(s.parent!=NoParent&&(changed[s.parent/32]&(uint32_t(1)<<(s.parent%32)))))
      changed[i/32]|=uint32_t(1)<<(i%32);
  }
  #if NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE
  if(!inPlace)
  #endif
  cache.nodes=previous.nodes;cache.words=previous.words;cache.bandHeight=previous.bandHeight;
  cache.visibleHeight=visibleHeight;cache.mediaFirst=previous.mediaFirst;cache.mediaEnd=previous.mediaEnd;
  cache.bands.assign(cache.words*32,0);
#if NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE
  if(inPlace)geometryMemoCache.lines.swap(cache.lines);
  const auto &previousLines=inPlace?geometryMemoCache.lines:previous.lines;
#else
  const auto &previousLines=previous.lines;
#endif
  cache.lines.clear();
  for(uint16_t i=0;i<p.count;++i){auto &g=cache.nodes[i];const auto oldLineFirst=previous.nodes[i].lineFirst;const auto oldLineCount=previous.nodes[i].lineCount;const auto &s=values[i];
    const bool dirty=(changed[i/32]&(uint32_t(1)<<(i%32)))!=0;
    if(dirty){prepareCacheGeometryNode(p,values,cache,i,false);++stats.geometryNodes;++stats.incrementalGeometryNodes;}
    else {++stats.geometryReuses;++stats.incrementalGeometryReuses;}
    Rect c=intersect(g.rect,g.clip);if(visibleHeight>=0)c=intersect(c,{0,0,p.width,visibleHeight});
    if(dirty||geometryMemoKey[i].text.revision!=s.text.revision||geometryMemoKey[i].text.compiled!=s.text.compiled){
      if(g.visible&&g.alpha&&c.width>0&&c.height>0)prepareText(p,cache,g,p.nodes[i],s,styleFor(p,values,i));
      else {g.lineFirst=uint32_t(cache.lines.size());g.lineCount=0;g.textY=0;}
    }else{
      g.lineFirst=uint32_t(cache.lines.size());
      if(oldLineCount){cache.lines.insert(cache.lines.end(),previousLines.begin()+oldLineFirst,
        previousLines.begin()+oldLineFirst+oldLineCount);++stats.textLayoutReuses;}
    }
    if(!g.visible||!g.alpha||c.width<=0||c.height<=0)continue;
    const int first=clamp32(c.y/cache.bandHeight,0,31),last=clamp32((c.y+c.height-1)/cache.bandHeight,0,31);
    for(int band=first;band<=last;++band)cache.bands[size_t(band)*cache.words+i/32]|=uint32_t(1)<<(i%32);
  }
  #if NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE
  if(inPlace){++stats.frameGeometryInPlaceFrames;stats.frameGeometryInPlaceNodeBytes+=uint64_t(p.count)*sizeof(detail::FrameGeometry)*2;}
  #endif
  ++stats.incrementalGeometryFrames;return true;
}
#endif
void clearFrameGeometryMemo() {
#if NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE
  geometryMemoFrame=nullptr;
#endif
#if NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY
  geometryMemoIncrementalReady=false;
#endif
#if NICE_GFX_UI_TEXT_LAYOUT_MEMO
  clearTextLayoutMemo();
#endif
  geometryMemoPage=nullptr;RetainedVector<FrameGeometryKey>().swap(geometryMemoKey);geometryMemoCache=FrameCache{};
  geometryMemoPanelRoot=NoParent;geometryMemoMarqueeId=0;geometryMemoMenuMotion=geometryMemoEllipsis=false;
}
NICE_GFX_UI_GEOMETRY_IRAM_ATTR void retainFrameGeometry(const PageDesc &p,const RetainedVector<State> &values,const FrameCache &cache,bool borrowFrame=false) {
  geometryMemoPanelRoot=NoParent;
  if(behaviorVisuals.size()==1){const auto &visual=behaviorVisuals.front();
    if(visual.root>0&&size_t(visual.root)<values.size()&&visual.kind==BehaviorKind::Modal&&visual.motion==DialogMotion::Spring&&
       std::binary_search(systemPanelRoots.begin(),systemPanelRoots.end(),p.nodes[visual.root].id))geometryMemoPanelRoot=uint16_t(visual.root);
  }
  geometryMemoKey.resize(values.size());
  for(size_t i=0;i<values.size();++i){
    auto &key=geometryMemoKey[i];key.assign(values[i]);
    key.modalMember=i==geometryMemoPanelRoot||(key.parent!=NoParent&&key.parent<i&&geometryMemoKey[key.parent].modalMember);
  }
  geometryMemoMarqueeId=snapshotMarqueeId;geometryMemoMenuMotion=menuMotionEnabled();geometryMemoEllipsis=menuMotionConfig.ellipsis;
  #if NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE
  if(borrowFrame&&&cache==&rasterFrame.current.geometry){geometryMemoFrame=&cache;geometryMemoPage=&p;return;}
  geometryMemoFrame=nullptr;
  #else
  (void)borrowFrame;
  #endif
  geometryMemoCache=cache;geometryMemoPage=&p;
}
// Reuse layout only for one registered opaque full-screen system panel. Every
// key except its root Y translation, and every text-layout context, must match.
// Clip and row bands are rebuilt: clipped rectangles cannot simply be shifted
// because the physical screen stays fixed while the panel moves through it.
bool prepareTranslatedSystemPanelGeometry(const PageDesc &p,const RetainedVector<State> &values,FrameCache &cache,int32_t visibleHeight){
  if(geometryMemoPage!=&p||values.size()!=p.count||geometryMemoKey.size()!=values.size()||geometryMemoCache.nodes.size()!=values.size()||
     geometryMemoCache.visibleHeight!=visibleHeight||snapshotTransition||rasterFrame.preview.page||snapshotKeyboard.visible||snapshotInput||
     snapshotPressed>=0||focusStyleCount||styledFocus()||buildingSurface||nativeCacheRepairing||foregroundCapture||groupMotion.active||backPrepared||backOffset||menuMoving()||
     behaviorVisuals.size()!=1||geometryMemoPanelRoot==NoParent||snapshotMarqueeId!=geometryMemoMarqueeId||
     menuMotionEnabled()!=geometryMemoMenuMotion||bool(menuMotionConfig.ellipsis)!=geometryMemoEllipsis)return false;
  const auto &visual=behaviorVisuals.front();const uint16_t root=geometryMemoPanelRoot;
  if(root>=values.size()||visual.root!=root||visual.kind!=BehaviorKind::Modal||visual.motion!=DialogMotion::Spring||visual.reveal!=1024||
     !std::binary_search(systemPanelRoots.begin(),systemPanelRoots.end(),p.nodes[root].id))return false;
  const auto &value=values[root];const auto &node=p.nodes[root];const auto &oldRoot=geometryMemoCache.nodes[root];
  const Rect screen{0,0,p.width,p.height};const auto &style=styleFor(p,values,root);
  if(value.parent!=0||node.parent!=0||node.kind!=Kind::Container||value.kind!=Kind::Container||value.dynamic||
     (value.flags&(Hidden|Disabled|Transparent))||!(value.flags&Clip)||value.opacity!=255||value.animationX||node.motion||
     node.animation.effect!=AnimationEffect::None||node.exitAnimation||style.radius||style.gradient||style.borderWidth||
     node.rect.x||node.rect.y||node.rect.width!=p.width||node.rect.height!=p.height||
     visual.full.x||visual.full.y||visual.full.width!=p.width||visual.full.height!=p.height||
     visual.visual.x||visual.visual.y!=value.animationY||visual.visual.width!=p.width||visual.visual.height!=p.height||
     value.animationY>0||value.animationY<-int32_t(p.height)||geometryMemoKey[root].animationY>0||geometryMemoKey[root].animationY<-int32_t(p.height)||
     !oldRoot.visible||oldRoot.alpha!=255||oldRoot.rect.x||oldRoot.rect.y!=geometryMemoKey[root].animationY||
     oldRoot.rect.width!=p.width||oldRoot.rect.height!=p.height||!modalBaseSameRect(oldRoot.clip,screen))return false;
  const int64_t delta=int64_t(value.animationY)-geometryMemoKey[root].animationY;
  if(!delta)return false;
  for(uint16_t i=0;i<p.count;++i){const auto &key=geometryMemoKey[i];
    if(!key.sameTranslationContext(values[i],i==root)||
       (values[i].parent!=NoParent&&values[i].parent>=i))return false;
    // Hidden nodes may retain an unused old lineFirst. Only nonempty ranges
    // are ever dereferenced, and all of those are checked before cache writes.
    const auto &old=geometryMemoCache.nodes[i];
    if(old.lineCount&&(size_t(old.lineFirst)>geometryMemoCache.lines.size()||
       size_t(old.lineCount)>geometryMemoCache.lines.size()-size_t(old.lineFirst)))return false;
    if(!key.modalMember)continue;
    if(i!=root&&(viewportOverlayRoot(p,i)||p.nodes[i].motion))return false;
    const int64_t shifted=int64_t(geometryMemoCache.nodes[i].rect.y)+delta;
    if(shifted<INT32_MIN||shifted>INT32_MAX)return false;
#if NICE_GFX_UI_SHAPE_INK_DAMAGE
    const int64_t ink=int64_t(geometryMemoCache.nodes[i].paintRect.y)+delta;
    if(ink<INT32_MIN||ink>INT32_MAX)return false;
#endif
  }
  cache.nodes=geometryMemoCache.nodes;cache.words=geometryMemoCache.words;cache.bandHeight=geometryMemoCache.bandHeight;
  cache.visibleHeight=visibleHeight;cache.mediaFirst=geometryMemoCache.mediaFirst;cache.mediaEnd=geometryMemoCache.mediaEnd;
  cache.bands.assign(cache.words*32,0);cache.lines.clear();
  for(uint16_t i=0;i<p.count;++i){auto &g=cache.nodes[i];const auto &old=geometryMemoCache.nodes[i];
    if(geometryMemoKey[i].modalMember){
      g.rect.y=int32_t(int64_t(g.rect.y)+delta);
#if NICE_GFX_UI_SHAPE_INK_DAMAGE
      g.paintRect.y=int32_t(int64_t(g.paintRect.y)+delta);
#endif
      if(i==root)g.clip=screen;
      else {const auto parent=values[i].parent;g.clip=cache.nodes[parent].clip;
        if((values[parent].flags&Clip)||scrolling(values[parent].kind))g.clip=intersect(g.clip,childBox(p,values,parent,cache.nodes[parent].rect));
      }
    }
    Rect clipped=intersect(g.rect,g.clip),oldClipped=intersect(old.rect,old.clip);
    if(visibleHeight>=0){clipped=intersect(clipped,{0,0,p.width,visibleHeight});oldClipped=intersect(oldClipped,{0,0,p.width,visibleHeight});}
    g.lineCount=0;
    if(!g.visible||!g.alpha||clipped.width<=0||clipped.height<=0)continue;
    // An offscreen leaf had no old text layout. Build its real layout on its
    // first visible frame; later positions can reuse these owned line ranges.
    if(!old.visible||!old.alpha||oldClipped.width<=0||oldClipped.height<=0)
      prepareText(p,cache,g,p.nodes[i],values[i],styleFor(p,values,i));
    else {
      g.lineFirst=uint32_t(cache.lines.size());g.lineCount=old.lineCount;
      if(old.lineCount){cache.lines.insert(cache.lines.end(),geometryMemoCache.lines.begin()+old.lineFirst,geometryMemoCache.lines.begin()+old.lineFirst+old.lineCount);++stats.textLayoutReuses;}
    }
    const int first=clamp32(clipped.y/cache.bandHeight,0,31),last=clamp32((clipped.y+clipped.height-1)/cache.bandHeight,0,31);
    for(int band=first;band<=last;++band)cache.bands[size_t(band)*cache.words+i/32]|=uint32_t(1)<<(i%32);
  }
  stats.geometryReuses+=p.count;return true;
}
NICE_GFX_UI_GEOMETRY_IRAM_ATTR void prepareFrameGeometry(const PageDesc &p,const RetainedVector<State> &values,FrameCache &cache,int32_t visibleHeight=-1,bool allowIncremental=false) {
  GeometryTimer geometryTimer(*this);
#if NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY
  const bool incremental=incrementalGeometryContext(p,values,allowIncremental);
#else
  (void)allowIncremental;
#endif
#if NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE && NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY
  if(geometryMemoFrame){
    if(incremental&&geometryMemoFrame==&cache&&&cache==&rasterFrame.current.geometry&&geometryMemoPage==&p&&
       prepareIncrementalFrameGeometry(p,values,cache,visibleHeight,true)){
      retainFrameGeometry(p,values,cache,true);geometryMemoIncrementalReady=true;return;
    }
    // Never read a borrowed cache after a page/context/cache mismatch. This
    // path deliberately rebuilds instead of preserving an uncertain memo.
    geometryMemoFrame=nullptr;geometryMemoPage=nullptr;geometryMemoIncrementalReady=false;
    ++stats.frameGeometryInPlaceInvalidations;
  }
#endif
  if(geometryMemoPage==&p&&geometryMemoKey.size()==values.size()&&geometryMemoCache.nodes.size()==values.size()&&geometryMemoCache.visibleHeight==visibleHeight){
    const bool rootMoved=geometryMemoPanelRoot!=NoParent&&
      size_t(geometryMemoPanelRoot)<values.size()&&
      geometryMemoKey[geometryMemoPanelRoot].animationY!=values[geometryMemoPanelRoot].animationY;
    bool same=!rootMoved;for(size_t i=0;same&&i<values.size();++i)if(!geometryMemoKey[i].matches(values[i])){same=false;break;}
    if(same){prepareVisualCache(p,values,geometryMemoKey,geometryMemoCache,cache);retainFrameGeometry(p,values,cache,
#if NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE && NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY
      incremental
#else
      false
#endif
    );
#if NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY
      geometryMemoIncrementalReady=incremental;
#endif
return;}
#if NICE_GFX_UI_SYSTEM_PANEL_TRANSLATION_GEOMETRY
    if(prepareTranslatedSystemPanelGeometry(p,values,cache,visibleHeight)){
      geometryMemoKey[geometryMemoPanelRoot].animationY=values[geometryMemoPanelRoot].animationY;
      geometryMemoCache=cache;
#if NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY
      geometryMemoIncrementalReady=false;
#endif
      return;
    }
#endif
  }
#if NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY
  if(incremental&&prepareIncrementalFrameGeometry(p,values,cache,visibleHeight)){
    retainFrameGeometry(p,values,cache,
#if NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE && NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY
      incremental
#else
      false
#endif
    );geometryMemoIncrementalReady=true;return;
  }
#endif
  for(const auto &entry:surfaces)if(entry.page==&p&&entry.ready&&entry.key.size()==values.size()&&entry.geometry.nodes.size()==values.size()&&entry.geometry.visibleHeight==visibleHeight){
    bool same=true;for(size_t i=0;i<values.size();++i)if(!sameSurfaceGeometry(entry.key[i],values[i])){same=false;break;}
    if(same){prepareVisualCache(p,values,entry.key,entry.geometry,cache);retainFrameGeometry(p,values,cache,
#if NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE && NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY
      incremental
#else
      false
#endif
    );
#if NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY
      geometryMemoIncrementalReady=incremental;
#endif
return;}
  }
  prepareCache(p,values,cache,visibleHeight);retainFrameGeometry(p,values,cache,
#if NICE_GFX_UI_FRAME_GEOMETRY_IN_PLACE && NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY
      incremental
#else
      false
#endif
    );
#if NICE_GFX_UI_INCREMENTAL_FRAME_GEOMETRY
      geometryMemoIncrementalReady=incremental;
#endif

}
