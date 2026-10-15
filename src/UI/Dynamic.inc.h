// Included by GFX_UI.cpp after View::Impl, inside nice_gfx::htmlui.
// These APIs update retained native state; they never parse HTML or CSS.
namespace {
bool sameDynamicRect(Rect a,Rect b) {
  return a.x==b.x&&a.y==b.y&&a.width==b.width&&a.height==b.height;
}
bool validDynamicRect(Rect r) {
  return r.width>=0&&r.height>=0&&r.x>=-32768&&r.y>=-32768&&
    int64_t(r.x)+r.width<=32767&&int64_t(r.y)+r.height<=32767;
}
bool sameDynamicStyle(const Style &a,const Style &b) {
  return a.background==b.background&&a.color==b.color&&a.borderColor==b.borderColor&&
    a.trackColor==b.trackColor&&a.fillColor==b.fillColor&&a.thumbColor==b.thumbColor&&
    a.pressedColor==b.pressedColor&&a.disabledColor==b.disabledColor&&
    a.borderWidth==b.borderWidth&&a.radius==b.radius&&a.fontSize==b.fontSize&&a.textAlign==b.textAlign&&
    a.paddingTop==b.paddingTop&&a.paddingRight==b.paddingRight&&a.paddingBottom==b.paddingBottom&&a.paddingLeft==b.paddingLeft&&
    a.trackSize==b.trackSize&&a.thumbWidth==b.thumbWidth&&a.thumbHeight==b.thumbHeight&&a.thumbRadius==b.thumbRadius&&
    a.font==b.font&&a.gradient==b.gradient;
}
template<class Runtime> bool dynamicDescendant(const Runtime &s,int child,int ancestor) {
  for(int node=child;node>=0;node=s.parent(node))if(node==ancestor)return true;
  return false;
}
template<class Runtime> bool validDynamicBounds(const Runtime &s,int index,Rect r) {
  if(!validDynamicRect(r))return false;
  const auto &page=s.page();const auto &state=s.state();const Rect old=s.layoutRect(page,state,index);
  // A pager owns an authored page stride. Runtime resizing would also require
  // rebuilding every direct page's geometry, so this API only moves pagers.
  if(page.nodes[index].kind==Kind::Pager&&(r.width!=old.width||r.height!=old.height))return false;
  const int parent=s.parent(index);
  if(parent>=0&&page.nodes[parent].kind==Kind::Pager&&!sameDynamicRect(r,old))return false;
  if(!validGradient(s.styleFor(page,state,index).gradient,r))return false;
  const int64_t dx=int64_t(r.x)-old.x,dy=int64_t(r.y)-old.y;
  for(uint16_t child=uint16_t(index+1);child<page.count;++child)if(dynamicDescendant(s,child,index)) {
    const Rect next=s.layoutRect(page,state,child);
    if(int64_t(next.x)+dx < -32768||int64_t(next.y)+dy < -32768||
       int64_t(next.x)+dx+next.width>32767||int64_t(next.y)+dy+next.height>32767)return false;
  }
  return true;
}
template<class Runtime> bool validDynamicStyle(const Runtime &s,int index,const Style &style) {
  const auto &page=s.page();const auto &node=page.nodes[index];const Rect rect=s.layoutRect(page,s.state(),index);
  if(style.fontSize>7||style.textAlign>2||!validGradient(style.gradient,rect))return false;
  if(hasText(node.kind)&&(!style.font||!style.fontSize))return false;
  if(style.font&&style.font!=s.styleFor(page,s.state(),index).font&&!validFont(*style.font))return false;
  if(node.kind==Kind::Pager&&!sameDynamicRect(contentBox(rect,style),contentBox(rect,s.styleFor(page,s.state(),index))))return false;
  return true;
}
template<class Runtime> std::shared_ptr<DynamicNode> nextDynamicNode(const Runtime &s,int index) {
  const auto &current=s.state()[index].dynamic;
  return current?std::make_shared<DynamicNode>(*current):std::make_shared<DynamicNode>();
}
template<class Runtime> void finishDynamicMutation(View &view,Runtime &s,int index) {
  // Extent changes can clamp an ancestor scroll even when the edited child is
  // currently off screen. Unrelated gestures retain their current position.
  bool cancelPointer=s.backAnimation.active;
  for(int candidate:{s.target,s.activeControl,s.owner})if(candidate>=0&&dynamicDescendant(s,candidate,index))cancelPointer=true;
  for(uint16_t i=0;i<s.page().count;++i) {
    if(s.page().nodes[i].kind!=Kind::Scroll||(!dynamicDescendant(s,i,index)&&!dynamicDescendant(s,index,i)))continue;
    const int32_t oldX=s.state()[i].scrollX,oldY=s.state()[i].scrollY;
    s.scroll(i,s.state()[i].scrollX,s.state()[i].scrollY,false);
    if(oldX!=s.state()[i].scrollX||oldY!=s.state()[i].scrollY) {
      if(s.animation.node==i)s.animation.active=false;if(s.inertia.node==i)s.inertia.active=false;
      for(int candidate:{s.target,s.activeControl,s.owner})if(candidate>=0&&dynamicDescendant(s,candidate,i))cancelPointer=true;
    }
  }
  s.invalidateSubtree(index);
  // Geometry/style may invalidate grab offsets, drag ownership or input hits.
  // Cancel after committing so callbacks see the new state; processing defers
  // navigation until this whole mutation has completed.
  if(cancelPointer&&(s.down||s.backAnimation.active))view.pointerCancel();
}
template<class Runtime> void commitDynamicNode(View &view,Runtime &s,int index,const std::shared_ptr<DynamicNode> &next) {
  const bool nested=s.processing;s.processing=true;
  s.invalidateSubtree(index);
  s.state()[index].dynamic=next->boundsOverride||next->styleOverride?next:std::shared_ptr<const DynamicNode>();
  s.dynamicChanged(index);finishDynamicMutation(view,s,index);
  if(!nested)s.endProcessing();
}
}

