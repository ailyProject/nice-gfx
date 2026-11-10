// Keyed Native565 affine pair sampler; default ON for ESP32-P4, OFF elsewhere; explicit overrides are retained.
#ifndef NICE_GFX_UI_KEYED_AFFINE_PAIRS
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_UI_KEYED_AFFINE_PAIRS 1
#else
#define NICE_GFX_UI_KEYED_AFFINE_PAIRS 0
#endif
#endif
#define NICE_GFX_UI_KEYED_AFFINE_PAIRS_VERSION 1
#ifndef NICE_GFX_UI_KEYED_AFFINE_ROW_MAP
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_UI_KEYED_AFFINE_ROW_MAP 1
#else
#define NICE_GFX_UI_KEYED_AFFINE_ROW_MAP 0
#endif
#endif
#define NICE_GFX_UI_KEYED_AFFINE_ROW_MAP_VERSION 1
// Included inside View::Impl. Local visual leaves sample their source directly
// into the destination tile. No layer bitmap, mask buffer or frame allocation.
#ifndef NICE_GFX_S31_MOTION_TEXTURE_BUDGET
#define NICE_GFX_S31_MOTION_TEXTURE_BUDGET 0
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31) && NICE_GFX_S31_MOTION_TEXTURE_BUDGET > 0
// Optional per-View immutable flash texture copies. Only synchronous motion
// kernels borrow these pixels; neither State nor Refresh retains a cache pointer.
struct MotionTextureCache {
  struct Slot {
    const uint8_t *source=nullptr;uint16_t *pixels=nullptr;
    size_t sourceBytes=0,sourceStride=0,allocated=0;
    uint16_t width=0,height=0;PixelOrder order=PixelOrder::Wire;
    uint64_t used=0;
  };
  Slot slots[8];size_t resident=0;uint64_t clock=0;
  MotionTextureCache()=default;
  MotionTextureCache(const MotionTextureCache &)=delete;
  MotionTextureCache &operator=(const MotionTextureCache &)=delete;
  ~MotionTextureCache(){for(auto &slot:slots)if(slot.pixels)heap_caps_free(slot.pixels);}
  size_t bytes() const {return resident;}
  void discard(Slot &slot){if(slot.pixels){resident-=slot.allocated;heap_caps_free(slot.pixels);}slot=Slot{};}
  const uint16_t *native(const uint8_t *frame,size_t sourceBytes,size_t stride,
                         uint16_t width,uint16_t height,PixelOrder order){
    const size_t budget=size_t(NICE_GFX_S31_MOTION_TEXTURE_BUDGET),rowBytes=size_t(width)*2;
    if(!frame||!width||!height||stride<rowBytes||size_t(width)>SIZE_MAX/height/2||
       size_t(height-1)>(SIZE_MAX-rowBytes)/stride)return nullptr;
    const size_t needed=size_t(width)*height*2,extent=size_t(height-1)*stride+rowBytes;
    const uintptr_t address=reinterpret_cast<uintptr_t>(frame);
    if(needed>budget||sourceBytes<extent||sourceBytes>UINTPTR_MAX-address||
       !esp_ptr_in_drom(frame)||!esp_ptr_in_drom(reinterpret_cast<const void *>(address+sourceBytes-1)))return nullptr;
    for(auto &slot:slots)if(slot.pixels&&slot.source==frame&&slot.sourceBytes==sourceBytes&&
       slot.sourceStride==stride&&slot.width==width&&slot.height==height&&slot.order==order){slot.used=++clock;return slot.pixels;}
    Slot *destination=nullptr;
    for(;;){
      for(auto &slot:slots)if(!slot.pixels){destination=&slot;break;}
      if(destination&&resident<=budget-needed)break;
      Slot *oldest=nullptr;for(auto &slot:slots)if(slot.pixels&&(!oldest||slot.used<oldest->used))oldest=&slot;
      if(!oldest)return nullptr;discard(*oldest);destination=nullptr;
    }
    uint16_t *pixels=static_cast<uint16_t *>(heap_caps_malloc(needed,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
    if(!pixels)return nullptr;
    for(size_t y=0;y<height;++y)memcpy(pixels+y*width,frame+y*stride,rowBytes);
    if(order==PixelOrder::Wire){
      size_t i=0,count=size_t(width)*height;
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      typedef uint32_t TextureWord __attribute__((__may_alias__));auto *pairs=reinterpret_cast<TextureWord *>(pixels);
      for(;i+1<count;i+=2,++pairs)*pairs=rgb565_blend::swapPairBytes(*pairs);
#endif
      for(;i<count;++i)pixels[i]=rgb565_blend::fromWire(pixels[i]);
    }
    destination->source=frame;destination->pixels=pixels;destination->sourceBytes=sourceBytes;
    destination->sourceStride=stride;destination->allocated=needed;destination->width=width;
    destination->height=height;destination->order=order;destination->used=++clock;resident+=needed;
    return pixels;
  }
};
MotionTextureCache motionTextureCache;
#endif
static bool motionRoundPixel(int32_t x,int32_t y,int32_t width,int32_t height,int32_t radius) {
  if(x<0||y<0||x>=width||y>=height)return false;
  if(!radius)return true;
  const int32_t cx=x<radius?radius:x>=width-radius?width-radius:0;
  const int32_t cy=y<radius?radius:y>=height-radius?height-radius:0;
  if(!cx||!cy)return true;
  const int64_t dx=int64_t(x)*2+1-2*cx,dy=int64_t(y)*2+1-2*cy;
  return dx*dx+dy*dy<=int64_t(radius)*radius*4;
}
static uint16_t motionMediaColor(const uint8_t *p,uint8_t bytesPerPixel,bool ram,PixelOrder order) {
  if((bytesPerPixel&0x7f)==2){
    if(ram&&order==PixelOrder::Native){uint16_t color;memcpy(&color,p,2);return color;}
    return ram?uint16_t(uint16_t(p[0])<<8|p[1]):uint16_t(uint16_t(pgm_read_byte(p))<<8|pgm_read_byte(p+1));
  }
  const uint8_t b=ram?*p:pgm_read_byte(p);
  return uint16_t((((b>>5)*31+3)/7)<<11|(((b>>2)&7)*9)<<5|((b&3)*31+1)/3);
}
// Every visited point, including the otherwise-unused final loop increments,
// lies in the affine rectangle [0,width] x [0,height]. Evaluate all four
// corners in int64 before permitting any int32 conversion or arithmetic.
static bool motionRange32(int64_t origin,int32_t stepX,int32_t stepY,int32_t width,int32_t height) {
  const int64_t x=int64_t(stepX)*width,y=int64_t(stepY)*height;
  const int64_t corners[4]={origin,origin+x,origin+y,origin+x+y};
  for(unsigned i=0;i<4;++i)if(corners[i]<INT32_MIN||corners[i]>INT32_MAX)return false;
  return width>=0&&height>=0;
}

#ifndef NICE_GFX_UI_KEYED_PIE
#define NICE_GFX_UI_KEYED_PIE 0
#endif
#if NICE_GFX_UI_KEYED_PIE
struct KeyedPieUiStats {
  nice_gfx::platform::rgb565_p4_blend::SafeStats safe;
  uint64_t rows=0,eligibleBlocks=0,acceptedBlocks=0,fallbackBlocks=0,keyBlocks=0,peelPixels=0,pathMicros=0;
#if NICE_GFX_UI_AXIS_MEDIA_PIE
  uint64_t axisDraws=0,axisRuns=0,axisDirectBlocks=0,axisScratchBlocks=0,axisAcceptedBlocks=0,axisFallbackBlocks=0;
#endif
};
mutable KeyedPieUiStats keyedPieUi;
#endif
struct MotionMedia32Tile {
  Rect clipped;DirtyRect area;uint16_t *pixels;size_t stride;
  int32_t localX,localY,textureX,textureY;
  int32_t localXX,localXY,localYX,localYY,textureXX,textureXY,textureYX,textureYY;
  uint32_t left,top,maskWidth,maskHeight,sourceWidth,sourceHeight;
  const uint8_t *frame;size_t sourceStride;uint8_t bytesPerPixel,alpha;bool ram;PixelOrder order;uint16_t colorKey;
#if NICE_GFX_UI_KEYED_PIE
  KeyedPieUiStats *pieUi=nullptr;uint32_t (*pieClock)()=nullptr;
#endif
#if NICE_GFX_UI_AXIS_MEDIA_CACHE
  detail::MotionAxisCache *axisCache=nullptr;const void *immutableFrameIdentity=nullptr;
#if NICE_GFX_UI_AXIS_MEDIA_FULL_FOOTPRINT
  Rect fullFootprint{};
#endif
#endif
#if NICE_GFX_UI_AXIS_MEDIA_PPA && NICE_GFX_UI_AXIS_MEDIA_CACHE
  void *axisPpaContext=nullptr;
  Status (*axisPpa)(void *,const detail::MotionAxisCache::Texture &,const DirtyRect &,size_t,size_t,uint8_t,uint16_t)=nullptr;
  Status *axisPpaError=nullptr;
#endif
};
#if NICE_GFX_UI_AXIS_MEDIA_PPA && NICE_GFX_UI_AXIS_MEDIA_CACHE
Status axisPpaError=Status::Ok;
bool axisPpaPaintDomain(const DirtyRect &area,const uint16_t *pixels,size_t stride)const{
  const auto *target=nativeOverlayTarget;
  return nativeGpuPaintAllowed()&&ppaBlendEnabled&&rgb565_ppa::Context::available()&&target&&target->fencedAtomicMipiRmw&&
    renderOrder==PixelOrder::Native&&!displayPolicy.monochrome&&!buildingSurface&&!foregroundCapture&&
    !nativeCacheRepairing&&!stagingNativeTarget&&!snapshotTransition&&!rasterFrame.preview.page&&
    !renderingViewport&&!groupMotion.active&&!groupMotion.prepared&&!snapshotInput&&!snapshotKeyboard.visible&&
    !snapshotAnimationSurface&&!snapshotAnimationLayerCount&&!viewportOverlayLevels&&
    area.x>=0&&area.y>=0&&area.width>0&&area.height>0&&
    int64_t(area.x)+area.width<=target->width&&int64_t(area.y)+area.height<=target->height&&
    stride==target->stride&&pixels==target->pixels+size_t(area.y)*stride+size_t(area.x);
}
static Status paintAxisPpa(void *context,const detail::MotionAxisCache::Texture &texture,const DirtyRect &region,
    size_t sourceX,size_t sourceY,uint8_t alpha,uint16_t key){
  auto &s=*static_cast<Impl *>(context);if(!s.nativeGpuPaintAllowed())return Status::Unsupported;const auto *target=s.nativeOverlayTarget;
  if(!alpha||!target||!target->fencedAtomicMipiRmw||!texture.ppaPlaneFor(s.motionAxisCache)||
     region.width<=0||region.height<=0||region.pixels()<NICE_GFX_UI_AXIS_MEDIA_PPA_MIN_PIXELS)return Status::Unsupported;
  platform::OverlayView source;source.data=reinterpret_cast<const uint8_t*>(texture.pixels);
  source.width=texture.width;source.height=texture.height;source.stride=texture.stride;source.bytes=texture.pixelBytes;
  source.format=platform::OverlayFormat::Rgb565;source.alpha=alpha;source.colorKey=true;source.key=key;
  ++s.stats.axisPpaAttempts;const uint32_t started=s.profilingClock?s.profilingClock():0;
  const Status result=s.ppaFade.blendOverlayIntoNative(*target,region.x,region.y,source,sourceX,sourceY,region.width,region.height);
  if(s.profilingClock)s.stats.axisPpaMicros+=s.profilingClock()-started;
  if(result==Status::Ok){++s.stats.axisPpaBlits;s.stats.axisPpaPixels+=region.pixels();++s.stats.keyedGpuBlits;
    ++s.stats.ppaBlendCalls;s.stats.ppaBlendPixels+=region.pixels();}
  else if(result==Status::Unsupported)++s.stats.axisPpaRejects;
  else ++s.stats.axisPpaErrors;
  return result;
}
#endif
#if NICE_GFX_UI_AXIS_MEDIA_PIE && NICE_GFX_UI_KEYED_PIE && NICE_GFX_UI_AXIS_MEDIA_CACHE && NICE_GFX_UI_AXIS_MEDIA_RUNS
struct AxisMediaPieCounts {uint64_t runs=0,eligible=0,accepted=0,fallback=0,direct=0,scratch=0,peeled=0;};
// Only trusted cached runs enter: every source texel is visible, contiguous
// Native565 RAM. Full blocks never cross a run, row, ROI or transparent hole.
static
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("O3")))
#elif defined(__GNUC__)
__attribute__((noinline))
#endif
NICE_GFX_UI_MEDIA_SHAPE_IRAM_ATTR void motionAxisMediaPieRun(uint16_t *destination,const uint16_t *source,size_t count,uint8_t alpha,
    const nice_gfx::platform::rgb565_p4_blend::Plan &plan,
    nice_gfx::platform::rgb565_p4_blend::SafeStats &safe,AxisMediaPieCounts &stats){
  // This isolated noinline helper owns128 bytes of stack, not every media draw.
  alignas(16) uint16_t scratch[64];
  const auto cpu=[&](uint16_t *out,const uint16_t *front,size_t pixels) NICE_GFX_UI_MEDIA_SHAPE_IRAM_ATTR {
    if((reinterpret_cast<uintptr_t>(out)&3)&&pixels){const uint16_t color=*front++;if(color!=*out)*out=rgb565_blend::color(color,*out,alpha);++out;--pixels;}
    typedef uint32_t AxisPiePair __attribute__((__may_alias__));
    while(pixels>=2){uint32_t colors;memcpy(&colors,front,4);auto *pair=reinterpret_cast<AxisPiePair*>(out);const uint32_t under=*pair;if(colors!=under)*pair=rgb565_blend::colorPair(colors,under,alpha);out+=2;front+=2;pixels-=2;}
    if(pixels){const uint16_t color=*front;if(color!=*out)*out=rgb565_blend::color(color,*out,alpha);}
  };
  const size_t peel=std::min<size_t>(count,(16-(reinterpret_cast<uintptr_t>(destination)&15))%16/2);
  cpu(destination,source,peel);destination+=peel;source+=peel;count-=peel;stats.peeled+=peel;++stats.runs;
  size_t blocks=count/8;const size_t tail=count%8;
  if(!(reinterpret_cast<uintptr_t>(source)&15)){
    if(blocks){stats.eligible+=blocks;stats.direct+=blocks;
      if(nice_gfx::platform::rgb565_p4_blend::tryBlocks(destination,source,blocks,plan,&safe))stats.accepted+=blocks;
      else{stats.fallback+=blocks;cpu(destination,source,blocks*8);}
      destination+=blocks*8;source+=blocks*8;
    }
  }else while(blocks){const size_t chunk=std::min<size_t>(blocks,8);const size_t pixels=chunk*8;
    memcpy(scratch,source,pixels*sizeof(uint16_t));stats.eligible+=chunk;stats.scratch+=chunk;
    if(nice_gfx::platform::rgb565_p4_blend::tryBlocks(destination,scratch,chunk,plan,&safe))stats.accepted+=chunk;
    else{stats.fallback+=chunk;cpu(destination,source,pixels);}
    destination+=pixels;source+=pixels;blocks-=chunk;
  }
  cpu(destination,source,tail);
}
struct AxisMediaPieDraw {
  const MotionMedia32Tile &tile;KeyedPieUiStats *ui=nullptr;uint32_t started=0;uint64_t rows=0;AxisMediaPieCounts counts;
  alignas(16) unsigned char planStorage[sizeof(nice_gfx::platform::rgb565_p4_blend::Plan)];
  nice_gfx::platform::rgb565_p4_blend::Plan *plan=nullptr;
  explicit AxisMediaPieDraw(const MotionMedia32Tile &p,bool opaque):tile(p){
    if(!opaque&&p.alpha>0&&p.alpha<255&&p.pieUi&&nice_gfx::platform::rgb565_p4_blend::verified()){
      ui=p.pieUi;started=p.pieClock?p.pieClock():0;plan=new(planStorage) nice_gfx::platform::rgb565_p4_blend::Plan(p.alpha);
    }
  }
  AxisMediaPieDraw(const AxisMediaPieDraw&)=delete;AxisMediaPieDraw&operator=(const AxisMediaPieDraw&)=delete;
  void row(){if(ui)++rows;}
  bool paint(uint16_t *target,const uint16_t *source,int32_t count){
    if(!ui||!target||!source||count<8||count>256)return false;
    motionAxisMediaPieRun(target,source,size_t(count),tile.alpha,*plan,ui->safe,counts);return true;
  }
  ~AxisMediaPieDraw(){if(ui){ui->rows+=rows;ui->eligibleBlocks+=counts.eligible;ui->acceptedBlocks+=counts.accepted;ui->fallbackBlocks+=counts.fallback;ui->peelPixels+=counts.peeled;
    ++ui->axisDraws;ui->axisRuns+=counts.runs;ui->axisDirectBlocks+=counts.direct;ui->axisScratchBlocks+=counts.scratch;ui->axisAcceptedBlocks+=counts.accepted;ui->axisFallbackBlocks+=counts.fallback;
    if(tile.pieClock)ui->pathMicros+=uint32_t(tile.pieClock()-started);
  }}
};
#endif

