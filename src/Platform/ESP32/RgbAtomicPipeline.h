#pragma once
#include "../../Display/Transport.h"
#include <stdint.h>

namespace nice_gfx { namespace detail {
// UI-owner state only. The ISR reports a completed submission separately;
// acknowledge() is called by the producer after consuming that exact fence.
class RgbAtomicPipeline {
 public:
  static constexpr uint8_t None=255,BufferCount=3;
  uint8_t front()const{return front_;}uint8_t pending()const{return pending_;}
  uint8_t drawing()const{return drawing_;}uint8_t latest()const{return latest_;}
  uint32_t generation()const{return generation_;}bool faulted()const{return faulted_;}
  bool acquire(){
    if(faulted_)return false;if(drawing_!=None)return true;
    for(uint8_t i=0;i<BufferCount;++i)if(i!=front_&&i!=pending_){drawing_=i;return true;}
    return false;
  }
  bool acknowledge(){if(faulted_||pending_==None)return false;front_=pending_;pending_=None;return true;}
  bool canSubmit()const{return !faulted_&&drawing_!=None&&pending_==None;}
  bool submit(){
    if(!canSubmit())return false;pending_=latest_=drawing_;drawing_=None;
    if(!++generation_)++generation_;return true;
  }
  // An abort never changes the front, pending frame, latest base or generation.
  // Every later acquisition rebases all pixels outside its declared damage.
  void abort(){drawing_=None;}
  void poison(){faulted_=true;}
  // Conservative inheritance for three buffers: use the newest accepted frame
  // (possibly pending), never the older displayed front. Copy all unchanged
  // pixels; fullscreen damage needs no clone. Bounded fragmentation falls back
  // to a complete clone, which is always safe before any caller paint occurs.
  template<size_t Capacity> static size_t inheritance(const DirtyRect &full,
      const DirtyRect *damage,size_t count,DirtyRect (&needed)[Capacity],DirtyRect (&next)[Capacity]
#if NICE_GFX_P4_NATIVE_PRESENTATION_HISTORY_COALESCE
      ,bool *capacityFallback=nullptr
#endif
      ){
    static_assert(Capacity>0,"inheritance scratch required");
#if NICE_GFX_P4_NATIVE_PRESENTATION_HISTORY_COALESCE
    if(capacityFallback)*capacityFallback=false;
#endif
    needed[0]=full;size_t used=1;
    if(!count||!damage)return used; // Unknown damage conservatively clones all.
    for(size_t cut=0;cut<count;++cut){size_t n=0;bool overflow=false;const auto &b=damage[cut];
      for(size_t i=0;i<used&&!overflow;++i){const auto &a=needed[i];
        const int32_t left=a.x>b.x?a.x:b.x,top=a.y>b.y?a.y:b.y;
        const int32_t right=a.x+a.width<b.x+b.width?a.x+a.width:b.x+b.width;
        const int32_t bottom=a.y+a.height<b.y+b.height?a.y+a.height:b.y+b.height;
        const auto add=[&](int32_t x,int32_t y,int32_t w,int32_t h){if(w<=0||h<=0)return;
          if(n==Capacity){overflow=true;return;}next[n++]=DirtyRect(x,y,w,h);};
        if(right<=left||bottom<=top)add(a.x,a.y,a.width,a.height);
        else {add(a.x,a.y,a.width,top-a.y);add(a.x,bottom,a.width,a.y+a.height-bottom);
          add(a.x,top,left-a.x,bottom-top);add(right,top,a.x+a.width-right,bottom-top);}
      }
      if(overflow){
#if NICE_GFX_P4_NATIVE_PRESENTATION_HISTORY_COALESCE
        if(capacityFallback)*capacityFallback=true;
#endif
        needed[0]=full;return 1;
      }
      used=n;for(size_t i=0;i<n;++i)needed[i]=next[i];
    }
    return used;
  }
  template<size_t Capacity> static size_t inheritance(const DirtyRect &full,
      const DirtyRect *damage,size_t count,DirtyRect (&needed)[Capacity]){
    DirtyRect next[Capacity];return inheritance(full,damage,count,needed,next);
  }
 private:
  uint8_t front_=0,pending_=None,drawing_=None,latest_=0;
  uint32_t generation_=0;bool faulted_=false;
};
}}
