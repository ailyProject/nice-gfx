#pragma once
#include <vector>
#include <stdint.h>
#include <cstdlib>
#include <new>
#include <type_traits>
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#include <esp_heap_caps.h>
#endif
namespace nice_gfx { namespace htmlui {
// Large retained state/geometry belongs beside the page cache in S3 PSRAM.
// Keep tiles, input and small control allocations in internal RAM. This does
// not change the application's global malloc policy or other target backends.
template<class T> struct RetainedAllocator {
  using value_type=T;
  using is_always_equal=std::true_type;
  RetainedAllocator() noexcept=default;
  template<class U> RetainedAllocator(const RetainedAllocator<U>&) noexcept {}
  T *allocate(size_t count) {
    void *memory=nullptr;
    if(count<=SIZE_MAX/sizeof(T)) {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
      if(count*sizeof(T)>=1024)memory=heap_caps_malloc(count*sizeof(T),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#endif
      if(!memory)memory=std::malloc(count*sizeof(T));
    }
    if(!memory) {
#if defined(__cpp_exceptions)
      throw std::bad_alloc();
#else
      std::abort();
#endif
    }
    return static_cast<T *>(memory);
  }
  void deallocate(T *memory,size_t) noexcept {std::free(memory);}
  template<class U> bool operator==(const RetainedAllocator<U>&) const noexcept{return true;}
  template<class U> bool operator!=(const RetainedAllocator<U>&) const noexcept{return false;}
};
#if defined(CONFIG_IDF_TARGET_ESP32S3)
template<class T> using RetainedVector=std::vector<T,RetainedAllocator<T>>;
#else
template<class T> using RetainedVector=std::vector<T>;
#endif
}}
