#pragma once
#include "../Shapes.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#include <esp_heap_caps.h>
#endif

#ifndef NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE
#define NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE 1
#endif
#ifndef NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE_BYTES
#define NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE_BYTES 12582912
#endif
#define NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE_VERSION 2
#ifndef NICE_GFX_UI_OUTLINED_CACHE_POINTS
#define NICE_GFX_UI_OUTLINED_CACHE_POINTS 1
#endif
#define NICE_GFX_UI_OUTLINED_CACHE_POINTS_VERSION 1
#ifndef NICE_GFX_UI_OUTLINED_CACHE_SMALL_ROI
#define NICE_GFX_UI_OUTLINED_CACHE_SMALL_ROI 1
#endif
#ifndef NICE_GFX_UI_OUTLINED_CACHE_TRACKED_SPANS
#define NICE_GFX_UI_OUTLINED_CACHE_TRACKED_SPANS 1
#endif
#ifndef NICE_GFX_UI_OUTLINED_CACHE_VALUE_KEYS
#define NICE_GFX_UI_OUTLINED_CACHE_VALUE_KEYS 1
#endif

namespace nice_gfx { namespace htmlui { namespace detail {

struct OutlinedGeometryCacheStats {
  uint64_t hits=0,misses=0,rejected=0,boundaryRejected=0,evictions=0,allocationFailures=0;
  uint64_t fullBuildMicros=0,roiBuildMicros=0,extractMicros=0;
  uint64_t coldReuses=0,smallRoiBypasses=0;
  uint64_t fullBuildMaskBytes=0,requestedMaskBytes=0,extractedBytes=0;
  size_t arenaBytes=0,usedBytes=0,peakUsedBytes=0,entries=0;
};

// Coverage only, never target pixels. The owner borrows a lease synchronously
// until its next insert. The single UI task never publishes these masks to DMA.
class MotionShapeGeometryCache {
 public:
  static constexpr size_t MaximumBudget=12u*1024u*1024u;
  static constexpr size_t Reserve=4u*1024u*1024u;
  static constexpr size_t BucketCount=4096;
  static constexpr size_t BucketBytes=BucketCount*sizeof(uint32_t);
  using Allocate=void *(*)(size_t);
  using Free=void (*)(void *);
  using Available=size_t (*)();
  struct Geometry {
    const ShapeDesc *identity=nullptr;
    Rect content{},pivot{},bounds{};
    uint16_t scale=1024;int16_t rotation=0,pivotX=50,pivotY=50;uint8_t radius=0;
    Geometry()=default;
    Geometry(const ShapeDesc *id,Rect c,Rect p,Rect b,uint16_t z,int16_t angle,int16_t px,int16_t py,uint8_t r)
        :identity(id),content(c),pivot(p),bounds(b),scale(z),rotation(angle),pivotX(px),pivotY(py),radius(r){}
  };
  struct ScalarKey {
    uintptr_t identity=0;
    uint32_t fields[27]{};
    bool operator==(const ScalarKey &other)const{
      if(identity!=other.identity)return false;
      for(size_t i=0;i<27;++i)if(fields[i]!=other.fields[i])return false;
      return true;
    }
  };
#if NICE_GFX_UI_OUTLINED_CACHE_POINTS
  struct Key:ScalarKey {
    uint32_t count=0;ShapePoint points[64]{};
    bool operator==(const Key &other)const{return count<=64&&other.count<=64&&ScalarKey::operator==(other)&&count==other.count&&(!count||!memcmp(points,other.points,count*sizeof(ShapePoint)));}
  };
#else
  using Key=ScalarKey;
#endif
  static size_t pointCount(const Key &key){
#if NICE_GFX_UI_OUTLINED_CACHE_POINTS
    return key.count;
#else
    (void)key;return 0;
#endif
  }
  struct SegmentInk {uint16_t x=0,y=0;uint8_t width=0,height=0;};
  static_assert(sizeof(SegmentInk)==6,"bounded exact segment ink metadata");
  struct Lease {const uint32_t *words=nullptr;const SegmentInk *ink=nullptr;Rect bounds{};size_t rowWords=0,bytes=0,segments=0;};