template<int Format,bool Opaque>
static
#if defined(__GNUC__)
__attribute__((noinline))
#endif
void motionMedia32Kernel(const MotionMedia32Tile &p) {
  int32_t rowX=p.localX,rowY=p.localY,rowU=p.textureX,rowV=p.textureY;
  for(int32_t y=0;y<p.clipped.height;++y){
    int32_t xLocal=rowX,yLocal=rowY,u=rowU,v=rowV;
    uint16_t *target=p.pixels+size_t(p.clipped.y+y-p.area.y)*p.stride+size_t(p.clipped.x-p.area.x);
    for(int32_t x=0;x<p.clipped.width;++x,++target,xLocal+=p.localXX,yLocal+=p.localYX,u+=p.textureXX,v+=p.textureYX){
      // Unsigned subtraction is defined even outside the positive interval.
      // Each mask/source extent is below INT32_MAX and was checked at setup.
      if(uint32_t(xLocal)-p.left>=p.maskWidth||uint32_t(yLocal)-p.top>=p.maskHeight||
         uint32_t(u)>=p.sourceWidth||uint32_t(v)>=p.sourceHeight)continue;
      const uint8_t *source=p.frame+size_t(v>>16)*p.sourceStride+size_t(u>>16)*(Format==2?1:Format>=0?2:p.bytesPerPixel);
      uint16_t color;
      if(Format<0)color=motionMediaColor(source,p.bytesPerPixel,p.ram,p.order);
      else if(Format==1)memcpy(&color,source,2);
      else if(Format==0)color=uint16_t(uint16_t(pgm_read_byte(source))<<8|pgm_read_byte(source+1));
      else {const uint8_t b=pgm_read_byte(source);color=uint16_t((((b>>5)*31+3)/7)<<11|(((b>>2)&7)*9)<<5|((b&3)*31+1)/3);}
      *target=rgb565_blend::toWire((Opaque||(Format<0&&p.alpha==255))?color:rgb565_blend::color(color,rgb565_blend::fromWire(*target),p.alpha));
    }
    rowX+=p.localXY;rowY+=p.localYY;rowU+=p.textureXY;rowV+=p.textureYY;
  }
}

// Clip a pixel-index interval against one quantized affine coordinate. The
// result remains half-open; divisions operate on int64 signed numerators.
static bool motionClipSpan(int32_t origin,int32_t step,uint32_t lower,uint32_t upper,int32_t &first,int32_t &end) {
  if(!step)return int64_t(origin)>=lower&&int64_t(origin)<upper;
  int64_t begin,limit;
  if(step>0){
    begin=motion_geometry::ceilDivide(int64_t(lower)-origin,step);
    limit=motion_geometry::floorDivide(int64_t(upper)-1-origin,step)+1;
  }else{
    const int64_t divisor=-int64_t(step);
    begin=motion_geometry::ceilDivide(int64_t(origin)-(int64_t(upper)-1),divisor);
    limit=motion_geometry::floorDivide(int64_t(origin)-lower,divisor)+1;
  }
  // Reject before narrowing an unbounded division result to int32.
  if(begin>=end||limit<=first)return false;
  if(begin>first)first=int32_t(begin);
  if(limit<end)end=int32_t(limit);
  return first<end;
}

template<int Format> static inline
#if defined(__GNUC__)
__attribute__((always_inline))
#endif
uint16_t motionMediaSample(const uint8_t *source) {
  if(Format==1){uint16_t color;memcpy(&color,source,2);return color;}
  if(Format==0)return uint16_t(uint16_t(pgm_read_byte(source))<<8|pgm_read_byte(source+1));
  const uint8_t b=pgm_read_byte(source);return uint16_t((((b>>5)*31+3)/7)<<11|(((b>>2)&7)*9)<<5|((b&3)*31+1)/3);
}
// An opaque plain image replaces its background only where a texel exists.
// Paint the local-box minus valid-texture spans separately, so edge rounding
// retains the authored background without blending image pixels a second time.
static void motionMediaBackdrop(const MotionMedia32Tile &p,uint16_t background) {
  int32_t rowX=p.localX,rowY=p.localY,rowU=p.textureX,rowV=p.textureY;
  for(int32_t y=0;y<p.clipped.height;++y){
    int32_t first=0,end=p.clipped.width;
    if(motionClipSpan(rowX,p.localXX,p.left,p.left+p.maskWidth,first,end)&&
       motionClipSpan(rowY,p.localYX,p.top,p.top+p.maskHeight,first,end)){
      int32_t imageFirst=first,imageEnd=end;
      const bool image=motionClipSpan(rowU,p.textureXX,0,p.sourceWidth,imageFirst,imageEnd)&&
        motionClipSpan(rowV,p.textureYX,0,p.sourceHeight,imageFirst,imageEnd);
      const auto fill=[&](int32_t begin,int32_t stop){
        uint16_t *target=p.pixels+size_t(p.clipped.y+y-p.area.y)*p.stride+size_t(p.clipped.x+begin-p.area.x);
        for(int32_t x=begin;x<stop;++x,++target)*target=rgb565_blend::toWire(p.alpha==255?background:
          rgb565_blend::color(background,rgb565_blend::fromWire(*target),p.alpha));
      };
      if(image){fill(first,imageFirst);fill(imageEnd,end);}else fill(first,end);
    }
    rowX+=p.localXY;rowY+=p.localYY;rowU+=p.textureXY;rowV+=p.textureYY;
  }
}
// Keep the generic sampler available for AB builds and all other platforms.
#ifndef NICE_GFX_S31_MOTION_STAGE
#define NICE_GFX_S31_MOTION_STAGE 1
#endif
template<int Format,bool Opaque>
static
#if defined(__GNUC__)
__attribute__((noinline))
#if !defined(__clang__)
__attribute__((optimize("O3")))
#endif
#endif
void motionMediaSpanStaged(const MotionMedia32Tile &p) {
  if(p.clipped.width<=0||p.clipped.height<=0)return;
  // Task stacks are internal RAM on the S31 Arduino/IDF path. Isolate the
  // bounded scratch here so ordinary draws do not inherit this stack frame.
  alignas(16) uint16_t temporary[128];
  const uint8_t *const frame=p.frame;const size_t sourceStride=p.sourceStride;
  const int32_t textureXX=p.textureXX,textureYX=p.textureYX;const uint8_t alpha=p.alpha;
  uint16_t storedBackground=0;bool usePalette=false;
  if(!Opaque&&Format!=2){
    uint16_t samples[16];unsigned at=0,votes=0;
    for(int y=0;y<4;++y)for(int x=0;x<4;++x){
      const int32_t sx=p.clipped.x+int32_t(int64_t(p.clipped.width-1)*x/3);
      const int32_t sy=p.clipped.y+int32_t(int64_t(p.clipped.height-1)*y/3);
      const uint16_t sample=p.pixels[size_t(sy-p.area.y)*p.stride+size_t(sx-p.area.x)];samples[at++]=sample;
      if(!votes){storedBackground=sample;votes=1;}else if(sample==storedBackground)++votes;else --votes;
    }
    unsigned matches=0;for(unsigned i=0;i<16;++i)matches+=samples[i]==storedBackground;
    usePalette=matches>=12;
  }
  const uint16_t nativeBackground=rgb565_blend::fromWire(storedBackground);
  const uint32_t backgroundPair=uint32_t(nativeBackground)|(uint32_t(nativeBackground)<<16);
  const rgb565_blend::SolidBackgroundLut palette(nativeBackground,alpha);
  int32_t rowX=p.localX,rowY=p.localY,rowU=p.textureX,rowV=p.textureY;
  for(int32_t y=0;y<p.clipped.height;++y){
    int32_t first=0,end=p.clipped.width;
    if(motionClipSpan(rowX,p.localXX,p.left,p.left+p.maskWidth,first,end)&&
       motionClipSpan(rowY,p.localYX,p.top,p.top+p.maskHeight,first,end)&&
       motionClipSpan(rowU,p.textureXX,0,p.sourceWidth,first,end)&&
       motionClipSpan(rowV,p.textureYX,0,p.sourceHeight,first,end)){
      int32_t u=int32_t(int64_t(rowU)+int64_t(textureXX)*first),v=int32_t(int64_t(rowV)+int64_t(textureYX)*first);
      uint16_t *target=p.pixels+size_t(p.clipped.y+y-p.area.y)*p.stride+size_t(p.clipped.x+first-p.area.x);
      if((reinterpret_cast<uintptr_t>(target)&3)&&first<end){
        const uint8_t *source=frame+size_t(v>>16)*sourceStride+size_t(u>>16)*(Format==2?1:2);
        const uint16_t color=motionMediaSample<Format>(source);
        if(Opaque)*target=rgb565_blend::toWire(color);
        else {const uint16_t under=rgb565_blend::fromWire(*target);if(color!=under)*target=rgb565_blend::toWire(
          usePalette&&under==nativeBackground?palette.lookupNative(color):rgb565_blend::color(color,under,alpha));}
        ++target;++first;u+=textureXX;v+=textureYX;
      }
      while(first<end){
        const int32_t count=std::min<int32_t>(128,end-first);
        if(!Opaque)memcpy(temporary,target,size_t(count)*2);
        int32_t x=0;
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        typedef uint32_t MotionBlendWord __attribute__((__may_alias__));
        for(;x+1<count;x+=2){
          const uint8_t *a=frame+size_t(v>>16)*sourceStride+size_t(u>>16)*(Format==2?1:2);
          u+=textureXX;v+=textureYX;
          const uint8_t *b=frame+size_t(v>>16)*sourceStride+size_t(u>>16)*(Format==2?1:2);
          u+=textureXX;v+=textureYX;
          const uint32_t colors=uint32_t(motionMediaSample<Format>(a))|(uint32_t(motionMediaSample<Format>(b))<<16);
          auto *pair=reinterpret_cast<MotionBlendWord *>(temporary+x);
          if(Opaque)*pair=rgb565_blend::swapPairBytes(colors);
          else {const uint32_t under=rgb565_blend::swapPairBytes(*pair);
            if(colors!=under){const uint32_t blended=usePalette&&under==backgroundPair?
              uint32_t(palette.lookupNative(uint16_t(colors)))|(uint32_t(palette.lookupNative(uint16_t(colors>>16)))<<16):
              rgb565_blend::colorPair(colors,under,alpha);
              *pair=rgb565_blend::swapPairBytes(blended);}
          }
        }
#endif
        for(;x<count;++x,u+=textureXX,v+=textureYX){
          const uint8_t *source=frame+size_t(v>>16)*sourceStride+size_t(u>>16)*(Format==2?1:2);
          const uint16_t color=motionMediaSample<Format>(source);
          if(Opaque)temporary[x]=rgb565_blend::toWire(color);
          else {const uint16_t under=rgb565_blend::fromWire(temporary[x]);if(color!=under)temporary[x]=rgb565_blend::toWire(
            usePalette&&under==nativeBackground?palette.lookupNative(color):rgb565_blend::color(color,under,alpha));}
        }
        memcpy(target,temporary,size_t(count)*2);target+=count;first+=count;
      }
    }
    rowX+=p.localXY;rowY+=p.localYY;rowU+=p.textureXY;rowV+=p.textureYY;
  }
}
template<int Format,bool Opaque,bool Keyed=false>
static
#if defined(__GNUC__)
__attribute__((noinline))
#endif
void motionMediaSpanKernel(const MotionMedia32Tile &p) {
#if defined(CONFIG_IDF_TARGET_ESP32S31) && NICE_GFX_S31_MOTION_STAGE
  if(!Keyed&&p.clipped.width>=32&&uint64_t(p.clipped.width)*p.clipped.height>=4096){
    motionMediaSpanStaged<Format,Opaque>(p);return;
  }
#endif
  int32_t rowX=p.localX,rowY=p.localY,rowU=p.textureX,rowV=p.textureY;
  for(int32_t y=0;y<p.clipped.height;++y){
    int32_t first=0,end=p.clipped.width;
    if(motionClipSpan(rowX,p.localXX,p.left,p.left+p.maskWidth,first,end)&&
       motionClipSpan(rowY,p.localYX,p.top,p.top+p.maskHeight,first,end)&&
       motionClipSpan(rowU,p.textureXX,0,p.sourceWidth,first,end)&&
       motionClipSpan(rowV,p.textureYX,0,p.sourceHeight,first,end)){
      // The proof covers final coordinates; the skipped product itself may
      // exceed int32, so form the sum in int64 before narrowing it.
      int32_t u=int32_t(int64_t(rowU)+int64_t(p.textureXX)*first),v=int32_t(int64_t(rowV)+int64_t(p.textureYX)*first);
      uint16_t *target=p.pixels+size_t(p.clipped.y+y-p.area.y)*p.stride+size_t(p.clipped.x+first-p.area.x);
      int32_t x=first;
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      if(!Opaque&&!Keyed){
        // Span clipping proved every sampled texel. Gather two independent
        // affine samples, then share the exact /255 channel arithmetic. Keep
        // keyed pixels scalar so a transparent texel never touches its underlay.
        if((reinterpret_cast<uintptr_t>(target)&3)&&x<end){
          const uint8_t *source=p.frame+size_t(v>>16)*p.sourceStride+size_t(u>>16)*(Format==2?1:2);
          const uint16_t color=motionMediaSample<Format>(source),under=rgb565_blend::fromWire(*target);
          if(color!=under)*target=rgb565_blend::toWire(rgb565_blend::color(color,under,p.alpha));
          ++target;++x;u+=p.textureXX;v+=p.textureYX;
        }
        typedef uint32_t MotionBlendWord __attribute__((__may_alias__));
        for(;x+1<end;x+=2,target+=2){
          const uint8_t *a=p.frame+size_t(v>>16)*p.sourceStride+size_t(u>>16)*(Format==2?1:2);
          u+=p.textureXX;v+=p.textureYX;
          const uint8_t *b=p.frame+size_t(v>>16)*p.sourceStride+size_t(u>>16)*(Format==2?1:2);
          u+=p.textureXX;v+=p.textureYX;
          const uint32_t colors=uint32_t(motionMediaSample<Format>(a))|(uint32_t(motionMediaSample<Format>(b))<<16);
          auto *pair=reinterpret_cast<MotionBlendWord *>(target);
          const uint32_t under=rgb565_blend::swapPairBytes(*pair);
          if(colors!=under)*pair=rgb565_blend::swapPairBytes(rgb565_blend::colorPair(colors,under,p.alpha));
        }
      }
#endif
      for(;x<end;++x,++target,u+=p.textureXX,v+=p.textureYX){
        const uint8_t *source=p.frame+size_t(v>>16)*p.sourceStride+size_t(u>>16)*(Format==2?1:2);
        const uint16_t color=motionMediaSample<Format>(source);
        if(Keyed&&color==p.colorKey)continue;
        if(Opaque)*target=rgb565_blend::toWire(color);
        else {const uint16_t under=rgb565_blend::fromWire(*target);if(color!=under)*target=rgb565_blend::toWire(rgb565_blend::color(color,under,p.alpha));}
      }
    }
    rowX+=p.localXY;rowY+=p.localYY;rowU+=p.textureXY;rowV+=p.textureYY;
  }
}

