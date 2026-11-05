#pragma once
#include "../Shapes.h"
#include "AlignedMemory.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <new>
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#include <esp_heap_caps.h>
#endif

namespace nice_gfx { namespace htmlui { namespace detail {
// An immutable ARGB8888 foreground, rather than a screenshot of the backdrop.
// Opaque shape spans overwrite one another in the original paint order. The
// PPA applies node/ancestor opacity once when composing onto the live frame.
class ShapeOverlayCache {
 public:
  struct Texture {
    const uint32_t *pixels=nullptr;
    size_t width=0,height=0,stride=0,bytes=0;
  };
  struct NativeSpan {uint16_t x=0,y=0,width=0,color=0;};
  struct NativeTexture {
    const NativeSpan *spans=nullptr;size_t count=0,bytes=0,width=0,height=0;
    const uint32_t *argb=nullptr;size_t argbBytes=0;
    NativeTexture()=default;
    NativeTexture(const NativeSpan *runs,size_t runCount,size_t allocated,size_t w,size_t h,
                  const uint32_t *raster=nullptr,size_t rasterBytes=0)
        :spans(runs),count(runCount),bytes(allocated),width(w),height(h),argb(raster),argbBytes(rasterBytes){}
    // The cache emits ordered, bounded, immutable runs. Raw borrowers still
    // need the complete preflight. Copying a lease preserves its certificate;
    // changing any of its public source metadata revokes that certificate.
    bool trustedNativeSpans()const{return certified_&&spans==certifiedSpans_&&count==certifiedCount_&&
      bytes==certifiedBytes_&&width==certifiedWidth_&&height==certifiedHeight_&&
      argb==certifiedArgb_&&argbBytes==certifiedArgbBytes_;}
   private:
    friend class ShapeOverlayCache;
    void certify(){certifiedSpans_=spans;certifiedCount_=count;certifiedBytes_=bytes;certifiedWidth_=width;
      certifiedHeight_=height;certifiedArgb_=argb;certifiedArgbBytes_=argbBytes;certified_=true;}
    const NativeSpan *certifiedSpans_=nullptr;const uint32_t *certifiedArgb_=nullptr;
    size_t certifiedCount_=0,certifiedBytes_=0,certifiedWidth_=0,certifiedHeight_=0,certifiedArgbBytes_=0;
    bool certified_=false;
  };
  using Allocate=void *(*)(size_t);
  using Free=void (*)(void *);
  static constexpr size_t DefaultBudget=2u*1024u*1024u,SlotCount=64;
  explicit ShapeOverlayCache(size_t budget=DefaultBudget,Allocate allocate=allocateMemory,Free release=freeMemory)
      :budget_(budget),allocate_(allocate),free_(release){}
  ~ShapeOverlayCache(){clear();}
  ShapeOverlayCache(const ShapeOverlayCache &)=delete;
  ShapeOverlayCache &operator=(const ShapeOverlayCache &)=delete;
  void clear(){for(auto &slot:slots_)discard(slot);clock_=hits_=misses_=evictions_=0;}
  size_t bytes()const{return bytes_;}
  uint64_t hits()const{return hits_;}
  uint64_t misses()const{return misses_;}
  uint64_t evictions()const{return evictions_;}

  bool resolve(const ShapeDesc &shape,Rect box,uint8_t radius,Texture &output){
    output=Texture{};
    Key key{};if(!makeKey(shape,box,radius,key))return false;
    const size_t stride=(size_t(box.width)+15)&~size_t(15),height=size_t(box.height);
    if(height>SIZE_MAX/stride/4)return false;
    const size_t bytes=stride*height*4;
    if(bytes>budget_)return false;
    const uint32_t hash=hashKey(key);Slot *slot=find(key,hash);
    if(slot&&slot->pixels)++hits_;
    else {
      ++misses_;
      if(!slot)for(auto &entry:slots_)if(!occupied(entry)){slot=&entry;break;}
      if(!slot){slot=lru();if(!slot)return false;discard(*slot);++evictions_;}
      while(bytes_>budget_-bytes){auto *oldest=lru(slot);if(!oldest)return false;discard(*oldest);++evictions_;}
      auto *pixels=static_cast<uint32_t *>(allocate_(bytes));
      if(!pixels)return false;
      if(reinterpret_cast<uintptr_t>(pixels)&63){free_(pixels);return false;}
      memset(pixels,0,bytes);
      Recorder recorder{pixels,stride,int32_t(box.width),int32_t(box.height)};
      const Rect local{0,0,box.width,box.height};
      renderShape(recorder,shape,local,local,radius);
      slot->key=key;slot->hash=hash;slot->pixels=pixels;slot->stride=stride;slot->bytes=bytes;bytes_+=bytes;
    }
    slot->used=++clock_;
    output={slot->pixels,key.width,key.height,slot->stride,slot->bytes};return true;
  }

