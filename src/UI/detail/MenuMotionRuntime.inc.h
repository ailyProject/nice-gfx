// View::Impl members. One sparse scroll slot per Scroll node, prepared at page
// entry/configuration. Sampling and target changes never resize this storage.
MenuMotionConfig menuMotionConfig{};
MenuMotionConfig baseMenuMotionConfig{};bool baseMenuMotionConfigured=false;
void applyPageMenuMotion() {
  menuMotionConfig=baseMenuMotionConfig;menuMotionConfigured=baseMenuMotionConfigured;
  if(project)if(const auto *local=inputPage())if(local->menuMotion&&validMenuMotion(*local->menuMotion)){
    menuMotionConfig=*local->menuMotion;menuMotionConfigured=true;
    if(baseMenuMotionConfigured){if(!local->menuPresetOverride)menuMotionConfig.preset=baseMenuMotionConfig.preset;
      MenuDynamics *channels[]={&menuMotionConfig.camera,&menuMotionConfig.position,&menuMotionConfig.size,&menuMotionConfig.decoration,&menuMotionConfig.overlay};
      const MenuDynamics *base[]={&baseMenuMotionConfig.camera,&baseMenuMotionConfig.position,&baseMenuMotionConfig.size,&baseMenuMotionConfig.decoration,&baseMenuMotionConfig.overlay};
      for(unsigned i=0;i<5;++i)if(!(local->menuDynamicsOverride&(1u<<i)))*channels[i]=*base[i];
      if(!(local->menuDynamicsOverride&32))menuMotionConfig.mode=baseMenuMotionConfig.mode;
    }
  }
  menuMotionConfig=resolveMenuPreset(menuMotionConfig);
}
bool menuMotionConfigured=false,menuIndicatorReady=false,menuClockReady=false,menuApplyingScroll=false,menuMotionSuspended=false;
uint32_t menuUpdatedAt=0;
const PageDesc *menuMotionPage=nullptr;
menu_motion::Axis menuIndicatorAxes[4];
struct MenuScrollMotion { int node=-1;menu_motion::Axis x,y;bool tracking=false; };
std::vector<MenuScrollMotion> menuScrollMotions;
bool snapshotMenuEnabled=false,snapshotMenuVisible=false;
MenuMotionConfig snapshotMenuConfig{};
Rect snapshotMenuRect{},snapshotMenuClip{};

