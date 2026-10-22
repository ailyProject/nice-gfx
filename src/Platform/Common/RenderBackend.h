#pragma once
#include "../../Display/Transport.h"

// Compile-time backend selection lives in Platform. Operations are per region,
// with no virtual calls in the pixel loop. Unsupported operations leave the
// caller's existing CPU renderer responsible for the complete region.
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include "../ESP32/Rgb565PpaBlend.h"
namespace nice_gfx { namespace platform { namespace render_backend {
using Stats = rgb565_ppa::Stats;
using Context = rgb565_ppa::Context;
} } }
#else
namespace nice_gfx { namespace platform { namespace render_backend {
struct Stats {
  uint64_t completed=0,driverMicros=0,conversionMicros=0;
  uint64_t copyCompleted=0,copyDriverMicros=0;
  uint32_t rejected=0,failures=0;
  int lastError=0;
  size_t scratchBytes=0;
};

// No accelerator on this target. Keep the same rejection and borrowing contract
// as a disabled hardware backend: no allocations, no destination writes and no
// borrowed output. The UI therefore takes its unchanged software fallback.
class Context {
 public:
  Context()=default;
  Context(const Context&)=delete;
  Context&operator=(const Context&)=delete;
  static constexpr bool available(){return false;}
  const Stats &stats()const{return stats_;}
  bool blend(uint16_t *,size_t,const uint16_t *,size_t,size_t,size_t,uint8_t){return reject();}
  const uint16_t *blendSourcesToWire(const uint16_t *,size_t,const uint16_t *,size_t,size_t,size_t,uint8_t){reject();return nullptr;}
  const uint16_t *blendSourcesToNative(const uint16_t *,size_t,const uint16_t *,size_t,size_t,size_t,uint8_t){reject();return nullptr;}
  bool blendSourcesIntoNative(uint16_t *,size_t,size_t,const uint16_t *,size_t,const uint16_t *,size_t,size_t,size_t,uint8_t){return reject();}
  bool blendSolidIntoNative(uint16_t *,size_t,size_t,const uint16_t *,size_t,size_t,size_t,uint16_t,uint8_t){return reject();}
  Status blendSolidNativeIntoNative(uint16_t *,size_t,size_t,const uint16_t *,size_t,size_t,size_t,uint16_t,uint8_t){reject();return Status::Unsupported;}
  bool canCopyIntoNative(const NativeFramebuffer &,int32_t,int32_t,PixelView,size_t,size_t,size_t,size_t,size_t)const{return false;}
  Status copyIntoNative(const NativeFramebuffer &,int32_t,int32_t,PixelView,size_t,size_t,size_t,size_t,size_t){return Status::Unsupported;}
  bool close(){return true;}
 private:
  bool reject(){++stats_.rejected;return false;}
  Stats stats_;
};
} } }
#endif
