#pragma once
#include "Animation.h"

namespace nice_gfx { namespace htmlui {
struct TransitionSample { int32_t outX, outY, inX, inY; uint8_t opacity; bool active; };
struct TransitionPhaseSample { int32_t x, y; uint8_t opacity; bool active; };
inline bool validTransition(const TransitionDesc &a) {
  return uint8_t(a.effect)<=uint8_t(TransitionEffect::Fade) &&
    uint8_t(a.easing)<=uint8_t(AnimationEasing::EaseInOut) &&
    (a.effect==TransitionEffect::None || (a.duration>=1 && a.duration<=60000));
}
// 10-bit progress, matching html-ui/transition.js. Back reverses slide direction.
inline TransitionSample sampleTransition(const TransitionDesc &a,uint32_t elapsed,int32_t width,int32_t height,bool reverse=false) {
  TransitionSample s{0,0,0,0,255,false};
  if(a.effect==TransitionEffect::None||!a.duration)return s;
  const uint32_t progress=elapsed>=a.duration?1024:(elapsed<<10)/a.duration;
  const int32_t q=int32_t(animation_detail::easeQ10(progress,a.easing));
  s.active=elapsed<a.duration;
  if(a.effect==TransitionEffect::Fade)s.opacity=uint8_t(255*q/1024);
  else {
    const bool vertical=a.effect==TransitionEffect::SlideUp||a.effect==TransitionEffect::SlideDown;
    const int32_t distance=vertical?height:width;
    const int sign=(a.effect==TransitionEffect::SlideRight||a.effect==TransitionEffect::SlideDown?-1:1)*(reverse?-1:1);
    // Split before multiplying: exact truncation toward zero for any int32
    // distance, including negative/large standalone API inputs, without int64.
    const int32_t step=(distance/1024)*q+(distance%1024)*q/1024;
    if(vertical){s.outY=-sign*step;s.inY=sign*(distance-step);}
    else{s.outX=-sign*step;s.inX=sign*(distance-step);}
  }
  return s;
}
// A single page over the destination background. The legacy sampler above
// remains the paired incoming/outgoing transition used without page overrides.
inline TransitionPhaseSample sampleTransitionPhase(const TransitionDesc &a,uint32_t elapsed,int32_t width,int32_t height,bool entering,bool reverse=false) {
  const TransitionSample s=sampleTransition(a,elapsed,width,height,reverse);
  return {entering?s.inX:s.outX,entering?s.inY:s.outY,
    uint8_t(!entering&&a.effect==TransitionEffect::Fade?255-s.opacity:s.opacity),s.active};
}
} }
