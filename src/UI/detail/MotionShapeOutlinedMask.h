#pragma once
#include "MediaShapeIram.h"
#include "../../Display/Transport.h"
#include "../../Platform/Common/UiMemory.h"
#include "../Shapes.h"
#include "Rgb565Blend.h"
#include "SolidForegroundLut.h"
#ifndef NICE_GFX_UI_OUTLINED_PACKED_ALPHA
#define NICE_GFX_UI_OUTLINED_PACKED_ALPHA 0
#endif
#if NICE_GFX_UI_OUTLINED_PACKED_ALPHA
#include "SolidForegroundBlend.h"
#endif
#ifndef NICE_GFX_UI_OUTLINED_FOREGROUND_LUT
#define NICE_GFX_UI_OUTLINED_FOREGROUND_LUT 1
#endif
#ifndef NICE_GFX_UI_OUTLINED_FOREGROUND_LUT_MIN_COVERED
#define NICE_GFX_UI_OUTLINED_FOREGROUND_LUT_MIN_COVERED 256
#endif
#include "MotionShapeGeometryCache.h"
#include <string.h>

// Per-View outlined mask; default ON for ESP32-P4, OFF elsewhere; explicit overrides are retained.
#ifndef NICE_GFX_UI_OUTLINED_MASK
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define NICE_GFX_UI_OUTLINED_MASK 1
#else
#define NICE_GFX_UI_OUTLINED_MASK 0
#endif
#endif
#define NICE_GFX_UI_OUTLINED_MASK_VERSION 1
#ifndef NICE_GFX_UI_OUTLINED_MASK_OPAQUE
#define NICE_GFX_UI_OUTLINED_MASK_OPAQUE 1
#endif
#define NICE_GFX_UI_OUTLINED_MASK_OPAQUE_VERSION 1

// Isolated A/B candidates; preserve the established raster and exact /255 kernel.
#ifndef NICE_GFX_UI_OUTLINED_MASK_FAST_SPANS
#define NICE_GFX_UI_OUTLINED_MASK_FAST_SPANS 0
#endif
#ifndef NICE_GFX_UI_OUTLINED_MASK_RUN_BLEND
#define NICE_GFX_UI_OUTLINED_MASK_RUN_BLEND 0
#endif
#ifndef NICE_GFX_UI_OUTLINED_MASK_INLINE_CTZ
#define NICE_GFX_UI_OUTLINED_MASK_INLINE_CTZ 0
#endif

namespace nice_gfx { namespace htmlui { namespace detail {

// Only a no-fill stroke is admitted: every foreground span then has the same
// final color. Coverage union replaces repeated foreground writes, and global
// opacity is applied exactly once after the established rasterizer finishes.
// One UI owner borrows the mask synchronously. No frame/State retains it.
class MotionShapeOutlinedMask {
 public:
  static constexpr size_t Budget = 16u * 1024u;
  using Allocate = void *(*)(size_t);
  using Free = void (*)(void *);
  explicit MotionShapeOutlinedMask(size_t budget=Budget, Allocate allocate=allocateMemory, Free release=freeMemory)
      :budget_(std::min(budget,Budget)&~size_t(3)),allocate_(allocate),free_(release){}
  ~MotionShapeOutlinedMask(){if(words_&&free_)free_(words_);}
  MotionShapeOutlinedMask(const MotionShapeOutlinedMask&)=delete;
  MotionShapeOutlinedMask&operator=(const MotionShapeOutlinedMask&)=delete;
  size_t bytes()const{return words_?budget_:0;}
  uint32_t coveredPixels()const{return coveredPixels_;}
  SolidForegroundLutStats foregroundLutStats()const{
#if NICE_GFX_UI_OUTLINED_FOREGROUND_LUT
    return foregroundLutStats_;
#else
    return {};
#endif
  }
  void resetForegroundLutStats(){
#if NICE_GFX_UI_OUTLINED_FOREGROUND_LUT
    foregroundLutStats_=SolidForegroundLutStats{};
#endif
  }
  bool allocationAttempted()const{return attempted_;}
  size_t geometryCacheBytes()const{
#if NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE
    return geometryCache_.bytes();
#else
    return 0;
#endif
  }
  OutlinedGeometryCacheStats geometryCacheStats()const{
#if NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE
    return geometryCache_.stats();
#else
    return OutlinedGeometryCacheStats{};
#endif
  }
  void resetGeometryCacheStats(){
#if NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE
    geometryCache_.resetStats();
#endif
  }
  bool configureGeometryCache(size_t budget,MotionShapeGeometryCache::Allocate allocate,MotionShapeGeometryCache::Free release,MotionShapeGeometryCache::Available available){
#if NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE
    return geometryCache_.configure(budget,allocate,release,available);
#else
    (void)budget;(void)allocate;(void)release;(void)available;return false;
#endif
  }