#if NICE_GFX_UI_NATIVE_AFFINE_MEDIA
// Identical quantized coordinates, row-span proofs and /255 arithmetic as the
// Wire sampler. Native output avoids two whole-ROI byte-swap passes. The source
// remains borrowed only for this synchronous call; no new texture is allocated.
#if NICE_GFX_UI_KEYED_AFFINE_ROW_MAP
// Scale/axis flips only: horizontal texels are constant across all rows. Keep
// the exact quantized source indices in bounded internal task-stack storage;
// only this noinline specialization owns the 512-byte map, not every draw.
#if NICE_GFX_UI_AXIS_MEDIA_CACHE && NICE_GFX_UI_AXIS_MEDIA_FULL_FOOTPRINT
// For axis-aligned quantized transforms, moving an integer output pixel
// changes the Q16 local/texture coordinate by exactly its Q16 coefficient:
// floor((N+100*k*step)/100) == floor(N/100)+k*step, for either sign.
// The absolute footprint origin is only a subset locator. Exact full sample
// arrays remain the texture key, so integer translation can reuse pixels.
static bool motionMediaFullAxisOrigin(int32_t origin,int32_t step,int32_t from,int32_t to,int32_t length,int32_t &out){
  // Any int32 origin + int32 step * (int32 to - int32 from) fits int64:
  // the extreme positive/negative combinations reach at most INT64_MAX/MIN.
  const int64_t wide=int64_t(origin)+int64_t(step)*(int64_t(to)-from);
  if(wide<INT32_MIN||wide>INT32_MAX)return false;out=int32_t(wide);
  return motionRange32(wide,step,0,length,0);
}
template<bool Opaque>
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("O3")))
#elif defined(__GNUC__)
__attribute__((noinline))
#endif
static NICE_GFX_UI_MEDIA_SHAPE_IRAM_ATTR bool motionMediaNativeFullFootprint(const MotionMedia32Tile &p,int32_t first,int32_t end){
  const Rect full=p.fullFootprint;
  if(full.width<=0||full.height<=0||p.localXY||p.localYX||p.textureXY||p.textureYX||
     uint64_t(p.left)+p.maskWidth>UINT32_MAX||uint64_t(p.top)+p.maskHeight>UINT32_MAX||
     int64_t(full.x)+full.width>INT32_MAX||int64_t(full.y)+full.height>INT32_MAX)return false;
  int32_t localX,localY,textureX,textureY;
  if(!motionMediaFullAxisOrigin(p.localX,p.localXX,p.clipped.x,full.x,full.width,localX)||
     !motionMediaFullAxisOrigin(p.localY,p.localYY,p.clipped.y,full.y,full.height,localY)||
     !motionMediaFullAxisOrigin(p.textureX,p.textureXX,p.clipped.x,full.x,full.width,textureX)||
     !motionMediaFullAxisOrigin(p.textureY,p.textureYY,p.clipped.y,full.y,full.height,textureY))return false;
  int32_t fullFirstX=0,fullEndX=full.width,fullFirstY=0,fullEndY=full.height;
  if(!motionClipSpan(localX,p.localXX,p.left,p.left+p.maskWidth,fullFirstX,fullEndX)||
     !motionClipSpan(textureX,p.textureXX,0,p.sourceWidth,fullFirstX,fullEndX)||
     !motionClipSpan(localY,p.localYY,p.top,p.top+p.maskHeight,fullFirstY,fullEndY)||
     !motionClipSpan(textureY,p.textureYY,0,p.sourceHeight,fullFirstY,fullEndY))return false;
  const int32_t width=fullEndX-fullFirstX,height=fullEndY-fullFirstY;
  if(width<=0||width>256||height<=0||height>256)return false;
  int32_t firstY=0,endY=p.clipped.height;
  if(!motionClipSpan(p.localY,p.localYY,p.top,p.top+p.maskHeight,firstY,endY)||
     !motionClipSpan(p.textureY,p.textureYY,0,p.sourceHeight,firstY,endY))return true;
  const int64_t offsetX=int64_t(p.clipped.x)+first-(int64_t(full.x)+fullFirstX),offsetY=int64_t(p.clipped.y)+firstY-(int64_t(full.y)+fullFirstY);
  const int32_t count=end-first,rows=endY-firstY;
  if(count<=0||rows<=0||offsetX<0||offsetY<0||offsetX+count>width||offsetY+rows>height)return false;
  const int32_t sourceX=int32_t(offsetX),sourceY=int32_t(offsetY);
  alignas(4) uint16_t sampleX[256],sampleY[256];
  int32_t u=int32_t(int64_t(textureX)+int64_t(p.textureXX)*fullFirstX),v=int32_t(int64_t(textureY)+int64_t(p.textureYY)*fullFirstY);
  for(int32_t x=0;x<width;++x,u+=p.textureXX)sampleX[x]=uint16_t(uint32_t(u)>>16);
  for(int32_t y=0;y<height;++y,v+=p.textureYY)sampleY[y]=uint16_t(uint32_t(v)>>16);
  detail::MotionAxisCache::Texture texture;
  if(!p.axisCache->resolve(p.immutableFrameIdentity,p.frame,p.sourceStride,p.sourceWidth>>16,p.sourceHeight>>16,
       sampleX,size_t(width),sampleY,size_t(height),texture,p.colorKey))return false;
  const uint16_t key=p.colorKey;const uint8_t alpha=p.alpha;
#if NICE_GFX_UI_AXIS_MEDIA_PPA
  if(p.axisPpa&&p.axisPpaError){
    const DirtyRect destination(p.clipped.x+first,p.clipped.y+firstY,count,rows);
    const Status accelerated=p.axisPpa(p.axisPpaContext,texture,destination,sourceX,sourceY,alpha,key);
    if(accelerated==Status::Ok){p.axisCache->recordFullFootprint(true,uint64_t(count)*uint32_t(rows));return true;}
    // An accepted operation may already have blended. Never re-blend it or
    // write later pixels after a hardware/synchronization error.
    if(accelerated!=Status::Unsupported){*p.axisPpaError=accelerated;return true;}
  }
#endif
#if NICE_GFX_UI_AXIS_MEDIA_RUNS
  if(texture.rowOffsets){
#if NICE_GFX_UI_AXIS_MEDIA_PIE && NICE_GFX_UI_KEYED_PIE
    AxisMediaPieDraw pieDraw(p,Opaque);
#endif
    uint64_t painted=0,segments=0;
    for(int32_t row=0;row<rows;++row){
#if NICE_GFX_UI_AXIS_MEDIA_PIE && NICE_GFX_UI_KEYED_PIE
      pieDraw.row();
#endif
      const size_t textureRow=size_t(sourceY+row);const uint16_t *sourceRow=texture.pixels+textureRow*texture.stride;
      const uint32_t begin=texture.rowOffsets[textureRow],stop=texture.rowOffsets[textureRow+1];
      for(uint32_t index=begin;index<stop;++index){
        const auto &run=texture.runs[index];const int32_t start=std::max<int32_t>(run.start,sourceX),finish=std::min<int32_t>(int32_t(run.start)+run.length,sourceX+count);
        if(start>=finish)continue;const int32_t length=finish-start;
        const uint16_t *source=sourceRow+start;
        uint16_t *target=p.pixels+size_t(p.clipped.y+firstY+row-p.area.y)*p.stride+size_t(p.clipped.x+first+start-sourceX-p.area.x);
        painted+=uint32_t(length);++segments;
        if(Opaque){memcpy(target,source,size_t(length)*sizeof(uint16_t));continue;}
#if NICE_GFX_UI_AXIS_MEDIA_PIE && NICE_GFX_UI_KEYED_PIE
        if(pieDraw.paint(target,source,length))continue;
#endif
        int32_t x=0;
#if NICE_GFX_UI_KEYED_AFFINE_PAIRS && defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        if((reinterpret_cast<uintptr_t>(target)&3)&&x<length){const uint16_t color=source[x];if(color!=*target)*target=rgb565_blend::color(color,*target,alpha);++target;++x;}
        typedef uint32_t FullRunNativePair __attribute__((__may_alias__));
        for(;x+1<length;x+=2,target+=2){uint32_t colors;memcpy(&colors,source+x,sizeof(colors));auto *pair=reinterpret_cast<FullRunNativePair*>(target);const uint32_t under=*pair;if(colors!=under)*pair=rgb565_blend::colorPair(colors,under,alpha);}
#endif
        for(;x<length;++x,++target){const uint16_t color=source[x];if(color!=*target)*target=rgb565_blend::color(color,*target,alpha);}
      }
    }
    p.axisCache->recordRunPaint(Opaque,segments,painted,uint64_t(count)*uint32_t(rows)-painted);
    p.axisCache->recordFullFootprint(true,uint64_t(count)*uint32_t(rows));return true;
  }
#endif
  for(int32_t row=0;row<rows;++row){
    const uint16_t *source=texture.pixels+size_t(sourceY+row)*texture.stride+sourceX;
    uint16_t *target=p.pixels+size_t(p.clipped.y+firstY+row-p.area.y)*p.stride+size_t(p.clipped.x+first-p.area.x);
    int32_t x=0;
#if NICE_GFX_UI_KEYED_AFFINE_PAIRS && defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    if((reinterpret_cast<uintptr_t>(target)&3)&&x<count){const uint16_t color=source[x];if(color!=key){if(Opaque)*target=color;else if(color!=*target)*target=rgb565_blend::color(color,*target,alpha);}++target;++x;}
    typedef uint32_t FullNativePair __attribute__((__may_alias__));
    for(;x+1<count;x+=2,target+=2){const uint16_t a=source[x],b=source[x+1];
      if(a==key){if(b==key)continue;if(Opaque)target[1]=b;else if(b!=target[1])target[1]=rgb565_blend::color(b,target[1],alpha);}
      else if(b==key){if(Opaque)target[0]=a;else if(a!=target[0])target[0]=rgb565_blend::color(a,target[0],alpha);}
      else {auto *pair=reinterpret_cast<FullNativePair*>(target);const uint32_t colors=uint32_t(a)|(uint32_t(b)<<16);if(Opaque)*pair=colors;else {const uint32_t under=*pair;if(colors!=under)*pair=rgb565_blend::colorPair(colors,under,alpha);}}
    }
#endif
    for(;x<count;++x,++target){const uint16_t color=source[x];if(color==key)continue;if(Opaque)*target=color;else if(color!=*target)*target=rgb565_blend::color(color,*target,alpha);}
  }
  p.axisCache->recordFullFootprint(true,uint64_t(count)*uint32_t(rows));return true;
}
#endif

