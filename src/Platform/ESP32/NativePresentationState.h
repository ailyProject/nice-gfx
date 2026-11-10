#pragma once
#include "../../Display/Transport.h"
#include "RgbAtomicPipeline.h"
#if NICE_GFX_P4_NATIVE_PRESENTATION_HISTORY_COALESCE
#include "../../Display/DirtyRegions.h"
#endif
#include "NativePresentationOwner.h"
#include <stdint.h>

namespace nice_gfx { namespace detail {

// Same UI owner only. The ISR publishes a sequence, never this mutable state.
// The transport proves these are the complete SDK allocations before bind().
using NativeDrawingLease=nice_gfx::NativeDrawingLease;
using NativePresentReceipt=nice_gfx::NativePresentationReceipt;

class NativePresentationState {
 public:
  static constexpr uint8_t None=255,Count=3;
  static constexpr size_t DamageCapacity=16;
#if NICE_GFX_P4_NATIVE_PRESENTATION_HISTORY_COALESCE
  struct HistoryStats {
    uint64_t plans=0,initialFullPlans=0,unknownDamageFullPlans=0,abortedFullPlans=0;
    uint64_t outputCapacityFallbacks=0,historyAtCapacityAdds=0,historyCoalescedAdds=0;
    uint64_t copiedRectangles=0,copiedPixels=0;
    size_t maximumHistoryRectangles=0;
  };
  const HistoryStats &historyStats()const{return historyStats_;}
#endif

  Status bind(uint64_t owner,uint16_t *const (&frames)[Count],size_t width,size_t height,size_t stride){
    if(faulted_||owner_||!owner||!width||!height||width>stride||stride>0x3fff||height>0x3fff||
       height>SIZE_MAX/stride/2||(stride*2)%64)return fault();
    const size_t bytes=stride*height*2;
    for(uint8_t i=0;i<Count;++i){
      const uintptr_t address=reinterpret_cast<uintptr_t>(frames[i]);
      if(!address||(address&63)||address>UINTPTR_MAX-bytes)return fault();
      for(uint8_t j=0;j<i;++j){
        const uintptr_t other=reinterpret_cast<uintptr_t>(frames[j]);
        if(address<other+bytes&&other<address+bytes)return fault();
      }
    }
    owner_=owner;width_=width;height_=height;stride_=stride;bytes_=bytes;
    for(uint8_t i=0;i<Count;++i)buffers_[i].pixels=frames[i];
    buffers_[0].staleFull=false;
    return Status::Ok;
  }

  Status acquire(const DirtyRect *damage,size_t count,bool discard,NativeDrawingLease &lease){
    lease=NativeDrawingLease{};
    if(faulted_||!owner_)return Status::IoError;
    if(drawing_!=None||ready_!=None)return Status::Busy;
    if(nextFrame_==UINT64_MAX)return fault();
    uint8_t target=None;
    for(uint8_t i=0;i<Count;++i)if(i!=front_&&i!=pending_){target=i;break;}
    if(target==None)return Status::Busy;
    if(buffers_[target].epoch==UINT64_MAX)return fault();
    if(count&& !damage)return Status::InvalidArgument;
    const bool unknown=count>DamageCapacity||!count;
    if(unknown){
      damage_[0]=DirtyRect(0,0,int32_t(width_),int32_t(height_));count=1;
      buffers_[target].staleFull=true;
#if NICE_GFX_P4_NATIVE_PRESENTATION_HISTORY_COALESCE
      buffers_[target].stale.clear();buffers_[target].fullReason=FullReason::UnknownDamage;
#else
      buffers_[target].staleCount=0;
#endif
    }else{
      for(size_t i=0;i<count;++i){const DirtyRect &r=damage[i];
        if(r.x<0||r.y<0||r.width<=0||r.height<=0||size_t(r.x)>width_||
           size_t(r.width)>width_-size_t(r.x)||size_t(r.y)>height_||
           size_t(r.height)>height_-size_t(r.y))return Status::InvalidArgument;
      }
      for(size_t i=0;i<count;++i)damage_[i]=damage[i];
    }
    damageCount_=count;discard_=discard&&!unknown;drawing_=target;
    lease.owner=owner_;lease.frame=nextFrame_++;lease.epoch=++buffers_[target].epoch;
    lease.target=target;lease.source=latest_;lease.sourceVersion=buffers_[latest_].version;
    drawingLease_=lease;
    return Status::Ok;
  }