Rect View::layoutBounds(Id id) const {
  const auto &s=*impl_;const int index=s.find(id);
  return index<0?Rect{0,0,0,0}:s.layoutRect(s.page(),s.state(),index);
}
Style View::style(Id id) const {
  const auto &s=*impl_;const int index=s.find(id);
  return index<0?Style{}:s.styleFor(s.page(),s.state(),index);
}
bool View::setBounds(Id id,Rect rect) {
  auto &s=*impl_;const int index=s.find(id);if(index<0||!validDynamicBounds(s,index,rect))return false;
  const auto &node=s.page().nodes[index];const Rect current=s.layoutRect(s.page(),s.state(),index);
  if(sameDynamicRect(current,rect))return true;
  auto next=nextDynamicNode(s,index);
  next->offsetX+=(rect.x-current.x);next->offsetY+=(rect.y-current.y);
  next->width=rect.width;next->height=rect.height;
  next->boundsOverride=next->offsetX||next->offsetY||rect.width!=node.rect.width||rect.height!=node.rect.height;
  commitDynamicNode(*this,s,index,next);return true;
}
bool View::resetBounds(Id id) {
  auto &s=*impl_;const int index=s.find(id);if(index<0)return false;
  const auto &current=s.state()[index].dynamic;if(!current||!current->boundsOverride)return true;
  Rect restored=s.layoutRect(s.page(),s.state(),index);restored.x-=current->offsetX;restored.y-=current->offsetY;
  restored.width=s.page().nodes[index].rect.width;restored.height=s.page().nodes[index].rect.height;
  if(!validDynamicBounds(s,index,restored))return false;
  auto next=nextDynamicNode(s,index);next->boundsOverride=false;next->offsetX=next->offsetY=0;
  commitDynamicNode(*this,s,index,next);return true;
}
bool View::setStyle(Id id,const Style &style) {
  auto &s=*impl_;const int index=s.find(id);if(index<0||!validDynamicStyle(s,index,style))return false;
  if(sameDynamicStyle(s.styleFor(s.page(),s.state(),index),style))return true;
  auto next=nextDynamicNode(s,index);next->style=style;next->styleOverride=!sameDynamicStyle(s.page().nodes[index].style,style);
  commitDynamicNode(*this,s,index,next);return true;
}
bool View::resetStyle(Id id) {
  auto &s=*impl_;const int index=s.find(id);if(index<0)return false;
  const auto &current=s.state()[index].dynamic;if(!current||!current->styleOverride)return true;
  const Style &restored=s.page().nodes[index].style;if(!validDynamicStyle(s,index,restored))return false;
  auto next=nextDynamicNode(s,index);next->styleOverride=false;commitDynamicNode(*this,s,index,next);return true;
}
bool View::beginUpdate() { if(impl_->updateDepth==UINT16_MAX)return false;++impl_->updateDepth;return true; }
bool View::endUpdate() { if(!impl_->updateDepth)return false;--impl_->updateDepth;return true; }

