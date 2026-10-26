#pragma once
#include "../Display/Capabilities.h"
#include "Compiled.h"
#include <stddef.h>

namespace nice_gfx { namespace htmlui {

// UI decisions depend on capabilities, never a controller name. All input,
// focus, routing and business state remain live while pixels are rate limited.
struct DisplayPolicy {
  bool monochrome = false;
  bool slowRefresh = false;
  uint32_t frameIntervalMs = 0;

  static DisplayPolicy from(const DisplayCapabilities &caps) {
    DisplayPolicy result;
    result.monochrome = caps.monochrome(); result.slowRefresh = caps.slowRefresh();
    result.frameIntervalMs = caps.recommendedFrameMs;
    if (result.slowRefresh && result.frameIntervalMs < caps.minRefreshMs)
      result.frameIntervalMs = caps.minRefreshMs;
    return result;
  }
  bool terminalAnimation(const AnimationDesc &animation, const MotionDesc *motion = nullptr) const {
    if (slowRefresh) return true;
    if (!monochrome || motion) return false;
    return animation.effect == AnimationEffect::FadeIn || animation.effect == AnimationEffect::FadeOut ||
           animation.effect == AnimationEffect::Pulse;
  }
  size_t defaultTilePixels(uint16_t width) const { return size_t(width) * (monochrome ? 4 : 16); }
  static uint16_t foregroundOn(uint16_t background) {
    const unsigned r=background>>11,g=(background>>5)&63,b=background&31;
    const unsigned luminance=77*((r<<3)|(r>>2))+150*((g<<2)|(g>>4))+29*((b<<3)|(b>>2));
    return luminance+128u >= 128u*256u ? 0x0000 : 0xffff;
  }
};

} }
