// Included inside View::Impl after prepareText(). Resource preparation only:
// no state/frame/context swap, rendering, refresh, input, event or clock tick.
// prepareText's Cache parameter is templated so these bounded stack lines use
// the identical layout implementation without allocating a geometry/page cache.
struct PanelPrepareLines {
  static constexpr size_t Capacity=64;
  detail::FrameTextLine values[Capacity];size_t count=0;bool overflow=false;
  size_t size()const{return count;}
  const detail::FrameTextLine *data()const{return values;}
  void clear(){count=0;overflow=false;}
  void push_back(const detail::FrameTextLine &value){if(count<Capacity)values[count++]=value;else overflow=true;}
};
struct PanelPrepareLayout {PanelPrepareLines lines;};
void prepareSystemPanelTextResources(int root) {
#if NICE_GFX_UI_SYSTEM_PANEL_TEXT_PREPARE && NICE_GFX_UI_TEXT_MASK_NATIVE
  if(!project||history.empty()||displayPolicy.monochrome||root<0)return;
  const auto &p=page();const auto &values=state();
  if(root>=p.count||p.count>512||values.size()!=p.count||
     !std::binary_search(systemPanelRoots.begin(),systemPanelRoots.end(),p.nodes[root].id))return;
  constexpr size_t MaxMasks=32,PerMaskBytes=32768,TotalMaskBytes=128*1024;
  constexpr uint32_t BudgetMicros=12000;
  const uint32_t started=workClock();
  const auto priorHits=textMaskCache.hits(),priorBuilds=textMaskCache.builds(),priorRejects=textMaskCache.rejected();
  ++stats.systemPanelTextPrepareCalls;
  struct Complete {
    Impl &s;uint32_t started;uint64_t hits,builds,rejects;
    ~Complete(){
      const auto dh=s.textMaskCache.hits()-hits,db=s.textMaskCache.builds()-builds,dr=s.textMaskCache.rejected()-rejects;
      s.stats.systemPanelTextPrepareHits+=dh;s.stats.systemPanelTextPrepareBuilds+=db;
      s.stats.textMaskHits+=dh;s.stats.textMaskBuilds+=db;s.stats.textMaskRejects+=dr;
      s.stats.textMaskBytes=s.textMaskCache.bytes();
      if(s.profilingClock)s.stats.systemPanelTextPrepareMicros+=uint32_t(s.workClock()-started);
    }
  } complete{*this,started,priorHits,priorBuilds,priorRejects};
  PanelPrepareLayout layout;size_t attempted=0,bytes=0;
  for(uint16_t i=0;i<p.count;++i){
    if(attempted>=MaxMasks||bytes>=TotalMaskBytes||uint32_t(workClock()-started)>=BudgetMicros){++stats.systemPanelTextPrepareBudgetStops;break;}
    if(i==root||!descendant(i,root))continue;
    const auto &n=p.nodes[i];const auto &v=values[i];
    const bool caption=n.kind==Kind::Button;
    if(!caption&&n.kind!=Kind::Text&&n.kind!=Kind::Label)continue;
    if((!caption&&!(v.flags&Transparent))||n.instrument||v.instrument||n.id==snapshotMarqueeId)continue;
    bool eligible=true;
    for(int at=i;at>=0;at=values[at].parent==NoParent?-1:values[at].parent){
      // Ignore only the panel's own Hidden flag; independently hidden content
      // is not going to paint. Ancestors above this global root do not hide it.
      if((at!=root&&descendant(at,root)&&(values[at].flags&Hidden))||(values[at].flags&Disabled)){eligible=false;break;}
      if(p.nodes[at].motion){const auto &pose=motionPose(p,values,at);
        if(at!=i||pose.scale!=1024||pose.rotation%360||pose.reveal!=1024){eligible=false;break;}}
    }
    if(!eligible)continue;
    const auto &style=styleFor(p,values,i);
    if(!style.font||!style.font->bitmap||!style.font->height||!style.font->width||!style.fontSize||style.fontSize>7||
       size_t(style.font->height)*style.fontSize>128||size_t(style.font->width)*style.fontSize>128||
       (!caption&&(style.radius||style.gradient||style.borderWidth)))continue;
    const char *text=v.text.c_str();size_t length=0;while(length<=2048&&text[length])++length;
    if(!length||length>2048)continue;
    Geometry geometry{};geometry.rect=rect(p,values,i);
    const Rect box=textBox(geometry.rect,n,style);
    if(box.width<=0||box.height<=0||box.width>2048||box.height>512)continue;
    const size_t pitch=(size_t(box.width)+63)&~size_t(63),maskBytes=pitch*size_t(box.height);
    const size_t lineHeight=size_t(style.font->height)*style.fontSize;
    if(maskBytes>PerMaskBytes||maskBytes>TotalMaskBytes-bytes||
       (size_t(box.height)+lineHeight-1)/lineHeight>PanelPrepareLines::Capacity)continue;
    layout.lines.clear();prepareText(layout,geometry,n,v,style);
    if(layout.lines.overflow||!geometry.lineCount||geometry.ellipsisCount||geometry.lineCount>layout.lines.size())continue;
    detail::TextMaskView mask;
    const detail::TextMaskLayout key{style.font,text,layout.lines.data(),geometry.lineCount,
      box.width,box.height,geometry.textY,style.fontSize,style.textAlign,(v.flags&NoAntialias)==0};
    ++attempted;bytes+=maskBytes;++stats.systemPanelTextPrepareNodes;stats.systemPanelTextPreparePixels+=maskBytes;
    if(!textMaskCache.resolve(key,mask))++stats.systemPanelTextPrepareRejects;
  }
#else
  (void)root;
#endif
}
