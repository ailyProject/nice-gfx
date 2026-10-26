// View::Impl members. Timed playback allocates only for media nodes that use it.
const MediaPlaybackDesc &mediaPlayback(Id id) const {
  static const MediaPlaybackDesc automatic;
  const auto at=std::lower_bound(mediaPolicies.begin(),mediaPolicies.end(),id,[](const MediaPlaybackDesc &a,Id b){return a.id<b;});
  return at!=mediaPolicies.end()&&at->id==id?*at:automatic;
}
void removeMediaTween(int i) {
  mediaTweens.erase(std::remove_if(mediaTweens.begin(),mediaTweens.end(),[&](const MediaTween &t){return t.page==&page()&&t.node==i;}),mediaTweens.end());
}
bool hasMediaTween(int i) const {
  return std::find_if(mediaTweens.begin(),mediaTweens.end(),[&](const MediaTween &t){return t.page==&page()&&t.node==i;})!=mediaTweens.end();
}
void resetMediaPlayback(int i) {
  auto &v=state()[i];v.mediaFrame=0;v.mediaAt=0;v.mediaPending=true;
  v.mediaPaused=!mediaPlayback(page().nodes[i].id).autoplay;removeMediaTween(i);
}
bool seekMedia(int i,int32_t frame) {
  const auto *m=media(page().nodes[i],state()[i]);
  if(!m||frame<0||frame>=m->frames)return false;
  removeMediaTween(i);auto &v=state()[i];v.mediaPaused=true;v.mediaPending=true;
  if(v.mediaFrame!=frame){v.mediaFrame=uint16_t(frame);invalidate(i);}return true;
}
void advanceMediaTweens(uint32_t now) {
  for(size_t at=0;at<mediaTweens.size();) {
    auto &t=mediaTweens[at];
    if(t.page!=&page()){mediaTweens.erase(mediaTweens.begin()+at);continue;}
    auto &v=state()[t.node];
    if(t.pending){t.at=now;t.pending=false;}
    const uint32_t elapsed=uint32_t(now-t.at);
    // Integer division truncates the travelled distance toward zero in both directions.
    const int32_t distance=int32_t(t.to)-t.from;
    const uint16_t frame=elapsed>=t.duration?t.to:uint16_t(int32_t(t.from)+int32_t(int64_t(distance)*elapsed/t.duration));
    if(frame!=v.mediaFrame){v.mediaFrame=frame;invalidate(t.node);}
    if(elapsed>=t.duration){v.mediaPaused=true;v.mediaPending=true;mediaTweens.erase(mediaTweens.begin()+at);}
    else ++at;
  }
}
int mediaScrubTarget(int source) const {
  if(source<0)return -1;
  const auto &binding=mediaPlayback(page().nodes[source].id);
  if(!binding.scrubTarget||policy(source).ownership==GestureOwnership::Parent||!detail::acceptsAxis(policy(source).axis,true))return -1;
  const int i=find(binding.scrubTarget);
  if(i<0||page().nodes[i].kind!=Kind::Media||!visible(page(),state(),i,true))return -1;
  const auto *m=media(page().nodes[i],state()[i]);return m&&m->frames>1?i:-1;
}
void acquireMediaScrub(int source) {
  // The legacy reserved back edge also precedes a media scrub in coordinated
  // mode. Ordinary ranges/scrollers retain their existing navigation order.
  if(edgeBack&&history.size()>1&&startX>=0&&startX<24&&x>startX&&contactModal<0){
    const char *from=page().path,*to=project->pages[history[history.size()-2]].path;
    bool prepared=routeEvent(EventType::BeforeNavigate,from,to,RouteAction::Back);
    if(prepared&&down&&guard){guarding=true;prepared=guard(guardContext,from,to,RouteAction::Back);guarding=false;}
    if(down&&prepared){backPrepared=true;drag=Drag::Back;cancelTarget();}
    else {routeEvent(EventType::NavigationCancel,from,to,RouteAction::Back);if(down){drag=Drag::Consumed;cancelTarget();}}
    return;
  }
  mediaDragTarget=mediaScrubTarget(source);
  if(mediaDragTarget<0)return;
  mediaDragFrom=state()[mediaDragTarget].mediaFrame;
  mediaDragWidth=std::max<int32_t>(1,rect(source).width);
  mediaDragLoop=mediaPlayback(page().nodes[source].id).loop;
  seekMedia(mediaDragTarget,mediaDragFrom);
  acquireOwner(source,Drag::Media);
}
void updateMediaScrub(int32_t dx) {
  if(mediaDragTarget<0)return;
  const auto *m=media(page().nodes[mediaDragTarget],state()[mediaDragTarget]);
  if(!m||!m->frames)return;
  const int64_t raw=int64_t(mediaDragFrom)+int64_t(dx)*m->frames/mediaDragWidth;
  const int32_t frame=mediaDragLoop?int32_t((raw%m->frames+m->frames)%m->frames):int32_t(std::max<int64_t>(0,std::min<int64_t>(m->frames-1,raw)));
  seekMedia(mediaDragTarget,frame);
}
