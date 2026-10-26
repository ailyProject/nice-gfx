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

struct MotionMedia32Tile {
  Rect clipped;DirtyRect area;uint16_t *pixels;size_t stride;
  int32_t localX,localY,textureX,textureY;
  int32_t localXX,localXY,localYX,localYY,textureXX,textureXY,textureYX,textureYY;
  uint32_t left,top,maskWidth,maskHeight,sourceWidth,sourceHeight;
  const uint8_t *frame;size_t sourceStride;uint8_t bytesPerPixel,alpha;bool ram;PixelOrder order;uint16_t colorKey;
};
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
  uint16_t *pixels;DirtyRect area;size_t stride;uint8_t alpha;
  void fillRect(int32_t x,int32_t y,int32_t width,int32_t height,uint16_t color){
    const Rect r=intersect({x,y,width,height},{area.x,area.y,area.width,area.height});
    const uint16_t wire=rgb565_blend::toWire(color);
    for(int32_t row=r.y;row<r.y+r.height;++row){
      uint16_t *target=pixels+size_t(row-area.y)*stride+size_t(r.x-area.x);
      for(int32_t col=0;col<r.width;++col,++target)
        *target=alpha==255?wire:rgb565_blend::toWire(rgb565_blend::color(color,rgb565_blend::fromWire(*target),alpha));
    }
  }
};

void paintTransformedNode(int i,Rect clipped,const DirtyRect &area,uint16_t *pixels,size_t stride,uint8_t alpha) {
  const NodeDesc &n=rasterFrame.current.page->nodes[i];const State &v=rasterFrame.current.values[i];
  const Style &style=styleFor(*rasterFrame.current.page,rasterFrame.current.values,i);const auto &pose=motionPose(*rasterFrame.current.page,rasterFrame.current.values,i);
  Rect box=poseRect(*rasterFrame.current.page,rasterFrame.current.values,i);box.x+=paintOffsetX;box.y+=paintOffsetY;
  const Rect content=contentBox({0,0,box.width,box.height},style);
  const Rect mask=motion_geometry::reveal({0,0,box.width,box.height},pose.reveal,n.motion->flags);
  const motion_geometry::Inverse inverse(pose.scale,pose.rotation,n.motion->pivotX,n.motion->pivotY,n.kind==Kind::Shape);
  const bool disabled=(v.flags&Disabled)!=0,pressed=i==snapshotPressed;
  const bool transparent=disabled?(v.flags&DisabledTransparent)!=0:pressed?(v.flags&ActiveTransparent)!=0:(v.flags&Transparent)!=0;
  const bool gradient=style.gradient&&(disabled?(v.flags&GradientDisabled)!=0:pressed?(v.flags&GradientActive)!=0:true);
  const uint16_t background=disabled?style.disabledColor:pressed?style.pressedColor:style.background;
  const int32_t radius=std::min<int32_t>(style.radius,std::min(box.width,box.height)/2),border=style.borderWidth;
  const MediaDesc *source=n.kind==Kind::Media?media(n,v):nullptr;
  const Image::Impl *image=source&&v.imageOverride?v.image.impl_:nullptr;
  size_t sourceStride=source?(image?image->source.strideBytes:mediaStrideBytes(*source)):0;
  const uint8_t *frame=source?mediaPixels(*source)+size_t(v.mediaFrame)*mediaFrameBytes(*source,sourceStride):nullptr;
  const bool keyed=source&&mediaColorKeyed(*source);
  const uint16_t colorKey=keyed?(uint16_t(pgm_read_byte(source->pixels))<<8|pgm_read_byte(source->pixels+1)):0;
  const bool plainMedia=source&&content.x==0&&content.y==0&&content.width==box.width&&content.height==box.height;
  const MediaRoundClip mediaClip({0,0,box.width,box.height},style);
  const bool roundedMedia=source&&style.radius&&mediaClip.rounded();
  bool ram=image&&!image->flash;PixelOrder order=image?image->source.order:PixelOrder::Wire;
#if defined(CONFIG_IDF_TARGET_ESP32S31) && NICE_GFX_S31_MOTION_TEXTURE_BUDGET > 0
  if(source&&!v.imageOverride&&source->bytesPerPixel==2){
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
    if(transparent&&!border&&!gradient&&shape.closed&&(n.shape->flags&ShapeFill)&&shape.stroke<=0){
      MotionShapeFill target{pixels,area,stride,alpha};shape.paint(target,clipped);return;
    }
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
        uint32_t(source->width)*65536,uint32_t(source->height)*65536,frame,sourceStride,source->bytesPerPixel,alpha,ram,order,colorKey};
      if(opaquePlainMedia)motionMediaBackdrop(tile,background);
      if(keyed){
        // Color-key leaves retain span clipping and 32-bit affine stepping.
        // A transparent texel skips both the read of the underlay and blending.
        if(alpha==255)motionMediaSpanKernel<0,true,true>(tile);else motionMediaSpanKernel<0,false,true>(tile);
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
        *target=rgb565_blend::toWire(alpha==255?color:rgb565_blend::color(color,rgb565_blend::fromWire(*target),alpha));continue;
      }
      const uint16_t under=rgb565_blend::fromWire(*target);uint16_t color=under;bool painted=false;
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
      if(painted)*target=rgb565_blend::toWire(alpha==255?color:rgb565_blend::color(color,under,alpha));
    }
  }
}