  NICE_GFX_UI_MEDIA_SHAPE_IRAM_ATTR bool paint(const shape_raster::TransformedShape &shape, Rect clipped,
      const DirtyRect &area, uint16_t *pixels, size_t stride, uint8_t alpha
#if NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE
      ,const MotionShapeGeometryCache::Geometry *geometry=nullptr,uint32_t (*clock)()=nullptr
#endif
      ){
    coveredPixels_=0;
    // Optional opaque strokes publish coverage by stores only. Invalid/oversized
    // inputs return before allocating, clearing, or touching the destination.
    if(!alpha||(!NICE_GFX_UI_OUTLINED_MASK_OPAQUE&&alpha==255)||!pixels||!shape.count||shape.stroke<=0||
       !(shape.shape.flags&ShapeStroke)||(shape.shape.flags&ShapeFill)||
       clipped.width<=0||clipped.height<=0||area.width<=0||area.height<=0||
       stride<size_t(area.width)||clipped.x<area.x||clipped.y<area.y||
       int64_t(clipped.x)+clipped.width>INT32_MAX||int64_t(clipped.y)+clipped.height>INT32_MAX||
       int64_t(clipped.x)+clipped.width>int64_t(area.x)+area.width||
       int64_t(clipped.y)+clipped.height>int64_t(area.y)+area.height||
       size_t(area.height-1)>(SIZE_MAX-size_t(area.width))/stride)return false;
    const size_t rowWords=(size_t(clipped.width)+31)/32;
    if(!budget_||rowWords>budget_/sizeof(uint32_t)||
       size_t(clipped.height)>budget_/(rowWords*sizeof(uint32_t)))return false;
    const size_t required=rowWords*size_t(clipped.height)*sizeof(uint32_t);
    if(!words_){
      // One bounded attempt per View lifetime. OOM never becomes per-frame
      // malloc pressure; later paints continue through the unchanged fallback.
      if(attempted_||!allocate_||!free_)return false;
      attempted_=true;void *memory=allocate_(budget_);
      if(!memory)return false;
      if(reinterpret_cast<uintptr_t>(memory)%alignof(uint32_t)){free_(memory);return false;}
      words_=static_cast<uint32_t*>(memory);
    }
    bool coverageReady=false;
#if NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE
    MotionShapeGeometryCache::Key key;MotionShapeGeometryCache::Lease cached;
    bool admitted=geometry&&MotionShapeGeometryCache::makeKey(shape,*geometry,key);
    size_t fullWords=0,fullBytes=0;
    if(admitted){const Rect full=geometry->bounds;
      admitted=clipped.x>=full.x&&clipped.y>=full.y&&int64_t(clipped.x)+clipped.width<=int64_t(full.x)+full.width&&int64_t(clipped.y)+clipped.height<=int64_t(full.y)+full.height;
      fullWords=(size_t(full.width)+31)/32;
      admitted=admitted&&fullWords&&fullWords<=budget_/sizeof(uint32_t)&&size_t(full.height)<=budget_/(fullWords*sizeof(uint32_t));
      if(admitted){fullBytes=fullWords*size_t(full.height)*sizeof(uint32_t);
#if !NICE_GFX_UI_OUTLINED_CACHE_SMALL_ROI
        admitted=geometryCache_.canStore(fullBytes,shape.closed?shape.count:shape.count?shape.count-1:0,MotionShapeGeometryCache::pointCount(key));
#endif
      }
    }
    if(admitted){
      const bool hit=geometryCache_.find(key,cached);
      bool buildAllowed=!hit;
#if NICE_GFX_UI_OUTLINED_CACHE_SMALL_ROI
      if(buildAllowed&&uint64_t(clipped.width)*uint64_t(clipped.height)*4<uint64_t(geometry->bounds.width)*uint64_t(geometry->bounds.height)){
        geometryCache_.smallRoiBypassed();buildAllowed=false;
      }
      if(buildAllowed)buildAllowed=geometryCache_.canStore(fullBytes,shape.closed?shape.count:shape.count?shape.count-1:0,MotionShapeGeometryCache::pointCount(key));
#endif
      if(buildAllowed){
        const uint32_t at=clock?clock():0;
        memset(words_,0,fullBytes);memset(fullInk_,0,sizeof(fullInk_));
        TrackedMask fullCanvas{{words_,fullWords,geometry->bounds,shape.shape.strokeColor,true},fullInk_};shape_raster::Raster<TrackedMask> raster{fullCanvas,geometry->bounds};
        for(uint8_t i=1;i<shape.count;++i){
#if NICE_GFX_UI_OUTLINED_CACHE_TRACKED_SPANS
          fullCanvas.begin(i-1);
#else
          fullCanvas.segment=i-1;
#endif
          raster.capsule(shape.points[i-1],shape.points[i],shape.stroke,shape.shape.strokeColor);
#if NICE_GFX_UI_OUTLINED_CACHE_TRACKED_SPANS
          fullCanvas.finish();
#endif
        }
        if(shape.closed){
#if NICE_GFX_UI_OUTLINED_CACHE_TRACKED_SPANS
          fullCanvas.begin(shape.count-1);
#else
          fullCanvas.segment=shape.count-1;
#endif
          raster.capsule(shape.points[shape.count-1],shape.points[0],shape.stroke,shape.shape.strokeColor);
#if NICE_GFX_UI_OUTLINED_CACHE_TRACKED_SPANS
          fullCanvas.finish();
#endif
        }
        geometryCache_.noteFullBuild(fullBytes,required,clock?uint32_t(clock()-at):0);
        const size_t segments=fullCanvas.bounded?(shape.closed?shape.count:shape.count?shape.count-1:0):0;
        if(fullCanvas.target.singleColor)geometryCache_.store(key,fullWords,words_,fullBytes,cached,segments?fullInk_:nullptr,segments);
      }
      const bool stable=cached.words&&MotionShapeGeometryCache::canExtract(shape,cached,clipped);
      if(stable){const uint32_t at=clock?clock():0;
        coverageReady=MotionShapeGeometryCache::extract(cached,clipped,words_,rowWords);
        if(coverageReady){geometryCache_.noteExtract(required,clock?uint32_t(clock()-at):0);if(hit)geometryCache_.servedHit();else geometryCache_.servedColdReuse();}
      }
      if(!coverageReady){if(cached.words&&!stable)geometryCache_.boundaryRejected();else geometryCache_.rejected();}
    }else if(geometry)geometryCache_.rejected();
#endif
    if(!coverageReady){
#if NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE
      const uint32_t at=clock?clock():0;
#endif
      memset(words_,0,required);Mask canvas{words_,rowWords,clipped,shape.shape.strokeColor,true};shape.paint(canvas,clipped);
      // A full cache rejection never changes the original clipped fallback.
      if(!canvas.singleColor)return false;
#if NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE
      geometryCache_.noteRoiBuild(clock?uint32_t(clock()-at):0);
#endif
    }
    // Probe only the established scratch coverage, never target holes. Tiny
    // masks retain the original scalar/pair kernel without table build work.
#if NICE_GFX_UI_OUTLINED_PACKED_ALPHA
    const SolidForegroundBlend foregroundBlend(shape.shape.strokeColor,alpha);
#endif
#if NICE_GFX_UI_OUTLINED_FOREGROUND_LUT && !NICE_GFX_UI_OUTLINED_PACKED_ALPHA
    const bool useForegroundLut=alpha<255&&SolidForegroundLut::enoughCoverage(words_,rowWords*size_t(clipped.height),NICE_GFX_UI_OUTLINED_FOREGROUND_LUT_MIN_COVERED,foregroundLutStats_.probeWords);
    if(useForegroundLut){++foregroundLutStats_.draws;
      if(foregroundLut_.matches(shape.shape.strokeColor,alpha))++foregroundLutStats_.reuses;
      else{uint32_t at=0;
#if NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE
        if(clock)at=clock();
#endif
        foregroundLut_.reset(shape.shape.strokeColor,alpha);++foregroundLutStats_.builds;
#if NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE
        if(clock)foregroundLutStats_.buildMicros+=uint32_t(clock()-at);
#endif
      }
    }else if(alpha<255)++foregroundLutStats_.smallBypasses;
#endif
    uint32_t covered=0;
    for(int32_t y=0;y<clipped.height;++y){
      auto *target=pixels+size_t(int64_t(clipped.y)-area.y+y)*stride+size_t(int64_t(clipped.x)-area.x);
      const auto *row=words_+size_t(y)*rowWords;
#if NICE_GFX_UI_OUTLINED_MASK_RUN_BLEND
      const uint32_t foreground=uint32_t(shape.shape.strokeColor)|(uint32_t(shape.shape.strokeColor)<<16);
      for(size_t word=0;word<rowWords;++word){uint32_t bits=row[word];
        while(bits){
          unsigned first=0;
#if NICE_GFX_UI_OUTLINED_MASK_INLINE_CTZ
          first=coverageCtz(bits);
#elif defined(__GNUC__) || defined(__clang__)
          first=unsigned(__builtin_ctz(bits));
#else
          while(!(bits&(uint32_t(1)<<first)))++first;
#endif
          const uint32_t shifted=bits>>first;unsigned length=0;
#if NICE_GFX_UI_OUTLINED_MASK_INLINE_CTZ
          length=shifted==~uint32_t(0)?32:coverageCtz(~shifted);
#elif defined(__GNUC__) || defined(__clang__)
          length=shifted==~uint32_t(0)?32:unsigned(__builtin_ctz(~shifted));
#else
          while(length<32-first&&(shifted&(uint32_t(1)<<length)))++length;
#endif
          const uint32_t run=length==32?~uint32_t(0):((uint32_t(1)<<length)-1)<<first;
          bits&=~run;covered+=length;
          uint16_t *out=target+word*32+first;
#if NICE_GFX_UI_OUTLINED_MASK_OPAQUE
          if(alpha==255){
            if((reinterpret_cast<uintptr_t>(out)&3)&&length){*out++=shape.shape.strokeColor;--length;}
            while(length>=2){memcpy(out,&foreground,4);out+=2;length-=2;}
            if(length)*out=shape.shape.strokeColor;
            continue;
          }
#endif
#if NICE_GFX_UI_OUTLINED_PACKED_ALPHA
          if((reinterpret_cast<uintptr_t>(out)&3)&&length){*out=foregroundBlend.blendNative(*out);++out;--length;}
#if defined(__GNUC__)
          // The original head peel proves four-byte alignment. may_alias keeps
          // the existing uint16_t storage valid under strict alias analysis.
          typedef uint32_t BlendWord __attribute__((__may_alias__));
          auto *pairs=reinterpret_cast<BlendWord*>(out);
          while(length>=2){const uint32_t background=*pairs;*pairs=foregroundBlend.blendPair(background);++pairs;length-=2;}
          out=reinterpret_cast<uint16_t*>(pairs);
#else
          while(length>=2){uint32_t background;memcpy(&background,out,4);const uint32_t blended=foregroundBlend.blendPair(background);memcpy(out,&blended,4);out+=2;length-=2;}
#endif
          if(length)*out=foregroundBlend.blendNative(*out);
#else
#if NICE_GFX_UI_OUTLINED_FOREGROUND_LUT
          if(useForegroundLut){
            if((reinterpret_cast<uintptr_t>(out)&3)&&length){*out=foregroundLut_.lookupNative(*out);++out;--length;}
            while(length>=2){uint32_t background;memcpy(&background,out,4);const uint32_t blended=foregroundLut_.lookupPair(background);memcpy(out,&blended,4);out+=2;length-=2;}
            if(length)*out=foregroundLut_.lookupNative(*out);
            continue;
          }
#endif
          if((reinterpret_cast<uintptr_t>(out)&3)&&length){*out=rgb565_blend::color(shape.shape.strokeColor,*out,alpha);++out;--length;}
          while(length>=2){uint32_t background;memcpy(&background,out,4);const uint32_t blended=rgb565_blend::colorPair(foreground,background,alpha);memcpy(out,&blended,4);out+=2;length-=2;}
          if(length)*out=rgb565_blend::color(shape.shape.strokeColor,*out,alpha);
#endif
        }
      }
#else
      for(size_t word=0;word<rowWords;++word){uint32_t bits=row[word];
        while(bits){
          unsigned bit=0;
#if defined(__GNUC__) || defined(__clang__)
          bit=unsigned(__builtin_ctz(bits));
#else
          while(!(bits&(uint32_t(1)<<bit)))++bit;
#endif
          bits&=bits-1;const size_t at=word*32+bit;
#if NICE_GFX_UI_OUTLINED_MASK_OPAQUE
          if(alpha==255)target[at]=shape.shape.strokeColor;else
#endif
#if NICE_GFX_UI_OUTLINED_PACKED_ALPHA
          target[at]=foregroundBlend.blendNative(target[at]);
#elif NICE_GFX_UI_OUTLINED_FOREGROUND_LUT
          target[at]=useForegroundLut?foregroundLut_.lookupNative(target[at]):rgb565_blend::color(shape.shape.strokeColor,target[at],alpha);
#else
          target[at]=rgb565_blend::color(shape.shape.strokeColor,target[at],alpha);
#endif
          ++covered;
        }
      }
#endif
    }
    coveredPixels_=covered;
#if NICE_GFX_UI_OUTLINED_FOREGROUND_LUT && !NICE_GFX_UI_OUTLINED_PACKED_ALPHA
    if(useForegroundLut)foregroundLutStats_.pixels+=covered;
#endif
    return true;
  }

