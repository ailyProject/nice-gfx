#pragma once
#include "../Core/Status.h"
#include <stddef.h>
#include <stdint.h>
namespace nice_gfx { namespace htmlui {
// A decoder writes native RGB565. It may report padded rows in strideBytes,
// but must stay within capacityBytes and leave width/height unchanged.
struct JpegDecodeTarget {
  uint16_t *pixels=nullptr;
  size_t capacityBytes=0, strideBytes=0;
  uint16_t width=0, height=0;
};
using JpegDecodeHandler = Status (*)(void *context,const uint8_t *jpeg,size_t bytes,JpegDecodeTarget &target);
struct JpegMediaStats {
  uint64_t decodedFrames=0,cacheHits=0,decodeFailures=0;
  size_t outputBytes=0,inputBytes=0,cachedInstances=0,cacheBudgetBytes=8388608;
  uint64_t evictedInstances=0;
  // Wall time around the synchronous hardware driver call, including its
  // parsing/cache/DMA wait; copyMicros covers input memcpy and tail clearing.
  // These cumulative diagnostics are not display frame-rate measurements.
  uint64_t decodeMicros=0,copyMicros=0;
  uint64_t hardwareDecodeCalls=0,directFlashCalls=0,copiedInputCalls=0,copiedInputBytes=0;
  uint64_t directFlashFallbacks=0;
  uint64_t directFramebufferFrames=0;
};
} }
