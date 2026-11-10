#pragma once
#include "AlignedMemory.h"
#include "AxisMediaPpa.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#include <esp_heap_caps.h>
#endif

#ifndef NICE_GFX_UI_AXIS_MEDIA_FULL_FOOTPRINT
#define NICE_GFX_UI_AXIS_MEDIA_FULL_FOOTPRINT 1
#endif

#ifndef NICE_GFX_UI_AXIS_MEDIA_RUNS
#define NICE_GFX_UI_AXIS_MEDIA_RUNS 1
#endif

namespace nice_gfx { namespace htmlui { namespace detail {
// Only immutable, application-lifetime source identities may be cached. The
// borrowed native source is read during resolve() and is never retained.
// Exact sampled-index arrays, rather than approximate transforms, form the key.
class MotionAxisCache {
 public:
  struct Run {uint16_t start=0,length=0;};
  struct Texture {
    const uint16_t *pixels=nullptr;size_t stride=0;const uint32_t *rowOffsets=nullptr;const Run *runs=nullptr;uint32_t runCount=0;
    size_t width=0,height=0,pixelBytes=0;
    // A short synchronous lease: resolving another texture may evict it.
    // Public field mutation cannot turn an unrelated/guarded view into DMA.
    bool ppaPlaneFor(const MotionAxisCache &owner)const{
      return certifiedOwner_==&owner&&pixels==certifiedPixels_&&stride==certifiedStride_&&
        width==certifiedWidth_&&height==certifiedHeight_&&pixelBytes==certifiedBytes_&&certifiedBytes_;
    }
   private:
    friend class MotionAxisCache;
    const MotionAxisCache *certifiedOwner_=nullptr;const uint16_t *certifiedPixels_=nullptr;
    size_t certifiedStride_=0,certifiedWidth_=0,certifiedHeight_=0,certifiedBytes_=0;
  };
  struct Statistics {uint64_t hits=0,misses=0,evictions=0,rejected=0;size_t bytes=0;
    uint64_t runBuilds=0,runBuildRejected=0,runPaints=0,runSegments=0,runOpaquePixels=0,runBlendPixels=0,runSkippedPixels=0;size_t runMetadataBytes=0;
    uint64_t fullFootprintPaints=0,fullFootprintFallbacks=0,fullFootprintSubsetPixels=0;};
#ifndef NICE_GFX_UI_AXIS_MEDIA_CACHE_BYTES
#define NICE_GFX_UI_AXIS_MEDIA_CACHE_BYTES 3145728u
#endif
  explicit MotionAxisCache(size_t budget=NICE_GFX_UI_AXIS_MEDIA_CACHE_BYTES):budget_(budget){}
  ~MotionAxisCache(){for(auto &slot:slots_)discard(slot);}
  MotionAxisCache(const MotionAxisCache&)=delete;
  MotionAxisCache&operator=(const MotionAxisCache&)=delete;
  const Statistics &statistics()const{return stats_;}
  void recordFullFootprint(bool painted,uint64_t pixels=0){if(painted){++stats_.fullFootprintPaints;stats_.fullFootprintSubsetPixels+=pixels;}else ++stats_.fullFootprintFallbacks;}
  void recordRunPaint(bool opaque,uint64_t segments,uint64_t pixels,uint64_t skipped){
    ++stats_.runPaints;stats_.runSegments+=segments;stats_.runSkippedPixels+=skipped;
    if(opaque)stats_.runOpaquePixels+=pixels;else stats_.runBlendPixels+=pixels;
  }
  bool resolve(const void *immutableIdentity,const uint8_t *nativeSource,
      size_t sourceStride,size_t sourceWidth,size_t sourceHeight,
      const uint16_t *sampleX,size_t width,const uint16_t *sampleY,size_t height,Texture &out,uint16_t colorKey=0){
    out=Texture{};const uint16_t runKey=NICE_GFX_UI_AXIS_MEDIA_RUNS?colorKey:0;
    if(!immutableIdentity||!nativeSource||!sampleX||!sampleY||!width||!height||
       width>256||height>256||!sourceWidth||sourceWidth>65535||!sourceHeight||sourceHeight>65535||
       sourceStride<sourceWidth*2||sourceHeight>SIZE_MAX/sourceStride||
       reinterpret_cast<uintptr_t>(nativeSource)>UINTPTR_MAX-sourceHeight*sourceStride){++stats_.rejected;return false;}
    for(size_t x=0;x<width;++x)if(sampleX[x]>=sourceWidth){++stats_.rejected;return false;}
    for(size_t y=0;y<height;++y)if(sampleY[y]>=sourceHeight){++stats_.rejected;return false;}
    uint32_t hash=2166136261u;
    for(size_t x=0;x<width;++x)hash=(hash^sampleX[x])*16777619u;
    for(size_t y=0;y<height;++y)hash=(hash^sampleY[y])*16777619u;
    for(auto &slot:slots_)if(slot.memory&&slot.identity==immutableIdentity&&slot.hash==hash&&
        slot.width==width&&slot.height==height&&slot.sourceWidth==sourceWidth&&slot.sourceHeight==sourceHeight&&slot.sourceStride==sourceStride&&slot.runKey==runKey&&
        !memcmp(slot.xMap(),sampleX,width*2)&&!memcmp(slot.yMap(),sampleY,height*2)){
      slot.used=++clock_;++stats_.hits;texture(slot,out);return true;
    }
    ++stats_.misses;
    // The experimental PPA input owns complete 64-byte pixel lines. Maps and
    // runs remain separate from that plane; all sampled texels stay exact.
    const size_t alignment=NICE_GFX_UI_AXIS_MEDIA_PPA?32u:8u;
    const size_t stride=(width+alignment-1)&~(alignment-1),pixelBytes=stride*height*2;
    const size_t needed=pixelBytes+(width+height)*2;
    if(needed>budget_){++stats_.rejected;return false;}
    while(stats_.bytes>budget_-needed){auto *slot=lru();if(!slot)break;discard(*slot);++stats_.evictions;}
    Slot *slot=nullptr;for(auto &entry:slots_)if(!entry.memory){slot=&entry;break;}
    if(!slot){slot=lru();if(!slot){++stats_.rejected;return false;}discard(*slot);++stats_.evictions;}
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    if(heap_caps_get_free_size(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT)<needed+2u*1024u*1024u){++stats_.rejected;return false;}
    auto *memory=static_cast<uint16_t*>(heap_caps_aligned_alloc(64,needed,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
#else
    auto *memory=static_cast<uint16_t*>(allocateAlignedMemory<64>(needed));
#endif
    if(!memory){++stats_.rejected;return false;}
    for(size_t y=0;y<height;++y){
      const uint8_t *source=nativeSource+size_t(sampleY[y])*sourceStride;uint16_t *target=memory+y*stride;
      for(size_t x=0;x<width;++x)memcpy(target+x,source+size_t(sampleX[x])*2,2);
      for(size_t x=width;x<stride;++x)target[x]=0;
    }
    slot->memory=memory;slot->identity=immutableIdentity;slot->hash=hash;slot->used=++clock_;
    slot->width=width;slot->height=height;slot->stride=stride;slot->sourceWidth=sourceWidth;slot->sourceHeight=sourceHeight;slot->sourceStride=sourceStride;slot->bytes=needed;slot->runKey=runKey;
    memcpy(slot->xMap(),sampleX,width*2);memcpy(slot->yMap(),sampleY,height*2);
    stats_.bytes+=needed;
#if NICE_GFX_UI_AXIS_MEDIA_RUNS
    buildRuns(*slot,colorKey);
#endif
    texture(*slot,out);return true;
  }
 private:
  struct Slot {
    uint16_t *memory=nullptr;const void *identity=nullptr;uint64_t used=0;
    uint32_t hash=0;size_t width=0,height=0,stride=0,sourceWidth=0,sourceHeight=0,sourceStride=0,bytes=0;
    void *runMemory=nullptr;size_t runBytes=0;uint32_t runCount=0;uint16_t runKey=0;
    uint16_t *xMap()const{return memory+stride*height;}
    uint16_t *yMap()const{return xMap()+width;}
  };
  void texture(const Slot &slot,Texture &out)const{
    out=Texture{};out.pixels=slot.memory;out.stride=slot.stride;
    out.width=slot.width;out.height=slot.height;out.pixelBytes=slot.stride*slot.height*2;
#if NICE_GFX_UI_AXIS_MEDIA_PPA
    if(!(reinterpret_cast<uintptr_t>(slot.memory)&63)&&!(out.pixelBytes&63)&&!((slot.stride*2)&63)){
      out.certifiedOwner_=this;out.certifiedPixels_=out.pixels;out.certifiedStride_=out.stride;
      out.certifiedWidth_=out.width;out.certifiedHeight_=out.height;out.certifiedBytes_=out.pixelBytes;
    }
#endif
#if NICE_GFX_UI_AXIS_MEDIA_RUNS
    if(slot.runMemory){
      out.rowOffsets=static_cast<const uint32_t*>(slot.runMemory);
      out.runs=reinterpret_cast<const Run*>(out.rowOffsets+slot.height+1);out.runCount=slot.runCount;
    }
#endif
  }
  void buildRuns(Slot &slot,uint16_t colorKey){
    // Count at most ceil(width/2) non-key runs per row. Width/height are
    // already in [1,256], so the largest run count is 32768 (not 65536).
    uint32_t count=0;
    for(size_t y=0;y<slot.height;++y){
      const uint16_t *row=slot.memory+y*slot.stride;bool active=false;
      for(size_t x=0;x<slot.width;++x){const bool visible=row[x]!=colorKey;if(visible&&!active)++count;active=visible;}
    }
    const size_t bytes=(slot.height+1)*sizeof(uint32_t)+size_t(count)*sizeof(Run);
    // Metadata is optional. It never evicts another texture or turns a
    // successful ordinary texture resolve into a failure.
    if(stats_.bytes>budget_||bytes>budget_-stats_.bytes){++stats_.runBuildRejected;return;}
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    if(heap_caps_get_free_size(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT)<bytes+2u*1024u*1024u){++stats_.runBuildRejected;return;}
    void *memory=heap_caps_aligned_alloc(64,bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
    void *memory=allocateAlignedMemory<64>(bytes);
#endif
    if(!memory){++stats_.runBuildRejected;return;}
    auto *offsets=static_cast<uint32_t*>(memory);auto *runs=reinterpret_cast<Run*>(offsets+slot.height+1);
    uint32_t at=0;offsets[0]=0;
    for(size_t y=0;y<slot.height;++y){
      const uint16_t *row=slot.memory+y*slot.stride;size_t x=0;
      while(x<slot.width){
        if(row[x]==colorKey){++x;continue;}
        const size_t first=x;do{++x;}while(x<slot.width&&row[x]!=colorKey);
        runs[at++]={uint16_t(first),uint16_t(x-first)};
      }
      offsets[y+1]=at;
    }
    slot.runMemory=memory;slot.runBytes=bytes;slot.runCount=count;slot.bytes+=bytes;
    stats_.bytes+=bytes;stats_.runMetadataBytes+=bytes;++stats_.runBuilds;
  }

  Slot *lru(){Slot *result=nullptr;for(auto &slot:slots_)if(slot.memory&&(!result||slot.used<result->used))result=&slot;return result;}
  void discard(Slot &slot){if(slot.runMemory){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    heap_caps_free(slot.runMemory);
#else
    freeAlignedMemory<64>(slot.runMemory);
#endif
    stats_.runMetadataBytes-=slot.runBytes;}if(slot.memory){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    heap_caps_free(slot.memory);
#else
    freeAlignedMemory<64>(slot.memory);
#endif
    stats_.bytes-=slot.bytes;}slot=Slot{};
  }
  Slot slots_[192];size_t budget_;uint64_t clock_=0;Statistics stats_;
};
}}}
