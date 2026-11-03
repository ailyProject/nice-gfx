// Included inside View::Impl. Lifecycle state is sparse and allocated only at
// configure time; ticking performs no container growth or timer allocation.
struct BehaviorState {
  BehaviorDesc desc;
  uint16_t page=0;
  bool running=false,paused=false,pending=false,started=false,open=false,closing=false;
  uint32_t elapsed=0,accumulator=0,ticks=0,revision=0;
  Id savedFocus=0;
  int32_t originalValue=0;
  std::string originalText;
  Rect full{},visual{};
  uint16_t reveal=1024;
  menu_motion::Axis motionAxis;
  uint32_t motionElapsed=0;int32_t motionFrom=0;bool closeCommit=true;
};
std::vector<BehaviorState> behaviors;
struct BehaviorVisual {int root=-1;BehaviorKind kind=BehaviorKind::Toast;uint8_t dim=0;Rect full{},visual{};uint16_t reveal=1024;DialogMotion motion=DialogMotion::None;};
std::vector<BehaviorVisual> behaviorVisuals;
View *behaviorOwner=nullptr;
uint32_t behaviorClock=0;
bool behaviorClockReady=false,behaviorRouting=false;
uint16_t behaviorPage=NoParent;
uint16_t navigationLimit=64;

bool navigationFeedback(const char *message) {
  if(!behaviorOwner)return false;
  for(auto &b:behaviors)if(b.page==history.back()&&b.desc.kind==BehaviorKind::Toast&&(b.desc.flags&BehaviorSystemFeedback)){
    if(b.desc.target)behaviorOwner->setText(b.desc.target,message);
    const bool nested=processing;processing=true;const bool result=startBehaviorState(b);if(!nested)endProcessing();return result;
  }
  return false;
}