  template<size_t Capacity> Status inheritance(const NativeDrawingLease &lease,
      DirtyRect (&needed)[Capacity],DirtyRect (&scratch)[Capacity],size_t &count){
    count=0;
    if(faulted_||!matches(lease,drawingLease_)||drawing_==None)return fault();
    const DirtyRect full(0,0,int32_t(width_),int32_t(height_));
    const Buffer &target=buffers_[drawing_];
#if NICE_GFX_P4_NATIVE_PRESENTATION_HISTORY_COALESCE
    ++historyStats_.plans;
    bool capacityFallback=false;
    if(target.staleFull){
      switch(target.fullReason){
        case FullReason::Initial:++historyStats_.initialFullPlans;break;
        case FullReason::UnknownDamage:++historyStats_.unknownDamageFullPlans;break;
        case FullReason::Aborted:++historyStats_.abortedFullPlans;break;
      }
      count=RgbAtomicPipeline::inheritance(full,discard_?damage_:nullptr,
        discard_?damageCount_:0,needed,scratch,&capacityFallback);
    }else{
      DirtyRect parts[Capacity];
      for(size_t i=0;i<target.stale.size();++i){
        const size_t pieces=RgbAtomicPipeline::inheritance(target.stale[i],discard_?damage_:nullptr,
          discard_?damageCount_:0,parts,scratch,&capacityFallback);
        if(capacityFallback||pieces>Capacity-count){
          capacityFallback=true;
          count=RgbAtomicPipeline::inheritance(full,discard_?damage_:nullptr,
            discard_?damageCount_:0,needed,scratch);
          break;
        }
        for(size_t part=0;part<pieces;++part)needed[count++]=parts[part];
      }
    }
    if(capacityFallback)++historyStats_.outputCapacityFallbacks;
    historyStats_.copiedRectangles+=count;
    for(size_t i=0;i<count;++i)historyStats_.copiedPixels+=needed[i].pixels();
    return Status::Ok;
#else
    if(target.staleFull){
      count=RgbAtomicPipeline::inheritance(full,discard_?damage_:nullptr,
        discard_?damageCount_:0,needed,scratch);
      return Status::Ok;
    }
    DirtyRect parts[Capacity];
    for(size_t i=0;i<target.staleCount;++i){
      const size_t pieces=RgbAtomicPipeline::inheritance(target.stale[i],discard_?damage_:nullptr,
        discard_?damageCount_:0,parts,scratch);
      if(pieces>Capacity-count){
        count=RgbAtomicPipeline::inheritance(full,discard_?damage_:nullptr,
          discard_?damageCount_:0,needed,scratch);
        return Status::Ok;
      }
      for(size_t part=0;part<pieces;++part)needed[count++]=parts[part];
    }
    return Status::Ok;
#endif
  }

  Status ready(const NativeDrawingLease &lease){
    if(faulted_||!matches(lease,drawingLease_)||drawing_!=lease.target||ready_!=None)return fault();
    buffers_[drawing_].version=lease.frame;
    buffers_[drawing_].staleFull=false;
#if NICE_GFX_P4_NATIVE_PRESENTATION_HISTORY_COALESCE
    buffers_[drawing_].stale.clear();
#else
    buffers_[drawing_].staleCount=0;
#endif
    ready_=drawing_;drawing_=None;readyLease_=lease;drawingLease_=NativeDrawingLease{};
    return Status::Ok;
  }

