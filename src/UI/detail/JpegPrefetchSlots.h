#pragma once
#include <stddef.h>
#include <stdint.h>

namespace nice_gfx { namespace htmlui { namespace detail {
// Allocation-free state machine. Its owner serializes calls; a working slot
// and every outstanding read lease are excluded from replacement.
template<class Key,size_t Count=4> class JpegPrefetchSlots {
 public:
  static_assert(Count>0&&Count<=255,"bounded JPEG prefetch slots");
  struct Token {uint8_t index=255;uint32_t generation=0;};
  struct Work {Token token;Key key{};};
  struct Entry {Key key{};uint64_t used=0;uint32_t generation=0;uint16_t readers=0;bool ready=false,working=false,failed=false;};
  void request(const Key *keys,size_t count){
    requested_=0;
    for(size_t i=0;keys&&i<count&&requested_<Count;++i){bool duplicate=false;
      for(size_t j=0;j<requested_;++j)if(requests_[j]==keys[i]){duplicate=true;break;}
      if(!duplicate)requests_[requested_++]=keys[i];
    }
  }
  void cancel(){requested_=0;}
  bool begin(Work &work){
    for(size_t request=0;request<requested_;++request){
      const Key &key=requests_[request];bool present=false;
      for(const auto &slot:slots_)if(slot.key==key&&(slot.ready||slot.working||slot.failed)){present=true;break;}
      if(present)continue;
      size_t victim=Count;
      for(size_t i=0;i<Count;++i){const auto &slot=slots_[i];if(slot.readers||slot.working)continue;
        bool wanted=false;for(size_t j=0;(slot.ready||slot.failed)&&j<requested_;++j)if(slot.key==requests_[j]){wanted=true;break;}
        if(wanted)continue;
        if(victim==Count||(!slot.ready&&slots_[victim].ready)||
           (slot.ready==slots_[victim].ready&&slot.used<slots_[victim].used))victim=i;
      }
      if(victim==Count)return false;
      auto &slot=slots_[victim];slot.key=key;slot.ready=slot.failed=false;slot.working=true;slot.used=++clock_;
      if(!++slot.generation)++slot.generation;
      work={Token{uint8_t(victim),slot.generation},key};return true;
    }
    return false;
  }
  bool complete(const Work &work,bool success){
    if(work.token.index>=Count)return false;auto &slot=slots_[work.token.index];
    if(!slot.working||slot.generation!=work.token.generation||!(slot.key==work.key))return false;
    slot.working=false;slot.ready=success;slot.failed=!success;slot.used=++clock_;return true;
  }
  bool acquire(const Key &key,Token &token){
    for(size_t i=0;i<Count;++i){auto &slot=slots_[i];
      if(slot.ready&&slot.key==key&&slot.readers!=UINT16_MAX){++slot.readers;slot.used=++clock_;token={uint8_t(i),slot.generation};return true;}
    }
    token=Token{};return false;
  }
  bool release(Token token){
    if(token.index>=Count)return false;auto &slot=slots_[token.index];
    if(slot.generation!=token.generation||!slot.readers)return false;--slot.readers;return true;
  }
  const Entry &entry(size_t index)const{return slots_[index];}
  size_t requested()const{return requested_;}
 private:
  Entry slots_[Count]{};Key requests_[Count]{};size_t requested_=0;uint64_t clock_=0;
};
} } }
