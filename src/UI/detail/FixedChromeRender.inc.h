// Included in View::Impl after the ordinary node/opaque-cover painters.
#if NICE_GFX_UI_FIXED_CHROME_PPA
detail::FixedChromeCache fixedChromeCache;
detail::FixedChromeKey fixedChromeKeyScratch;
bool foregroundCapture=false;int foregroundPrefix=-1;
struct FixedChromeApplied {Rect strips[2]{};uint8_t count=0;};
static bool fixedChromeInside(Rect inner,Rect outer){return inner.width>0&&inner.height>0&&inner.x>=outer.x&&inner.y>=outer.y&&
  int64_t(inner.x)+inner.width<=int64_t(outer.x)+outer.width&&int64_t(inner.y)+inner.height<=int64_t(outer.y)+outer.height;}
static bool fixedChromeContains(const FixedChromeApplied &applied,Rect clipped){
  for(uint8_t i=0;i<applied.count;++i)if(fixedChromeInside(clipped,applied.strips[i]))return true;return false;
}
static bool appendFixedChromeKey(detail::FixedChromeKey &key,const void *bytes,size_t count){
  return key.append(bytes,count);
}
template<class T> static bool appendFixedChromeKey(detail::FixedChromeKey &key,const T &value){return appendFixedChromeKey(key,&value,sizeof(value));}
bool fixedChromeTextOutsideStrip(uint16_t i,Rect strip){
  const auto &p=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;const auto &cache=*paintCache;
  const auto &n=p.nodes[i];const auto &v=values[i];const auto &g=cache.nodes[i];
  if((n.kind!=Kind::Text&&n.kind!=Kind::Label)||v.kind!=n.kind||v.dynamic||v.imageOverride||v.instrument||n.instrument||
     focusStyle(i)||snapshotPressed==i)return false;
  // Every state must be background-free. A border/focus/transform may paint
  // outside the text cell, so uncertain nodes retain the original rectangle.
  constexpr uint16_t transparent=Transparent|ActiveTransparent|DisabledTransparent;
  if((v.flags&transparent)!=transparent)return false;
  for(int at=i;at>=0;at=values[at].parent==NoParent?-1:values[at].parent)
    if((values[at].flags&Disabled)||values[at].dynamic||p.nodes[at].motion)return false;
  const auto &style=styleFor(p,values,i);
  if(style.borderWidth||!style.font||!style.fontSize||!style.font->height||g.textY<0||
     g.lineFirst>cache.lines.size()||g.lineCount>cache.lines.size()-g.lineFirst)return false;
  const Rect box=textBox(g.rect,n,style);
  if(box.width<=0||box.height<=0||!g.lineCount)return true;
  // text() uses the prepared content box/textY and primary font height for
  // every line. drawChar() resamples fallback glyphs to that primary height;
  // renderTextGlyph() clips AA and NoAA to [y,y+height*size), with no halo.
  // Keep the complete content width: alignment, glyph advance and fallback
  // widths cannot make this conservative vertical bound underestimate ink.
  const int64_t top=int64_t(box.y)+g.textY,bottom=std::min<int64_t>(int64_t(box.y)+box.height,
    top+int64_t(style.font->height)*style.fontSize*g.lineCount);
  if(top<INT32_MIN||top>INT32_MAX||bottom<INT32_MIN||bottom>INT32_MAX||bottom-top>INT32_MAX)return false;
  if(bottom<=top)return true;
  const Rect cells=intersect({box.x,int32_t(top),box.width,int32_t(bottom-top)},g.clip),overlap=intersect(cells,strip);
  if(overlap.width<=0||overlap.height<=0)return true;
#if NICE_GFX_UI_TEXT_MASK_NATIVE || NICE_GFX_UI_TEXT_MASK_PPA
  // Font names need not equal their storage height: e.g. a 16px font may
  // include extra descent rows. Resolve the existing exact coverage mask to
  // certify those rows as empty; never guess a tighter glyph height.
  const detail::TextMaskLayout layout{style.font,v.text.c_str(),cache.lines.data()+g.lineFirst,g.lineCount,
    box.width,box.height,g.textY,style.fontSize,style.textAlign,(v.flags&NoAntialias)==0};
  const auto hits=textMaskCache.hits(),builds=textMaskCache.builds(),rejects=textMaskCache.rejected();
  const uint32_t started=profilingClock?profilingClock():0;detail::TextMaskView mask;
  const bool resolved=textMaskCache.resolve(layout,mask);
  if(profilingClock)stats.textResolveMicros+=profilingClock()-started;
  stats.textMaskHits+=textMaskCache.hits()-hits;stats.textMaskBuilds+=textMaskCache.builds()-builds;
  stats.textMaskRejects+=textMaskCache.rejected()-rejects;stats.textMaskBytes=textMaskCache.bytes();
  if(!resolved)return false;if(mask.empty)return true;
  Rect ink;if(!mask.inkBounds(ink))return false;
  const int64_t x=int64_t(box.x)+ink.x,y=int64_t(box.y)+ink.y;
  if(x<INT32_MIN||x>INT32_MAX||y<INT32_MIN||y>INT32_MAX)return false;
  ink.x=int32_t(x);ink.y=int32_t(y);const Rect painted=intersect(intersect(ink,g.clip),strip);
  return painted.width<=0||painted.height<=0;
#else
  return false;
#endif
}
bool fixedChromeKey(int prefix,Rect strip,detail::FixedChromeKey &key){
  key.clear();const auto &p=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;const auto &cache=*paintCache;
  const PageDesc *identity=&p;
  if(!appendFixedChromeKey(key,identity)||!appendFixedChromeKey(key,prefix)||!appendFixedChromeKey(key,strip))return false;
  const auto *backdrop=media(p.nodes[prefix],values[prefix]);
  if(!backdrop||!appendFixedChromeKey(key,*backdrop))return false; // Deliberately omit the sampled background frame.
  bool any=false;
  for(uint16_t i=uint16_t(prefix+1);i<p.count;++i){const auto &g=cache.nodes[i];if(!g.visible||!g.alpha)continue;
    const Rect visible=intersect(g.rect,g.clip),intersection=intersect(visible,strip);if(intersection.width<=0||intersection.height<=0)continue;
    if(emptyContainerPaint(i))continue;
    if(!fixedChromeInside(visible,strip)&&fixedChromeTextOutsideStrip(i,strip))continue;
    const auto &n=p.nodes[i];const auto &v=values[i];
    if(!fixedChromeInside(visible,strip)||g.alpha!=255||v.kind!=n.kind||v.dynamic||v.imageOverride||v.instrument||n.instrument||
       n.motion||v.animating||v.animationPending||focusStyle(i)||snapshotPressed==i)return false;
    if(n.kind!=Kind::Container&&n.kind!=Kind::Media&&n.kind!=Kind::Shape&&n.kind!=Kind::Text&&n.kind!=Kind::Label)return false;
    for(int at=i;at>=0;at=values[at].parent==NoParent?-1:values[at].parent)
      if((values[at].flags&Disabled)||values[at].dynamic||values[at].animating||values[at].animationPending||p.nodes[at].motion)return false;
    const auto &style=styleFor(p,values,i);
    if(!appendFixedChromeKey(key,i)||!appendFixedChromeKey(key,n)||!appendFixedChromeKey(key,style)||
       !appendFixedChromeKey(key,g.rect)||!appendFixedChromeKey(key,g.clip)||!appendFixedChromeKey(key,g.alpha)||
       !appendFixedChromeKey(key,v.flags)||!appendFixedChromeKey(key,v.value)||!appendFixedChromeKey(key,v.scrollX)||
       !appendFixedChromeKey(key,v.scrollY)||!appendFixedChromeKey(key,v.mediaFrame)||!appendFixedChromeKey(key,v.motionRevision)||
       !appendFixedChromeKey(key,g.textY)||!appendFixedChromeKey(key,g.lineCount))return false;
    if(g.lineFirst>cache.lines.size()||g.lineCount>cache.lines.size()-g.lineFirst)return false;
    // The layout is semantic data. FrameTextLine can contain ABI padding;
    // serializing it would make an equivalent temporary snapshot a cold key.
    for(size_t line=0;line<g.lineCount;++line){const auto &text=cache.lines[g.lineFirst+line];
      if(!appendFixedChromeKey(key,text.begin)||!appendFixedChromeKey(key,text.end)||!appendFixedChromeKey(key,text.width))return false;}
    const char *text=v.text.c_str();size_t length=0;while(length<4096&&text[length])++length;if(length==4096)return false;
    if(!appendFixedChromeKey(key,length)||!appendFixedChromeKey(key,text,length+1))return false;
    if(style.font&&(!appendFixedChromeKey(key,*style.font)||(style.font->fallback&&!appendFixedChromeKey(key,*style.font->fallback))))return false;
    if(style.gradient&&!appendFixedChromeKey(key,*style.gradient))return false;
    if(n.shape&&(!appendFixedChromeKey(key,*n.shape)||(n.shape->count&&!appendFixedChromeKey(key,n.shape->points,size_t(n.shape->count)*sizeof(ShapePoint)))))return false;
    if(n.kind==Kind::Media){const auto *source=media(n,v);if(!source||mediaJpeg(*source)||source->frames!=1||v.mediaFrame||!appendFixedChromeKey(key,*source))return false;}
    any=true;
  }
  return any;
}
struct FixedChromeCapture {
  Impl *self;Rect strip;int prefix;DirtyRect outerArea;uint16_t *outerPixels;size_t outerCapacity,outerStride;Status restored=Status::Ok;
};
static Status captureFixedChrome(void *context,const detail::FixedChromeCache::WireSurface &surface,uint16_t){
  auto &request=*static_cast<FixedChromeCapture *>(context);auto &s=*request.self;
  struct Restore {
    Impl &s;FixedChromeCapture &request;RuntimeStats stats;bool capture,building,full,viewport,transition,input,aa;
    int prefix;PixelOrder order;const PageDesc *preview;const Surface *surface;const Surface *animation;
    FrameCache *paint;int32_t x,y,cursorX,cursorY;const MonoFont *font;uint8_t size,datum;uint32_t color,bg,bitmapFg,bitmapBg,decoderBuffer;uint8_t decoderState;
    Restore(Impl &owner,FixedChromeCapture &r):s(owner),request(r),stats(owner.stats),capture(owner.foregroundCapture),building(owner.buildingSurface),
      full(owner.renderingFullFrameSurface),viewport(owner.renderingViewport),transition(owner.snapshotTransition),input(owner.snapshotInput),aa(owner.canvas.getAntialiasing()),
      prefix(owner.foregroundPrefix),order(owner.renderOrder),preview(owner.rasterFrame.preview.page),surface(owner.snapshotSurface),animation(owner.snapshotAnimationSurface),
      paint(owner.paintCache),x(owner.paintOffsetX),y(owner.paintOffsetY),cursorX(owner.canvas.getCursorX()),cursorY(owner.canvas.getCursorY()),font(owner.canvas.getFont()),
      size(owner.canvas.textsize),datum(owner.canvas.textdatum),color(owner.canvas.textcolor),bg(owner.canvas.textbgcolor),bitmapFg(owner.canvas.bitmap_fg),bitmapBg(owner.canvas.bitmap_bg),
      decoderBuffer(owner.canvas.decoderBuffer),decoderState(owner.canvas.decoderState){
      s.foregroundCapture=true;s.foregroundPrefix=r.prefix;s.buildingSurface=true;s.renderingFullFrameSurface=true;s.renderingViewport=false;
      s.snapshotTransition=s.snapshotInput=false;s.renderOrder=PixelOrder::Wire;s.rasterFrame.preview.page=nullptr;s.snapshotSurface=nullptr;s.snapshotAnimationSurface=nullptr;s.paintOffsetX=s.paintOffsetY=0;
    }
    ~Restore(){
      s.foregroundCapture=capture;s.foregroundPrefix=prefix;s.buildingSurface=building;s.renderingFullFrameSurface=full;s.renderingViewport=viewport;
      s.snapshotTransition=transition;s.snapshotInput=input;s.renderOrder=order;s.rasterFrame.preview.page=preview;s.snapshotSurface=surface;s.snapshotAnimationSurface=animation;s.paintCache=paint;s.paintOffsetX=x;s.paintOffsetY=y;
      request.restored=s.canvas.attach(request.outerArea,request.outerPixels,request.outerCapacity,request.outerStride,order);
      s.canvas.setAntialiasing(aa);s.canvas.setFont(font);s.canvas.setTextSize(size);s.canvas.setCursor(cursorX,cursorY);
      s.canvas.textdatum=datum;s.canvas.textcolor=color;s.canvas.textbgcolor=bg;s.canvas.bitmap_fg=bitmapFg;s.canvas.bitmap_bg=bitmapBg;s.canvas.decoderBuffer=decoderBuffer;s.canvas.decoderState=decoderState;
      s.stats=stats; // Outer RenderTimer includes capture; avoid nested timing/count duplication.
    }
  };
  Status result;{Restore restore(s,request);result=render(&s,DirtyRect(request.strip.x,request.strip.y,request.strip.width,request.strip.height),surface.pixels,surface.stride);}
  return result==Status::Ok?request.restored:result;
}
Status paintFixedChrome(int prefix,const DirtyRect &area,uint16_t *pixels,size_t capacity,size_t stride,FixedChromeApplied &applied){
  applied=FixedChromeApplied{};
  if(foregroundCapture||buildingSurface||renderOrder!=PixelOrder::Native||!nativeOverlayTarget||stagingNativeTarget||renderingViewport||snapshotTransition||
     rasterFrame.preview.page||snapshotKeyboard.visible||displayPolicy.monochrome||!ppaBlendEnabled||!rgb565_ppa::Context::available()||paintOffsetX||paintOffsetY)return Status::Ok;
  const auto &p=*rasterFrame.current.page;const auto &cache=*paintCache;
  const DirtyRect full(0,0,p.width,p.height);const int background=opaqueCoverAt(cache,full,0,0);
  if(background!=prefix||background<0)return Status::Ok;
  const auto *bg=media(p.nodes[background],rasterFrame.current.values[background]);if(!bg||!mediaJpeg(*bg))return Status::Ok;
  Rect candidates[2]{};bool present[2]={false,false};
  for(uint16_t i=uint16_t(prefix+1);i<p.count;++i){const auto &n=p.nodes[i];const auto &v=rasterFrame.current.values[i];const auto &g=cache.nodes[i];
    if(n.kind!=Kind::Media||n.motion||v.imageOverride||v.dynamic||v.animating||v.animationPending||!g.visible||g.alpha!=255)continue;
    const auto *source=media(n,v);if(!source||!mediaColorKeyed(*source)||source->frames!=1||v.mediaFrame||g.rect.x||g.rect.width!=p.width||
      g.rect.height<=0||g.rect.height>p.height/3||g.rect.width!=source->width||g.rect.height!=source->height)continue;
    const bool top=g.rect.y==0,bottom=int64_t(g.rect.y)+g.rect.height==p.height;if(!top&&!bottom)continue;
    const size_t which=top?0:1;if(present[which])continue;candidates[which]=g.rect;present[which]=true;
  }
  auto &key=fixedChromeKeyScratch;
  for(size_t candidate=0;candidate<2;++candidate){if(!present[candidate])continue;const Rect strip=candidates[candidate],dirty=intersect(strip,{area.x,area.y,area.width,area.height});
    if(dirty.width<=0||dirty.height<=0)continue;
    if(!fixedChromeKey(prefix,strip,key)){++stats.fixedChromeRejects;continue;}
    FixedChromeCapture request{this,strip,prefix,area,pixels,capacity,stride};platform::OverlayView overlay;
    const auto hits=fixedChromeCache.hits(),misses=fixedChromeCache.misses(),builds=fixedChromeCache.builds();
    Status status=fixedChromeCache.resolve(key.data(),key.size(),strip.width,strip.height,(size_t(strip.width)+31)&~size_t(31),captureFixedChrome,&request,overlay);
    stats.fixedChromeCacheHits+=fixedChromeCache.hits()-hits;stats.fixedChromeCacheMisses+=fixedChromeCache.misses()-misses;stats.fixedChromeBuilds+=fixedChromeCache.builds()-builds;
    if(status==Status::Unsupported){++stats.fixedChromeRejects;continue;}if(status!=Status::Ok)return status;
    const size_t sx=size_t(dirty.x-strip.x),sy=size_t(dirty.y-strip.y);
    if(!ppaFade.canBlendOverlayIntoNative(*nativeOverlayTarget,dirty.x,dirty.y,overlay,sx,sy,dirty.width,dirty.height)){++stats.fixedChromeRejects;continue;}
    const uint32_t started=profilingClock?profilingClock():0;
    status=ppaFade.blendOverlayIntoNative(*nativeOverlayTarget,dirty.x,dirty.y,overlay,sx,sy,dirty.width,dirty.height);
    if(status==Status::Unsupported){++stats.fixedChromeRejects;continue;}if(status!=Status::Ok)return status;
    ++stats.fixedChromeBlits;++stats.ppaBlendCalls;stats.ppaBlendPixels+=uint64_t(dirty.width)*dirty.height;
    if(profilingClock)stats.ppaBlendMicros+=profilingClock()-started;
    applied.strips[applied.count++]=strip;
  }
  return Status::Ok;
}
#else
static constexpr bool foregroundCapture=false;
static constexpr int foregroundPrefix=-1;
#endif