  // Call before entering the SDK. Even a non-OK SDK result may retain leases.
  Status accept(const NativeDrawingLease &lease,NativePresentReceipt &receipt){
    receipt=NativePresentReceipt{};
    if(faulted_||pending_!=None||ready_!=lease.target||!matches(lease,readyLease_))return fault();
    if(retirementPending_)return Status::Busy;
    pending_=ready_;latest_=pending_;ready_=None;readyLease_=NativeDrawingLease{};
    for(uint8_t buffer=0;buffer<Count;++buffer)if(buffer!=pending_){
      Buffer &stale=buffers_[buffer];
      if(stale.staleFull)continue;
#if NICE_GFX_P4_NATIVE_PRESENTATION_HISTORY_COALESCE
      for(size_t i=0;i<damageCount_;++i){
        const size_t before=stale.stale.size();
        if(before==DamageCapacity)++historyStats_.historyAtCapacityAdds;
        stale.stale.add(damage_[i],uint16_t(width_),uint16_t(height_));
        if(stale.stale.size()<before+1)++historyStats_.historyCoalescedAdds;
        if(stale.stale.size()>historyStats_.maximumHistoryRectangles)
          historyStats_.maximumHistoryRectangles=stale.stale.size();
      }
#else
      if(damageCount_>DamageCapacity-stale.staleCount){stale.staleFull=true;stale.staleCount=0;continue;}
      for(size_t i=0;i<damageCount_;++i)stale.stale[stale.staleCount++]=damage_[i];
#endif
    }
    receipt.owner=owner_;receipt.frame=lease.frame;receipt.epoch=lease.epoch;receipt.target=pending_;
    pendingReceipt_=receipt;
    return Status::Ok;
  }

  // The original driver supplies its post-draw baseline and pinned-core 1/2 rule.
  Status arm(NativePresentReceipt &receipt,uint32_t baseline,uint8_t required){
    if(faulted_||!sameSubmission(receipt,pendingReceipt_)||pending_!=receipt.target||
       pendingReceipt_.required||(required!=1&&required!=2))return fault();
    receipt.baseline=baseline;receipt.required=required;pendingReceipt_=receipt;
    return Status::Ok;
  }

  // A sticky fault still drains the original armed display receipt. Retired
  // then permits lease cleanup, while IoError prevents View publication/retry.
  // An unarmed uncertain SDK failure needs a separate driver-quiescence proof.
  Status observe(const NativePresentReceipt &receipt,uint32_t sequence,bool &retired){
    retired=false;
    if(!sameSubmission(receipt,pendingReceipt_)||pending_!=receipt.target||
       !receipt.required||receipt.required!=pendingReceipt_.required||
       receipt.baseline!=pendingReceipt_.baseline)return fault();
    const uint32_t advance=sequence-receipt.baseline;
    if(advance<receipt.required||advance>=0x80000000u)return faulted_?Status::IoError:Status::Busy;
    front_=pending_;pending_=None;lastRetiredFrame_=receipt.frame;
    retiredReceipt_=receipt;retirementPending_=true;
    retiredStatus_=faulted_?Status::IoError:Status::Ok;
    pendingReceipt_=NativePresentReceipt{};retired=true;
    return faulted_?Status::IoError:Status::Ok;
  }

  // Public finish may prove a hardware fence before its View consumes it.
  // Keep that exact receipt until one consumer claim. New SDK submission waits.
  Status claim(const NativePresentReceipt &receipt,bool &retired){
    retired=false;
    if(!retirementPending_||!sameSubmission(receipt,retiredReceipt_)||
       receipt.baseline!=retiredReceipt_.baseline||receipt.required!=retiredReceipt_.required)return fault();
    retired=true;retirementPending_=false;retiredReceipt_=NativePresentReceipt{};
    return faulted_?Status::IoError:retiredStatus_;
  }

