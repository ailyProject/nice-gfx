// View::Impl members. All candidates use the initial hit path and cumulative
// displacement. No candidate allocation or per-sample hit testing is needed.
bool localCandidate(int i,bool horizontal) const {
  if(i<0||policy(i).ownership==GestureOwnership::Parent||!detail::acceptsAxis(policy(i).axis,horizontal))return false;
  if(page().nodes[i].kind==Kind::Range)return horizontal==!(page().nodes[i].flags&VerticalRange);
  return policy(i).ownership==GestureOwnership::Local&&!scrolling(page().nodes[i].kind);
}
void acquireOwner(int i,Drag kind) {
  owner=i;drag=kind;
  if(kind==Drag::Scroll||kind==Drag::Pager){startScrollX=state()[i].scrollX;startScrollY=state()[i].scrollY;cancelTarget();}
  if(down){ownerStarted=true;dispatch(i,EventType::GestureStart);}
}
void recognizeCoordinated(int32_t dx,int32_t dy,uint32_t now) {
  const auto decision=detail::decideGesture(dx,dy);
  if(decision==detail::GestureDecision::Pending)return;
  if(decision==detail::GestureDecision::Reject){drag=Drag::Consumed;cancelTarget();return;}
  const bool horizontal=decision==detail::GestureDecision::X;
  const SwipeDirection direction=horizontal?(dx<0?SwipeDirection::Left:SwipeDirection::Right):(dy<0?SwipeDirection::Up:SwipeDirection::Down);
  const bool axisAllowed=horizontal?allowX:allowY;
  const bool navigationAllowed=axisAllowed&&contactModal<0;
  const auto *edge=navigationAllowed?swipeBinding(direction,true):nullptr;
  if(edge&&edge->overrideControls){claimGesture(edge,now);return;}
  // The deepest declared local recognizer (e.g. map/canvas) and the native
  // control participate in the same ranking. Explicit claims are candidates.
  for(int a=target;a>=0;a=parent(a)){
    if(localCandidate(a,horizontal)){acquireOwner(a,page().nodes[a].kind==Kind::Range?Drag::Range:Drag::Custom);return;}
    if(a==pointerClaim&&detail::acceptsAxis(pointerClaimAxis,horizontal)&&policy(a).ownership!=GestureOwnership::Parent){acquireOwner(a,Drag::Custom);return;}
  }
  // Label-associated controls can live outside the target's ancestor path.
  if(localCandidate(activeControl,horizontal)){acquireOwner(activeControl,page().nodes[activeControl].kind==Kind::Range?Drag::Range:Drag::Custom);return;}
  bool contained=false,chainAllowed=true;
  if(axisAllowed)for(int a=target;a>=0;a=parent(a)){
    if(!scrolling(page().nodes[a].kind))continue;
    const bool axis=page().nodes[a].kind==Kind::Pager||(state()[a].flags&Horizontal);
    if(axis!=horizontal)continue; // Containment blocks only its own axis.
    const auto &p=policy(a);
    if(p.ownership==GestureOwnership::Parent)continue;
    if(!p.navigationChain)chainAllowed=false;
    if(!detail::acceptsAxis(p.axis,horizontal))continue;
    int32_t mx,my;extent(a,mx,my);
    const int32_t pos=axis?state()[a].scrollX:state()[a].scrollY,maxPos=axis?mx:my,delta=axis?dx:dy;
    if(maxPos>0&&((delta<0&&pos<maxPos)||(delta>0&&pos>0))){acquireOwner(a,page().nodes[a].kind==Kind::Pager?Drag::Pager:Drag::Scroll);return;}
    if(state()[a].flags&OverscrollContain){contained=true;break;}
  }
  if(navigationAllowed&&!contained&&chainAllowed){
    if(edge){claimGesture(edge,now);return;}
    if(const auto *binding=swipeBinding(direction,false)){claimGesture(binding,now);return;}
    if(horizontal){int index=-1;const auto *g=navigationGroup(page().path,&index);
      if(g&&g->swipe&&contains(navigationViewport(page().path),startX,startY)){
        const int next=index+(dx<0?1:-1);
        if(next>=0&&next<g->count&&prepareGroup(routeIndex(g->paths[next]),now)){drag=Drag::Group;cancelTarget();return;}
      }
    }
    if(!down)return;
    if(swipeHome&&startY>=int32_t(page().height)-24&&startY<page().height&&dy<0&&!horizontal){drag=Drag::Home;cancelTarget();return;}
    if(edgeBack&&history.size()>1&&startX>=0&&startX<24&&dx>0&&horizontal){
      const char *from=page().path,*to=project->pages[history[history.size()-2]].path;
      bool prepared=routeEvent(EventType::BeforeNavigate,from,to,RouteAction::Back);
      if(prepared&&down&&guard){guarding=true;prepared=guard(guardContext,from,to,RouteAction::Back);guarding=false;}
      if(down&&prepared){backPrepared=true;drag=Drag::Back;cancelTarget();return;}
      routeEvent(EventType::NavigationCancel,from,to,RouteAction::Back);
    }
  }
  if(down){drag=Drag::Consumed;cancelTarget();}
}
