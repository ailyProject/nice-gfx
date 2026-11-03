// Optional advanced menu chrome and one focused text marquee. All transient
// state is inline; no text copies, per-node clocks or per-frame allocation.
menu_motion::Axis menuDecorationAxis,menuOverlayAxis;
Id menuMarqueeId=0,menuAdvancedFocus=0,snapshotMarqueeId=0;
uint64_t menuMarqueeRevision=0;
uint32_t menuMarqueeElapsed=0;int32_t menuMarqueeOffset=0,snapshotMarqueeOffset=0;
int snapshotMenuOptionCount=0,snapshotMenuOptionIndex=0,snapshotMenuRatio=0,snapshotMenuReveal=1024;
int snapshotMenuScrollNode=-1,snapshotMenuCaptionNode=-1;Rect snapshotMenuScrollRect{};int32_t snapshotMenuScrollOffset=0,snapshotMenuScrollMax=0;
bool monochromeLightMode=false,snapshotLightMode=false;
void compactMenuRows() {
  if(!project||!menuMotionEnabled()||!menuMotionConfig.compactRows)return;
  for(int parentNode=0;parentNode<int(page().count);++parentNode)if(page().nodes[parentNode].kind==Kind::Scroll){
    const bool horizontal=state()[parentNode].flags&Horizontal;int32_t removed=0;
    for(int i=parentNode+1;i<int(page().count);++i){if(page().nodes[i].parent!=parentNode)continue;
      const Rect authored=page().nodes[i].rect;const int32_t start=horizontal?authored.x:authored.y;
      int32_t span=horizontal?authored.width:authored.height;
      for(int j=i+1;j<int(page().count);++j)if(page().nodes[j].parent==parentNode){span=(horizontal?page().nodes[j].rect.x:page().nodes[j].rect.y)-start;break;}
      const int32_t offset=-removed;
      if(!(state()[i].flags&Hidden)){
        const auto &old=state()[i].dynamic;const int32_t oldOffset=old?(horizontal?old->offsetX:old->offsetY):0;
        if(oldOffset!=offset){invalidateSubtree(i);auto next=old?std::make_shared<DynamicNode>(*old):std::make_shared<DynamicNode>();next->boundsOverride=true;next->width=authored.width;next->height=authored.height;if(horizontal)next->offsetX=offset;else next->offsetY=offset;state()[i].dynamic=next;dynamicChanged(i);invalidateSubtree(i);}
      }else removed+=std::max<int32_t>(0,span);
    }
    scroll(parentNode,state()[parentNode].scrollX,state()[parentNode].scrollY,false);
  }
}