  // Build final nontransparent, constant-color runs once. Ordinary PPA users
  // retain the original lazy ARGB cache without any additional scan/allocation.
  bool resolveNative(const ShapeDesc &shape,Rect box,uint8_t radius,NativeTexture &output){
    output=NativeTexture{};Key key{};if(!makeKey(shape,box,radius,key))return false;
    Slot *slot=find(key,hashKey(key));
    if(slot&&slot->nativeReady){
      ++hits_;slot->used=++clock_;releaseArgb(*slot);
      output={slot->nativeSpans,slot->nativeCount,slot->nativeBytes,key.width,key.height,nullptr,0};output.certify();return true;
    }
    Texture texture;if(!resolve(shape,box,radius,texture))return false;
    slot=find(key,hashKey(key));if(!slot)return false;
    if(!slot->nativeReady){
      size_t count=0;
      const auto visit=[&](NativeSpan *spans)->bool{
        size_t at=0;
        for(size_t y=0;y<texture.height;++y){const uint32_t *row=texture.pixels+y*texture.stride;
          for(size_t x=0;x<texture.width;){const uint32_t color=row[x];if(!(color>>24)){++x;continue;}
            if((color>>24)!=255)return false;const size_t first=x++;
            while(x<texture.width&&row[x]==color)++x;
            if(at>=65536)return false;
            if(spans)spans[at]={uint16_t(first),uint16_t(y),uint16_t(x-first),uint16_t(((color>>19)&31)<<11|((color>>10)&63)<<5|((color>>3)&31))};
            ++at;
          }
        }
        count=at;return true;
      };
      if(!visit(nullptr))return false;
      const size_t needed=(count*sizeof(NativeSpan)+63)&~size_t(63);
      if(needed>budget_)return false;
      while(bytes_>budget_-needed){auto *oldest=lru(slot);if(!oldest)return false;discard(*oldest);++evictions_;}
      NativeSpan *spans=needed?static_cast<NativeSpan *>(allocate_(needed)):nullptr;
      if(needed&&!spans)return false;
      if(spans&&(reinterpret_cast<uintptr_t>(spans)&63)){free_(spans);return false;}
      if(spans&&!visit(spans)){free_(spans);return false;}
      slot->nativeSpans=spans;slot->nativeCount=count;slot->nativeBytes=needed;slot->nativeReady=true;bytes_+=needed;
    }
    // Final runs contain every visible RGB565 color. Keeping the transient
    // full ARGB raster would evict sparse SVG chrome on every frame. Any later
    // PPA resolve can rebuild that raster under the same key and lease rules.
    releaseArgb(*slot);
    output={slot->nativeSpans,slot->nativeCount,slot->nativeBytes,key.width,key.height,nullptr,0};output.certify();return true;
  }

