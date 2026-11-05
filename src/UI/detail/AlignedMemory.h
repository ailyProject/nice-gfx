#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <new>

namespace nice_gfx { namespace htmlui { namespace detail {
// Arduino cores such as SAMD still use C++14. Keep the same alignment contract
// without requiring the C++17 aligned-new overloads in those toolchains.
template<size_t Alignment>
inline void *allocateAlignedMemory(size_t bytes){
  static_assert(Alignment>=alignof(void *)&&!(Alignment&(Alignment-1)),
                "Alignment must be a power of two and align a pointer");
#if defined(__cpp_aligned_new) && __cpp_aligned_new>=201606
  return ::operator new(bytes,std::align_val_t(Alignment),std::nothrow);
#else
  constexpr size_t overhead=sizeof(void *)+Alignment-1;
  if(bytes>SIZE_MAX-overhead)return nullptr;
  void *raw=::malloc(bytes+overhead);if(!raw)return nullptr;
  const uintptr_t aligned=(reinterpret_cast<uintptr_t>(raw)+overhead)&~uintptr_t(Alignment-1);
  reinterpret_cast<void **>(aligned)[-1]=raw;
  return reinterpret_cast<void *>(aligned);
#endif
}
template<size_t Alignment>
inline void freeAlignedMemory(void *memory){
#if defined(__cpp_aligned_new) && __cpp_aligned_new>=201606
  ::operator delete(memory,std::align_val_t(Alignment));
#else
  if(memory)::free(reinterpret_cast<void **>(memory)[-1]);
#endif
}
}}}