bool View::layoutChildren(Id id,LayoutAxis axis,int32_t gap,bool resizeContainer) {
  auto &s=*impl_;const int index=s.find(id);
  if(index<0||gap<0||(axis!=LayoutAxis::Horizontal&&axis!=LayoutAxis::Vertical))return false;
  const auto &page=s.page();const Kind kind=page.nodes[index].kind;
  if(kind!=Kind::Container&&kind!=Kind::Scroll)return false;
  const bool horizontal=axis==LayoutAxis::Horizontal;
  const Rect parent=s.layoutRect(page,s.state(),index);const Style skin=s.styleFor(page,s.state(),index);const Rect content=contentBox(parent,skin);
  std::vector<std::pair<int,std::shared_ptr<DynamicNode>>> changes;size_t visibleCount=0;int64_t cursor=horizontal?content.x:content.y;
  for(uint16_t i=uint16_t(index+1);i<page.count;++i) {
    if(page.nodes[i].parent!=index||(s.state()[i].flags&Hidden))continue;
    const Rect current=s.layoutRect(page,s.state(),i);Rect child=current;
    if(cursor>32767)return false;
    child.x=horizontal?int32_t(cursor):content.x;child.y=horizontal?content.y:int32_t(cursor);
    if(!validDynamicBounds(s,i,child))return false;
    if(!sameDynamicRect(child,current)) {
      auto next=nextDynamicNode(s,i);next->offsetX+=child.x-current.x;next->offsetY+=child.y-current.y;next->width=child.width;next->height=child.height;
      next->boundsOverride=next->offsetX||next->offsetY||child.width!=page.nodes[i].rect.width||child.height!=page.nodes[i].rect.height;
      changes.push_back({i,next});
    }
    ++visibleCount;cursor+=(horizontal?child.width:child.height)+int64_t(gap);
  }
  Rect resized=parent;
  if(resizeContainer) {
    const int64_t used=!visibleCount?0:cursor-(horizontal?content.x:content.y)-gap;
    const int64_t size=used+2*skin.borderWidth+(horizontal?int64_t(skin.paddingLeft)+skin.paddingRight:int64_t(skin.paddingTop)+skin.paddingBottom);
    if(size>65535)return false;
    if(horizontal)resized.width=int32_t(size);else resized.height=int32_t(size);
    if(!validDynamicBounds(s,index,resized))return false;
  }
  // Reject the entire layout before mutating anything; a batch prevents an
  // intermediate layout from starting a render, even inside event callbacks.
  if(resizeContainer&&!sameDynamicRect(parent,resized)) {
    auto next=nextDynamicNode(s,index);next->width=resized.width;next->height=resized.height;
    next->boundsOverride=next->offsetX||next->offsetY||resized.width!=page.nodes[index].rect.width||resized.height!=page.nodes[index].rect.height;
    changes.push_back({index,next});
  }
  if(changes.empty())return true;
  if(!beginUpdate())return false;
  const bool nested=s.processing;s.processing=true;s.invalidateSubtree(index);
  for(const auto &change:changes) {
    const auto &next=change.second;s.state()[change.first].dynamic=next->boundsOverride||next->styleOverride?next:std::shared_ptr<const DynamicNode>();s.dynamicChanged(change.first);
  }
  finishDynamicMutation(*this,s,index);
  endUpdate();if(!nested)s.endProcessing();return true;
}
