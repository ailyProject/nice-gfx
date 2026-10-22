// Included inside View::Impl, after State. Animated nodes alone own a slot.
// Pose values are copied into each immutable raster snapshot; clocks and
// completion epochs remain live and never share mutable heap objects.
struct MotionRuntime {
  uint16_t scale=1024,reveal=1024;
  int16_t rotation=0;
  uint16_t node=NoParent;
  uint32_t epoch=0,presented=0;
  int32_t baseX=0,baseY=0;
  uint8_t baseOpacity=255;
  bool ending=false,hiding=false;
};
std::vector<std::vector<MotionRuntime>> pageMotions;
bool motionCompletionPending=false;

const MotionRuntime &motionPose(const PageDesc &p,const RetainedVector<State> &values,int i) const {
  static const MotionRuntime neutral;
  if(i<0||size_t(i)>=values.size()||values[i].motionSlot==NoParent)return neutral;
  const std::vector<MotionRuntime> *poses=nullptr;
  if(&values==&rasterFrame.current.values)poses=&rasterFrame.current.motions;
  else if(&values==&rasterFrame.preview.values)poses=&rasterFrame.preview.motions;
  else if(&values==&transition.outgoing)poses=&transition.outgoingMotions;
  else if(project && &p>=project->pages && &p<project->pages+project->count)
    poses=&pageMotions[size_t(&p-project->pages)];
  const size_t slot=values[i].motionSlot;
  return poses&&slot<poses->size()?(*poses)[slot]:neutral;
}
MotionRuntime *liveMotion(int i) {
  if(i<0||size_t(i)>=state().size()||state()[i].motionSlot==NoParent)return nullptr;
  auto &poses=pageMotions[history.back()];const size_t slot=state()[i].motionSlot;
  return slot<poses.size()?&poses[slot]:nullptr;
}
void initializeMotionPage(uint16_t index) {
  const auto &p=project->pages[index];auto &values=pages[index];auto &poses=pageMotions[index];
  size_t count=0;
  for(uint16_t i=0;i<p.count;++i)if(p.nodes[i].motion||p.nodes[i].animation.effect!=AnimationEffect::None||p.nodes[i].exitAnimation)++count;
  poses.resize(count);size_t slot=0;
  for(uint16_t i=0;i<p.count;++i){const auto &n=p.nodes[i];
    if(n.motion||n.animation.effect!=AnimationEffect::None||n.exitAnimation){values[i].motionSlot=uint16_t(slot);poses[slot++].node=i;}
  }
  // Reserve both raster sides and the departure copy before any clock starts.
  rasterFrame.current.motions.reserve(std::max(rasterFrame.current.motions.capacity(),count));
  rasterFrame.preview.motions.reserve(std::max(rasterFrame.preview.motions.capacity(),count));
  transition.outgoingMotions.reserve(std::max(transition.outgoingMotions.capacity(),count));
  if(count){
    size_t depth=0;
    for(const auto &pose:poses){size_t levels=1;for(uint16_t a=p.nodes[pose.node].parent;a!=NoParent;a=p.nodes[a].parent)++levels;depth=std::max(depth,levels);}
    if(eventPaths.empty())eventPaths.emplace_back();
    eventPaths.front().reserve(std::max(eventPaths.front().capacity(),depth));
  }
}
void resetMotion(int i,bool restorePose=true) {
  auto *m=liveMotion(i);if(!m)return;
  const bool wasEnding=m->ending;
  if(!++m->epoch)++m->epoch;
  m->presented=0;m->ending=m->hiding=false;
  if(restorePose){
    if(m->scale!=1024||m->reveal!=1024||m->rotation)++state()[i].motionRevision;
    m->scale=m->reveal=1024;m->rotation=0;
  }
  if(wasEnding)refreshMotionPending();
}
void refreshMotionPending() {
  motionCompletionPending=false;
  if(project)for(const auto &pending:pageMotions[history.back()])if(pending.ending){motionCompletionPending=true;break;}
}
void cancelVisibilityMotions(int root,bool includeRoot=true) {
  // Immediate visibility keeps legacy main clocks/poses intact. A departure
  // hidden by an ancestor is canceled, and an unpresented End cannot later
  // fire from a frame that never displayed that node.
  for(auto &m:pageMotions[history.back()]){
    const uint16_t node=m.node;if(node==root&&!includeRoot)continue;
    int a=node;while(a>=0&&a!=root)a=parent(a);if(a!=root)continue;
    if(m.hiding)state()[node].animating=state()[node].animationPending=false;
    if(!++m.epoch)++m.epoch;
    m.presented=0;m.ending=m.hiding=false;
  }
  refreshMotionPending();
}
void finishMotionSample(int i) {
  auto *m=liveMotion(i);if(!m||m->ending)return;
  m->ending=true;m->presented=0;motionCompletionPending=true;
  // A terminal pose with no changed pixels still needs a fence if an older
  // frame is being transferred. Otherwise the displayed pose is already exact.
  if(!refresh->dirty()){
    if(!refresh->busy())m->presented=m->epoch;
    else refresh->invalidate(DirtyRect(page().width/2,page().height/2,1,1));
  }
}
bool motionsPending() const {
  return motionCompletionPending;
}
void completeMotionFrame() {
  if(!project||rasterFrame.current.page!=&page())return;
  auto &poses=pageMotions[history.back()];
  for(size_t i=0;i<rasterFrame.current.motions.size()&&i<poses.size();++i){const auto &shown=rasterFrame.current.motions[i];auto &live=poses[i];
    if(shown.ending&&shown.epoch==live.epoch&&live.ending)live.presented=shown.epoch;
  }
}
bool hasAnimationEndListener(int index) const {
  if(listeners->empty())return false;
  for(int i=index;i>=0;i=parent(i)){
    const Listener key{page().nodes[i].id,EventType::AnimationEnd,nullptr,nullptr,false};
    const auto at=std::lower_bound(listeners->begin(),listeners->end(),key,listenerBefore);
    if(at!=listeners->end()&&at->id==key.id&&at->type==key.type)return true;
    const Listener capture{key.id,key.type,nullptr,nullptr,true};
    const auto cat=std::lower_bound(listeners->begin(),listeners->end(),capture,listenerBefore);
    if(cat!=listeners->end()&&cat->id==key.id&&cat->type==key.type)return true;
  }
  return false;
}
void settleMotions() {
  if(!project||processing||!motionCompletionPending)return;
  const bool nested=processing;processing=true;
  motionCompletionPending=false;
  // Callbacks may restart another node. Inspect its epoch and completion flag
  // afresh rather than retaining pointers or an unbounded callback queue.
  for(size_t slot=0;slot<pageMotions[history.back()].size();++slot){auto *m=&pageMotions[history.back()][slot];
    if(!m->ending)continue;
    if(m->presented!=m->epoch){motionCompletionPending=true;continue;}
    const uint16_t i=m->node;
    const uint32_t completedEpoch=m->epoch;
    const bool hide=m->hiding;m->ending=m->hiding=false;
    if(hide)commitVisibility(i,false);
    // Focus and PointerCancel notifications from Hidden can restart this node.
    // Such a new run supersedes the old completion just like explicit stop.
    m=liveMotion(i);
    if(m&&m->epoch==completedEpoch&&hasAnimationEndListener(i))dispatch(i,EventType::AnimationEnd,hide?1:0);
  }
  if(!nested)endProcessing();
}
bool requestShow(int i,bool cancelPointer=true) {
  if(i<0)return false;
  invalidateSubtree(i);auto *m=liveMotion(i);
  if(m)resetMotion(i);
  auto &v=state()[i];v.animating=v.animationPending=false;v.animationX=v.animationY=0;v.opacity=255;
  const uint32_t showEpoch=m?m->epoch:0;
  commitVisibility(i,true,cancelPointer);
  const auto trigger=page().nodes[i].animation.trigger;
  m=liveMotion(i);
  if(!(v.flags&Hidden)&&(!m||m->epoch==showEpoch)&&
     (trigger==AnimationTrigger::Show||trigger==AnimationTrigger::Enter))startAnimation(i,0,true);
  invalidateSubtree(i);
  return true;
}
bool requestHide(int i,bool cancelPointer=true) {
  if(i<0)return false;
  if(state()[i].flags&Hidden)return true;
  const auto *a=page().nodes[i].exitAnimation;auto *m=liveMotion(i);
  if(!a||a->effect==AnimationEffect::None||!m){
    resetMotion(i,false);state()[i].animating=state()[i].animationPending=false;
    commitVisibility(i,false,cancelPointer);return true;
  }
  if(m->hiding)return true;
  resetMotion(i,false);m=liveMotion(i);
  auto &v=state()[i];m->baseX=v.animationX;m->baseY=v.animationY;m->baseOpacity=v.opacity;m->hiding=true;
  v.animationAt=0;v.animationPending=true;v.animating=true;
  return true;
}
void commitVisibility(int i,bool shown,bool cancelPointer=true) {
  if(i<0||bool(!(state()[i].flags&Hidden))==shown)return;
  ++inputLayoutEpoch;invalidateSubtree(i);
  if(shown)state()[i].flags&=~Hidden;else {state()[i].flags|=Hidden;cancelVisibilityMotions(i,false);}
  invalidateSubtree(i);
  if(cancelPointer&&down&&!pointerAvailable()){releaseKeyboardPointer();cancel(true);}
  ensureInputAvailable();
  const auto *link=componentFor(i);if(link&&link->panel==i)expansionFocus(*link,shown);
  if(buildKeyboard(softInput))invalidateKeyboardKeys();
}
