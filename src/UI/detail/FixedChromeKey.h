#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <new>

namespace nice_gfx { namespace htmlui { namespace detail {
// Reused optional-cache identity storage. Failure must skip the optimization,
// including builds with exceptions disabled; never use a growing STL vector in
// the paint path. The exact byte limit also bounds descriptor/text key work.
class FixedChromeKey {
 public:
  static constexpr size_t Limit=65536;
  using Allocate=uint8_t *(*)(size_t);using Free=void (*)(uint8_t *);
  explicit FixedChromeKey(Allocate allocate=allocateMemory,Free release=freeMemory):allocate_(allocate),free_(release){}
  ~FixedChromeKey(){if(data_)free_(data_);}
  FixedChromeKey(const FixedChromeKey &)=delete;FixedChromeKey &operator=(const FixedChromeKey &)=delete;
  void clear(){size_=0;}
  const uint8_t *data()const{return data_;}
  size_t size()const{return size_;}
  size_t capacity()const{return capacity_;}
  bool append(const void *bytes,size_t count){
    if(!bytes||count>Limit-size_)return false;
    const uintptr_t input=reinterpret_cast<uintptr_t>(bytes),owned=reinterpret_cast<uintptr_t>(data_);
    if(count>UINTPTR_MAX-input||(data_&&count&&input<owned+capacity_&&owned<input+count))return false;
    const size_t needed=size_+count;
    if(needed>capacity_){
      if(!allocate_||!free_)return false;
      size_t next=capacity_?capacity_:1024;while(next<needed)next*=2;
      uint8_t *memory=allocate_(next);if(!memory)return false;
      if(size_)memcpy(memory,data_,size_);
      if(data_)free_(data_);data_=memory;capacity_=next;
    }
    if(count)memcpy(data_+size_,bytes,count);size_=needed;return true;
  }
 private:
  static uint8_t *allocateMemory(size_t bytes){return new(std::nothrow) uint8_t[bytes];}
  static void freeMemory(uint8_t *memory){delete[] memory;}
  uint8_t *data_=nullptr;size_t size_=0,capacity_=0;Allocate allocate_;Free free_;
};
}}} // namespace nice_gfx::htmlui::detail
