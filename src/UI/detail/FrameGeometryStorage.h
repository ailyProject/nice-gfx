#pragma once
#include "RetainedStorage.h"

#ifndef NICE_GFX_UI_FRAME_GEOMETRY_INTERNAL_NODES
#define NICE_GFX_UI_FRAME_GEOMETRY_INTERNAL_NODES 0
#endif
#ifndef NICE_GFX_UI_FRAME_GEOMETRY_EARLY_RESERVE
#define NICE_GFX_UI_FRAME_GEOMETRY_EARLY_RESERVE 0
#endif
#if (NICE_GFX_UI_FRAME_GEOMETRY_INTERNAL_NODES || NICE_GFX_UI_FRAME_GEOMETRY_EARLY_RESERVE) && defined(CONFIG_IDF_TARGET_ESP32P4)
#include <esp_memory_utils.h>
#endif

namespace nice_gfx { namespace htmlui { namespace detail {

constexpr size_t FrameGeometryInternalReserve=192u*1024u;
#if NICE_GFX_UI_FRAME_GEOMETRY_INTERNAL_NODES && defined(CONFIG_IDF_TARGET_ESP32P4)
constexpr bool FrameGeometryInternalAllocatorAvailable=true;
// C61 allocation policy. The reserve constrains the preferred attempt. Its
// original malloc fallback and compatible free family remain in force.
template<class T> struct FrameGeometryAllocator {
  using value_type=T;
  using is_always_equal=std::true_type;
  FrameGeometryAllocator() noexcept=default;
  template<class U> FrameGeometryAllocator(const FrameGeometryAllocator<U>&) noexcept {}
  T *allocate(size_t count) {
    void *memory=nullptr;
    if(count<=SIZE_MAX/sizeof(T)) {
      const size_t bytes=count*sizeof(T);
      memory=platform::allocateUiMetadata(bytes,FrameGeometryInternalReserve);
      if(memory&&heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)<FrameGeometryInternalReserve) {
        platform::freeUiMemory(memory);memory=nullptr;
      }
      if(!memory)memory=::malloc(bytes);
    }
    if(!memory) {
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
      throw std::bad_alloc();
#else
      std::abort();
#endif
    }
    return static_cast<T*>(memory);
  }
  void deallocate(T *memory,size_t) noexcept {platform::freeUiMemory(memory);}
  template<class U> bool operator==(const FrameGeometryAllocator<U>&) const noexcept{return true;}
  template<class U> bool operator!=(const FrameGeometryAllocator<U>&) const noexcept{return false;}
};
template<class T> using FrameGeometryVector=std::vector<T,FrameGeometryAllocator<T>>;
#else
constexpr bool FrameGeometryInternalAllocatorAvailable=false;
template<class T> using FrameGeometryVector=RetainedVector<T>;
#endif

#if NICE_GFX_UI_FRAME_GEOMETRY_EARLY_RESERVE && NICE_GFX_UI_FRAME_GEOMETRY_INTERNAL_NODES && defined(CONFIG_IDF_TARGET_ESP32P4) && (defined(__cpp_exceptions) || defined(_CPPUNWIND))
constexpr bool FrameGeometryEarlyReserveAvailable=true;
#else
constexpr bool FrameGeometryEarlyReserveAvailable=false;
#endif

template<class Nodes> bool reserveFrameGeometryNodes(Nodes &nodes,size_t count) noexcept {
#if NICE_GFX_UI_FRAME_GEOMETRY_EARLY_RESERVE && NICE_GFX_UI_FRAME_GEOMETRY_INTERNAL_NODES && defined(CONFIG_IDF_TARGET_ESP32P4) && (defined(__cpp_exceptions) || defined(_CPPUNWIND))
  if(!count||count>1024||count>(64u*1024u)/sizeof(typename Nodes::value_type))return false;
  try {nodes.reserve(count);return true;}
  catch(const std::bad_alloc&){return false;}
#else
  (void)nodes;(void)count;return false;
#endif
}

struct FrameGeometryMemoryDomain {bool spanValid=false,internal=false,external=false;};
inline FrameGeometryMemoryDomain frameGeometryMemoryDomain(const void *memory,size_t bytes) noexcept {
  FrameGeometryMemoryDomain result;
  const uintptr_t address=reinterpret_cast<uintptr_t>(memory);
  if(!memory||!bytes||address>UINTPTR_MAX-(bytes-1))return result;
  result.spanValid=true;
#if (NICE_GFX_UI_FRAME_GEOMETRY_INTERNAL_NODES || NICE_GFX_UI_FRAME_GEOMETRY_EARLY_RESERVE) && defined(CONFIG_IDF_TARGET_ESP32P4)
  const void *last=reinterpret_cast<const void*>(address+bytes-1);
  result.internal=esp_ptr_internal(memory)&&esp_ptr_internal(last);
  result.external=esp_ptr_external_ram(memory)&&esp_ptr_external_ram(last);
#endif
  return result;
}

} } }