#if NICE_GFX_UI_KEYED_PIE
// Only the direct Native565 keyed rowMap path uses this helper. A full row is
// sampled before any destination access, into this task's bounded native RAM.
// Adapter rejection is side-effect-free and falls back to the original math.
#ifndef NICE_GFX_UI_KEYED_PIE_KEYMASK
#define NICE_GFX_UI_KEYED_PIE_KEYMASK 1
#endif
#if NICE_GFX_UI_KEYED_PIE_KEYMASK
static
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("O3")))
#elif defined(__GNUC__)
__attribute__((noinline))
#endif
void motionMediaNativeKeyedRowsPie(const MotionMedia32Tile &p,const uint16_t *sampleX,
    int32_t first,int32_t end,const nice_gfx::platform::rgb565_p4_blend::Plan &plan){
  auto &ui=*p.pieUi;const uint16_t key=p.colorKey;const uint8_t alpha=p.alpha;
  const int32_t count=end-first;
  const uint32_t started=p.pieClock?p.pieClock():0;
  alignas(16) uint16_t sampled[264];uint8_t occupied[32];
  uint64_t rows=0,eligible=0,accepted=0,fallback=0,keyBlocks=0,peeled=0;
  int32_t rowY=p.localY,rowV=p.textureY;
  for(int32_t y=0;y<p.clipped.height;++y,rowY+=p.localYY,rowV+=p.textureYY){
    if(uint32_t(rowY)-p.top>=p.maskHeight||uint32_t(rowV)>=p.sourceHeight)continue;
    const uint8_t *sourceRow=p.frame+size_t(uint32_t(rowV)>>16)*p.sourceStride;
    auto *target=p.pixels+size_t(p.clipped.y+y-p.area.y)*p.stride+size_t(p.clipped.x+first-p.area.x);
    const uint32_t residue=(reinterpret_cast<uintptr_t>(target)&15u)>>1;
    uint16_t *source=sampled+residue;
    const int32_t prefix=std::min<int32_t>(count,int32_t((8u-residue)&7u));
    const int32_t blockCount=(count-prefix)/8;
    const auto sample=[&](int32_t x){uint16_t color;memcpy(&color,sourceRow+size_t(sampleX[x])*2,2);return color;};
    // Finish the exact original source sampling before any target access.
    // Each aligned eight-lane block gets one occupancy proof while gathering.
    // Source-only masks are never inferred from target pixels or alpha.
    for(int32_t x=0;x<prefix;++x)source[x]=sample(x);
    for(int32_t b=0;b<blockCount;++b){
      const int32_t x=prefix+b*8;
      const uint16_t a=sample(x),b1=sample(x+1),c=sample(x+2),d=sample(x+3);
      const uint16_t e=sample(x+4),f=sample(x+5),g=sample(x+6),h=sample(x+7);
      source[x]=a;source[x+1]=b1;source[x+2]=c;source[x+3]=d;
      source[x+4]=e;source[x+5]=f;source[x+6]=g;source[x+7]=h;
      occupied[b]=uint8_t((a!=key)|((b1!=key)<<1)|((c!=key)<<2)|((d!=key)<<3)|
        ((e!=key)<<4)|((f!=key)<<5)|((g!=key)<<6)|((h!=key)<<7));
    }
    const int32_t tail=prefix+blockCount*8;
    for(int32_t x=tail;x<count;++x)source[x]=sample(x);
    ++rows;peeled+=uint32_t(prefix);
    const auto one=[&](int32_t x){const uint16_t color=source[x];
      if(color!=key){const volatile uint16_t *pixel=target+x;const uint16_t under=*pixel;
        if(color!=under)target[x]=rgb565_blend::color(color,under,alpha);}};
    const auto pair=[&](int32_t x,unsigned mask){
      if(mask==0)return;
      if(mask==1){const uint16_t color=source[x];const volatile uint16_t *pixel=target+x;const uint16_t under=*pixel;
        if(color!=under)target[x]=rgb565_blend::color(color,under,alpha);}
      else if(mask==2){const uint16_t color=source[x+1];const volatile uint16_t *pixel=target+x+1;const uint16_t under=*pixel;
        if(color!=under)target[x+1]=rgb565_blend::color(color,under,alpha);}
      else {typedef uint32_t PieKeymaskPair __attribute__((__may_alias__));
        auto *dst=reinterpret_cast<PieKeymaskPair*>(target+x);
        const uint32_t colors=uint32_t(source[x])|(uint32_t(source[x+1])<<16),under=*dst;
        if(colors!=under)*dst=rgb565_blend::colorPair(colors,under,alpha);}
    };
    for(int32_t x=0;x<prefix;++x)one(x);
    int32_t b=0;
    while(b<blockCount){
      const int32_t x=prefix+b*8;const unsigned mask=occupied[b];
      if(mask==255){
        int32_t stop=b+1;while(stop<blockCount&&occupied[stop]==255)++stop;
        const size_t blocks=size_t(stop-b);eligible+=blocks;
        if(nice_gfx::platform::rgb565_p4_blend::tryBlocks(target+x,source+x,blocks,plan,&ui.safe))accepted+=blocks;
        else {fallback+=blocks;for(int32_t at=x;at<prefix+stop*8;at+=2)pair(at,3);}
        b=stop;
      }else{
        ++keyBlocks;
        // An entirely transparent block performs no destination load or store.
        if(mask){pair(x,mask&3);pair(x+2,(mask>>2)&3);pair(x+4,(mask>>4)&3);pair(x+6,(mask>>6)&3);}
        ++b;
      }
    }
    int32_t x=tail;
    for(;x+1<count;x+=2)pair(x,unsigned(source[x]!=key)|(unsigned(source[x+1]!=key)<<1));
    if(x<count)one(x);
  }
  // Preserve the public counter semantics, with one publication per draw.
  ui.rows+=rows;ui.eligibleBlocks+=eligible;ui.acceptedBlocks+=accepted;
  ui.fallbackBlocks+=fallback;ui.keyBlocks+=keyBlocks;ui.peelPixels+=peeled;
  if(p.pieClock)ui.pathMicros+=uint32_t(p.pieClock()-started);
}
#else
static
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("O3")))
#elif defined(__GNUC__)
__attribute__((noinline))
#endif
void motionMediaNativeKeyedRowsPie(const MotionMedia32Tile &p,const uint16_t *sampleX,
    int32_t first,int32_t end,const nice_gfx::platform::rgb565_p4_blend::Plan &plan){
  auto &ui=*p.pieUi;const uint16_t key=p.colorKey;const uint8_t alpha=p.alpha;
  const int32_t count=end-first;
  const uint32_t started=p.pieClock?p.pieClock():0;
  alignas(16) uint16_t sampled[264];
  int32_t rowY=p.localY,rowV=p.textureY;
  for(int32_t y=0;y<p.clipped.height;++y,rowY+=p.localYY,rowV+=p.textureYY){
    if(uint32_t(rowY)-p.top>=p.maskHeight||uint32_t(rowV)>=p.sourceHeight)continue;
    const uint8_t *sourceRow=p.frame+size_t(uint32_t(rowV)>>16)*p.sourceStride;
    auto *target=p.pixels+size_t(p.clipped.y+y-p.area.y)*p.stride+size_t(p.clipped.x+first-p.area.x);
    // Matching halfword residue makes source/destination become 16B aligned
    // together after at most7 peeled pixels. 7+256 fits inside264 halfwords.
    uint16_t *source=sampled+((reinterpret_cast<uintptr_t>(target)&15u)>>1);
    for(int32_t x=0;x<count;++x)memcpy(source+x,sourceRow+size_t(sampleX[x])*2,2);
    ++ui.rows;
    const auto one=[&](int32_t x){const uint16_t color=source[x];
      if(color!=key){const volatile uint16_t *pixel=target+x;const uint16_t under=*pixel;
        if(color!=under)target[x]=rgb565_blend::color(color,under,alpha);}};
    const auto pair=[&](int32_t x){const uint16_t a=source[x],b=source[x+1];
      if(a==key){if(b!=key){const volatile uint16_t *pixel=target+x+1;const uint16_t under=*pixel;
          if(b!=under)target[x+1]=rgb565_blend::color(b,under,alpha);}}
      else if(b==key){const volatile uint16_t *pixel=target+x;const uint16_t under=*pixel;
        if(a!=under)target[x]=rgb565_blend::color(a,under,alpha);}
      else {
        typedef uint32_t PieFallbackPair __attribute__((__may_alias__));
        auto *dst=reinterpret_cast<PieFallbackPair*>(target+x);const uint32_t colors=uint32_t(a)|(uint32_t(b)<<16),under=*dst;
        if(colors!=under)*dst=rgb565_blend::colorPair(colors,under,alpha);
      }
    };
    int32_t x=0;
    for(;x<count&&(reinterpret_cast<uintptr_t>(target+x)&15u);++x){one(x);++ui.peelPixels;}
    while(x+8<=count){
      // This proof reads ONLY sampled texels. No target lane of a mixed/key
      // block is read by the adapter, including a single key inside a pair.
      size_t blocks=0;
      while(x+int32_t((blocks+1)*8)<=count){
        const auto *s=source+x+blocks*8;
        if(s[0]==key||s[1]==key||s[2]==key||s[3]==key||s[4]==key||s[5]==key||s[6]==key||s[7]==key)break;
        ++blocks;
      }
      if(blocks){
        ui.eligibleBlocks+=blocks;
        if(nice_gfx::platform::rgb565_p4_blend::tryBlocks(target+x,source+x,blocks,plan,&ui.safe)){
          ui.acceptedBlocks+=blocks;x+=int32_t(blocks*8);continue;
        }
        ui.fallbackBlocks+=blocks;
        const int32_t stop=x+int32_t(blocks*8);for(;x<stop;x+=2)pair(x);
      }else{
        ++ui.keyBlocks;for(unsigned lane=0;lane<8;lane+=2)pair(x+int32_t(lane));x+=8;
      }
    }
    for(;x+1<count;x+=2)pair(x);
    if(x<count)one(x);
  }
  if(p.pieClock)ui.pathMicros+=uint32_t(p.pieClock()-started);
}
#endif

#endif