  explicit MotionShapeGeometryCache(size_t budget=NICE_GFX_UI_OUTLINED_GEOMETRY_CACHE_BYTES,
      Allocate allocate=allocateMemory,Free release=freeMemory,Available available=availableMemory)
      :budget_(budget<=MaximumBudget?budget:MaximumBudget),allocate_(allocate),free_(release),available_(available){}
  ~MotionShapeGeometryCache(){if(arena_&&free_)free_(arena_);}
  MotionShapeGeometryCache(const MotionShapeGeometryCache&)=delete;
  MotionShapeGeometryCache&operator=(const MotionShapeGeometryCache&)=delete;
  bool configure(size_t budget,Allocate allocate,Free release,Available available){
    if(attempted_||arena_||budget>MaximumBudget||!allocate||!release||!available)return false;
    budget_=budget;allocate_=allocate;free_=release;available_=available;return true;
  }
  bool attempted()const{return attempted_;}
  size_t bytes()const{return arena_?budget_:0;}
  OutlinedGeometryCacheStats stats()const{auto result=stats_;result.arenaBytes=bytes();result.usedBytes=used_;result.entries=entries_;return result;}
  void resetStats(){stats_=OutlinedGeometryCacheStats{};stats_.peakUsedBytes=used_;}
  void rejected(){++stats_.rejected;}
  void boundaryRejected(){++stats_.boundaryRejected;++stats_.rejected;}
  void servedHit(){++stats_.hits;}
  void servedColdReuse(){++stats_.coldReuses;}
  void smallRoiBypassed(){++stats_.smallRoiBypasses;}
  void noteFullBuild(size_t fullBytes,size_t requestedBytes,uint32_t micros){stats_.fullBuildMaskBytes+=fullBytes;stats_.requestedMaskBytes+=requestedBytes;stats_.fullBuildMicros+=micros;}
  void noteRoiBuild(uint32_t micros){stats_.roiBuildMicros+=micros;}
  void noteExtract(size_t bytes,uint32_t micros){stats_.extractedBytes+=bytes;stats_.extractMicros+=micros;}

