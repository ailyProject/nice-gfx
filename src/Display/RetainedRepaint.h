#pragma once
#include "Transport.h"

namespace nice_gfx { namespace detail {
// Serialized single-producer certificates. There are no pixel copies here.
// A receipt is provisional until certify + the real presentation fence succeed.
class RetainedRepaintTags {
 public:
  static constexpr size_t Capacity=4;
  struct Tag {RetainedRepaintRequest request{};uint64_t token=0;};
  struct Pending {Tag tag{};RetainedRepaintReceipt receipt{};bool certified=false;};
  void reset(){abort();for(auto &buffer:tags_)for(auto &tag:buffer)tag=Tag{};
    if(epoch_==UINT64_MAX){exhausted_=true;return;}++epoch_;}
  uint64_t epoch()const{return epoch_;}
  bool open()const{return count_!=0;}
  static bool overlaps(const DirtyRect&a,const DirtyRect&b){return a.width>0&&a.height>0&&b.width>0&&b.height>0&&
    int64_t(a.x)<int64_t(b.x)+b.width&&int64_t(b.x)<int64_t(a.x)+a.width&&
    int64_t(a.y)<int64_t(b.y)+b.height&&int64_t(b.y)<int64_t(a.y)+a.height;}
  static bool equal(const DirtyRect&a,const DirtyRect&b){return a.x==b.x&&a.y==b.y&&a.width==b.width&&a.height==b.height;}
  static bool equal(const RetainedRepaintRequest&a,const RetainedRepaintRequest&b){return a.owner==b.owner&&a.revision==b.revision&&equal(a.rect,b.rect);}
  // Every inheritance/unknown writer calls this BEFORE touching the target.
  // It revokes outstanding receipts too; the caller must abort, never publish.
  void written(unsigned buffer,const DirtyRect&r){if(buffer>1){reset();return;}
    for(auto &tag:tags_[buffer])if(overlaps(tag.request.rect,r))tag=Tag{};
    if(open()&&buffer==buffer_)for(size_t i=0;i<count_;++i)if(overlaps(pending_[i].tag.request.rect,r))valid_=false;}
  void ordinaryDamage(unsigned buffer,const DirtyRect *damage,size_t count){
    if(buffer>1)return;
    if(!damage||!count){for(auto &tag:tags_[buffer])tag=Tag{};if(open()&&buffer==buffer_)valid_=false;return;}
    for(size_t i=0;i<count;++i)written(buffer,damage[i]);}
  Status begin(unsigned buffer,uintptr_t physical,const RetainedRepaintRequest *requests,size_t count,
      const DirtyRect *damage,size_t damageCount,RetainedRepaintReceipt *receipts){
    if(exhausted_||!epoch_||open()||buffer>1||!physical||!requests||!receipts||!count||count>Capacity||!damage||!damageCount)return Status::Unsupported;
    if(sequence_>UINT64_MAX-count){exhausted_=true;return Status::Unsupported;}
    for(size_t i=0;i<count;++i){const auto&r=requests[i];bool covered=false;
      if(!r.owner||!r.revision||r.rect.x<0||r.rect.y<0||r.rect.width<=0||r.rect.height<=0)return Status::InvalidArgument;
      for(size_t j=0;j<damageCount;++j){const auto&d=damage[j];covered|=r.rect.x>=d.x&&r.rect.y>=d.y&&int64_t(r.rect.x)+r.rect.width<=int64_t(d.x)+d.width&&int64_t(r.rect.y)+r.rect.height<=int64_t(d.y)+d.height;}
      if(!covered)return Status::InvalidArgument;
      for(size_t j=0;j<i;++j)if(r.owner==requests[j].owner||overlaps(r.rect,requests[j].rect))return Status::InvalidArgument;
    }
    buffer_=buffer;valid_=true;
    for(size_t i=0;i<count;++i){auto &p=pending_[i];p=Pending{};p.tag.request=requests[i];
      uint64_t previous=0;for(const auto&t:tags_[buffer])if(t.token&&equal(t.request,requests[i]))previous=t.token;
      p.tag.token=++sequence_;p.receipt={physical,epoch_,previous,p.tag.token,previous!=0};receipts[i]=p.receipt;}
    // Ordinary damage outside the requested ownership must still revoke tags.
    // Existing requested tags are consumed once, then replaced only at fence.
    for(size_t i=0;i<damageCount;++i)for(auto&t:tags_[buffer])if(overlaps(t.request.rect,damage[i]))t=Tag{};
    count_=count;return Status::Ok;
  }
  Status validate(const RetainedRepaintReceipt *receipts,size_t count)const{
    if(!valid_||!count_||count!=count_||!receipts)return Status::Busy;
    for(size_t i=0;i<count;++i){const auto&a=receipts[i];const auto&b=pending_[i].receipt;
      if(a.physicalBuffer!=b.physicalBuffer||a.allocationEpoch!=b.allocationEpoch||a.previousToken!=b.previousToken||a.nextToken!=b.nextToken||a.retained!=b.retained)return Status::InvalidArgument;}
    return Status::Ok;
  }
  Status certify(const RetainedRepaintReceipt *receipts,size_t count){const auto s=validate(receipts,count);if(s!=Status::Ok)return s;
    for(size_t i=0;i<count_;++i)pending_[i].certified=true;return Status::Ok;}
  bool commitReady()const{if(!valid_||!count_)return false;for(size_t i=0;i<count_;++i)if(!pending_[i].certified)return false;return true;}
  void fencedCommit(bool success){if(success&&commitReady())for(size_t i=0;i<count_;++i)tags_[buffer_][i]=pending_[i].tag;else if(count_)for(auto&t:tags_[buffer_])t=Tag{};
    count_=0;valid_=false;}
  void abort(){fencedCommit(false);}
 private:
  Tag tags_[2][Capacity]{};Pending pending_[Capacity]{};
  size_t count_=0;unsigned buffer_=0;uint64_t epoch_=1,sequence_=0;bool valid_=false,exhausted_=false;
};
}}