template<bool Opaque>
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("O3")))
#elif defined(__GNUC__)
__attribute__((noinline))
#endif
static NICE_GFX_UI_MEDIA_SHAPE_IRAM_ATTR void motionMediaNativeKeyedRows(const MotionMedia32Tile &p){
  if(p.clipped.width<=0||p.clipped.width>256||p.clipped.height<=0)return;
  int32_t first=0,end=p.clipped.width;
  if(!motionClipSpan(p.localX,p.localXX,p.left,p.left+p.maskWidth,first,end)||
     !motionClipSpan(p.textureX,p.textureXX,0,p.sourceWidth,first,end))return;
#if NICE_GFX_UI_AXIS_MEDIA_CACHE && NICE_GFX_UI_AXIS_MEDIA_FULL_FOOTPRINT
  if(p.axisCache&&p.immutableFrameIdentity){
    if(motionMediaNativeFullFootprint<Opaque>(p,first,end))return;
    p.axisCache->recordFullFootprint(false);
  }
#endif
  alignas(4) uint16_t sampleX[256];
  int32_t u=int32_t(int64_t(p.textureX)+int64_t(p.textureXX)*first);
  for(int32_t x=first;x<end;++x,u+=p.textureXX)sampleX[x-first]=uint16_t(uint32_t(u)>>16);
  const uint16_t key=p.colorKey;const uint8_t alpha=p.alpha;
#if NICE_GFX_UI_AXIS_MEDIA_CACHE
  if(p.axisCache&&p.immutableFrameIdentity&&p.clipped.height<=256){
    int32_t firstY=0,endY=p.clipped.height;
    if(!motionClipSpan(p.localY,p.localYY,p.top,p.top+p.maskHeight,firstY,endY)||
       !motionClipSpan(p.textureY,p.textureYY,0,p.sourceHeight,firstY,endY))return;
    alignas(4) uint16_t sampleY[256];
    int32_t v=int32_t(int64_t(p.textureY)+int64_t(p.textureYY)*firstY);
    for(int32_t y=firstY;y<endY;++y,v+=p.textureYY)sampleY[y-firstY]=uint16_t(uint32_t(v)>>16);
    detail::MotionAxisCache::Texture texture;
    if(p.axisCache->resolve(p.immutableFrameIdentity,p.frame,p.sourceStride,p.sourceWidth>>16,p.sourceHeight>>16,
        sampleX,size_t(end-first),sampleY,size_t(endY-firstY),texture,key)){
#if NICE_GFX_UI_AXIS_MEDIA_RUNS
      if(texture.rowOffsets){
#if NICE_GFX_UI_AXIS_MEDIA_PIE && NICE_GFX_UI_KEYED_PIE
        AxisMediaPieDraw pieDraw(p,Opaque);
#endif
        uint64_t painted=0,segments=0;
        for(int32_t y=firstY;y<endY;++y){
#if NICE_GFX_UI_AXIS_MEDIA_PIE && NICE_GFX_UI_KEYED_PIE
          pieDraw.row();
#endif
          const size_t row=size_t(y-firstY);const uint16_t *sourceRow=texture.pixels+row*texture.stride;
          const uint32_t begin=texture.rowOffsets[row],stop=texture.rowOffsets[row+1];
          // A transparent row does not even form a destination pointer.
          for(uint32_t index=begin;index<stop;++index){
            const auto &run=texture.runs[index];const int32_t count=run.length;
            const uint16_t *source=sourceRow+run.start;
            uint16_t *target=p.pixels+size_t(p.clipped.y+y-p.area.y)*p.stride+size_t(p.clipped.x+first+run.start-p.area.x);
            painted+=uint32_t(count);++segments;
            if(Opaque){memcpy(target,source,size_t(count)*sizeof(uint16_t));continue;}
#if NICE_GFX_UI_AXIS_MEDIA_PIE && NICE_GFX_UI_KEYED_PIE
            if(pieDraw.paint(target,source,count))continue;
#endif
            int32_t x=0;
#if NICE_GFX_UI_KEYED_AFFINE_PAIRS && defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            if((reinterpret_cast<uintptr_t>(target)&3)&&x<count){
              const uint16_t color=source[x];if(color!=*target)*target=rgb565_blend::color(color,*target,alpha);++x;++target;
            }
            typedef uint32_t RunNativePair __attribute__((__may_alias__));
            for(;x+1<count;x+=2,target+=2){
              uint32_t colors;memcpy(&colors,source+x,sizeof(colors));auto *pair=reinterpret_cast<RunNativePair*>(target);
              const uint32_t under=*pair;if(colors!=under)*pair=rgb565_blend::colorPair(colors,under,alpha);
            }
#endif
            for(;x<count;++x,++target){const uint16_t color=source[x];if(color!=*target)*target=rgb565_blend::color(color,*target,alpha);}
          }
        }
        p.axisCache->recordRunPaint(Opaque,segments,painted,uint64_t(end-first)*uint32_t(endY-firstY)-painted);
        return;
      }
#endif

      for(int32_t y=firstY;y<endY;++y){
        const uint16_t *source=texture.pixels+size_t(y-firstY)*texture.stride;
        uint16_t *target=p.pixels+size_t(p.clipped.y+y-p.area.y)*p.stride+size_t(p.clipped.x+first-p.area.x);
        int32_t x=0;const int32_t count=end-first;
#if NICE_GFX_UI_KEYED_AFFINE_PAIRS && defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        if((reinterpret_cast<uintptr_t>(target)&3)&&x<count){const uint16_t color=source[x];
          if(color!=key){if(Opaque)*target=color;else if(color!=*target)*target=rgb565_blend::color(color,*target,alpha);}++target;++x;}
        typedef uint32_t CachedNativePair __attribute__((__may_alias__));
        for(;x+1<count;x+=2,target+=2){const uint16_t a=source[x],b=source[x+1];
          if(a==key){if(b==key)continue;if(Opaque)target[1]=b;else if(b!=target[1])target[1]=rgb565_blend::color(b,target[1],alpha);}
          else if(b==key){if(Opaque)target[0]=a;else if(a!=target[0])target[0]=rgb565_blend::color(a,target[0],alpha);}
          else {auto *pair=reinterpret_cast<CachedNativePair*>(target);const uint32_t colors=uint32_t(a)|(uint32_t(b)<<16);
            if(Opaque)*pair=colors;else {const uint32_t under=*pair;if(colors!=under)*pair=rgb565_blend::colorPair(colors,under,alpha);}}
        }
#endif
        for(;x<count;++x,++target){const uint16_t color=source[x];if(color==key)continue;
          if(Opaque)*target=color;else if(color!=*target)*target=rgb565_blend::color(color,*target,alpha);}
      }
      return;
    }
  }
#endif

#if NICE_GFX_UI_KEYED_PIE
  // Preserve all cache fast paths above. This opt-in targets only direct rowMap.
  // An unverified/unsupported binary keeps its original sampler, with no scratch.
  if(!Opaque&&p.pieUi&&nice_gfx::platform::rgb565_p4_blend::verified()){
    const nice_gfx::platform::rgb565_p4_blend::Plan plan(alpha);
    motionMediaNativeKeyedRowsPie(p,sampleX,first,end,plan);return;
  }
#endif
  int32_t rowY=p.localY,rowV=p.textureY;
  for(int32_t y=0;y<p.clipped.height;++y,rowY+=p.localYY,rowV+=p.textureYY){
    if(uint32_t(rowY)-p.top>=p.maskHeight||uint32_t(rowV)>=p.sourceHeight)continue;
    const uint8_t *sourceRow=p.frame+size_t(uint32_t(rowV)>>16)*p.sourceStride;
    const auto sample=[&](int32_t index){uint16_t color;memcpy(&color,sourceRow+size_t(sampleX[index])*2,2);return color;};
    auto *target=p.pixels+size_t(p.clipped.y+y-p.area.y)*p.stride+size_t(p.clipped.x+first-p.area.x);
    int32_t x=first;
#if NICE_GFX_UI_KEYED_AFFINE_PAIRS && defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    if((reinterpret_cast<uintptr_t>(target)&3)&&x<end){const uint16_t color=sample(x-first);
      if(color!=key){if(Opaque)*target=color;else if(color!=*target)*target=rgb565_blend::color(color,*target,alpha);}
      ++target;++x;
    }
    typedef uint32_t RowNativePair __attribute__((__may_alias__));
    for(;x+1<end;x+=2,target+=2){const uint16_t a=sample(x-first),b=sample(x+1-first);
      if(a==key){if(b==key)continue;if(Opaque)target[1]=b;else if(b!=target[1])target[1]=rgb565_blend::color(b,target[1],alpha);}
      else if(b==key){if(Opaque)target[0]=a;else if(a!=target[0])target[0]=rgb565_blend::color(a,target[0],alpha);}
      else {auto *pair=reinterpret_cast<RowNativePair*>(target);const uint32_t colors=uint32_t(a)|(uint32_t(b)<<16);
        if(Opaque)*pair=colors;else {const uint32_t under=*pair;if(colors!=under)*pair=rgb565_blend::colorPair(colors,under,alpha);}}
    }
#endif
    for(;x<end;++x,++target){const uint16_t color=sample(x-first);if(color==key)continue;
      if(Opaque)*target=color;else if(color!=*target)*target=rgb565_blend::color(color,*target,alpha);
    }
  }
}
#endif

template<int Format,bool Opaque,bool Keyed>
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("O3")))
#elif defined(__GNUC__)
__attribute__((noinline))
#endif
static void motionMediaNative32Kernel(const MotionMedia32Tile &p){
#if NICE_GFX_UI_KEYED_AFFINE_ROW_MAP
  if(Keyed&&Format==1&&p.clipped.width>0&&p.clipped.width<=256&&
     !p.localXY&&!p.localYX&&!p.textureXY&&!p.textureYX){motionMediaNativeKeyedRows<Opaque>(p);return;}
#endif
  int32_t rowX=p.localX,rowY=p.localY,rowU=p.textureX,rowV=p.textureY;
  for(int32_t y=0;y<p.clipped.height;++y){
    int32_t first=0,end=p.clipped.width;
    if(motionClipSpan(rowX,p.localXX,p.left,p.left+p.maskWidth,first,end)&&
       motionClipSpan(rowY,p.localYX,p.top,p.top+p.maskHeight,first,end)&&
       motionClipSpan(rowU,p.textureXX,0,p.sourceWidth,first,end)&&
       motionClipSpan(rowV,p.textureYX,0,p.sourceHeight,first,end)){
      int32_t u=int32_t(int64_t(rowU)+int64_t(p.textureXX)*first),v=int32_t(int64_t(rowV)+int64_t(p.textureYX)*first);
      auto *target=p.pixels+size_t(p.clipped.y+y-p.area.y)*p.stride+size_t(p.clipped.x+first-p.area.x);
      int32_t x=first;
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#if NICE_GFX_UI_KEYED_AFFINE_PAIRS
      if(Keyed){
        // A keyed halfword never touches its underlay. Peel one destination
        // pixel so subsequent packed accesses are naturally 32-bit aligned.
        if((reinterpret_cast<uintptr_t>(target)&3)&&x<end){
          const auto *source=p.frame+size_t(v>>16)*p.sourceStride+size_t(u>>16)*(Format==2?1:2);
          const uint16_t color=motionMediaSample<Format>(source);
          if(color!=p.colorKey){if(Opaque)*target=color;else if(color!=*target)*target=rgb565_blend::color(color,*target,p.alpha);}
          ++target;++x;u+=p.textureXX;v+=p.textureYX;
        }
        typedef uint32_t KeyedNativePair __attribute__((__may_alias__));
        for(;x+1<end;x+=2,target+=2){
          const auto *a=p.frame+size_t(v>>16)*p.sourceStride+size_t(u>>16)*(Format==2?1:2);u+=p.textureXX;v+=p.textureYX;
          const auto *b=p.frame+size_t(v>>16)*p.sourceStride+size_t(u>>16)*(Format==2?1:2);u+=p.textureXX;v+=p.textureYX;
          const uint16_t firstColor=motionMediaSample<Format>(a),secondColor=motionMediaSample<Format>(b);
          if(firstColor==p.colorKey){
            if(secondColor==p.colorKey)continue;
            if(Opaque)target[1]=secondColor;else if(secondColor!=target[1])target[1]=rgb565_blend::color(secondColor,target[1],p.alpha);
          }else if(secondColor==p.colorKey){
            if(Opaque)target[0]=firstColor;else if(firstColor!=target[0])target[0]=rgb565_blend::color(firstColor,target[0],p.alpha);
          }else{
            const uint32_t colors=uint32_t(firstColor)|(uint32_t(secondColor)<<16);
            auto *pair=reinterpret_cast<KeyedNativePair*>(target);
            if(Opaque)*pair=colors;
            else {const uint32_t under=*pair;if(colors!=under)*pair=rgb565_blend::colorPair(colors,under,p.alpha);}
          }
        }
      }
#endif
      if(!Keyed){
        if((reinterpret_cast<uintptr_t>(target)&3)&&x<end){
          const auto *source=p.frame+size_t(v>>16)*p.sourceStride+size_t(u>>16)*(Format==2?1:2);
          const uint16_t color=motionMediaSample<Format>(source);*target=Opaque?color:rgb565_blend::color(color,*target,p.alpha);
          ++target;++x;u+=p.textureXX;v+=p.textureYX;
        }
        typedef uint32_t NativePair __attribute__((__may_alias__));
        for(;x+1<end;x+=2,target+=2){
          const auto *a=p.frame+size_t(v>>16)*p.sourceStride+size_t(u>>16)*(Format==2?1:2);u+=p.textureXX;v+=p.textureYX;
          const auto *b=p.frame+size_t(v>>16)*p.sourceStride+size_t(u>>16)*(Format==2?1:2);u+=p.textureXX;v+=p.textureYX;
          const uint32_t colors=uint32_t(motionMediaSample<Format>(a))|(uint32_t(motionMediaSample<Format>(b))<<16);
          auto *pair=reinterpret_cast<NativePair*>(target);if(Opaque)*pair=colors;else if(colors!=*pair)*pair=rgb565_blend::colorPair(colors,*pair,p.alpha);
        }
      }
#endif
      for(;x<end;++x,++target,u+=p.textureXX,v+=p.textureYX){
        const auto *source=p.frame+size_t(v>>16)*p.sourceStride+size_t(u>>16)*(Format==2?1:2);const uint16_t color=motionMediaSample<Format>(source);
        if(Keyed&&color==p.colorKey)continue;
        if(Opaque)*target=color;else if(color!=*target)*target=rgb565_blend::color(color,*target,p.alpha);
      }
    }
    rowX+=p.localXY;rowY+=p.localYY;rowU+=p.textureXY;rowV+=p.textureYY;
  }
}
#endif