  Status abort(const NativeDrawingLease &lease){
    if(faulted_)return Status::IoError;
    if(drawing_==lease.target&&matches(lease,drawingLease_)){
      buffers_[drawing_].staleFull=true;
#if NICE_GFX_P4_NATIVE_PRESENTATION_HISTORY_COALESCE
      buffers_[drawing_].stale.clear();buffers_[drawing_].fullReason=FullReason::Aborted;
#else
      buffers_[drawing_].staleCount=0;
#endif
      drawing_=None;drawingLease_=NativeDrawingLease{};return Status::Ok;
    }
    if(ready_==lease.target&&matches(lease,readyLease_)){
      buffers_[ready_].staleFull=true;
#if NICE_GFX_P4_NATIVE_PRESENTATION_HISTORY_COALESCE
      buffers_[ready_].stale.clear();buffers_[ready_].fullReason=FullReason::Aborted;
#else
      buffers_[ready_].staleCount=0;
#endif
      ready_=None;readyLease_=NativeDrawingLease{};return Status::Ok;
    }
    return fault();
  }

  Status fault(){faulted_=true;return Status::IoError;}
  bool faulted()const{return faulted_;}
  bool busy()const{return pending_!=None||drawing_!=None||ready_!=None||retirementPending_;}
  bool retirementPending()const{return retirementPending_;}
  bool canPrepare()const{return owner_&&!faulted_&&drawing_==None&&ready_==None;}
  uint8_t front()const{return front_;}uint8_t pending()const{return pending_;}
  uint8_t drawing()const{return drawing_;}uint8_t prepared()const{return ready_;}
  uint8_t latest()const{return latest_;}
  uint64_t lastRetiredFrame()const{return lastRetiredFrame_;}
  uint16_t *pixels(uint8_t index)const{return index<Count?buffers_[index].pixels:nullptr;}
  size_t bytes()const{return bytes_;}size_t width()const{return width_;}
  size_t height()const{return height_;}size_t stride()const{return stride_;}
  size_t damageCount()const{return damageCount_;}
  const DirtyRect &damage(size_t index)const{return damage_[index];}

 private:
#if NICE_GFX_P4_NATIVE_PRESENTATION_HISTORY_COALESCE
  enum class FullReason:uint8_t {Initial,UnknownDamage,Aborted};
#endif
  struct Buffer {
    uint16_t *pixels=nullptr;
    uint64_t epoch=0,version=0;
#if NICE_GFX_P4_NATIVE_PRESENTATION_HISTORY_COALESCE
    DirtyRegions<DamageCapacity> stale{};
    FullReason fullReason=FullReason::Initial;
#else
    DirtyRect stale[DamageCapacity]{};
    size_t staleCount=0;
#endif
    bool staleFull=true;
  };
  static bool matches(const NativeDrawingLease&a,const NativeDrawingLease&b){
    return a.owner&&a.owner==b.owner&&a.frame==b.frame&&a.epoch==b.epoch&&
      a.target==b.target&&a.source==b.source&&a.sourceVersion==b.sourceVersion;
  }
  static bool sameSubmission(const NativePresentReceipt&a,const NativePresentReceipt&b){
    return a.owner&&a.owner==b.owner&&a.frame==b.frame&&a.epoch==b.epoch&&a.target==b.target;
  }
  Buffer buffers_[Count]{};
  uint64_t owner_=0,nextFrame_=1,lastRetiredFrame_=0;
  size_t width_=0,height_=0,stride_=0,bytes_=0,damageCount_=0;
  uint8_t front_=0,pending_=None,drawing_=None,ready_=None,latest_=0;
  bool discard_=false,faulted_=false;
  bool retirementPending_=false;
  Status retiredStatus_=Status::Ok;
  DirtyRect damage_[DamageCapacity]{};
  NativeDrawingLease drawingLease_{},readyLease_{};
  NativePresentReceipt pendingReceipt_{};
  NativePresentReceipt retiredReceipt_{};
#if NICE_GFX_P4_NATIVE_PRESENTATION_HISTORY_COALESCE
  HistoryStats historyStats_{};
#endif
};

}} // namespace nice_gfx::detail