 private:
  struct Key {ShapeDesc shape{};ShapePoint points[64]{};uint16_t width=0,height=0;uint8_t radius=0;};
  struct Slot {Key key{};uint32_t *pixels=nullptr;size_t stride=0,bytes=0;uint32_t hash=0;uint64_t used=0;
    NativeSpan *nativeSpans=nullptr;size_t nativeCount=0,nativeBytes=0;bool nativeReady=false;};
  bool makeKey(const ShapeDesc &shape,Rect box,uint8_t radius,Key &key)const{
    // Match the bounded integer-translation domain checked by the native
    // shape parity suite; unusual coordinates retain the ordinary rasterizer.
    if(!budget_||!allocate_||!free_||box.width<=0||box.height<=0||box.width>4096||box.height>4096||
       box.x < -1024||box.y < -1024||int64_t(box.x)+box.width>2048||int64_t(box.y)+box.height>2048||
       shape.count>64||(shape.count&&!shape.points))return false;
    if(uint8_t(shape.type)>uint8_t(ShapeType::Polygon)||shape.flags>3||shape.strokeWidth>64||
       !shape.viewWidth||shape.viewWidth>4096||!shape.viewHeight||shape.viewHeight>4096||
       ((shape.flags&ShapeStroke)&&!shape.strokeWidth))return false;
    const bool vertices=shape.type==ShapeType::Line||shape.type==ShapeType::Polyline||shape.type==ShapeType::Polygon;
    if(vertices){if((shape.type==ShapeType::Line?shape.count!=2:shape.count<(shape.type==ShapeType::Polygon?3:2))||
       ((shape.type==ShapeType::Line||shape.type==ShapeType::Polyline)&&(shape.flags&ShapeFill)))return false;
      for(uint8_t i=0;i<shape.count;++i)if(shape.points[i].x>shape.viewWidth||shape.points[i].y>shape.viewHeight)return false;
    }else if(shape.count||shape.points)return false;
    key.shape=shape;key.shape.points=nullptr;
    key.width=uint16_t(box.width);key.height=uint16_t(box.height);key.radius=radius;
    if(shape.count)memcpy(key.points,shape.points,size_t(shape.count)*sizeof(ShapePoint));return true;
  }
  static bool occupied(const Slot &slot){return slot.pixels||slot.nativeReady;}
  Slot *find(const Key &key,uint32_t hash){for(auto &entry:slots_)if(occupied(entry)&&entry.hash==hash&&sameKey(entry.key,key))return &entry;return nullptr;}
  struct Recorder {
    uint32_t *pixels;size_t stride;int32_t width,height;
    void fillRect(int32_t x,int32_t y,int32_t w,int32_t h,uint32_t color){
      const Rect r=shape_raster::intersection({x,y,w,h},{0,0,width,height});
      if(r.width<=0||r.height<=0)return;
      const uint32_t red=(color>>11)&31,green=(color>>5)&63,blue=color&31;
      const uint32_t argb=0xff000000u|(((red<<3)|(red>>2))<<16)|(((green<<2)|(green>>4))<<8)|((blue<<3)|(blue>>2));
      for(int32_t row=0;row<r.height;++row){auto *out=pixels+size_t(r.y+row)*stride+r.x;
        for(int32_t column=0;column<r.width;++column)out[column]=argb;}
    }
  };
  static uint32_t hashKey(const Key &key){
    uint32_t hash=2166136261u;const auto add=[&](uint32_t value){hash=(hash^value)*16777619u;};
    const auto &s=key.shape;add(uint8_t(s.type));add(s.fillColor);add(s.strokeColor);add(s.strokeWidth);
    add(s.flags);add(s.count);add(s.viewWidth);add(s.viewHeight);add(key.width);add(key.height);add(key.radius);
    for(uint8_t i=0;i<s.count;++i){add(key.points[i].x);add(key.points[i].y);}return hash;
  }
  static bool sameKey(const Key &a,const Key &b){
    const auto &x=a.shape,&y=b.shape;
    return a.width==b.width&&a.height==b.height&&a.radius==b.radius&&x.type==y.type&&x.fillColor==y.fillColor&&
      x.strokeColor==y.strokeColor&&x.strokeWidth==y.strokeWidth&&x.flags==y.flags&&x.count==y.count&&
      x.viewWidth==y.viewWidth&&x.viewHeight==y.viewHeight&&!memcmp(a.points,b.points,size_t(x.count)*sizeof(ShapePoint));
  }
  Slot *lru(Slot *keep=nullptr){Slot *oldest=nullptr;for(auto &slot:slots_)if(&slot!=keep&&occupied(slot)&&(!oldest||slot.used<oldest->used))oldest=&slot;return oldest;}
  void releaseArgb(Slot &slot){if(slot.pixels){free_(slot.pixels);bytes_-=slot.bytes;slot.pixels=nullptr;slot.stride=slot.bytes=0;}}
  void discard(Slot &slot){if(slot.pixels){free_(slot.pixels);bytes_-=slot.bytes;}if(slot.nativeSpans){free_(slot.nativeSpans);bytes_-=slot.nativeBytes;}slot=Slot{};}
  static void *allocateMemory(size_t bytes){
#if defined(CONFIG_IDF_TARGET_ESP32S31)
    return heap_caps_aligned_alloc(64,bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
    return allocateAlignedMemory<64>(bytes);
#endif
  }
  static void freeMemory(void *memory){
#if defined(CONFIG_IDF_TARGET_ESP32S31)
    heap_caps_free(memory);
#else
    freeAlignedMemory<64>(memory);
#endif
  }
  Slot slots_[SlotCount];size_t budget_=0,bytes_=0;uint64_t clock_=0,hits_=0,misses_=0,evictions_=0;
  Allocate allocate_;Free free_;
};
}}}