bool menuMotionEnabled() const {
  return menuMotionConfigured&&displayPolicy.monochrome&&!displayPolicy.slowRefresh;
}
Rect menuIndicatorRect() const {
  return {menuIndicatorAxes[0].pixels(),menuIndicatorAxes[1].pixels(),
    std::max<int32_t>(1,menuIndicatorAxes[2].pixels()),std::max<int32_t>(1,menuIndicatorAxes[3].pixels())};
}
void resetMenuMotion() {
  resetAdvancedMenu();
  if(project&&menuIndicatorReady)invalidatePageRect(menuIndicatorRect());
  menuIndicatorReady=menuClockReady=menuMotionSuspended=false;menuMotionPage=project?&page():nullptr;
  menuScrollMotions.clear();
  if(!project||!menuMotionEnabled())return;
  size_t count=0;for(uint16_t i=0;i<page().count;++i)if(page().nodes[i].kind==Kind::Scroll)++count;
  menuScrollMotions.reserve(count);
  for(uint16_t i=0;i<page().count;++i)if(page().nodes[i].kind==Kind::Scroll){
    MenuScrollMotion item;item.node=i;item.x.snap(state()[i].scrollX);item.y.snap(state()[i].scrollY);
    menuScrollMotions.push_back(item);
  }
}
MenuScrollMotion *menuScrollMotion(int node) {
  for(auto &item:menuScrollMotions)if(item.node==node)return &item;return nullptr;
}
void cancelMenuScroll(int node) {
  if(menuApplyingScroll)return;
  if(auto *item=menuScrollMotion(node)){item->tracking=false;item->x.snap(state()[node].scrollX);item->y.snap(state()[node].scrollY);}
}
bool revealMenuFocus(int i) {
  if(!menuMotionEnabled())return false;
  menuMotionSuspended=false;
  Rect projected=rect(i);
  for(int child=i,a=parent(i);a>=0;child=a,a=parent(a)){if(viewportOverlayRoot(page(),child))break;if(page().nodes[a].kind==Kind::Scroll){
    auto *item=menuScrollMotion(a);if(!item)continue;
    const Rect box=contentBox(rect(a),styleFor(page(),state(),a));
    const bool horizontal=(state()[a].flags&Horizontal)!=0;
    int64_t dx=0,dy=0;
    if(menuMotionConfig.scroll==MenuScroll::Center){
      dx=int64_t(projected.x)-box.x+(int64_t(projected.width)-box.width)/2;
      dy=int64_t(projected.y)-box.y+(int64_t(projected.height)-box.height)/2;
    }else{
      dx=projected.x<box.x?int64_t(projected.x)-box.x:
        int64_t(projected.x)+projected.width>int64_t(box.x)+box.width?int64_t(projected.x)+projected.width-box.x-box.width:0;
      dy=projected.y<box.y?int64_t(projected.y)-box.y:
        int64_t(projected.y)+projected.height>int64_t(box.y)+box.height?int64_t(projected.y)+projected.height-box.y-box.height:0;
    }
    int32_t mx,my;extent(a,mx,my);
    const int32_t sx=horizontal?int32_t(std::max<int64_t>(0,std::min<int64_t>(mx,int64_t(state()[a].scrollX)+dx))):0;
    const int32_t sy=horizontal?0:int32_t(std::max<int64_t>(0,std::min<int64_t>(my,int64_t(state()[a].scrollY)+dy)));
    if(!item->tracking){item->x.snap(state()[a].scrollX);item->y.snap(state()[a].scrollY);}
    item->x.aim(sx);item->y.aim(sy);item->tracking=item->x.active()||item->y.active();
    // Outer scroll targets account for the inner scroll's future displacement.
    projected.x-=sx-state()[a].scrollX;projected.y-=sy-state()[a].scrollY;
    if(menuMotionConfig.mode==MenuMotionMode::Instant){
      item->x.snap(sx);item->y.snap(sy);item->tracking=false;
      menuApplyingScroll=true;scroll(a,sx,sy);menuApplyingScroll=false;
    }
  }}
  return true;
}
void aimMenuIndicator(bool snap=false) {
  if(!menuMotionEnabled()||focusNode<0||size_t(focusNode)>=page().count||!focusable(focusNode,false))return;
  menuMotionSuspended=false;
  const Rect r=menuFitRect(focusNode);const int32_t values[4]={r.x,r.y,r.width,r.height};
  for(unsigned i=0;i<4;++i){if(snap||!menuIndicatorReady||menuMotionConfig.mode==MenuMotionMode::Instant)menuIndicatorAxes[i].snap(values[i]);else menuIndicatorAxes[i].aim(values[i]);}
  menuIndicatorReady=true;
}
bool menuMoving() const {
  if(!menuMotionEnabled())return false;
  // Completion of a page/keyboard transition can leave focus reveal queued.
  // Keep one tick pending so callers that drain busy() cannot stop before it.
  if(menuMotionSuspended&&!transition.active&&!groupMotion.active&&!backPrepared&&!softInput.visible&&
     focusNode>=0&&size_t(focusNode)<page().count&&focusable(focusNode,false))return true;
  for(const auto &item:menuScrollMotions)if(item.tracking&&(item.x.active()||item.y.active()))return true;
  if(menuIndicatorReady&&keyInput)for(const auto &axis:menuIndicatorAxes)if(axis.active())return true;
  return advancedMenuMoving();
}
void stepMenuMotion(uint32_t now) {
  if(!menuMotionEnabled())return;
  if(menuMotionPage!=&page())resetMenuMotion();
  if(transition.active||groupMotion.active||backPrepared||softInput.visible){
    if(!menuMotionSuspended){
      if(menuIndicatorReady)invalidatePageRect(menuIndicatorRect());
      menuIndicatorReady=menuClockReady=false;for(auto &item:menuScrollMotions)item.tracking=false;
      menuMotionSuspended=true;
    }
    return;
  }
  const bool resuming=menuMotionSuspended;menuMotionSuspended=false;
  if(resuming&&focusNode>=0&&size_t(focusNode)<page().count&&focusable(focusNode,false))revealMenuFocus(focusNode);
  const uint32_t elapsed=menuClockReady?std::min<uint32_t>(uint32_t(now-menuUpdatedAt),menuMotionConfig.maxDeltaMs):0;
  menuUpdatedAt=now;menuClockReady=true;
  if(focusNode<0||size_t(focusNode)>=page().count||!focusable(focusNode,false)){
    if(menuIndicatorReady)invalidatePageRect(menuIndicatorRect());
    menuIndicatorReady=menuClockReady=false;for(auto &item:menuScrollMotions)item.tracking=false;return;
  }
  const Rect before=menuIndicatorReady?menuIndicatorRect():Rect{};
  for(size_t at=0;at<menuScrollMotions.size();++at){
    auto &item=menuScrollMotions[at];if(!item.tracking)continue;
    const int node=item.node;item.x.step(elapsed,menuMotionConfig,menuMotionConfig.camera);item.y.step(elapsed,menuMotionConfig,menuMotionConfig.camera);
    int32_t mx,my;extent(node,mx,my);
    const int32_t sx=clamp32(item.x.pixels(),0,mx),sy=clamp32(item.y.pixels(),0,my);
    if(sx!=item.x.pixels())item.x.snap(sx);if(sy!=item.y.pixels())item.y.snap(sy);
    item.tracking=item.x.active()||item.y.active();
    menuApplyingScroll=true;scroll(node,sx,sy);menuApplyingScroll=false;
  }
  if(!keyInput){menuIndicatorReady=false;if(!menuMoving())menuClockReady=false;return;}
  aimMenuIndicator();
  if(menuIndicatorReady){
    for(unsigned i=0;i<4;++i)menuIndicatorAxes[i].step(elapsed,menuMotionConfig,i<2?menuMotionConfig.position:menuMotionConfig.size);
    const Rect after=menuIndicatorRect();
    if(before.x!=after.x||before.y!=after.y||before.width!=after.width||before.height!=after.height){
      invalidatePageRect(before);invalidatePageRect(after);
    }
  }
  stepAdvancedMenu(elapsed);
  if(!menuMoving())menuClockReady=false;
}
void snapshotMenuMotion() {
  snapshotMenuEnabled=menuMotionEnabled();snapshotMenuVisible=false;
  if(!snapshotMenuEnabled||!menuIndicatorReady||!keyInput||!input||!input->width||focusNode<0||
     size_t(focusNode)>=page().count||!focusable(focusNode,false)||transition.active||groupMotion.active||backPrepared||softInput.visible)return;
  snapshotMenuVisible=true;snapshotMenuConfig=menuMotionConfig;
  snapshotMenuRect=menuIndicatorRect();snapshotMenuClip=clip(page(),state(),focusNode);
}
void paintMenuIndicator(DirtyRect area,uint16_t *pixels,size_t stride) {
  const Rect r=snapshotMenuRect,c=intersect(intersect(r,snapshotMenuClip),{area.x,area.y,area.width,area.height});
  if(c.width<=0||c.height<=0)return;
  const int32_t radius=std::min<int32_t>(snapshotMenuConfig.radius,std::min(r.width,r.height)/2);
  const auto inside=[](int32_t x,int32_t y,Rect b,int32_t curve){
    if(b.width<=0||b.height<=0||x<b.x||y<b.y||int64_t(x)>=int64_t(b.x)+b.width||int64_t(y)>=int64_t(b.y)+b.height)return false;
    const int64_t dx=std::max<int64_t>(0,std::max<int64_t>(std::min<int64_t>(int64_t(b.x)+curve,int64_t(b.x)+b.width-curve-1)-x,int64_t(x)-std::max<int64_t>(int64_t(b.x)+curve,int64_t(b.x)+b.width-curve-1)));
    const int64_t dy=std::max<int64_t>(0,std::max<int64_t>(std::min<int64_t>(int64_t(b.y)+curve,int64_t(b.y)+b.height-curve-1)-y,int64_t(y)-std::max<int64_t>(int64_t(b.y)+curve,int64_t(b.y)+b.height-curve-1)));
    return dx*dx+dy*dy<=int64_t(curve)*curve;
  };
  const int32_t inset=std::min<int32_t>(snapshotFocusWidth,(std::min(r.width,r.height)+1)/2);
  const Rect inner{r.x+inset,r.y+inset,std::max<int32_t>(0,r.width-2*inset),std::max<int32_t>(0,r.height-2*inset)};
  const int32_t innerRadius=std::max<int32_t>(0,radius-inset);
  for(int32_t y=c.y;y<c.y+c.height;++y)for(int32_t x=c.x;x<c.x+c.width;++x){
    if(snapshotMenuConfig.indicator==MenuIndicator::Underline){if(y<r.y+r.height-std::max<int32_t>(1,snapshotFocusWidth))continue;}
    else if(snapshotMenuConfig.indicator==MenuIndicator::Corners){const int32_t arm=std::max<int32_t>(2,std::min(r.width,r.height)/3),stroke=std::max<int32_t>(1,snapshotFocusWidth);const bool side=x<r.x+stroke||x>=r.x+r.width-stroke,edge=y<r.y+stroke||y>=r.y+r.height-stroke;const bool nearX=x<r.x+arm||x>=r.x+r.width-arm,nearY=y<r.y+arm||y>=r.y+r.height-arm;if(!((side&&nearY)||(edge&&nearX)))continue;}
    else if(!inside(x,y,r,radius)||(snapshotMenuConfig.indicator==MenuIndicator::Outline&&inside(x,y,inner,innerRadius)))continue;
    uint16_t &pixel=pixels[size_t(y-area.y)*stride+size_t(x-area.x)];
    const uint16_t value=renderOrder==PixelOrder::Wire?rgb565_blend::fromWire(pixel):pixel;
    const uint16_t inverse=DisplayPolicy::foregroundOn(value);
    pixel=renderOrder==PixelOrder::Wire?rgb565_blend::toWire(inverse):inverse;
  }
}
