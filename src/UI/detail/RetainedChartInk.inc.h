// Included inside the real View::Impl. Default OFF and no new clock reads.
struct RetainedChartKey {
  const PageDesc *page=nullptr;int index=-1;Rect box{};uint16_t bg=0,track=0;
  uint8_t radius=0,grid=0,gridX=0,minor=0;
  bool same(const RetainedChartKey&b)const{return page==b.page&&index==b.index&&box.x==b.box.x&&box.y==b.box.y&&box.width==b.box.width&&box.height==b.box.height&&bg==b.bg&&track==b.track&&radius==b.radius&&grid==b.grid&&gridX==b.gridX&&minor==b.minor;}
};
struct RetainedChartPixels {
  struct Free {void operator()(uint8_t*p)const{platform::freeUiMemory(p);}};
  std::unique_ptr<uint8_t[],Free> bits;size_t bytes=0;RetainedChartKey key{};uint64_t token=0;
  nice_gfx::TileCanvas::RetainedInkMask mask{};
};
struct RetainedChartBuffer {uintptr_t physical=0;uint64_t epoch=0;RetainedChartPixels charts[4];};
RetainedChartBuffer retainedBuffers[2];
RetainedChartKey retainedKeys[4];uint64_t retainedRevisions[4]{};
uint64_t retainedOwner=nextRetainedChartOwner(),retainedRevision=0;
nice_gfx::RetainedRepaintRequest retainedRequests[4];nice_gfx::RetainedRepaintReceipt retainedReceipts[4];
DirtyRect retainedExclusions[4];size_t retainedCount=0,retainedExcludeCount=0;
RetainedChartPixels *retainedPixels[4]{};bool retainedHits[4]{},retainedPainted[4]{};
bool retainedActive=false;
uint64_t retainedSkippedBefore=0,retainedInkBefore=0,retainedRestoredBefore=0;
int retainedIndex(int node)const{if(!retainedActive)return -1;for(size_t i=0;i<retainedCount;++i)if(retainedKeys[i].index==node)return int(i);return -1;}
void retainedForgetFrame(){retainedActive=false;retainedCount=retainedExcludeCount=0;canvas.setRetainedExclusions(nullptr,0);canvas.setRetainedBackgroundMask(nullptr);canvas.setRetainedInstrumentMask(nullptr);}
static bool retainedContains(Rect outer,Rect inner){return inner.width>0&&inner.height>0&&inner.x>=outer.x&&inner.y>=outer.y&&int64_t(inner.x)+inner.width<=int64_t(outer.x)+outer.width&&int64_t(inner.y)+inner.height<=int64_t(outer.y)+outer.height;}
Status retainedBegin(nice_gfx::Backend &panel,const DirtyRect *damage,size_t damageCount){
  retainedForgetFrame();
#if NICE_GFX_UI_RETAINED_CHART_INK && NICE_GFX_UI_CHART_FUSED_BACKDROP_GRID && !NICE_GFX_UI_NATIVE_PREFIX_CACHE
  if(!retainedOwner||retainedOwner>UINT64_MAX/8-1)return Status::Unsupported;
  if(!damageCount||!panel.supportsFrameBlit()||renderBudget.maxPixelsPerTick||buildingSurface||foregroundCapture||renderingViewport||
     snapshotTransition||transition.active||rasterFrame.preview.page||snapshotSurface||snapshotAnimationSurface||snapshotKeyboard.visible||snapshotInput||circularKeyboard||
     snapshotMenuEnabled||snapshotMarqueeId||styledFocus()||snapshotPressed>=0||groupMotion.active||groupMotion.prepared||viewportOverlayLevels||nativePrefixLimit>=0||!behaviorVisuals.empty()||behaviorVisualMoving()||
     coldFadeSurface||displayPolicy.monochrome||(!NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT&&ppaBlendEnabled&&rgb565_ppa::Context::available()))return Status::Unsupported;
  const auto &p=*rasterFrame.current.page;const auto&values=rasterFrame.current.values;const auto&cache=rasterFrame.current.geometry;
  if(cache.nodes.size()!=p.count)return Status::Unsupported;
#if NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
  // A frame with media keeps the original accelerated compositor. Small
  // charts cannot justify making every painter in this dirty frame CPU-only.
  for(uint16_t k=0;k<p.count;++k)if(cache.nodes[k].visible&&cache.nodes[k].alpha&&(p.nodes[k].kind==Kind::Media||values[k].kind==Kind::Media))return Status::Unsupported;
#endif
  for(uint16_t index=0;index<p.count;++index){const auto&n=p.nodes[index];const auto&v=values[index];const auto&g=cache.nodes[index];
    if(n.kind!=Kind::LineChart||v.kind!=Kind::LineChart||!g.visible||!n.instrument||!v.instrument||v.dynamic||g.alpha!=255||g.children||
       (v.flags&(Transparent|Disabled|Hidden))||focusStyle(index))continue;
    const Style s=styleFor(p,values,index);const auto&a=*n.instrument;const auto*o=a.chartStyle;
    if(!o||a.horizontal||o->fillMode||o->axes||o->legend||o->referenceCount||s.borderWidth||s.gradient||s.paddingTop||s.paddingRight||s.paddingBottom||s.paddingLeft||
       g.rect.width<2||g.rect.height<2||g.rect.width>2048||g.rect.height>2048||a.grid>10||o->gridX>24||o->minorGrid>5||
       a.grid*(o->minorGrid+1)>=64||o->gridX*(o->minorGrid+1)>=96)continue;
    if(!retainedContains(g.clip,g.rect))continue;
    const int radius=std::min<int32_t>(s.radius,std::min(g.rect.width,g.rect.height)/2),inset=std::max(radius,1);
    const Rect center{g.rect.x+inset,g.rect.y+inset,g.rect.width-2*inset,g.rect.height-2*inset};if(center.width<=0||center.height<=0)continue;
#if NICE_GFX_UI_RETAINED_CHART_CPU_CONTEXT
    if(center.width<128||center.height<80||uint64_t(center.width)*center.height<32768)continue;
#endif
    bool safe=true;size_t covers=0;
    for(size_t i=0;i<damageCount;++i){const auto&d=damage[i];if(retainedContains({d.x,d.y,d.width,d.height},g.rect))++covers;
      else if(nice_gfx::detail::RetainedRepaintTags::overlaps(d,{center.x,center.y,center.width,center.height}))safe=false;}
    if(covers!=1||!safe)continue;
    for(int at=index;at>=0;at=values[at].parent==NoParent?-1:values[at].parent){const auto&av=values[at];const auto&ag=cache.nodes[at];const auto&an=p.nodes[at];const Style as=styleFor(p,values,at);
      if(av.dynamic||av.animating||av.animationPending||(av.flags&Disabled)||ag.alpha!=255||as.gradient||
         (at!=index&&(an.kind!=Kind::Container||an.instrument||as.borderWidth||componentFor(at)))){safe=false;break;}
      if(an.motion){const auto&m=motionPose(p,values,at);if(m.scale!=1024||m.rotation%360||m.reveal!=1024){safe=false;break;}}
    }
    if(!safe)continue;
    for(uint16_t k=0;k<p.count&&safe;++k){if(k==index||!cache.nodes[k].visible||!cache.nodes[k].alpha)continue;
      bool ancestor=false;for(int at=values[index].parent==NoParent?-1:values[index].parent;at>=0;at=values[at].parent==NoParent?-1:values[at].parent)if(at==k){ancestor=true;break;}
      if(ancestor)continue;
      // Includes NOClip shape extents, before and after the chart. The real
      // painter is viewport-clipped to g.rect, but its actual paint bounds may
      // extend beyond the authored box; never use declared bounds alone.
      const Rect bounds=intersect(shapePaintBounds(p,values,k,cache.nodes[k].rect),cache.nodes[k].clip);
      safe=!nice_gfx::detail::RetainedRepaintTags::overlaps({bounds.x,bounds.y,bounds.width,bounds.height},{center.x,center.y,center.width,center.height});
    }
    if(!safe){++stats.retainedChartRejectedWriters;continue;}
    if(retainedCount==4){retainedForgetFrame();return Status::Unsupported;}
    const size_t slot=retainedCount;RetainedChartKey key{&p,int(index),g.rect,s.background,s.trackColor,uint8_t(radius),uint8_t(a.grid),uint8_t(o->gridX),uint8_t(o->minorGrid)};
    if(!retainedKeys[slot].same(key)){if(retainedRevision==UINT64_MAX){retainedForgetFrame();return Status::Unsupported;}retainedKeys[slot]=key;retainedRevisions[slot]=++retainedRevision;}
    retainedRequests[slot]={retainedOwner*8+slot+1,retainedRevisions[slot],{center.x,center.y,center.width,center.height}};++retainedCount;
  }
  if(!retainedCount)return Status::Unsupported;
  // Allocate both physical-buffer masks before beginning a backend transaction.
  // Failure cannot mutate a framebuffer or leave an untracked lease open.
  for(size_t i=0;i<retainedCount;++i){const auto&r=retainedRequests[i].rect;const size_t bytes=(size_t(r.width)*size_t(r.height)+7)/8;
    if(bytes>256*1024){retainedForgetFrame();return Status::Unsupported;}
    for(auto&buffer:retainedBuffers){auto&pixels=buffer.charts[i];if(pixels.bytes<bytes){std::unique_ptr<uint8_t[],RetainedChartPixels::Free> fresh(static_cast<uint8_t*>(platform::allocateRetained(bytes)));if(!fresh){retainedForgetFrame();return Status::Unsupported;}pixels.bits=std::move(fresh);pixels.bytes=bytes;pixels.token=0;}}
  }
  const auto s=panel.beginRetainedRepaintFrame(damage,damageCount,retainedRequests,retainedCount,retainedReceipts);
  if(s==Status::Ok)retainedActive=true;else retainedForgetFrame();return s;
#else
  (void)panel;(void)damage;(void)damageCount;return Status::Unsupported;
#endif
}
bool retainedBind(const nice_gfx::NativeFramebuffer&frame){
  if(!retainedActive)return true;
  if(!frame.fencedAtomicMipiRmw||!frame.pixels||retainedOwner>UINT64_MAX/8-1)return false;
  const uintptr_t physical=reinterpret_cast<uintptr_t>(frame.pixels);const uint64_t epoch=retainedReceipts[0].allocationEpoch;
  if(!epoch)return false;
  for(size_t i=0;i<retainedCount;++i)if(retainedReceipts[i].physicalBuffer!=physical||retainedReceipts[i].allocationEpoch!=epoch||!retainedReceipts[i].nextToken)return false;
  RetainedChartBuffer *target=nullptr;for(auto&b:retainedBuffers)if(b.physical==physical&&b.epoch==epoch)target=&b;
  if(!target)for(auto&b:retainedBuffers)if(!b.physical||b.epoch!=epoch){target=&b;b.physical=physical;b.epoch=epoch;for(auto&c:b.charts)c.token=0;break;}
  if(!target)return false;
  retainedExcludeCount=0;
  retainedSkippedBefore=canvas.retainedSkippedStores();retainedInkBefore=canvas.retainedInkStores();retainedRestoredBefore=canvas.retainedRestoredStores();
  for(size_t i=0;i<retainedCount;++i){auto&pixels=target->charts[i];retainedPixels[i]=&pixels;retainedPainted[i]=false;
    retainedHits[i]=retainedReceipts[i].retained&&retainedReceipts[i].previousToken&&pixels.token==retainedReceipts[i].previousToken&&pixels.key.same(retainedKeys[i]);
    pixels.mask={retainedRequests[i].rect,pixels.bits.get()};pixels.token=0;
    if(retainedHits[i]){retainedExclusions[retainedExcludeCount++]=retainedRequests[i].rect;++stats.retainedChartHits;}else ++stats.retainedChartCold;
  }
  return true;
}
void retainedCanvasExclusions(){canvas.setRetainedExclusions(retainedActive?retainedExclusions:nullptr,retainedActive?retainedExcludeCount:0);}
void retainedChartBackdropBegin(int index){const int at=retainedIndex(index);if(at<0)return;canvas.setRetainedExclusions(nullptr,0);canvas.setRetainedBackgroundMask(retainedHits[at]?&retainedPixels[at]->mask:nullptr);}
void retainedChartBackdropEnd(){canvas.setRetainedBackgroundMask(nullptr);}
void retainedChartInstrumentBegin(int index,bool prepared){const int at=retainedIndex(index);if(at<0)return;
  auto&pixels=*retainedPixels[at];memset(pixels.bits.get(),0,pixels.bytes);retainedPainted[at]=prepared;canvas.setRetainedInstrumentMask(prepared?&pixels.mask:nullptr);}
void retainedChartInstrumentEnd(){canvas.setRetainedInstrumentMask(nullptr);retainedCanvasExclusions();}
Status retainedCertify(nice_gfx::Backend&panel){if(!retainedActive)return Status::Ok;for(size_t i=0;i<retainedCount;++i)if(!retainedPainted[i])return Status::Unsupported;
  stats.retainedChartSkippedStores+=canvas.retainedSkippedStores()-retainedSkippedBefore;
  stats.retainedChartInkStores+=canvas.retainedInkStores()-retainedInkBefore;
  stats.retainedChartRestoredStores+=canvas.retainedRestoredStores()-retainedRestoredBefore;
  return panel.certifyRetainedRepaintFrame(retainedReceipts,retainedCount);}
void retainedCompleted(){if(retainedActive)for(size_t i=0;i<retainedCount;++i){retainedPixels[i]->key=retainedKeys[i];retainedPixels[i]->token=retainedReceipts[i].nextToken;}
  retainedForgetFrame();}