 private:
#if NICE_GFX_UI_OUTLINED_MASK_INLINE_CTZ
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((always_inline))
#endif
  static inline unsigned coverageCtz(uint32_t value){
    // Called only with nonzero input. The low bit is a power of two; all 32
    // products index distinct entries. Avoid the RV32 software helper call.
    static const uint8_t index[32]={0,1,28,2,29,14,24,3,30,22,20,15,25,17,4,8,31,27,13,23,21,19,16,7,26,12,18,6,11,5,10,9};
    return index[((value&(0u-value))*0x077cb531u)>>27];
  }
#endif
  struct Mask {
    uint32_t *words;size_t rowWords;Rect clip;uint16_t color;bool singleColor;
    NICE_GFX_UI_MEDIA_SHAPE_IRAM_ATTR void fillRect(int32_t left,int32_t top,int32_t width,int32_t height,uint16_t foreground){
      if(foreground!=color){singleColor=false;return;}
      if(width<=0||height<=0)return;
#if NICE_GFX_UI_OUTLINED_MASK_FAST_SPANS
      // Raster::span already clips its one-row writes. Check that exact domain
      // in unsigned differences before avoiding the general int64 clip path.
      if(height==1&&left>=clip.x&&top>=clip.y){
        const uint32_t begin=uint32_t(left)-uint32_t(clip.x),y=uint32_t(top)-uint32_t(clip.y);
        if(y<uint32_t(clip.height)&&begin<uint32_t(clip.width)&&uint32_t(width)<=uint32_t(clip.width)-begin){
          const uint32_t end=begin+uint32_t(width),firstWord=begin/32,lastWord=(end-1)/32;
          const uint32_t firstMask=~uint32_t(0)<<(begin&31),lastMask=(end&31)?(uint32_t(1)<<(end&31))-1:~uint32_t(0);
          auto *row=words+size_t(y)*rowWords;
          if(firstWord==lastWord)row[firstWord]|=firstMask&lastMask;
          else{row[firstWord]|=firstMask;for(size_t word=firstWord+1;word<lastWord;++word)row[word]=~uint32_t(0);row[lastWord]|=lastMask;}
          return;
        }
      }
#endif
      const int64_t first=std::max<int64_t>(left,clip.x),last=std::min<int64_t>(int64_t(left)+width,int64_t(clip.x)+clip.width);
      const int64_t firstY=std::max<int64_t>(top,clip.y),lastY=std::min<int64_t>(int64_t(top)+height,int64_t(clip.y)+clip.height);
      if(first>=last||firstY>=lastY)return;
      const size_t begin=size_t(first-clip.x),end=size_t(last-clip.x);
      const size_t firstWord=begin/32,lastWord=(end-1)/32;
      const uint32_t firstMask=~uint32_t(0)<<(begin&31);
      const uint32_t lastMask=(end&31)?(uint32_t(1)<<(end&31))-1:~uint32_t(0);
      for(int64_t y=firstY;y<lastY;++y){auto *row=words+size_t(y-clip.y)*rowWords;
        if(firstWord==lastWord)row[firstWord]|=firstMask&lastMask;
        else {row[firstWord]|=firstMask;for(size_t word=firstWord+1;word<lastWord;++word)row[word]=~uint32_t(0);row[lastWord]|=lastMask;}
      }
    }
  };
#if NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE
#if NICE_GFX_UI_OUTLINED_CACHE_TRACKED_SPANS
  struct TrackedMask {
    Mask target;MotionShapeGeometryCache::SegmentInk *ink;size_t segment=0;bool bounded=true;
    uint32_t left=UINT32_MAX,top=UINT32_MAX,right=0,bottom=0;
    void begin(size_t index){segment=index;left=top=UINT32_MAX;right=bottom=0;}
    void finish(){
      if(!bounded||left==UINT32_MAX)return;
      if(left>65535||top>65535||right-left>255||bottom-top>255){bounded=false;return;}
      ink[segment]={uint16_t(left),uint16_t(top),uint8_t(right-left),uint8_t(bottom-top)};
    }
    void note(uint32_t x,uint32_t y,uint32_t end,uint32_t endY){
      if(!bounded)return;left=std::min(left,x);top=std::min(top,y);right=std::max(right,end);bottom=std::max(bottom,endY);
    }
    // Only original Raster::span enters this helper. It emits height-one,
    // constant-color writes. Keep a checked narrow path and the old fallback
    // for any other call, without repeating Mask::fillRect's clip a second time.
    NICE_GFX_UI_MEDIA_SHAPE_IRAM_ATTR void fillRect(int32_t x,int32_t y,int32_t width,int32_t height,uint16_t color){
      const auto &clip=target.clip;
      if(color==target.color&&width>0&&height==1&&x>=clip.x&&y>=clip.y){
        const uint32_t begin=uint32_t(x)-uint32_t(clip.x),rowY=uint32_t(y)-uint32_t(clip.y);
        if(rowY<uint32_t(clip.height)&&begin<uint32_t(clip.width)&&uint32_t(width)<=uint32_t(clip.width)-begin){
          const uint32_t end=begin+uint32_t(width),firstWord=begin/32,lastWord=(end-1)/32;
          const uint32_t firstMask=~uint32_t(0)<<(begin&31),lastMask=(end&31)?(uint32_t(1)<<(end&31))-1:~uint32_t(0);
          auto *row=target.words+size_t(rowY)*target.rowWords;
          if(firstWord==lastWord)row[firstWord]|=firstMask&lastMask;
          else{row[firstWord]|=firstMask;for(size_t word=firstWord+1;word<lastWord;++word)row[word]=~uint32_t(0);row[lastWord]|=lastMask;}
          note(begin,rowY,end,rowY+1);return;
        }
      }
      target.fillRect(x,y,width,height,color);
      if(!bounded||color!=target.color||width<=0||height<=0)return;
      const int64_t l=std::max<int64_t>(x,clip.x),r=std::min<int64_t>(int64_t(x)+width,int64_t(clip.x)+clip.width);
      const int64_t t=std::max<int64_t>(y,clip.y),b=std::min<int64_t>(int64_t(y)+height,int64_t(clip.y)+clip.height);
      if(l<r&&t<b)note(uint32_t(l-clip.x),uint32_t(t-clip.y),uint32_t(r-clip.x),uint32_t(b-clip.y));
    }
  };
#else
  struct TrackedMask {
    Mask target;MotionShapeGeometryCache::SegmentInk *ink;size_t segment=0;bool bounded=true;
    NICE_GFX_UI_MEDIA_SHAPE_IRAM_ATTR void fillRect(int32_t left,int32_t top,int32_t width,int32_t height,uint16_t color){
      target.fillRect(left,top,width,height,color);
      if(!bounded||color!=target.color||width<=0||height<=0)return;
      const auto &clip=target.clip;
      const int64_t l=std::max<int64_t>(left,clip.x),r=std::min<int64_t>(int64_t(left)+width,int64_t(clip.x)+clip.width);
      const int64_t t=std::max<int64_t>(top,clip.y),b=std::min<int64_t>(int64_t(top)+height,int64_t(clip.y)+clip.height);
      if(l>=r||t>=b)return;
      auto &entry=ink[segment];uint32_t x=uint32_t(l-clip.x),y=uint32_t(t-clip.y),right=uint32_t(r-clip.x),bottom=uint32_t(b-clip.y);
      if(entry.width&&entry.height){x=std::min(x,uint32_t(entry.x));y=std::min(y,uint32_t(entry.y));right=std::max(right,uint32_t(entry.x)+entry.width);bottom=std::max(bottom,uint32_t(entry.y)+entry.height);}
      if(x>65535||y>65535||right-x>255||bottom-y>255){bounded=false;return;}
      entry={uint16_t(x),uint16_t(y),uint8_t(right-x),uint8_t(bottom-y)};
    }
  };
#endif
#endif
  static void *allocateMemory(size_t bytes){return platform::allocateUiMetadata(bytes,64u*1024u);}
  static void freeMemory(void *memory){platform::freeUiMemory(memory);}
#if NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE
  MotionShapeGeometryCache geometryCache_;
  MotionShapeGeometryCache::SegmentInk fullInk_[64];
#endif
#if NICE_GFX_UI_OUTLINED_FOREGROUND_LUT
  SolidForegroundLut foregroundLut_;SolidForegroundLutStats foregroundLutStats_;
#endif
  size_t budget_=0;Allocate allocate_=nullptr;Free free_=nullptr;
  uint32_t *words_=nullptr;uint32_t coveredPixels_=0;bool attempted_=false;
};

}}}