  // Scalar primitives preserve their legacy key. Optional point shapes copy
  // every authored point into a bounded value key and variable-size record tail.
  // No source descriptor/points lease is retained by the cache.
  static bool makeKey(const shape_raster::TransformedShape &shape,const Geometry &g,Key &key){
    const auto &s=shape.shape;
    bool pointShape=false;
#if NICE_GFX_UI_OUTLINED_CACHE_POINTS
    pointShape=s.type==ShapeType::Line||s.type==ShapeType::Polyline||s.type==ShapeType::Polygon;
    key.count=0;
    if(pointShape){
      if(!s.points||s.count>64||(s.type==ShapeType::Line?s.count!=2:s.count<(s.type==ShapeType::Polygon?3:2))||
         !s.viewWidth||s.viewWidth>4096||!s.viewHeight||s.viewHeight>4096)return false;
      for(uint8_t i=0;i<s.count;++i)if(s.points[i].x>s.viewWidth||s.points[i].y>s.viewHeight)return false;
      key.count=s.count;memcpy(key.points,s.points,size_t(s.count)*sizeof(ShapePoint));
    }
#endif
    if(!g.identity||(!pointShape&&(s.count||s.points||uint8_t(s.type)>uint8_t(ShapeType::Rect)))||
       !(s.flags&ShapeStroke)||(s.flags&ShapeFill)||!g.scale||
       g.bounds.width<=0||g.bounds.height<=0||
       g.bounds.x<-32768||g.bounds.y<-32768||int64_t(g.bounds.x)+g.bounds.width>32768||int64_t(g.bounds.y)+g.bounds.height>32768)return false;
#if NICE_GFX_UI_OUTLINED_CACHE_POINTS
    if(pointShape)key.identity=0;else
#endif
#if NICE_GFX_UI_OUTLINED_CACHE_VALUE_KEYS
    // Pointer-free primitives depend solely on the full scalar/absolute key.
    // Retain no descriptor lease; the current shape still checks clip semantics
    // and supplies its current foreground color/alpha on every served draw.
    key.identity=0;
#else
    key.identity=reinterpret_cast<uintptr_t>(g.identity);
#endif
    const Rect boxes[]={g.content,g.pivot,g.bounds};size_t at=0;
    for(auto r:boxes){key.fields[at++]=uint32_t(r.x);key.fields[at++]=uint32_t(r.y);key.fields[at++]=uint32_t(r.width);key.fields[at++]=uint32_t(r.height);}
    key.fields[at++]=g.scale;key.fields[at++]=uint32_t(int32_t(g.rotation));key.fields[at++]=uint32_t(int32_t(g.pivotX));key.fields[at++]=uint32_t(int32_t(g.pivotY));key.fields[at++]=g.radius;
    key.fields[at++]=uint8_t(s.type);key.fields[at++]=s.strokeWidth;key.fields[at++]=s.flags;key.fields[at++]=s.viewWidth;key.fields[at++]=s.viewHeight;key.fields[at++]=s.fillColor;key.fields[at++]=s.strokeColor;key.fields[at++]=shape.count;key.fields[at++]=shape.closed?1:0;
    uint32_t stroke;static_assert(sizeof(stroke)==sizeof(shape.stroke),"float coverage identity");memcpy(&stroke,&shape.stroke,sizeof(stroke));key.fields[at++]=stroke;
    return at==27;
  }
  static uint32_t hashKey(const Key &key){
    uint32_t h=2166136261u;uintptr_t identity=key.identity;
    for(size_t i=0;i<sizeof(identity);++i){h^=uint8_t(identity);h*=16777619u;identity>>=8;}
    for(uint32_t field:key.fields)for(unsigned i=0;i<4;++i){h^=uint8_t(field);h*=16777619u;field>>=8;}
#if NICE_GFX_UI_OUTLINED_CACHE_POINTS
    // Preserve the legacy primitive hash exactly. Point records retain complete
    // original coordinates, never a borrowed mutable source pointer or hash alone.
    if(key.count>64)return 0;
    if(key.count){for(unsigned i=0;i<4;++i){h^=uint8_t(key.count>>(i*8));h*=16777619u;}
      for(size_t n=0;n<key.count;++n)for(uint32_t v:{uint32_t(key.points[n].x),uint32_t(key.points[n].y)})for(unsigned i=0;i<2;++i){h^=uint8_t(v);h*=16777619u;v>>=8;}}
#endif
    return h?h:1;
  }
  bool canStore(size_t bytes,size_t segments=0,size_t points=0){
    if(!bytes||bytes>16u*1024u||segments>64||points>64||bytes>SIZE_MAX-sizeof(Record)-alignedPoints(points)-alignedInk(segments)-7)return false;
    if(!ensureStorage())return false;
    return aligned(sizeof(Record)+alignedPoints(points)+alignedInk(segments)+bytes)<=capacity_;
  }
  bool find(const Key &key,Lease &output){
    output=Lease{};if(pointCount(key)>64||!arena_){++stats_.misses;return false;}const uint32_t hash=hashKey(key);
    uint32_t link=buckets_[hash&(BucketCount-1)];size_t visited=0;
    while(link&&visited++<entries_){const auto *record=recordAt(link-1);
      if(record->prefix.hash==hash&&sameRecordKey(*record,key)){output=lease(*record);return true;}
      link=record->next;
    }
    ++stats_.misses;return false;
  }
  bool store(const Key &key,size_t rowWords,const uint32_t *words,size_t bytes,Lease &output,const SegmentInk *ink=nullptr,size_t segments=0){
    output=Lease{};if(!words||!rowWords||int32_t(key.fields[10])<=0||int32_t(key.fields[11])<=0)return false;
    const size_t height=uint32_t(key.fields[11]);
    if(segments>64||(segments&&!ink)||rowWords!=(size_t(key.fields[10])+31)/32||rowWords>SIZE_MAX/sizeof(uint32_t)||height>SIZE_MAX/(rowWords*sizeof(uint32_t))||rowWords*height*sizeof(uint32_t)!=bytes||!canStore(bytes,segments,pointCount(key)))return false;
    const uintptr_t input=reinterpret_cast<uintptr_t>(words),arena=reinterpret_cast<uintptr_t>(arena_);
    if(input>UINTPTR_MAX-bytes||(input<arena+budget_&&arena<input+bytes))return false;
    const size_t points=pointCount(key),pointBytes=alignedPoints(points),inkBytes=alignedInk(segments),required=aligned(sizeof(Record)+pointBytes+inkBytes+bytes);size_t gap=0;
    for(;;){if(!used_)head_=tail_=0;gap=capacity_-tail_<required?capacity_-tail_:0;
      if(required+gap<=capacity_-used_)break;
      if(!evictOne())return false;
    }
    if(gap){new(data_+tail_) Prefix{uint32_t(gap),0};used_+=gap;tail_=0;}
    const uint32_t hash=hashKey(key),offset=uint32_t(tail_);auto *record=new(data_+tail_) Record{};
    record->prefix={uint32_t(required),hash};record->next=buckets_[hash&(BucketCount-1)];record->maskBytes=uint32_t(bytes);record->rowWords=uint32_t(rowWords);record->segments=uint32_t(segments);record->key=key;
#if NICE_GFX_UI_OUTLINED_CACHE_POINTS
    record->pointCount=uint32_t(points);if(points)memcpy(record+1,key.points,points*sizeof(ShapePoint));
#endif
    if(segments)memcpy(reinterpret_cast<uint8_t*>(record+1)+pointBytes,ink,segments*sizeof(SegmentInk));
    memcpy(reinterpret_cast<uint8_t*>(record+1)+pointBytes+inkBytes,words,bytes);buckets_[hash&(BucketCount-1)]=offset+1;
    used_+=required;tail_+=required;if(tail_==capacity_)tail_=0;++entries_;stats_.peakUsedBytes=std::max(stats_.peakUsedBytes,used_);
    output=lease(*record);return true;
  }
  // Raster::capsule's coarse culling omits epsilon-border pixels for some
  // partial clips even though a full-mask span rounds them in. Reuse only if
  // every segment that contributed cached ink inside this ROI also survives
  // the ORIGINAL ROI's exact coarse cull and row test. Otherwise the owner
  // reruns the unchanged clipped raster. No geometric tolerance is invented.
  static bool canExtract(const shape_raster::TransformedShape &shape,const Lease &source,Rect clipped){
    if(!source.words||clipped.width<=0||clipped.height<=0||clipped.x<source.bounds.x||clipped.y<source.bounds.y||int64_t(clipped.x)+clipped.width>int64_t(source.bounds.x)+source.bounds.width||int64_t(clipped.y)+clipped.height>int64_t(source.bounds.y)+source.bounds.height)return false;
    if(clipped.x==source.bounds.x&&clipped.y==source.bounds.y&&clipped.width==source.bounds.width&&clipped.height==source.bounds.height)return true;
    const size_t count=shape.closed?shape.count:shape.count?shape.count-1:0;
    if(!count||!source.ink||source.segments!=count)return false;
    struct RowOnly{} rowOnly;shape_raster::Raster<RowOnly> raster{rowOnly,clipped};
    for(size_t i=0;i<count;++i){const auto &ink=source.ink[i];if(!ink.width||!ink.height)continue;
      const int64_t x=int64_t(source.bounds.x)+ink.x,y=int64_t(source.bounds.y)+ink.y;
      if(x+ink.width<=clipped.x||y+ink.height<=clipped.y||x>=int64_t(clipped.x)+clipped.width||y>=int64_t(clipped.y)+clipped.height)continue;
      const auto &a=shape.points[i],&b=shape.points[i+1<shape.count?i+1:0];
      if(std::max(a.x,b.x)+shape.stroke<clipped.x||std::min(a.x,b.x)-shape.stroke>clipped.x+clipped.width-1)return false;
      int32_t first,last;if(!raster.rows(std::min(a.y,b.y)-shape.stroke,std::max(a.y,b.y)+shape.stroke,first,last))return false;
    }
    return true;
  }
  static bool extract(const Lease &source,Rect clipped,uint32_t *output,size_t rowWords){
    if(!source.words||!output||!rowWords||!source.rowWords||!source.bytes||source.bytes>16u*1024u||source.bounds.width<=0||source.bounds.height<=0||clipped.width<=0||clipped.height<=0||
       clipped.x<source.bounds.x||clipped.y<source.bounds.y||
       int64_t(clipped.x)+clipped.width>int64_t(source.bounds.x)+source.bounds.width||
       int64_t(clipped.y)+clipped.height>int64_t(source.bounds.y)+source.bounds.height||
       rowWords!=(size_t(clipped.width)+31)/32||source.rowWords!=(size_t(source.bounds.width)+31)/32||
       size_t(source.bounds.height)>SIZE_MAX/source.rowWords/sizeof(uint32_t)||source.rowWords*size_t(source.bounds.height)*sizeof(uint32_t)!=source.bytes)return false;
    const size_t outputBytes=rowWords*size_t(clipped.height)*sizeof(uint32_t);
    const uintptr_t input=reinterpret_cast<uintptr_t>(source.words),target=reinterpret_cast<uintptr_t>(output);
    if(input>UINTPTR_MAX-source.bytes||target>UINTPTR_MAX-outputBytes||(input<target+outputBytes&&target<input+source.bytes))return false;
    const size_t x=size_t(int64_t(clipped.x)-source.bounds.x),y=size_t(int64_t(clipped.y)-source.bounds.y),word=x/32;const unsigned shift=unsigned(x&31);
    for(int32_t row=0;row<clipped.height;++row){const auto *input=source.words+(y+size_t(row))*source.rowWords;auto *target=output+size_t(row)*rowWords;
      if(!shift)memcpy(target,input+word,rowWords*sizeof(uint32_t));
      else for(size_t i=0;i<rowWords;++i){const size_t at=word+i;uint32_t bits=input[at]>>shift;if(at+1<source.rowWords)bits|=input[at+1]<<(32-shift);target[i]=bits;}
      if(clipped.width&31)target[rowWords-1]&=(uint32_t(1)<<(clipped.width&31))-1;
    }
    return true;
  }