BehaviorState *behavior(Id id) {
  auto at=std::lower_bound(behaviors.begin(),behaviors.end(),id,[](const BehaviorState &a,Id key){return a.desc.id<key;});
  return at!=behaviors.end()&&at->desc.id==id?&*at:nullptr;
}
bool behaviorPanel(const BehaviorState &b) const {
  return b.desc.kind==BehaviorKind::Toast||b.desc.kind==BehaviorKind::Modal||b.desc.kind==BehaviorKind::Editor;
}
bool behaviorModal(const BehaviorState &b) const {return b.desc.kind==BehaviorKind::Modal||b.desc.kind==BehaviorKind::Editor;}
bool behaviorKeepDraft(int target) const {for(const auto &b:behaviors)if(b.page==history.back()&&b.open&&b.running&&b.desc.kind==BehaviorKind::Editor&&valueTarget(b.desc.target)==target)return true;return false;}
bool behaviorInputBlocked() const {for(const auto &b:behaviors)if(b.page==history.back()&&b.open&&b.closing&&behaviorModal(b))return true;return false;}
Rect behaviorFocusRect(int node) const {
  Rect result=rect(node);
  for(const auto &b:behaviors)if(b.page==history.back()&&b.open&&b.desc.motion==DialogMotion::Spring){
    const int root=find(b.desc.id);if(root>=0&&descendant(node,root)){result.x-=b.visual.x-b.full.x;result.y-=b.visual.y-b.full.y;}
  }
  return result;
}
bool behaviorAnimated(const BehaviorState &b) const {return b.desc.motion!=DialogMotion::None&&!displayPolicy.slowRefresh&&menuMotionConfig.preset!=MenuPreset::Reduced;}
void cancelUnavailableEditors() {
  if(!project)return;const bool nested=processing;processing=true;
  for(auto &b:behaviors)if(b.page==history.back()&&b.open&&b.running&&b.desc.kind==BehaviorKind::Editor){
    const int root=find(b.desc.id),target=valueTarget(b.desc.target);
    if(root<0||target<0||!visible(page(),state(),root,true)||!visible(page(),state(),target,true)){
      finishBehaviorState(b,false);if(b.closing)completeBehaviorClose(b);
    }
  }
  if(!nested)endProcessing();
}
void prepareBehaviorDeparture() {
  const bool nested=processing;processing=true;
  for(auto &b:behaviors)if(b.page==history.back()&&b.open&&b.desc.kind==BehaviorKind::Editor){
    if(b.running)finishBehaviorState(b,false);
    if(b.closing)completeBehaviorClose(b);
  }
  if(!nested)endProcessing();
}
void updateBehaviorVisual(BehaviorState &b) {
  if(b.desc.motion==DialogMotion::Spring){
    const int32_t progress=b.motionAxis.pixels();const int64_t distance=b.desc.kind==BehaviorKind::Toast?-int64_t(b.full.y)-b.full.height:int64_t(page().height)-b.full.y;
    const int32_t offset=int32_t(distance*(1024-progress)/1024);const int node=find(b.desc.id);
    if(node>=0&&state()[node].animationY!=offset){invalidateSubtree(node);state()[node].animationY=offset;invalidateSubtree(node);}
    b.reveal=1024;b.visual=b.full;b.visual.y+=offset;return;
  }
  const int32_t progress=clamp32(b.motionAxis.pixels(),0,1024);b.reveal=uint16_t(progress);
  const int32_t height=int32_t(int64_t(b.full.height)*progress/1024);b.visual={b.full.x,b.full.y,b.full.width,height};
}
void completeBehaviorClose(BehaviorState &b) {
  const int node=find(b.desc.id);b.open=b.closing=false;b.reveal=0;b.motionAxis.snap(0);
  if(node>=0&&b.desc.motion==DialogMotion::Spring)state()[node].animationY=0;
  if(node>=0){requestHide(node);behaviorEmit(b,EventType::Hidden,b.closeCommit?1:0);}
  if(behaviorModal(b)&&b.savedFocus){const int saved=focusTarget(b.savedFocus);if(saved>=0&&focusable(saved,false))setFocus(saved);}
  refresh->invalidateAll();
}
bool behaviorEmit(BehaviorState &b,EventType type,int32_t value) {
  const int node=find(b.desc.id);if(node<0)return false;
  const int32_t original=state()[node].value;state()[node].value=value;
  dispatch(node,type,value);
  // Ordinary container values carry task progress. Modal values remain authors'.
  if(behaviorPanel(b))state()[node].value=original;
  return true;
}
bool finishBehaviorState(BehaviorState &b,bool commit) {
  if(!b.running&&!b.open)return true;
  if(b.closing)return true;
  const Id id=b.desc.id;const uint32_t revision=++b.revision;
  if(b.desc.kind==BehaviorKind::Editor&&editNode>=0&&valueTarget(b.desc.target)==editNode)editNode=-1;
  b.running=b.paused=b.pending=false;
  if(b.desc.kind==BehaviorKind::Editor&&behaviorOwner){
    const int target=valueTarget(b.desc.target);
    if(!commit&&target>=0){
      behaviorOwner->setValue(b.desc.target,b.originalValue);
      if(page().nodes[target].kind==Kind::InputField)behaviorOwner->setText(b.desc.target,b.originalText.c_str());
      dispatch(target,EventType::Input);
    }
    if(commit&&target>=0)dispatch(target,EventType::Change);
    behaviorEmit(b,commit?EventType::EditCommit:EventType::EditCancel,commit?1:0);
  }else if(b.desc.kind==BehaviorKind::Task)behaviorEmit(b,EventType::TaskEnd,commit?1:0);
  if(b.revision!=revision)return true;
  if(behaviorPanel(b)){
    b.closeCommit=commit;
    if(behaviorAnimated(b)&&b.motionAxis.position!=0){b.closing=true;b.motionFrom=b.motionAxis.pixels();b.motionElapsed=0;b.motionAxis.aim(0);refresh->invalidateAll();}
    else completeBehaviorClose(b);
  }
  return true;
}
bool startBehaviorState(BehaviorState &b) {
  if(!project||b.page!=history.back())return false;
  const int node=find(b.desc.id);if(node<0)return false;
  if(b.open&&!b.closing&&behaviorModal(b))return true;
  const bool reopening=b.open;
  const bool routing=behaviorRouting;behaviorRouting=true;
  struct RoutingGuard {bool &flag;bool previous;~RoutingGuard(){flag=previous;}} routingGuard{behaviorRouting,routing};
  b.elapsed=b.accumulator=b.ticks=0;b.paused=false;b.pending=true;b.running=true;b.started=true;++b.revision;
  if(behaviorPanel(b)){
    if(!reopening)b.savedFocus=focusNode>=0?page().nodes[focusNode].id:0;
    if(b.desc.kind==BehaviorKind::Editor&&behaviorOwner){
      b.originalValue=behaviorOwner->value(b.desc.target);
      const char *text=behaviorOwner->text(b.desc.target);b.originalText=text?text:"";
    }
    if(!reopening)b.full=rect(node);b.open=true;b.closing=false;b.motionElapsed=0;
    if(!reopening)b.motionAxis.snap(behaviorAnimated(b)?0:1024);
    b.motionFrom=b.motionAxis.pixels();b.motionAxis.aim(1024);
    if(!behaviorAnimated(b))b.motionAxis.snap(1024);
    requestShow(node);
    updateBehaviorVisual(b);
    const Id wanted=b.desc.initialFocus?b.desc.initialFocus:b.desc.target;
    int focus=wanted?focusTarget(wanted):-1;
    if(behaviorModal(b)&&focus<0)for(uint16_t i=0;i<page().count;++i)if(descendant(i,node)&&focusable(i,false)){focus=i;break;}
    if(focus>=0)setFocus(focus);
    behaviorEmit(b,EventType::Shown,1);
    if(b.desc.kind==BehaviorKind::Editor)behaviorEmit(b,EventType::EditStart,b.originalValue);
    refresh->invalidateAll();
  }else if(b.desc.kind==BehaviorKind::Task)behaviorEmit(b,EventType::TaskStart,0);
  return true;
}
bool dismissInputToasts() {
  bool consumed=false;const bool nested=processing;processing=true;
  for(auto &b:behaviors)if(b.page==history.back()&&b.open&&(b.desc.flags&BehaviorDismissOnInput)){
    if(b.desc.kind==BehaviorKind::Toast)finishBehaviorState(b,true);
    else if(b.desc.kind==BehaviorKind::Modal){consumed=true;finishBehaviorState(b,true);}
  }
  if(!nested)endProcessing();return consumed;
}
bool dismissBehaviorModal() {
  int selected=-1;
  for(size_t n=0;n<behaviors.size();++n)if(behaviors[n].page==history.back()&&behaviors[n].open&&behaviorModal(behaviors[n])){
    const int node=find(behaviors[n].desc.id);if(node>selected)selected=node;
  }
  if(selected<0)return false;
  auto *b=behavior(page().nodes[selected].id);return b&&finishBehaviorState(*b,false);
}
bool behaviorShow(int node) {
  if(behaviorRouting||node<0)return false;
  auto *b=behavior(page().nodes[node].id);
  if(!b||!behaviorPanel(*b))return false;
  behaviorRouting=true;const bool result=startBehaviorState(*b);behaviorRouting=false;return result;
}
bool behaviorHide(int node) {
  if(behaviorRouting||node<0)return false;
  auto *b=behavior(page().nodes[node].id);
  if(!b||!behaviorPanel(*b)||!b->open)return false;
  behaviorRouting=true;const bool result=finishBehaviorState(*b,b->desc.kind!=BehaviorKind::Editor);behaviorRouting=false;return result;
}
bool behaviorEditInput(int target,EventType type) {
  if(type!=EventType::Input&&type!=EventType::Change)return false;
  for(const auto &b:behaviors)if(b.running&&b.open&&b.desc.kind==BehaviorKind::Editor&&valueTarget(b.desc.target)==target&&(type==EventType::Change||!(b.desc.flags&BehaviorLiveEdit)))return true;
  return false;
}
void resetPageBehaviors() {
  const uint16_t next=history.back();
  if(behaviorPage==next)return;
  for(auto &b:behaviors){
    if(b.page==behaviorPage){
      if(behaviorPanel(b))for(uint16_t node=0;node<project->pages[b.page].count;++node)if(project->pages[b.page].nodes[node].id==b.desc.id){pages[b.page][node].flags|=Hidden;if(b.desc.motion==DialogMotion::Spring)pages[b.page][node].animationY=0;}
      if(b.open&&b.running&&b.desc.kind==BehaviorKind::Editor)for(uint16_t node=0;node<project->pages[b.page].count;++node)if(project->pages[b.page].nodes[node].id==b.desc.target){pages[b.page][node].value=b.originalValue;pages[b.page][node].text.set(b.originalText.c_str());}
      b.running=b.paused=b.pending=b.open=b.started=b.closing=false;++b.revision;
    }
    if(b.page==next&&!b.started&&(b.desc.flags&BehaviorAutoStart)){b.pending=true;b.running=true;}
  }
  behaviorPage=next;
}
void advanceBehaviors(uint32_t now) {
  resetPageBehaviors();
  cancelUnavailableEditors();
  const uint32_t dt=behaviorClockReady?uint32_t(now-behaviorClock):0;behaviorClock=now;behaviorClockReady=true;
  for(size_t n=0;n<behaviors.size();++n){
    auto &b=behaviors[n];if(b.page!=history.back()||(!b.running&&!b.closing)||b.paused)continue;
    if(b.pending){if(!b.started)startBehaviorState(b);b.pending=false;b.started=true;continue;}
    const uint32_t elapsed=dt>0x7fffffffu?0:dt;
    b.elapsed=uint32_t(std::min<uint64_t>(uint64_t(b.elapsed)+elapsed,0x7fffffffu));
    const uint32_t revision=b.revision;
    if(b.open&&behaviorAnimated(b)){
      const int32_t previous=b.motionAxis.pixels();
      if(b.desc.motion==DialogMotion::Circular){const uint32_t motionDt=menuMotionConfigured?uint32_t(double(std::min<uint32_t>(elapsed,menuMotionConfig.maxDeltaMs))*menuMotionConfig.speed):elapsed;b.motionElapsed=uint32_t(std::min<uint64_t>(b.desc.motionMs,uint64_t(b.motionElapsed)+motionDt));const double q=std::min(1.0,double(b.motionElapsed)/b.desc.motionMs);const double p=q<0.5?(1.0-std::sqrt(std::max(0.0,1.0-4*q*q)))/2.0:(std::sqrt(std::max(0.0,1.0-std::pow(-2*q+2,2)))+1)/2.0;const int32_t target=b.closing?0:1024;const int32_t next=b.motionFrom+int32_t((target-b.motionFrom)*p);if(q>=1)b.motionAxis.snap(target);else {b.motionAxis.position=int64_t(next)*256;b.motionAxis.target=int64_t(target)*256;}}
      else {MenuMotionConfig config=menuMotionConfig;config.mode=MenuMotionMode::Spring;config.responseMs=b.desc.motionMs;b.motionAxis.step(elapsed,config,config.overlay);}
      if(previous!=b.motionAxis.pixels()){updateBehaviorVisual(b);refresh->invalidateAll();}
      if(b.closing&&!b.motionAxis.active()){completeBehaviorClose(b);continue;}
    }
    else if(b.open&&b.motionAxis.active()){b.motionAxis.snap(b.closing?0:1024);updateBehaviorVisual(b);refresh->invalidateAll();if(b.closing){completeBehaviorClose(b);continue;}}
    if(b.closing)continue;
    if(b.desc.kind==BehaviorKind::Toast&&b.desc.durationMs&&b.elapsed>=b.desc.durationMs){finishBehaviorState(b,true);continue;}
    if(b.desc.kind==BehaviorKind::Timer||b.desc.kind==BehaviorKind::Task){
      const uint32_t interval=std::max<uint32_t>(1,b.desc.intervalMs);
      b.accumulator=uint32_t(std::min<uint64_t>(uint64_t(b.accumulator)+elapsed,0x7fffffffu));
      if(b.accumulator>=interval){const uint32_t count=b.accumulator/interval;b.accumulator%=interval;b.ticks=uint32_t(std::min<uint64_t>(uint64_t(b.ticks)+count,0x7fffffffu));
        const int32_t value=b.desc.kind==BehaviorKind::Timer?int32_t(b.ticks):b.desc.durationMs?int32_t(std::min<uint64_t>(100,uint64_t(b.elapsed)*100/b.desc.durationMs)):int32_t(b.ticks);
        behaviorEmit(b,b.desc.kind==BehaviorKind::Timer?EventType::Timer:EventType::TaskTick,value);
      }
      if(b.revision!=revision)continue;
      if(b.desc.durationMs&&b.elapsed>=b.desc.durationMs){
        if(b.desc.flags&BehaviorRepeat){b.elapsed%=b.desc.durationMs;behaviorEmit(b,EventType::TaskEnd,1);if(b.revision==revision)behaviorEmit(b,EventType::TaskStart,0);}
        else finishBehaviorState(b,true);
      }
    }
  }
}
bool behaviorMoving() const {for(const auto &b:behaviors)if(b.page==history.back()&&!b.paused&&b.open&&(b.closing||b.motionAxis.active()))return true;return false;}
bool behaviorVisualMoving()const{for(const auto &b:behaviorVisuals)if(b.reveal<1024||b.visual.x!=b.full.x||b.visual.y!=b.full.y)return true;return false;}
void snapshotBehaviors() {
  behaviorVisuals.clear();
  for(const auto &b:behaviors)if(b.page==history.back()&&b.open){const int root=find(b.desc.id);if(root>=0)behaviorVisuals.push_back({root,b.desc.kind,b.desc.dim,b.full,b.visual,b.reveal,b.desc.motion});}
}
bool behaviorVisualModal(const BehaviorVisual &b)const{return b.kind==BehaviorKind::Modal||b.kind==BehaviorKind::Editor;}
bool behaviorSnapshotDescendant(int node,int root)const{for(int i=node;i>=0;i=rasterFrame.current.values[i].parent==NoParent?-1:rasterFrame.current.values[i].parent)if(i==root)return true;return false;}
Rect behaviorClip(int node,Rect clipRect) const {
  for(const auto &b:behaviorVisuals)if(b.reveal<1024&&behaviorSnapshotDescendant(node,b.root)){if(b.motion==DialogMotion::Circular&&node!=b.root)return {0,0,0,0};clipRect=intersect(clipRect,b.visual);}
  return clipRect;
}
Rect behaviorPaintRect(int node,Rect authored) const {
  for(const auto &b:behaviorVisuals)if(b.root==node&&b.reveal<1024)return b.visual;
  return authored;
}
void paintBehaviorFrames(const DirtyRect &area) {
  if(snapshotTransition)return;canvas.setViewport(area.x,area.y,area.width,area.height,false);
  for(const auto &b:behaviorVisuals)if(b.motion==DialogMotion::Circular){const Rect r=b.full;const int32_t w=int32_t(int64_t(r.width)*b.reveal/1024),h=int32_t(int64_t(r.height)*b.reveal/1024);const uint16_t ink=DisplayPolicy::foregroundOn(styleFor(*rasterFrame.current.page,rasterFrame.current.values,b.root).background);
    if(w){canvas.drawFastHLine(r.x,r.y,w,ink);canvas.drawFastHLine(r.x+r.width-w,r.y+r.height-1,w,ink);}if(h){canvas.drawFastVLine(r.x,r.y,h,ink);canvas.drawFastVLine(r.x+r.width-1,r.y+r.height-h,h,ink);}}
}
void paintBehaviorBackdrop(Rect area,uint16_t *pixels,size_t stride) {
  // Dialog panels are opaque authored nodes. Only the surviving backdrop
  // outside the frontmost panel needs dimming; raw retained surfaces stay clean.
  if(snapshotTransition)return;
  const BehaviorVisual *front=nullptr;int frontNode=-1;
  for(const auto &b:behaviorVisuals)if(behaviorVisualModal(b)&&b.dim&&b.root>frontNode){front=&b;frontNode=b.root;}
  if(front){const auto &b=*front;const Rect box=b.visual;
    for(int32_t y=0;y<area.height;++y)for(int32_t x=0;x<area.width;++x){
      const int32_t px=area.x+x,py=area.y+y;
      if(contains(box,px,py))continue;
      bool toast=false;for(const auto &overlay:behaviorVisuals)if(overlay.kind==BehaviorKind::Toast&&contains(overlay.visual,px,py)){toast=true;break;}if(toast)continue;
      uint16_t &p=pixels[size_t(y)*stride+x];const uint16_t c=renderOrder==PixelOrder::Wire?rgb565_blend::fromWire(p):p;
      const uint16_t a=uint16_t(255-b.dim);const uint16_t color=uint16_t((((c>>11)*a/255)<<11)|((((c>>5)&63)*a/255)<<5)|((c&31)*a/255));p=renderOrder==PixelOrder::Wire?rgb565_blend::toWire(color):color;
    }
  }
}