// Experimental transparent Icon leaf only; the baseline remains untouched.
template<bool Gray,bool NoAA,bool Opaque>
void motionIconKernel(const nice_gfx::MonoIcon &icon,Rect box,Rect content,Rect mask,const motion_geometry::Inverse &inverse,
                      Rect clipped,const DirtyRect &area,uint16_t *pixels,size_t stride,uint16_t tint,uint8_t alpha) {
  const Rect glyph=intersect(mask,{content.x,content.y,icon.width,icon.height});
  if(glyph.width<=0||glyph.height<=0)return;
  const int64_t left=int64_t(glyph.x)*65536,right=int64_t(glyph.x+glyph.width)*65536;
  const int64_t top=int64_t(glyph.y)*65536,bottom=int64_t(glyph.y+glyph.height)*65536;
  const size_t sourceStride=Gray?(icon.width+1)/2:(icon.width+7)/8;
  const uint16_t wireTint=rgb565_blend::toWire(tint);
  for(int32_t y=clipped.y;y<clipped.y+clipped.height;++y){
    int64_t u,v;inverse.point(box,clipped.x,y,u,v);
    uint16_t *target=pixels+size_t(y-area.y)*stride+size_t(clipped.x-area.x);
    for(int32_t x=0;x<clipped.width;++x,++target,u+=inverse.xx,v+=inverse.yx){
      if(u<left||u>=right||v<top||v>=bottom)continue;
      const int32_t ix=int32_t(u>>16)-content.x,iy=int32_t(v>>16)-content.y;
      const uint8_t packed=pgm_read_byte(icon.bitmap+size_t(iy)*sourceStride+size_t(Gray?ix/2:ix/8));
      uint8_t coverage=Gray?uint8_t(((ix&1)?packed&15:packed>>4)*17):uint8_t((packed&(0x80>>(ix&7)))?255:0);
      if(NoAA)coverage=coverage>=128?255:0;
      if(!coverage)continue;
      if(Opaque&&coverage==255){*target=wireTint;continue;}
      const uint16_t under=rgb565_blend::fromWire(*target);
      const uint16_t color=coverage==255?tint:rgb565_blend::color(tint,under,coverage);
      *target=rgb565_blend::toWire(Opaque?color:rgb565_blend::color(color,under,alpha));
    }
  }
}

struct MotionIcon32Tile {
  Rect clipped;DirtyRect area;uint16_t *pixels;size_t stride;
  int32_t u,v,xx,xy,yx,yy;
  uint32_t left,right,top,bottom,contentX,contentY;
  const MonoIcon *icon;uint16_t tint;uint8_t alpha;
};
template<bool Gray,bool NoAA,bool Opaque>
static
#if defined(__GNUC__)
__attribute__((noinline))
#endif
void motionIconSpan32Kernel(const MotionIcon32Tile &p) {
  const MonoIcon &icon=*p.icon;
  const uint32_t sourceStride=Gray?(uint32_t(icon.width)+1)>>1:(uint32_t(icon.width)+7)>>3;
  const uint16_t wireTint=rgb565_blend::toWire(p.tint);
  int32_t rowU=p.u,rowV=p.v;
  for(int32_t y=0;y<p.clipped.height;++y){
    int32_t first=0,end=p.clipped.width;
    if(motionClipSpan(rowU,p.xx,p.left,p.right,first,end)&&motionClipSpan(rowV,p.yx,p.top,p.bottom,first,end)){
      int32_t u=int32_t(int64_t(rowU)+int64_t(p.xx)*first),v=int32_t(int64_t(rowV)+int64_t(p.yx)*first);
      uint16_t *target=p.pixels+size_t(p.clipped.y+y-p.area.y)*p.stride+size_t(p.clipped.x+first-p.area.x);
      for(int32_t x=first;x<end;++x,++target,u+=p.xx,v+=p.yx){
        // The span proves glyph containment and nonnegative coordinates.
        const uint32_t ix=(uint32_t(u)>>16)-p.contentX,iy=(uint32_t(v)>>16)-p.contentY;
        const uint8_t packed=pgm_read_byte(icon.bitmap+size_t(iy)*sourceStride+size_t(Gray?ix>>1:ix>>3));
        uint8_t coverage=Gray?uint8_t(((ix&1)?packed&15:packed>>4)*17):uint8_t((packed&(0x80>>(ix&7)))?255:0);
        if(NoAA)coverage=coverage>=128?255:0;
        if(!coverage)continue;
        if(Opaque&&coverage==255){*target=wireTint;continue;}
        const uint16_t under=rgb565_blend::fromWire(*target);
        const uint16_t color=coverage==255?p.tint:rgb565_blend::color(p.tint,under,coverage);
        *target=rgb565_blend::toWire(Opaque?color:rgb565_blend::color(color,under,p.alpha));
      }
    }
    rowU+=p.xy;rowV+=p.yy;
  }
}

// Exact number of consecutive samples whose nearest-neighbor texel is the
// current texel. Coord is nonnegative after the row-span proof.
static uint32_t motionTexelRun(uint32_t coord,int32_t step,uint32_t remaining) {
  if(!step)return remaining;
  const uint32_t fraction=coord&65535u;
  const uint32_t run=step>0?(65535u-fraction)/uint32_t(step)+1: fraction/uint32_t(-int64_t(step))+1;
  return run<remaining?run:remaining;
}

template<int Format,bool Opaque>
static
#if defined(__GNUC__)
__attribute__((noinline))
#endif
void motionMediaRunsKernel(const MotionMedia32Tile &p) {
  int32_t rowX=p.localX,rowY=p.localY,rowU=p.textureX,rowV=p.textureY;
  for(int32_t y=0;y<p.clipped.height;++y){
    int32_t first=0,end=p.clipped.width;
    if(motionClipSpan(rowX,p.localXX,p.left,p.left+p.maskWidth,first,end)&&
       motionClipSpan(rowY,p.localYX,p.top,p.top+p.maskHeight,first,end)&&
       motionClipSpan(rowU,p.textureXX,0,p.sourceWidth,first,end)&&
       motionClipSpan(rowV,p.textureYX,0,p.sourceHeight,first,end)){
      int32_t u=int32_t(int64_t(rowU)+int64_t(p.textureXX)*first),v=int32_t(int64_t(rowV)+int64_t(p.textureYX)*first);
      uint16_t *target=p.pixels+size_t(p.clipped.y+y-p.area.y)*p.stride+size_t(p.clipped.x+first-p.area.x);
      uint32_t remaining=uint32_t(end-first);
      while(remaining){
        const uint32_t uRun=motionTexelRun(uint32_t(u),p.textureXX,remaining);
        const uint32_t run=motionTexelRun(uint32_t(v),p.textureYX,uRun);
        const uint8_t *source=p.frame+size_t(uint32_t(v)>>16)*p.sourceStride+size_t(uint32_t(u)>>16)*(Format==2?1:2);
        uint16_t color;
        if(Format==1)memcpy(&color,source,2);
        else if(Format==0)color=uint16_t(uint16_t(pgm_read_byte(source))<<8|pgm_read_byte(source+1));
        else {const uint8_t b=pgm_read_byte(source);color=uint16_t((((b>>5)*31+3)/7)<<11|(((b>>2)&7)*9)<<5|((b&3)*31+1)/3);}
        if(Opaque){const uint16_t wireColor=rgb565_blend::toWire(color);for(uint32_t i=0;i<run;++i)*target++=wireColor;}
        else {
          uint32_t i=0;
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
          if((reinterpret_cast<uintptr_t>(target)&3)&&i<run){
            *target=rgb565_blend::toWire(rgb565_blend::color(color,rgb565_blend::fromWire(*target),p.alpha));++target;++i;
          }
          typedef uint32_t MotionBlendWord __attribute__((__may_alias__));
          const uint32_t colors=uint32_t(color)|(uint32_t(color)<<16);
          for(;i+1<run;i+=2,target+=2){auto *pair=reinterpret_cast<MotionBlendWord *>(target);
            *pair=rgb565_blend::swapPairBytes(rgb565_blend::colorPair(colors,rgb565_blend::swapPairBytes(*pair),p.alpha));}
#endif
          for(;i<run;++i,++target)*target=rgb565_blend::toWire(rgb565_blend::color(color,rgb565_blend::fromWire(*target),p.alpha));
        }
        // The original closed-rectangle proof covers the run's one-past end.
        // Products/sums use int64: intermediate products need not fit int32.
        u=int32_t(int64_t(u)+int64_t(p.textureXX)*run);v=int32_t(int64_t(v)+int64_t(p.textureYX)*run);
        remaining-=run;
      }
    }
    rowX+=p.localXY;rowY+=p.localYY;rowU+=p.textureXY;rowV+=p.textureYY;
  }
}

#ifndef NICE_GFX_UI_NATIVE_ALPHA_SHAPE
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_UI_NATIVE_ALPHA_SHAPE 1
#else
#define NICE_GFX_UI_NATIVE_ALPHA_SHAPE 0
#endif
#endif
// Shape fill and stroke may overlap. Compose their opaque coverage in a fixed
// 64-pixel destination row before applying node opacity exactly once. The small
// row block is independent of display dimensions and needs no heap allocation.
struct MotionShapeRow {
  uint16_t *pixels;int32_t x,y,width;uint64_t coverage;
  void fillRect(int32_t left,int32_t top,int32_t w,int32_t h,uint16_t color) {
    if(top>y||int64_t(top)+h<=y)return;
    const int32_t first=std::max(left,x),last=int32_t(std::min(int64_t(left)+w,int64_t(x)+width));
    const uint16_t wire=rgb565_blend::toWire(color);
    for(int32_t px=first;px<last;++px){const uint8_t at=uint8_t(px-x);pixels[at]=wire;coverage|=uint64_t(1)<<at;}
  }
};
// Fill-only contours emit disjoint even-odd spans. Blend those spans directly
// once per tile, instead of repeating polygon setup for each 64-pixel row block.
struct MotionShapeFill {
  uint16_t *pixels;DirtyRect area;size_t stride;uint8_t alpha;bool native=false;
  void fillRect(int32_t x,int32_t y,int32_t width,int32_t height,uint16_t color){
    const Rect r=intersect({x,y,width,height},{area.x,area.y,area.width,area.height});
    const uint16_t wire=native?color:rgb565_blend::toWire(color);
    for(int32_t row=r.y;row<r.y+r.height;++row){
      uint16_t *target=pixels+size_t(row-area.y)*stride+size_t(r.x-area.x);
      for(int32_t col=0;col<r.width;++col,++target)
        *target=alpha==255?wire:(native?rgb565_blend::color(color,*target,alpha):rgb565_blend::toWire(rgb565_blend::color(color,rgb565_blend::fromWire(*target),alpha)));
    }
  }
};

#ifndef NICE_GFX_UI_FAST_OPAQUE_MOTION
#define NICE_GFX_UI_FAST_OPAQUE_MOTION 1
#endif
void paintTransformedNodeWire(int i,Rect clipped,const DirtyRect &area,uint16_t *pixels,size_t stride,uint8_t alpha,bool nativeShape=false,bool nativeMedia=false) {
  const bool nativeOutput=NICE_GFX_UI_NATIVE_AFFINE_MEDIA&&nativeMedia;
  NodeDesc n=rasterFrame.current.page->nodes[i];const State &v=rasterFrame.current.values[i];
  Style style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,i);uint16_t paintFlags=v.flags;const auto *focusedStyle=focusStyle(i);applyFocusStyle(focusedStyle,style,paintFlags);
  ShapeDesc focusedShape{};if(n.shape){focusedShape=*n.shape;applyFocusShape(focusedStyle,focusedShape);n.shape=&focusedShape;}
  const auto &pose=motionPose(*rasterFrame.current.page,rasterFrame.current.values,i);
  Rect box=poseRect(*rasterFrame.current.page,rasterFrame.current.values,i);box.x+=paintOffsetX;box.y+=paintOffsetY;
  const Rect content=contentBox({0,0,box.width,box.height},style);
  const Rect mask=motion_geometry::reveal({0,0,box.width,box.height},pose.reveal,n.motion->flags);
  const motion_geometry::Inverse inverse(pose.scale,pose.rotation,n.motion->pivotX,n.motion->pivotY,n.kind==Kind::Shape);
  const bool disabled=(v.flags&Disabled)!=0,pressed=i==snapshotPressed;
  const bool transparent=disabled?(paintFlags&DisabledTransparent)!=0:pressed?(paintFlags&ActiveTransparent)!=0:(paintFlags&Transparent)!=0;
  const bool gradient=style.gradient&&(disabled?(paintFlags&GradientDisabled)!=0:pressed?(paintFlags&GradientActive)!=0:!(focusedStyle&&(focusedStyle->flags&FocusBackground)));
  const uint16_t background=disabled?style.disabledColor:pressed?style.pressedColor:style.background;
  const int32_t radius=std::min<int32_t>(style.radius,std::min(box.width,box.height)/2),border=style.borderWidth;
  const MediaDesc *source=n.kind==Kind::Media?media(n,v):nullptr;
  MediaDesc decodedMedia{};Image::Impl decodedImage(ImageView(),nullptr,nullptr,0);bool jpeg=false;
  const Image::Impl *image=source&&v.imageOverride?v.image.impl_:nullptr;
  if(source&&mediaJpeg(*source)){
    JpegDecodeTarget target;if(!decodeJpeg(*source,v.mediaFrame,target,n.id))return;
    decodedMedia={reinterpret_cast<const uint8_t *>(target.pixels),target.width,target.height,1,1,2};
    decodedImage.source=ImageView(target.pixels,target.width,target.height,target.capacityBytes,ImageFormat::RGB565,PixelOrder::Native,target.strideBytes);
    decodedImage.media=decodedMedia;source=&decodedMedia;image=&decodedImage;jpeg=true;
  }
  size_t sourceStride=source?(image?image->source.strideBytes:mediaStrideBytes(*source)):0;
  const uint8_t *frame=source?mediaPixels(*source)+(jpeg?0:size_t(v.mediaFrame)*mediaFrameBytes(*source,sourceStride)):nullptr;
