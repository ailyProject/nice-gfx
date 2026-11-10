// Included inside View::Impl after SystemModalPanelLease storage.
bool panelSourceExactKey(const PageDesc &p,const RetainedVector<State> &a,const RetainedVector<State> &b)const{
  if(a.size()!=p.count||b.size()!=p.count)return false;
  for(uint16_t i=0;i<p.count;++i)if(a[i].parent!=b[i].parent||a[i].kind!=b[i].kind||a[i].styleIndex!=b[i].styleIndex||!sameSurfaceNode(a[i],b[i]))return false;
  return true;
}
bool panelSourceNodeValueEqual(const State &source,const State &clone)const{
  if(source.kind!=clone.kind||source.focusOverride!=clone.focusOverride||source.mediaPaused!=clone.mediaPaused||
     source.animating||source.animationPending||clone.animating||clone.animationPending||
     source.imageOverride||clone.imageOverride||source.instrument||clone.instrument||source.implicitInput||clone.implicitInput||
     std::strcmp(source.text.c_str(),clone.text.c_str()))return false;
  if(source.text.compiled==clone.text.compiled&&source.text.revision==clone.text.revision)
    return sameSurfaceNode(source,clone);
  State normalized=clone;normalized.parent=source.parent;normalized.styleIndex=source.styleIndex;normalized.text=source.text;
  return sameSurfaceNode(source,normalized);
}
bool validSystemPanelPageSource(const Surface &surface,uint16_t root,uint16_t sourcePage,uint16_t handle)const{
  const auto &host=*rasterFrame.current.page,&p=project->pages[sourcePage];
  const auto &live=rasterFrame.current.values,&canonical=pages[sourcePage];
  const size_t count=p.count;
  if(surface.page!=&p||!surface.ready||!panelSourceExactKey(p,surface.key,canonical)||surface.geometry.nodes.size()!=count||
     p.width!=host.width||p.height!=host.height||root+count!=handle||handle>=host.count||
     navigationGroup(p.path)||surface.geometry.visibleHeight!=-1)return false;
  for(const auto &pair:systemChromePairs)if(pair.page==sourcePage)return false;
  const auto &sourceRoot=surface.geometry.nodes[0],&panel=rasterFrame.current.geometry.nodes[root];
  const Rect full{0,0,p.width,p.height};
  if(p.nodes[0].parent!=NoParent||canonical[0].parent!=NoParent||p.nodes[0].kind!=Kind::Container||
     (canonical[0].flags&(Hidden|Disabled|Transparent))||canonical[0].opacity!=255||canonical[0].animationX||canonical[0].animationY||
     p.nodes[0].motion||p.nodes[0].animation.effect!=AnimationEffect::None||p.nodes[0].exitAnimation||
     !sourceRoot.visible||sourceRoot.alpha!=255||!modalBaseSameRect(sourceRoot.rect,full)||!modalBaseContains(sourceRoot.clip,full)||
     ((canonical[0].flags^live[root].flags)&~uint16_t(Hidden|Clip))||
     ((p.nodes[0].flags^host.nodes[root].flags)&~uint16_t(Hidden|Clip))||
     p.background!=styleFor(host,live,root).background||
     !sameCompiledStyle(styleFor(p,canonical,0),styleFor(host,live,root)))return false;
  const auto &extra=host.nodes[handle];
  if(extra.kind!=Kind::Button||extra.parent!=root||live[handle].parent!=root||extra.motion||extra.animation.effect!=AnimationEffect::None||extra.exitAnimation)return false;
  // The compiler's source-node links are explicit. Admit only their complete,
  // contiguous bijection, then the sole generated direct-child close handle.
  size_t links=0;
  for(const auto &link:systemPanelMembers)if(link.page==history.back()&&behaviorSnapshotDescendant(link.node,root)){
    if(link.sourcePage!=sourcePage||!link.sourceNode||link.sourceNode>=count||link.node!=root+link.sourceNode)return false;
    ++links;
  }
  if(links!=count-1)return false;
  for(uint16_t i=0;i<host.count;++i)if(behaviorSnapshotDescendant(i,root)&&i!=handle&&(i<root||i>=root+count))return false;
  if(rasterFrame.current.geometry.nodes[handle].children)return false;
  const int32_t dy=panel.rect.y;
  for(uint16_t i=1;i<count;++i){
    const uint16_t at=uint16_t(root+i);const auto &a=p.nodes[i],&b=host.nodes[at];const auto &v=canonical[i],&w=live[at];
    if(a.parent==NoParent||b.parent!=root+a.parent||v.parent!=a.parent||w.parent!=b.parent||a.kind!=b.kind||
       !panelSourceNodeValueEqual(v,w)||!sameCompiledStyle(styleFor(p,canonical,i),styleFor(host,live,at))||
       a.minimum!=b.minimum||a.maximum!=b.maximum||a.step!=b.step||a.flags!=b.flags||
       a.pager.indicatorHeight!=b.pager.indicatorHeight||a.pager.dotSize!=b.pager.dotSize||a.pager.dotGap!=b.pager.dotGap||
       !modalBaseSameRect(a.rect,b.rect)||a.motion||b.motion||a.exitAnimation||b.exitAnimation||
       a.animation.effect!=AnimationEffect::None||b.animation.effect!=AnimationEffect::None||
       a.icon!=b.icon||a.media!=b.media||a.instrument||b.instrument||a.shape!=b.shape||a.inputField!=b.inputField)return false;
    const auto &old=surface.geometry.nodes[i],&current=rasterFrame.current.geometry.nodes[at];
    if(old.visible!=current.visible||old.alpha!=current.alpha)return false;
    const int64_t movedY=int64_t(old.rect.y)+dy;
    if(movedY<INT32_MIN||movedY>INT32_MAX||current.rect.x!=old.rect.x||current.rect.y!=movedY||
       current.rect.width!=old.rect.width||current.rect.height!=old.rect.height)return false;
    Rect expected=old.clip;const int64_t clipY=int64_t(expected.y)+dy;if(clipY<INT32_MIN||clipY>INT32_MAX)return false;
    expected.y=int32_t(clipY);expected=intersect(expected,full);
    const Rect expectedPaint=intersect(expected,current.rect),actualPaint=intersect(current.clip,current.rect);
    if((expectedPaint.width>0&&expectedPaint.height>0)||(actualPaint.width>0&&actualPaint.height>0))
      if(!modalBaseSameRect(expectedPaint,actualPaint))return false;
    if(old.lineFirst>surface.geometry.lines.size()||old.lineCount>surface.geometry.lines.size()-old.lineFirst)return false;
    if(current.visible&&current.alpha&&actualPaint.width>0&&actualPaint.height>0&&(a.kind==Kind::Text||a.kind==Kind::Label||a.kind==Kind::Button)){
      // textY is an offset inside textBox, while rect/clip carry translation.
      if(old.lineCount!=current.lineCount||old.ellipsisCount!=current.ellipsisCount||old.textY!=current.textY||
         current.lineFirst>rasterFrame.current.geometry.lines.size()||current.lineCount>rasterFrame.current.geometry.lines.size()-current.lineFirst)return false;
      for(size_t line=0;line<old.lineCount;++line){const auto &before=surface.geometry.lines[old.lineFirst+line],&now=rasterFrame.current.geometry.lines[current.lineFirst+line];
        if(before.begin!=now.begin||before.end!=now.end||before.width!=now.width)return false;
      }
    }
    if(componentCount&&a.kind==Kind::Button){const auto before=componentPaint(p,canonical,a.id),now=componentPaint(host,live,b.id);if(before.selected!=now.selected||before.expanded!=now.expanded)return false;}
  }
  return true;
}
bool prepareSystemPanelPageSource(const NativeFramebuffer &target,uint16_t root){
  const auto &host=*rasterFrame.current.page;const auto &live=rasterFrame.current.values;
  if(live.empty()||(live[0].flags&(Hidden|Disabled|Transparent))||live[0].opacity!=255)return false;
  uint16_t sourcePage=NoParent,handle=NoParent;
  for(const auto &pair:systemChromePairs)if(pair.page==history.back()&&pair.panel==root){handle=pair.handle;break;}
  for(const auto &link:systemPanelMembers)if(link.page==history.back()&&link.node==root+1&&link.sourceNode==1){sourcePage=link.sourcePage;break;}
  if(sourcePage==NoParent||sourcePage>=project->count||sourcePage==history.back()||pages[sourcePage].empty()||handle==NoParent)return false;
  const auto &p=project->pages[sourcePage];
  if(!p.width||!p.height||p.width!=host.width||p.height!=host.height||size_t(p.height)>SIZE_MAX/size_t(p.width)/2)return false;
  const size_t bytes=size_t(p.width)*p.height*2;
  const uintptr_t output=reinterpret_cast<uintptr_t>(target.pixels);
  if(!target.pixels||target.width!=host.width||target.height!=host.height||target.stride<target.width||
     target.height>SIZE_MAX/target.stride/2||target.bytes<target.stride*target.height*2||
     output>UINTPTR_MAX-target.bytes||compressedCapacity>SIZE_MAX/2||
     nativeMemoryOverlap(output,target.bytes,compressedStorage,compressedCapacity*2))return false;
  // Alias backends keep the normal renderer. This also proves a cold decode
  // cannot overwrite any portion of the actual output while choosing a slot.
  for(const auto &entry:surfaces)if(nativeMemoryOverlap(output,target.bytes,entry.pixels,bytes))return false;
  Surface *found=nullptr;
  for(auto &entry:surfaces)if(entry.page==&p&&entry.ready&&
      validSystemPanelPageSource(entry,root,sourcePage,handle)){found=&entry;break;}
  if(!found){
    auto *record=compressedSurfaceFor(p);
    if(!record||!panelSourceExactKey(p,record->key,pages[sourcePage]))return false;
    found=restoreSurfacePage(p,rasterFrame.current.page,rasterFrame.preview.page,coldFadeSurface,seedSurface,&pages[sourcePage]);
    if(!found||!validSystemPanelPageSource(*found,root,sourcePage,handle))return false;
  }
  const uintptr_t address=reinterpret_cast<uintptr_t>(found->pixels);
  if(address>UINTPTR_MAX-bytes||nativeSourceOverlap(p,pages[sourcePage],address,bytes,true))return false;
  if(!systemModalPanelLive.sharedSource||systemModalPanelSurface(systemModalPanelLive)!=found)invalidateSystemModalPanelLive();
  systemModalPanelLive={size_t(found-surfaces.data()),sourcePage,root,frameSourceOwner,found->pixelsRevision,rasterFrame.current.geometry.nodes[root].rect,false};
  systemModalPanelLive.sharedSource=true;systemModalPanelLive.hostPage=uint16_t(history.back());systemModalPanelLive.handle=handle;
  found->used=++surfaceClock;systemModalPanelFrame=systemModalPanelLive;return true;
}
