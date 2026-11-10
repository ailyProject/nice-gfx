#pragma once
#include "../Core/Status.h"
#include <stdint.h>

namespace nice_gfx {

struct NativeDrawingLease {
  uint64_t owner=0,frame=0,epoch=0,sourceVersion=0;
  uint8_t target=255,source=255;
};

struct NativePresentationReceipt {
  uint64_t owner=0,frame=0,epoch=0;
  uint32_t baseline=0;
  uint8_t target=255,required=0;
};

// Prepared retains a sealed drawing image. Accepted starts before entering the
// SDK because an error may still retain ownership. Retired means the original
// armed receipt reached its real fence. Only Ok + retired may publish a frame.
// A faulted receipt can retire with IoError without allowing publication/retry.
struct NativePresentationTransfer {
  Status status=Status::Unsupported;
  NativePresentationReceipt receipt{};
  bool prepared=false,accepted=false,retired=false;
};

struct NativePresentationStats {
  uint64_t preparedFrames=0,acceptedFrames=0,retiredFences=0,publishedFrames=0;
  uint64_t cpuOnlyFrames=0,nonStageFrames=0,typedAborts=0,errors=0;
  uint64_t specialDrains=0,endingDrains=0,onePerPollDeferrals=0;
  bool pending=false,ready=false,fatal=false;
};

}