#if NICE_GFX_UI_AXIS_MEDIA_CACHE
  const void *immutableFrameIdentity=nullptr;
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if(source&&!image&&!jpeg&&esp_ptr_in_drom(source->pixels)&&esp_ptr_in_drom(frame))immutableFrameIdentity=frame;
#endif
#endif
  const bool keyed=source&&mediaColorKeyed(*source);
  const uint16_t colorKey=keyed?(uint16_t(pgm_read_byte(source->pixels))<<8|pgm_read_byte(source->pixels+1)):0;
  const bool plainMedia=source&&content.x==0&&content.y==0&&content.width==box.width&&content.height==box.height;
  const MediaRoundClip mediaClip({0,0,box.width,box.height},style);
  const bool roundedMedia=source&&style.radius&&mediaClip.rounded();
  bool ram=image&&!image->flash;PixelOrder order=image?image->source.order:PixelOrder::Wire;
#if defined(CONFIG_IDF_TARGET_ESP32P4) && NICE_GFX_UI_NATIVE_CACHE_REPAIR
  // Affine sampling cannot use PPA's arbitrary-angle transform, but it can
  // still read one immutable native RAM texture rather than flash bytes for
  // every output sample. Reuse the existing bounded texture LRUs; neither
  // this painter nor a State keeps the pointer after the synchronous call.
  if(source&&!image&&mediaPixelBytes(*source)==2){
    const size_t bytes=mediaFrameBytes(*source,sourceStride);const uintptr_t address=reinterpret_cast<uintptr_t>(frame);
    if(bytes&&address<=UINTPTR_MAX-bytes&&esp_ptr_in_drom(source->pixels)&&esp_ptr_in_drom(frame)&&
       esp_ptr_in_drom(reinterpret_cast<const void*>(address+bytes-1))){
      platform::OverlayView texture;bool resolved=false;
      if(mediaColorKeyed(*source)){
#if NICE_GFX_UI_KEYED_MEDIA_PPA
        resolved=keyedMediaPpaCache.resolveFrame(*source,v.mediaFrame,source->width,source->height,texture);
#endif
      }else resolved=rgb565MediaCache.resolveFrame(*source,v.mediaFrame,source->width,source->height,texture);
      if(resolved){frame=texture.data;sourceStride=texture.stride*2;ram=true;order=PixelOrder::Native;}
    }
  }
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S31) && NICE_GFX_S31_MOTION_TEXTURE_BUDGET > 0
  if(source&&!image&&source->bytesPerPixel==2){
    if(const uint16_t *texture=motionTextureCache.native(frame,mediaFrameBytes(*source,sourceStride),sourceStride,
        source->width,source->height,order)){
      frame=reinterpret_cast<const uint8_t *>(texture);sourceStride=size_t(source->width)*2;
      ram=true;order=PixelOrder::Native;
    }
  }