 private:
  struct Prefix {uint32_t bytes,hash;};
  struct alignas(8) Record {Prefix prefix;uint32_t next=0,maskBytes=0,rowWords=0,segments=0;ScalarKey key;
#if NICE_GFX_UI_OUTLINED_CACHE_POINTS
    uint32_t pointCount=0;
#endif
  };
  static size_t recordPointCount(const Record &record){
#if NICE_GFX_UI_OUTLINED_CACHE_POINTS
    return record.pointCount;
#else
    (void)record;return 0;
#endif
  }
  static bool sameRecordKey(const Record &record,const Key &key){
    if(!(record.key==key)||recordPointCount(record)!=pointCount(key))return false;
#if NICE_GFX_UI_OUTLINED_CACHE_POINTS
    if(key.count&&memcmp(&record+1,key.points,key.count*sizeof(ShapePoint)))return false;
#endif
    return true;
  }
  static size_t alignedPoints(size_t count){return(count*sizeof(ShapePoint)+3)&~size_t(3);}
  static size_t aligned(size_t bytes){return(bytes+7)&~size_t(7);}
  static size_t alignedInk(size_t count){return(count*sizeof(SegmentInk)+3)&~size_t(3);}
  Record *recordAt(size_t offset){return reinterpret_cast<Record*>(data_+offset);}
  const Record *recordAt(size_t offset)const{return reinterpret_cast<const Record*>(data_+offset);}
  static Lease lease(const Record &record){Lease result;result.words=reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(&record+1)+alignedPoints(recordPointCount(record))+alignedInk(record.segments));result.ink=reinterpret_cast<const SegmentInk*>(reinterpret_cast<const uint8_t*>(&record+1)+alignedPoints(recordPointCount(record)));result.segments=record.segments;result.bounds={int32_t(record.key.fields[8]),int32_t(record.key.fields[9]),int32_t(record.key.fields[10]),int32_t(record.key.fields[11])};result.rowWords=record.rowWords;result.bytes=record.maskBytes;return result;}
  bool ensureStorage(){
    if(arena_)return true;if(attempted_)return false;attempted_=true;
    if(budget_<=BucketBytes+sizeof(Record)+8||budget_>MaximumBudget||!allocate_||!free_||!available_){++stats_.allocationFailures;return false;}
    const size_t available=available_();if(available<Reserve||budget_>available-Reserve){++stats_.allocationFailures;return false;}
    void *memory=allocate_(budget_);if(!memory){++stats_.allocationFailures;return false;}
    if(reinterpret_cast<uintptr_t>(memory)&7){free_(memory);++stats_.allocationFailures;return false;}
    arena_=static_cast<uint8_t*>(memory);buckets_=reinterpret_cast<uint32_t*>(arena_);memset(buckets_,0,BucketBytes);data_=arena_+BucketBytes;capacity_=(budget_-BucketBytes)&~size_t(7);return true;
  }
  bool evictOne(){
    if(!used_)return false;const auto *prefix=reinterpret_cast<const Prefix*>(data_+head_);const size_t bytes=prefix->bytes;
    if(bytes<sizeof(Prefix)||bytes>capacity_-head_||bytes>used_)return false;
    if(prefix->hash){const auto *old=recordAt(head_);uint32_t *link=&buckets_[prefix->hash&(BucketCount-1)];size_t visited=0;
      while(*link&&visited++<entries_){if(*link==head_+1){*link=old->next;break;}link=&recordAt(*link-1)->next;}
      if(!entries_)return false;--entries_;++stats_.evictions;
    }
    head_+=bytes;if(head_==capacity_)head_=0;used_-=bytes;if(!used_)head_=tail_=0;return true;
  }
  static size_t availableMemory(){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    return heap_caps_get_free_size(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
    return SIZE_MAX;
#endif
  }
  static void *allocateMemory(size_t bytes){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    return heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
    return malloc(bytes);
#endif
  }
  static void freeMemory(void *memory){
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    heap_caps_free(memory);
#else
    free(memory);
#endif
  }
  size_t budget_=0,capacity_=0,head_=0,tail_=0,used_=0,entries_=0;
  Allocate allocate_=nullptr;Free free_=nullptr;Available available_=nullptr;
  uint8_t *arena_=nullptr,*data_=nullptr;uint32_t *buckets_=nullptr;bool attempted_=false;
  OutlinedGeometryCacheStats stats_;
};

}}}
