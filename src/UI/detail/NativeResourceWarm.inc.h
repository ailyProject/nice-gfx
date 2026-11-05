// Included inside View::Impl. Populate immutable native resource caches only;
// no refresh transaction, navigation, input dispatch, animation tick or JPEG
// decode is performed. The single UI owner calls this before enabling input.
static void *allocateNativeWarmScratch(size_t bytes){
#ifdef NICE_GFX_UI_NATIVE_PREWARM_ALLOCATOR
  return NICE_GFX_UI_NATIVE_PREWARM_ALLOCATOR(bytes);
#elif defined(CONFIG_IDF_TARGET_ESP32S31)
  return heap_caps_aligned_alloc(64,bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
  return detail::allocateAlignedMemory<64>(bytes);
#endif
}
static void freeNativeWarmScratch(void *memory){
#if defined(CONFIG_IDF_TARGET_ESP32S31)
  heap_caps_free(memory);
#else
  detail::freeAlignedMemory<64>(memory);
#endif
}
static bool nativeWarmMemoryAvailable(size_t required){
#ifdef NICE_GFX_UI_NATIVE_PREWARM_AVAILABLE_BYTES
  return NICE_GFX_UI_NATIVE_PREWARM_AVAILABLE_BYTES()>=required;
#elif defined(CONFIG_IDF_TARGET_ESP32S31)
  return heap_caps_get_free_size(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT)>=required&&
    heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)>=65536;
#else
  (void)required;return true;
#endif
}
struct NativeWarmScope {
  Impl &s;detail::FrameContext<State,MotionRuntime> saved;
  FrameCache *paint;const NativeFramebuffer *overlay,*staging;
  const Surface *surface,*preview,*animation;PixelOrder order;
  bool building,full,viewport,transition,input,keyboard,aa;int pressed,focus,group,back;
  Id focusId;size_t layers;int32_t x,y,cursorX,cursorY,vx,vy,vw,vh;bool datum;
  const MonoFont *font;uint8_t size,textDatum;uint32_t color,bg,bitmapFg,bitmapBg,decoderBuffer;uint8_t decoderState;
  Status decode;
  NativeWarmScope(Impl &owner,const NativeFramebuffer &target):s(owner),saved(std::move(owner.rasterFrame)),
    paint(owner.paintCache),overlay(owner.nativeOverlayTarget),staging(owner.stagingNativeTarget),
    surface(owner.snapshotSurface),preview(owner.previewSurface),animation(owner.snapshotAnimationSurface),order(owner.renderOrder),
    building(owner.buildingSurface),full(owner.renderingFullFrameSurface),viewport(owner.renderingViewport),transition(owner.snapshotTransition),
    input(owner.snapshotInput),keyboard(owner.snapshotKeyboard.visible),aa(owner.canvas.getAntialiasing()),
    pressed(owner.snapshotPressed),focus(owner.snapshotFocus),group(owner.snapshotGroupIndex),back(owner.snapshotBackOffset),
    focusId(owner.snapshotFocusId),layers(owner.snapshotAnimationLayerCount),x(owner.paintOffsetX),y(owner.paintOffsetY),
    cursorX(owner.canvas.getCursorX()),cursorY(owner.canvas.getCursorY()),vx(owner.canvas.getViewportX()),vy(owner.canvas.getViewportY()),
    vw(owner.canvas.getViewportWidth()),vh(owner.canvas.getViewportHeight()),datum(owner.canvas.getViewportDatum()),font(owner.canvas.getFont()),
    size(owner.canvas.textsize),textDatum(owner.canvas.textdatum),color(owner.canvas.textcolor),bg(owner.canvas.textbgcolor),
    bitmapFg(owner.canvas.bitmap_fg),bitmapBg(owner.canvas.bitmap_bg),decoderBuffer(owner.canvas.decoderBuffer),decoderState(owner.canvas.decoderState),decode(owner.mediaDecodeStatus){
    s.rasterFrame.current.page=s.rasterFrame.preview.page=nullptr;
    s.paintCache=&s.rasterFrame.current.geometry;s.nativeOverlayTarget=&target;s.stagingNativeTarget=nullptr;
    s.snapshotSurface=s.previewSurface=s.snapshotAnimationSurface=nullptr;s.snapshotAnimationLayerCount=0;
    s.renderOrder=PixelOrder::Native;s.buildingSurface=false;s.renderingFullFrameSurface=true;s.renderingViewport=false;
    s.snapshotTransition=s.snapshotInput=s.snapshotKeyboard.visible=false;
    s.snapshotPressed=s.snapshotFocus=s.snapshotGroupIndex=-1;s.snapshotFocusId=0;s.snapshotBackOffset=0;s.paintOffsetX=s.paintOffsetY=0;
  }
  ~NativeWarmScope(){
    s.canvas.detach();s.rasterFrame=std::move(saved);s.paintCache=paint;s.nativeOverlayTarget=overlay;s.stagingNativeTarget=staging;
    s.snapshotSurface=surface;s.previewSurface=preview;s.snapshotAnimationSurface=animation;s.snapshotAnimationLayerCount=layers;
    s.renderOrder=order;s.buildingSurface=building;s.renderingFullFrameSurface=full;s.renderingViewport=viewport;s.snapshotTransition=transition;
    s.snapshotInput=input;s.snapshotKeyboard.visible=keyboard;s.snapshotPressed=pressed;s.snapshotFocus=focus;s.snapshotFocusId=focusId;
    s.snapshotGroupIndex=group;s.snapshotBackOffset=back;s.paintOffsetX=x;s.paintOffsetY=y;s.mediaDecodeStatus=decode;
    s.canvas.setAntialiasing(aa);s.canvas.setFont(font);s.canvas.setTextSize(size);s.canvas.setCursor(cursorX,cursorY);
    s.canvas.textdatum=textDatum;s.canvas.textcolor=color;s.canvas.textbgcolor=bg;s.canvas.bitmap_fg=bitmapFg;s.canvas.bitmap_bg=bitmapBg;
    s.canvas.decoderBuffer=decoderBuffer;s.canvas.decoderState=decoderState;s.canvas.setViewport(vx,vy,vw,vh,datum);
  }
};
#if NICE_GFX_UI_FIXED_CHROME_PPA
Status warmHiddenNativeChrome(int prefix,const DirtyRect &full,uint16_t *pixels,size_t capacity,size_t stride){
  if(prefix<0)return Status::Ok;
  const auto &p=*rasterFrame.current.page;const auto &values=rasterFrame.current.values;auto &cache=rasterFrame.current.geometry;
  Rect strips[2]{};bool present[2]={false,false};
  for(uint16_t i=uint16_t(prefix+1);i<p.count;++i){const auto &n=p.nodes[i];const auto &v=values[i];const auto &g=cache.nodes[i];
    if(n.kind!=Kind::Media||n.motion||v.imageOverride||v.dynamic||v.animating||v.animationPending||!g.visible||g.alpha!=255)continue;
    const auto *source=media(n,v);if(!source||!mediaColorKeyed(*source)||source->frames!=1||v.mediaFrame||g.rect.x||g.rect.width!=p.width||
      g.rect.height<=0||g.rect.height>p.height/3||g.rect.width!=source->width||g.rect.height!=source->height)continue;
    const bool top=g.rect.y==0,bottom=int64_t(g.rect.y)+g.rect.height==p.height;if(!top&&!bottom)continue;
    const size_t which=top?0:1;if(!present[which]){strips[which]=g.rect;present[which]=true;}
  }
  bool changed=false;
  for(uint16_t i=uint16_t(prefix+1);i<p.count;++i){auto &g=cache.nodes[i];if(!g.visible||!g.alpha)continue;
    // A parent-before-child geometry table lets the hidden pose propagate
    // without new storage or changes to State/motion clocks.
    if(values[i].parent!=NoParent&&!cache.nodes[values[i].parent].alpha){g.alpha=0;changed=true;continue;}
    if(emptyContainerPaint(i))continue;
    const Rect visible=intersect(g.rect,g.clip);if(visible.width<=0||visible.height<=0)continue;
    for(size_t strip=0;strip<2;++strip){if(!present[strip])continue;const Rect overlap=intersect(visible,strips[strip]);
      if(overlap.width<=0||overlap.height<=0||fixedChromeInside(visible,strips[strip])||fixedChromeTextOutsideStrip(i,strips[strip]))continue;
      // Enter animations can hide a body whose final text/shape crosses a
      // strip. Warm that exact content-hidden key as well as the terminal key.
      g.alpha=0;changed=true;break;
    }
  }
  if(!changed)return Status::Ok;
  FixedChromeApplied hidden;const Status result=paintFixedChrome(prefix,full,pixels,capacity,stride,hidden);
  prepareCache(p,values,cache); // Restore the terminal temporary geometry.
  return result;
}
#endif
bool warmNativeResources(){
#if !NICE_GFX_UI_SHAPE_NATIVE_SPANS && !NICE_GFX_UI_TEXT_MASK_NATIVE && !NICE_GFX_UI_FIXED_CHROME_PPA
  return false;
#else
  if(!project||!project->count||project->count>16||processing||updateDepth||down||refresh->busy()||canvas.attached()||
     groupMotion.active||backAnimation.active||displayPolicy.monochrome)return false;
#if defined(CONFIG_IDF_TARGET_ESP32S31)
  if(xPortInIsrContext())return false;
#endif
  constexpr size_t PixelCap=4u*1024u*1024u,MetadataCap=1024u*1024u,TextCap=256u*1024u,Margin=1024u*1024u;
  size_t maximumNodes=0,maximumLines=0,maximumBytes=0,totalText=0;
  for(uint16_t page=0;page<project->count;++page){const auto &p=project->pages[page];
    if(!p.count||p.count>4096||!p.width||!p.height||p.width!=display.width()||p.height!=display.height())return false;
    const size_t stride=(size_t(p.width)+31)&~size_t(31);
    if(size_t(p.height)>PixelCap/2/stride)return false;
    maximumBytes=std::max(maximumBytes,stride*p.height*2);maximumNodes=std::max(maximumNodes,size_t(p.count));size_t lines=0;
    for(uint16_t i=0;i<p.count;++i){const auto &n=p.nodes[i];const char *text=n.kind==Kind::InputField&&n.inputField?n.inputField->value:n.text;
      if(!text)text="";size_t length=0;while(length<=4096&&text[length])++length;if(length>4096||length>TextCap-totalText)return false;totalText+=length;
      // Auto/flex layout can grow beyond the authored rectangle. Every line
      // consumes at least one byte or one explicit line break; length+1 is a
      // layout-independent upper bound, including empty/input text nodes.
      if(hasText(n.kind))lines+=length+1;
    }
    maximumLines=std::max(maximumLines,lines);
  }
  const size_t words=(maximumNodes+31)/32;
  const size_t metadata=maximumNodes*(sizeof(State)+sizeof(Geometry))+maximumLines*sizeof(TextLine)+words*32*sizeof(uint32_t);
  if(metadata>MetadataCap)return false;
  size_t allowance=0,resident=0;
#if NICE_GFX_UI_SHAPE_OVERLAY_PPA || NICE_GFX_UI_SHAPE_NATIVE_SPANS
  allowance+=detail::ShapeOverlayCache::DefaultBudget;resident+=shapeOverlayCache.bytes();
#endif
#if NICE_GFX_UI_TEXT_MASK_PPA || NICE_GFX_UI_TEXT_MASK_NATIVE
  allowance+=detail::TextMaskCache::DefaultBudget;resident+=textMaskCache.bytes();
#endif
#if NICE_GFX_UI_FIXED_CHROME_PPA
  allowance+=detail::FixedChromeCache::DefaultBudget+detail::FixedChromeCache::MaxScratchBytes+detail::FixedChromeKey::Limit;
  resident+=fixedChromeCache.bytes()+fixedChromeKeyScratch.capacity();
#endif
  const size_t required=maximumBytes+128+metadata+Margin+(allowance>resident?allowance-resident:0);
  if(!nativeWarmMemoryAvailable(required))return false;
  void *allocation=allocateNativeWarmScratch(maximumBytes+128);if(!allocation)return false;
  struct ScratchGuard {void *memory;~ScratchGuard(){freeNativeWarmScratch(memory);}} scratch{allocation};
  if(reinterpret_cast<uintptr_t>(allocation)&63)return false;
  auto *base=static_cast<uint16_t *>(allocation);auto *pixels=base+32;
  for(size_t i=0;i<(maximumBytes+128)/2;++i)base[i]=0xa55a;
  NativeFramebuffer target{};bool complete=true;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  try {
#endif
    NativeWarmScope scope(*this,target);
    auto &values=rasterFrame.current.values;auto &cache=rasterFrame.current.geometry;
    values.reserve(maximumNodes);cache.nodes.reserve(maximumNodes);cache.lines.reserve(maximumLines);cache.bands.reserve(words*32);
    // End on the mounted initial route so bounded resource LRUs retain its
    // defaults even when a larger project cannot retain every optional key.
    for(uint16_t pass=0;pass<project->count;++pass){const uint16_t at=uint16_t((project->initial+1+pass)%project->count);const auto &p=project->pages[at];
      target={pixels,p.width,p.height,(size_t(p.width)+31)&~size_t(31),((size_t(p.width)+31)&~size_t(31))*p.height*2};
      values.clear();values.resize(p.count);rasterFrame.current.page=&p;
      for(uint16_t i=0;i<p.count;++i){const auto &n=p.nodes[i];auto &v=values[i];v.parent=n.parent;v.kind=n.kind;v.flags=n.flags;v.value=n.value;
        v.text.init(n.kind==Kind::InputField&&n.inputField?n.inputField->value:n.text);v.styleIndex=cacheCompiledStyle(n.style);v.opacity=255;
        v.switchPosition=v.switchFrom=v.switchTo=v.value?1000:0;
      }
      // This temporary compiled-default pose never enters the live page table.
      prepareCache(p,values,cache);paintCache=&cache;
      const DirtyRect full(0,0,p.width,p.height);
      if(canvas.attach(full,pixels,target.bytes/2,target.stride,PixelOrder::Native)!=Status::Ok)return false;
#if NICE_GFX_UI_FIXED_CHROME_PPA
      FixedChromeApplied chrome;const int prefix=opaqueCoverAt(cache,full,0,0);
      if(prefix>=0){const auto status=paintFixedChrome(prefix,full,pixels,target.bytes/2,target.stride,chrome);
        if(status!=Status::Ok)return false;}
#endif
      for(uint16_t i=0;i<p.count;++i){const auto &g=cache.nodes[i];if(!g.visible||!g.alpha)continue;const Rect clipped=intersect(g.rect,g.clip);
        if(clipped.width<=0||clipped.height<=0)continue;
#if NICE_GFX_UI_FIXED_CHROME_PPA
        if(fixedChromeContains(chrome,clipped))continue;
#endif
        Status result=Status::Ok;
        if(p.nodes[i].kind==Kind::Shape)result=paintShapeNative(i,clipped,g.alpha);
        else if(p.nodes[i].kind==Kind::Text||p.nodes[i].kind==Kind::Label)result=paintTextMaskNative(i,clipped,g.alpha);
        if(result!=Status::Ok)complete=false;
      }
#if NICE_GFX_UI_FIXED_CHROME_PPA
      if(warmHiddenNativeChrome(prefix,full,pixels,target.bytes/2,target.stride)!=Status::Ok)return false;
#endif
      canvas.detach();
      for(size_t row=0;row<target.height;++row)for(size_t col=target.width;col<target.stride;++col)
        if(pixels[row*target.stride+col]!=0xa55a)return false;
    }
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  }catch(const std::bad_alloc &){return false;}
#endif
  for(size_t i=0;i<32;++i)if(base[i]!=0xa55a||base[32+maximumBytes/2+i]!=0xa55a)return false;
  return complete;
#endif
}