#endif
  if(n.kind==Kind::Shape&&n.shape) {
    const Rect shapeBox{box.x+content.x,box.y+content.y,content.width,content.height};
    const shape_raster::TransformedShape shape(*n.shape,shapeBox,box,pose.scale,pose.rotation,style.radius,n.motion->pivotX,n.motion->pivotY);
#if NICE_GFX_UI_NATIVE_ALPHA_SHAPE
    if(nativeShape&&alpha<255){markShapePaintPath(ShapePaintPath::NativeAlphaRow);detail::paintMotionShapeNativeAlpha(shape,clipped,area,pixels,stride,alpha);return;}
#endif
    if(transparent&&!border&&!gradient&&((NICE_GFX_UI_FAST_OPAQUE_MOTION&&alpha==255)||(shape.closed&&(n.shape->flags&ShapeFill)&&shape.stroke<=0))){
      // Opaque fill/stroke overlap is already correct in paint order. Raster
      // once for the dirty rectangle, rather than once per 64-pixel row block.
      markShapePaintPath(ShapePaintPath::TransformedDirectFill);MotionShapeFill target{pixels,area,stride,alpha,nativeShape};shape.paint(target,clipped);return;
    }
    markShapePaintPath(ShapePaintPath::GenericTransformedRows);
    uint16_t under[64];
    for(int32_t y=clipped.y;y<clipped.y+clipped.height;++y)for(int32_t x=clipped.x;x<clipped.x+clipped.width;x+=64) {
      const int32_t width=std::min<int32_t>(64,clipped.x+clipped.width-x);
      uint16_t *target=pixels+size_t(y-area.y)*stride+size_t(x-area.x);
      if(alpha!=255)memcpy(under,target,size_t(width)*2);
      MotionShapeRow row{target,x,y,width,0};
      if(!transparent||border) {
        int64_t localX,localY;inverse.point(box,x,y,localX,localY);
        for(int32_t at=0;at<width;++at,localX+=inverse.xx,localY+=inverse.yx) {
          const int32_t lx=int32_t(localX>>16),ly=int32_t(localY>>16);
          const bool outer=motionRoundPixel(lx,ly,box.width,box.height,radius);
          uint16_t color=background;bool painted=outer&&!transparent;
          if(painted&&gradient){const auto &g=*style.gradient;int64_t q=(int64_t(g.origin)+int64_t(lx)*g.stepX+int64_t(ly)*g.stepY+32768)>>16;
            q=std::max<int64_t>(0,std::min<int64_t>(255,q));const size_t index=g.dither?size_t(q)*4+size_t((ly&1)*2+(lx&1)):size_t(q);color=pgm_read_word(g.colors+index);}
          if(outer&&border&&!motionRoundPixel(lx-border,ly-border,box.width-2*border,box.height-2*border,std::max<int32_t>(0,radius-border))){color=style.borderColor;painted=true;}
          if(painted)row.fillRect(x+at,y,1,1,color);
        }
      }
      shape.paint(row,{x,y,width,1});
      if(alpha!=255)for(int32_t at=0;at<width;++at)if(row.coverage&(uint64_t(1)<<at))
        target[at]=rgb565_blend::toWire(rgb565_blend::color(rgb565_blend::fromWire(target[at]),rgb565_blend::fromWire(under[at]),alpha));
    }
    return;
  }
  if(n.kind==Kind::Icon&&n.icon&&transparent&&!border&&!gradient){
    const Rect glyph=intersect(mask,{content.x,content.y,n.icon->width,n.icon->height});
    if(glyph.width<=0||glyph.height<=0)return;
    int64_t iconU,iconV;inverse.point(box,clipped.x,clipped.y,iconU,iconV);
    const int64_t glyphLeft=int64_t(glyph.x)*65536,glyphRight=int64_t(glyph.x+glyph.width)*65536;
    const int64_t glyphTop=int64_t(glyph.y)*65536,glyphBottom=int64_t(glyph.y+glyph.height)*65536;
    if(glyphLeft>=0&&glyphTop>=0&&glyphRight<=INT32_MAX&&glyphBottom<=INT32_MAX&&
       motionRange32(iconU,inverse.xx,inverse.xy,clipped.width,clipped.height)&&
       motionRange32(iconV,inverse.yx,inverse.yy,clipped.width,clipped.height)){
      const MotionIcon32Tile tile{clipped,area,pixels,stride,int32_t(iconU),int32_t(iconV),inverse.xx,inverse.xy,inverse.yx,inverse.yy,
        uint32_t(glyphLeft),uint32_t(glyphRight),uint32_t(glyphTop),uint32_t(glyphBottom),uint32_t(content.x),uint32_t(content.y),n.icon,style.color,alpha};
      if(n.icon->bitsPerPixel!=4){
        if(alpha==255)motionIconSpan32Kernel<false,false,true>(tile);else motionIconSpan32Kernel<false,false,false>(tile);
      }else if(v.flags&NoAntialias){
        if(alpha==255)motionIconSpan32Kernel<true,true,true>(tile);else motionIconSpan32Kernel<true,true,false>(tile);
      }else{
        if(alpha==255)motionIconSpan32Kernel<true,false,true>(tile);else motionIconSpan32Kernel<true,false,false>(tile);
      }
      return;
    }
    const auto &icon=*n.icon;
    if(icon.bitsPerPixel!=4){
      if(alpha==255)motionIconKernel<false,false,true>(icon,box,content,mask,inverse,clipped,area,pixels,stride,style.color,alpha);
      else motionIconKernel<false,false,false>(icon,box,content,mask,inverse,clipped,area,pixels,stride,style.color,alpha);
    }else if(v.flags&NoAntialias){
      if(alpha==255)motionIconKernel<true,true,true>(icon,box,content,mask,inverse,clipped,area,pixels,stride,style.color,alpha);
      else motionIconKernel<true,true,false>(icon,box,content,mask,inverse,clipped,area,pixels,stride,style.color,alpha);
    }else{
      if(alpha==255)motionIconKernel<true,false,true>(icon,box,content,mask,inverse,clipped,area,pixels,stride,style.color,alpha);
      else motionIconKernel<true,false,false>(icon,box,content,mask,inverse,clipped,area,pixels,stride,style.color,alpha);
    }
    return;
  }
  // Texture coefficients are quantized once, then anchored to the whole box.
  // Clipping/tile size cannot change the sampled texels. Every inner step is
  // addition; integer division is confined to this per-node setup.
  int32_t ux=0,uy=0,vx=0,vy=0;int64_t u0=0,v0=0;
  if(source&&content.width>0&&content.height>0){
    ux=int32_t(int64_t(inverse.xx)*source->width/content.width);uy=int32_t(int64_t(inverse.xy)*source->width/content.width);
    vx=int32_t(int64_t(inverse.yx)*source->height/content.height);vy=int32_t(int64_t(inverse.yy)*source->height/content.height);
    int64_t pivotU,pivotV;motion_geometry::pivot100(box,n.motion->pivotX,n.motion->pivotY,false,pivotU,pivotV);
    u0=(pivotU-int64_t(content.x)*100)*65536*source->width/(int64_t(content.width)*100);
    v0=(pivotV-int64_t(content.y)*100)*65536*source->height/(int64_t(content.height)*100);
  }
  const bool opaquePlainMedia=plainMedia&&!transparent&&!keyed&&!mediaMasked(*source)&&!radius;
  if(plainMedia&&!mediaPacked(*source)&&!roundedMedia&&(transparent||opaquePlainMedia)&&!border&&!gradient){
    if(mask.width<=0||mask.height<=0)return;
    int64_t localX,localY;inverse.point(box,clipped.x,clipped.y,localX,localY);
    int64_t dx,dy;inverse.offset100(box,clipped.x,clipped.y,dx,dy);
    const int64_t textureX=u0+motion_geometry::floorDivide(dx*ux+dy*uy,100),textureY=v0+motion_geometry::floorDivide(dx*vx+dy*vy,100);
    const int64_t left=int64_t(mask.x)*65536,top=int64_t(mask.y)*65536,right=int64_t(mask.x+mask.width)*65536,bottom=int64_t(mask.y+mask.height)*65536;
    if(left>=0&&top>=0&&right<=INT32_MAX&&bottom<=INT32_MAX&&
       motionRange32(localX,inverse.xx,inverse.xy,clipped.width,clipped.height)&&
       motionRange32(localY,inverse.yx,inverse.yy,clipped.width,clipped.height)&&
       motionRange32(textureX,ux,uy,clipped.width,clipped.height)&&
       motionRange32(textureY,vx,vy,clipped.width,clipped.height)){
      const MotionMedia32Tile tile{clipped,area,pixels,stride,int32_t(localX),int32_t(localY),int32_t(textureX),int32_t(textureY),
        inverse.xx,inverse.xy,inverse.yx,inverse.yy,ux,uy,vx,vy,uint32_t(left),uint32_t(top),uint32_t(right-left),uint32_t(bottom-top),
        uint32_t(source->width)*65536,uint32_t(source->height)*65536,frame,sourceStride,source->bytesPerPixel,alpha,ram,order,colorKey
#if NICE_GFX_UI_KEYED_PIE
        ,(nativeOutput&&nativeOverlayTarget&&!stagingNativeTarget&&!buildingSurface&&!foregroundCapture&&!nativeCacheRepairing?&keyedPieUi:nullptr),profilingClock
#endif
#if NICE_GFX_UI_AXIS_MEDIA_CACHE
        ,immutableFrameIdentity?&motionAxisCache:nullptr,immutableFrameIdentity
#if NICE_GFX_UI_AXIS_MEDIA_FULL_FOOTPRINT
        ,(nativeOutput&&keyed&&immutableFrameIdentity&&!inverse.xy&&!inverse.yx&&!uy&&!vx&&box.width>0&&box.width<=4096&&box.height>0&&box.height<=4096&&box.x>=-32768&&box.x<=32767&&box.y>=-32768&&box.y<=32767?
          motion_geometry::bounds(box,pose.scale,pose.rotation,n.motion->pivotX,n.motion->pivotY,false):Rect{})
#endif
#endif
#if NICE_GFX_UI_AXIS_MEDIA_PPA && NICE_GFX_UI_AXIS_MEDIA_CACHE
        ,this,(nativeOutput&&keyed&&immutableFrameIdentity&&!(v.flags&Disabled)&&!focusedStyle&&i!=snapshotPressed&&
          axisPpaPaintDomain(area,pixels,stride)?paintAxisPpa:nullptr),&axisPpaError
#endif
      };
#if NICE_GFX_UI_NATIVE_AFFINE_MEDIA
      if(nativeOutput){
        if(keyed){
          if(ram&&order==PixelOrder::Native){if(alpha==255)motionMediaNative32Kernel<1,true,true>(tile);else motionMediaNative32Kernel<1,false,true>(tile);}
          else {if(alpha==255)motionMediaNative32Kernel<0,true,true>(tile);else motionMediaNative32Kernel<0,false,true>(tile);}
        }else if(source->bytesPerPixel==1){
          if(alpha==255)motionMediaNative32Kernel<2,true,false>(tile);else motionMediaNative32Kernel<2,false,false>(tile);
        }else if(ram&&order==PixelOrder::Native){
          if(alpha==255)motionMediaNative32Kernel<1,true,false>(tile);else motionMediaNative32Kernel<1,false,false>(tile);
        }else {if(alpha==255)motionMediaNative32Kernel<0,true,false>(tile);else motionMediaNative32Kernel<0,false,false>(tile);}
        return;
      }
#endif
      if(opaquePlainMedia)motionMediaBackdrop(tile,background);
      if(keyed){
        // Color-key leaves retain span clipping and 32-bit affine stepping.
        // A transparent texel skips both the read of the underlay and blending.
        if(ram&&order==PixelOrder::Native){
          if(alpha==255)motionMediaSpanKernel<1,true,true>(tile);else motionMediaSpanKernel<1,false,true>(tile);
        }else{
          if(alpha==255)motionMediaSpanKernel<0,true,true>(tile);else motionMediaSpanKernel<0,false,true>(tile);
        }
      }else if(ux>=-8192&&ux<=8192&&uy>=-8192&&uy<=8192&&vx>=-8192&&vx<=8192&&vy>=-8192&&vy<=8192){
      if(source->bytesPerPixel==1){
        if(alpha==255)motionMediaRunsKernel<2,true>(tile);else motionMediaRunsKernel<2,false>(tile);
      }else if(ram&&order==PixelOrder::Native){
        if(alpha==255)motionMediaRunsKernel<1,true>(tile);else motionMediaRunsKernel<1,false>(tile);
      }else{
        if(alpha==255)motionMediaRunsKernel<0,true>(tile);else motionMediaRunsKernel<0,false>(tile);
      }
      }else{
      if(source->bytesPerPixel==1){
        if(alpha==255)motionMediaSpanKernel<2,true>(tile);else motionMediaSpanKernel<2,false>(tile);
      }else if(ram&&order==PixelOrder::Native){
        if(alpha==255)motionMediaSpanKernel<1,true>(tile);else motionMediaSpanKernel<1,false>(tile);
      }else{
        if(alpha==255)motionMediaSpanKernel<0,true>(tile);else motionMediaSpanKernel<0,false>(tile);
      }
      }
      return;
    }
  }
  for(int32_t y=clipped.y;y<clipped.y+clipped.height;++y){
    int64_t localX,localY;inverse.point(box,clipped.x,y,localX,localY);
    int64_t dx,dy;inverse.offset100(box,clipped.x,y,dx,dy);
    int64_t textureX=u0+motion_geometry::floorDivide(dx*ux+dy*uy,100),textureY=v0+motion_geometry::floorDivide(dx*vx+dy*vy,100);
    uint16_t *target=pixels+size_t(y-area.y)*stride+size_t(clipped.x-area.x);
    for(int32_t x=0;x<clipped.width;++x,++target,localX+=inverse.xx,localY+=inverse.yx,textureX+=ux,textureY+=vx){
      if(localX<int64_t(mask.x)*65536||localY<int64_t(mask.y)*65536||
         localX>=int64_t(mask.x+mask.width)*65536||localY>=int64_t(mask.y+mask.height)*65536)continue;
      const int32_t lx=int32_t(localX>>16),ly=int32_t(localY>>16);
      const bool insideMedia=!roundedMedia||mediaClip.containsFixed(localX,localY);
      if(plainMedia&&(transparent||(!mediaMasked(*source)&&!keyed))&&insideMedia&&textureX>=0&&textureY>=0&&textureX<int64_t(source->width)*65536&&textureY<int64_t(source->height)*65536){
        uint16_t color;
        if(!detail::sampleMediaPixel(*source,frame,sourceStride,size_t(textureX>>16),size_t(textureY>>16),ram,order,color))continue;
        const auto blended=alpha==255?color:rgb565_blend::color(color,nativeOutput?*target:rgb565_blend::fromWire(*target),alpha);
        *target=nativeOutput?blended:rgb565_blend::toWire(blended);continue;
      }
      const uint16_t under=nativeOutput?*target:rgb565_blend::fromWire(*target);uint16_t color=under;bool painted=false;
      const bool outer=motionRoundPixel(lx,ly,box.width,box.height,radius);
      if(outer&&!transparent){
        color=background;painted=true;
        if(gradient){const auto &g=*style.gradient;int64_t q=(int64_t(g.origin)+int64_t(lx)*g.stepX+int64_t(ly)*g.stepY+32768)>>16;
          q=std::max<int64_t>(0,std::min<int64_t>(255,q));const size_t index=g.dither?size_t(q)*4+size_t((ly&1)*2+(lx&1)):size_t(q);color=pgm_read_word(g.colors+index);}
      }
      if(outer&&border&&!motionRoundPixel(lx-border,ly-border,box.width-2*border,box.height-2*border,std::max<int32_t>(0,radius-border))){color=style.borderColor;painted=true;}
      if(source&&insideMedia&&content.width>0&&content.height>0&&lx>=content.x&&ly>=content.y&&lx<content.x+content.width&&ly<content.y+content.height&&
         textureX>=0&&textureY>=0&&textureX<int64_t(source->width)*65536&&textureY<int64_t(source->height)*65536){
        uint16_t sample;
        if(detail::sampleMediaPixel(*source,frame,sourceStride,size_t(textureX>>16),size_t(textureY>>16),ram,order,sample)){color=sample;painted=true;}
      }else if(n.kind==Kind::Icon&&n.icon){
        const int32_t ix=lx-content.x,iy=ly-content.y;const MonoIcon &icon=*n.icon;
        if(ix>=0&&iy>=0&&ix<icon.width&&iy<icon.height){const bool gray=icon.bitsPerPixel==4;const size_t row=gray?(icon.width+1)/2:(icon.width+7)/8;
          const uint8_t b=pgm_read_byte(icon.bitmap+size_t(iy)*row+size_t(gray?ix/2:ix/8));
          uint8_t coverage=gray?uint8_t(((ix&1)?b&15:b>>4)*17):uint8_t((b&(0x80>>(ix&7)))?255:0);
          if(v.flags&NoAntialias)coverage=coverage>=128?255:0;
          if(coverage){color=coverage==255?style.color:rgb565_blend::color(style.color,color,coverage);painted=true;}
        }
      }
      if(painted){const auto blended=alpha==255?color:rgb565_blend::color(color,under,alpha);*target=nativeOutput?blended:rgb565_blend::toWire(blended);}
    }
  }
}
void swapMotionRegion(Rect clipped,const DirtyRect &area,uint16_t *pixels,size_t stride){
  for(int32_t y=clipped.y;y<clipped.y+clipped.height;++y){auto *row=pixels+size_t(y-area.y)*stride+size_t(clipped.x-area.x);size_t count=clipped.width;
    while(count>=2){uint32_t pair;memcpy(&pair,row,4);pair=rgb565_blend::swapPairBytes(pair);memcpy(row,&pair,4);row+=2;count-=2;}if(count)*row=rgb565_blend::toWire(*row);
  }
}
void paintTransformedNode(int i,Rect clipped,const DirtyRect &area,uint16_t *pixels,size_t stride,uint8_t alpha){
  const bool native=!buildingSurface&&renderOrder==PixelOrder::Native;
  if(native){
    const auto &n=rasterFrame.current.page->nodes[i];const auto &v=rasterFrame.current.values[i];Style style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,i);uint16_t flags=v.flags;
    applyFocusStyle(focusStyle(i),style,flags);
    const bool transparent=(v.flags&Disabled)?(flags&DisabledTransparent)!=0:i==snapshotPressed?(flags&ActiveTransparent)!=0:(flags&Transparent)!=0;
#if NICE_GFX_UI_OUTLINED_MASK
    if(alpha>0&&(alpha<255||NICE_GFX_UI_OUTLINED_MASK_OPAQUE)&&n.kind==Kind::Shape&&n.shape&&n.motion&&transparent&&!style.borderWidth&&!style.gradient&&
       (n.shape->flags&ShapeStroke)&&!(n.shape->flags&ShapeFill)){
      ShapeDesc outline=*n.shape;applyFocusShape(focusStyle(i),outline);
      const auto &pose=motionPose(*rasterFrame.current.page,rasterFrame.current.values,i);
      Rect box=poseRect(*rasterFrame.current.page,rasterFrame.current.values,i);box.x+=paintOffsetX;box.y+=paintOffsetY;
      const Rect content=contentBox({0,0,box.width,box.height},style);
      const Rect shapeBox{box.x+content.x,box.y+content.y,content.width,content.height};
      const int32_t radius=std::min<int32_t>(style.radius,std::min(box.width,box.height)/2);
      const shape_raster::TransformedShape shape(outline,shapeBox,box,pose.scale,pose.rotation,radius,n.motion->pivotX,n.motion->pivotY);
#if NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE
      const detail::MotionShapeGeometryCache::Geometry *cacheKey=nullptr;
      detail::MotionShapeGeometryCache::Geometry cacheGeometry;
      if(!snapshotTransition&&!rasterFrame.preview.page&&paintCache&&size_t(i)<paintCache->nodes.size()){
        Rect full=paintCache->nodes[i].rect;full.x+=paintOffsetX;full.y+=paintOffsetY;
        cacheGeometry={n.shape,shapeBox,box,full,pose.scale,pose.rotation,n.motion->pivotX,n.motion->pivotY,uint8_t(radius)};cacheKey=&cacheGeometry;
      }
#endif
      if(motionOutlinedMask.paint(shape,clipped,area,pixels,stride,alpha
#if NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE
        ,cacheKey,profilingClock
#endif
        )){
        markShapePaintPath(ShapePaintPath::OutlinedMask);++stats.outlinedMaskHits;stats.outlinedMaskCoveredPixels+=motionOutlinedMask.coveredPixels();return;
      }
      ++stats.outlinedMaskFallbacks;
    }
#endif
    if(NICE_GFX_UI_FAST_OPAQUE_MOTION&&alpha==255&&n.kind==Kind::Shape&&n.shape&&transparent&&!style.borderWidth&&!style.gradient){paintTransformedNodeWire(i,clipped,area,pixels,stride,alpha,true);return;}
#if NICE_GFX_UI_NATIVE_ALPHA_SHAPE
    // Preserve final foreground coverage and /255 rounding while avoiding the
    // Wire565 round trip and repeated 64-column rasterization for small leaves.
    if(alpha>0&&alpha<255&&clipped.width>0&&clipped.width<=detail::NativeAlphaMotionShapeWidth&&
       n.kind==Kind::Shape&&n.shape&&transparent&&!style.borderWidth&&!style.gradient){
      paintTransformedNodeWire(i,clipped,area,pixels,stride,alpha,true);return;
    }
#endif

#if NICE_GFX_UI_NATIVE_AFFINE_MEDIA
    if(n.kind==Kind::Media&&transparent&&!style.borderWidth&&!style.gradient&&!style.radius&&
       !style.paddingTop&&!style.paddingRight&&!style.paddingBottom&&!style.paddingLeft&&!n.instrument&&!v.instrument){
      const auto *source=media(n,v);const Rect box=poseRect(*rasterFrame.current.page,rasterFrame.current.values,i);
      const auto &pose=motionPose(*rasterFrame.current.page,rasterFrame.current.values,i);
      if(source&&!mediaPacked(*source)&&!mediaMasked(*source)&&(source->bytesPerPixel==1||source->bytesPerPixel==2||mediaColorKeyed(*source)||mediaJpeg(*source))&&
         (uint64_t(source->width)*8192>=uint64_t(std::max<int32_t>(0,box.width))*pose.scale||uint64_t(source->height)*8192>=uint64_t(std::max<int32_t>(0,box.height))*pose.scale)){
        ++stats.nativeAffineMediaBlits;paintTransformedNodeWire(i,clipped,area,pixels,stride,alpha,false,true);return;
      }
    }
#endif
  }
  // The existing icon/media/fractional-alpha motion kernels consume Wire565.
  // Convert only their bounded ROI when painting an independent native cache.
  if(native)swapMotionRegion(clipped,area,pixels,stride);
  paintTransformedNodeWire(i,clipped,area,pixels,stride,alpha);
  if(native)swapMotionRegion(clipped,area,pixels,stride);
}