int menuTextNode(int owner) const {
  if(owner<0)return -1;
  if(*state()[owner].text.c_str())return owner;
  for(int i=owner+1;i<int(page().count);++i){if(!descendant(i,owner))break;
    if((page().nodes[i].kind==Kind::Text||page().nodes[i].kind==Kind::Label)&&*state()[i].text.c_str())return i;}
  return -1;
}
int32_t menuTextWidth(int node) {
  if(node<0)return 0;const auto &style=styleFor(page(),state(),node);if(!style.font)return 0;
  const char *s=state()[node].text.c_str();const size_t end=strlen(s);int64_t width=0;
  for(size_t at=0;at<end;){const auto cp=nextCode(s,at,end);if(cp=='\n'||cp=='\r')break;width+=nice_gfx::detail::monoAdvance(*style.font,cp)*style.fontSize;}
  return int32_t(std::min<int64_t>(INT32_MAX,width));
}
bool menuMarqueeText(int node) const {
  if(node<0)return false;const auto &style=styleFor(page(),state(),node);if(!style.font)return false;
  const Rect box=textBox(rect(node),page().nodes[node],style);const int32_t line=int32_t(style.font->height)*style.fontSize;
  if(box.width<=0||box.height<=0||line<=0||box.height>=2*line)return false;
  const char *text=state()[node].text.c_str();return !strchr(text,'\n')&&!strchr(text,'\r');
}
Rect menuFitRect(int owner) {
  Rect r=rect(owner);if(menuMotionConfig.fit!=MenuFit::Text)return r;
  const int textNode=menuTextNode(owner);if(textNode<0)return r;
  const Style &style=styleFor(page(),state(),textNode);const Rect box=textBox(rect(textNode),page().nodes[textNode],style);
  const int32_t width=std::min<int32_t>(box.width,menuTextWidth(textNode));
  int32_t x=box.x;if(style.textAlign==1)x+=(box.width-width)/2;else if(style.textAlign==2)x+=box.width-width;
  return intersect(r,{x-menuMotionConfig.textPadding,r.y,width+2*menuMotionConfig.textPadding,r.height});
}
void resetAdvancedMenu() {
  if(project&&menuMarqueeId){const int old=find(menuMarqueeId);if(old>=0){++state()[old].text.revision;invalidate(old);}}
  menuAdvancedFocus=menuMarqueeId=0;menuMarqueeElapsed=0;menuMarqueeOffset=0;
  menuDecorationAxis.snap(0);menuOverlayAxis.snap(0);
}
void stepAdvancedMenu(uint32_t elapsed) {
  if(!menuMotionConfig.optionBar&&!menuMotionConfig.tileCaption&&!menuMotionConfig.previousNext&&!menuMotionConfig.marquee.enabled)return;
  if(!menuMotionEnabled()||!keyInput||focusNode<0||!focusable(focusNode,false)){resetAdvancedMenu();return;}
  const Id id=page().nodes[focusNode].id;const int textNode=menuTextNode(focusNode);
  const uint64_t revision=textNode>=0?state()[textNode].text.revision:0;
  if(menuAdvancedFocus!=id||menuMarqueeRevision!=revision){
    if(menuMarqueeId){int old=find(menuMarqueeId);if(old>=0){++state()[old].text.revision;invalidate(old);}}
    menuAdvancedFocus=id;menuMarqueeRevision=revision;menuMarqueeElapsed=0;menuMarqueeOffset=0;
    menuMarqueeId=textNode>=0?page().nodes[textNode].id:0;menuOverlayAxis.snap(0);menuOverlayAxis.aim(1024);
    if(menuMotionConfig.marquee.enabled&&textNode>=0)menuMarqueeRevision=++state()[textNode].text.revision;
    if(textNode>=0)invalidate(textNode);
  }
  int count=0,index=0;for(int i=0;i<int(page().count);++i)if(i!=focusScope&&scopeFor(i)==scopeFor(focusNode)&&focusable(i,false)){if(i==focusNode)index=count;++count;}
  menuDecorationAxis.aim(count>1?index*1024/(count-1):0);
  const int oldDecor=menuDecorationAxis.pixels(),oldOverlay=menuOverlayAxis.pixels();
  menuDecorationAxis.step(elapsed,menuMotionConfig,menuMotionConfig.decoration);menuOverlayAxis.step(elapsed,menuMotionConfig,menuMotionConfig.overlay);
  if((menuMotionConfig.optionBar&&oldDecor!=menuDecorationAxis.pixels())||((menuMotionConfig.tileCaption||menuMotionConfig.previousNext)&&oldOverlay!=menuOverlayAxis.pixels()))refresh->invalidateAll();
  if(!menuMotionConfig.marquee.enabled||!menuMarqueeText(textNode))return;
  const auto &style=styleFor(page(),state(),textNode);const Rect box=textBox(rect(textNode),page().nodes[textNode],style);
  const int32_t travel=std::max<int32_t>(0,menuTextWidth(textNode)-box.width);
  if(!travel)return;
  menuMarqueeElapsed+=elapsed;
  const auto &m=menuMotionConfig.marquee;int32_t offset=0;
  if(menuMarqueeElapsed>m.dwellMs){
    const uint32_t leg=uint32_t((uint64_t(travel)*1000+m.speed-1)/m.speed),period=2*(leg+m.pauseMs);
    const uint32_t t=(menuMarqueeElapsed-m.dwellMs)%std::max<uint32_t>(1,period);
    if(t<leg)offset=int32_t(uint64_t(t)*travel/leg);else if(t<leg+m.pauseMs)offset=travel;
    else if(t<2*leg+m.pauseMs)offset=travel-int32_t(uint64_t(t-leg-m.pauseMs)*travel/leg);
  }
  if(offset!=menuMarqueeOffset){menuMarqueeOffset=offset;invalidate(textNode);}
}
bool advancedMenuMoving() const {
  if(!menuMotionEnabled()||!keyInput||focusNode<0)return false;
  if(menuMotionConfig.optionBar&&menuDecorationAxis.active())return true;
  if((menuMotionConfig.tileCaption||menuMotionConfig.previousNext)&&menuOverlayAxis.active())return true;
  // A focused overflowing title remains a live timer even during dwell/pause.
  return menuMotionConfig.marquee.enabled&&menuMarqueeId&&snapshotMarqueeId==menuMarqueeId;
}
void snapshotAdvancedMenu() {
  snapshotLightMode=monochromeLightMode;snapshotMarqueeId=0;snapshotMarqueeOffset=0;
  snapshotMenuOptionCount=snapshotMenuOptionIndex=0;snapshotMenuScrollNode=-1;
  if(!menuMotionEnabled()||!keyInput||focusNode<0)return;
  if(!menuMotionConfig.scrollbar&&!menuMotionConfig.optionBar&&!menuMotionConfig.tileCaption&&!menuMotionConfig.previousNext&&!menuMotionConfig.marquee.enabled)return;
  snapshotMenuCaptionNode=menuTextNode(focusNode);
  if(menuMotionConfig.marquee.enabled&&menuMarqueeId){const int node=find(menuMarqueeId);if(menuMarqueeText(node)){const auto &s=styleFor(page(),state(),node);if(menuTextWidth(node)>textBox(rect(node),page().nodes[node],s).width){snapshotMarqueeId=menuMarqueeId;snapshotMarqueeOffset=menuMarqueeOffset;}}}
  for(int i=0;i<int(page().count);++i)if(i!=focusScope&&scopeFor(i)==scopeFor(focusNode)&&focusable(i,false)){if(i==focusNode)snapshotMenuOptionIndex=snapshotMenuOptionCount;++snapshotMenuOptionCount;}
  snapshotMenuRatio=clamp32(menuDecorationAxis.pixels(),0,1024);snapshotMenuReveal=clamp32(menuOverlayAxis.pixels(),0,1024);
  // Directly focused readers own their scrollbar; visual overlay roots stop the search.
  for(int a=focusNode;a>=0;a=parent(a)){if(page().nodes[a].kind==Kind::Scroll){snapshotMenuScrollNode=a;snapshotMenuScrollRect=contentBox(rect(a),styleFor(page(),state(),a));int32_t mx,my;extent(a,mx,my);const bool h=state()[a].flags&Horizontal;snapshotMenuScrollMax=h?mx:my;snapshotMenuScrollOffset=h?state()[a].scrollX:state()[a].scrollY;break;}if(viewportOverlayRoot(page(),a))break;}
}
void paintAdvancedMenu(const DirtyRect &area) {
  if(!snapshotMenuVisible||snapshotTransition||rasterFrame.preview.page)return;
  canvas.setViewport(area.x,area.y,area.width,area.height,false);
  const uint16_t ink=snapshotLightMode?0:0xffff,paper=snapshotLightMode?0xffff:0;
  if(snapshotMenuConfig.scrollbar&&snapshotMenuScrollNode>=0&&snapshotMenuScrollMax>0){
    const Rect b=snapshotMenuScrollRect;const bool horizontal=rasterFrame.current.values[snapshotMenuScrollNode].flags&Horizontal;
    const int32_t length=horizontal?b.width:b.height,thumb=std::max<int32_t>(2,int64_t(length)*length/(length+snapshotMenuScrollMax));
    const int32_t offset=int64_t(length-thumb)*snapshotMenuScrollOffset/snapshotMenuScrollMax;
    if(horizontal){canvas.drawFastHLine(b.x,b.y+b.height-1,b.width,ink);canvas.fillRect(b.x+offset,b.y+b.height-2,thumb,2,ink);}
    else{canvas.drawFastVLine(b.x+b.width-1,b.y,b.height,ink);canvas.fillRect(b.x+b.width-2,b.y+offset,2,thumb,ink);}
  }
  if(snapshotMenuConfig.optionBar&&snapshotMenuOptionCount){const int32_t w=std::max<int32_t>(3,page().width/snapshotMenuOptionCount);canvas.drawFastHLine(0,page().height-1,page().width,ink);canvas.fillRect((page().width-w)*snapshotMenuRatio/1024,page().height-3,w,3,ink);}
  if(snapshotMenuConfig.previousNext&&snapshotMenuReveal>512){const int32_t y=snapshotMenuRect.y+snapshotMenuRect.height/2;
    if(snapshotMenuOptionIndex>0){canvas.drawLine(1,y,4,y-3,ink);canvas.drawLine(1,y,4,y+3,ink);}
    if(snapshotMenuOptionIndex+1<snapshotMenuOptionCount){canvas.drawLine(page().width-2,y,page().width-5,y-3,ink);canvas.drawLine(page().width-2,y,page().width-5,y+3,ink);}}
  if(snapshotMenuConfig.tileCaption){const int node=snapshotMenuCaptionNode;if(node>=0){const auto &s=styleFor(*rasterFrame.current.page,rasterFrame.current.values,node);if(s.font){const int h=s.font->height*s.fontSize+2,y=rasterFrame.current.page->height-h-4,w=rasterFrame.current.page->width*snapshotMenuReveal/1024;const Rect c=intersect({0,y,w,h},{area.x,area.y,area.width,area.height});canvas.setViewport(c.x,c.y,c.width,c.height,false);canvas.fillRect(0,y,w,h,paper);canvas.setFont(s.font);canvas.setTextSize(s.fontSize);canvas.setTextColor(ink);canvas.drawString(rasterFrame.current.values[node].text.c_str(),2,y+1);}}}
}
