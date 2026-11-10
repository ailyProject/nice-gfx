#pragma once
#include <atomic>
#include <stdint.h>

namespace nice_gfx {
namespace detail {
// Initialization task context only. Tokens are unique for this live process,
// independent of reused transport objects and SDK allocation addresses.
class NativePresentationOwnerFactory {
 public:
  uint64_t take()noexcept {
    uint64_t current=next_.load(std::memory_order_relaxed);
    while(current&&current<UINT64_MAX){
      if(next_.compare_exchange_weak(current,current+1,std::memory_order_relaxed))return current;
    }
    return 0;
  }
 private:
  std::atomic<uint64_t> next_{1};
};
inline uint64_t nextNativePresentationOwner()noexcept {
  static NativePresentationOwnerFactory factory;
  return factory.take();
}
}
}
