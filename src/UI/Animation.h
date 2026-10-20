#pragma once
#include "Compiled.h"

namespace nice_gfx { namespace htmlui {
struct AnimationSample { int32_t x, y; uint8_t opacity; bool active; };
inline bool validAnimation(const AnimationDesc &a) {
  return uint8_t(a.effect)<=uint8_t(AnimationEffect::SlideOutDown) &&
    uint8_t(a.easing)<=uint8_t(AnimationEasing::BounceOut) &&
    uint8_t(a.trigger)<=uint8_t(AnimationTrigger::Show) &&
    (a.effect==AnimationEffect::None || (a.duration>=1 && a.duration<=60000 && a.delay<=60000 && a.repeat<=1000 && a.distance<=1024));
}
inline bool validExitAnimation(const AnimationDesc &a) {
  return validAnimation(a)&&a.trigger==AnimationTrigger::Manual&&a.repeat==1&&
    (a.effect==AnimationEffect::None||a.effect==AnimationEffect::FadeOut||
     (a.effect>=AnimationEffect::SlideOutLeft&&a.effect<=AnimationEffect::SlideOutDown));
}
namespace animation_detail {
// q is Q10 in [0,1024]. Every cubic numerator is at most 2^30,
// so these curves need neither 64-bit multiply nor divide on 32-bit MCUs.
inline uint32_t easeQ10(uint32_t q,AnimationEasing easing) {
  if(easing==AnimationEasing::EaseIn)return (q*q*q)>>20;
  if(easing==AnimationEasing::EaseOut){const uint32_t r=1024-q;return 1024-((r*r*r)>>20);}
  if(easing==AnimationEasing::EaseInOut)return (q*q*(3072-2*q))>>20;
  if(easing==AnimationEasing::BackOut){
    const int32_t t=int32_t(q)-1024,t2=t*t/1024,t3=t2*t/1024;
    return uint32_t(1024+(2766*t3+1742*t2)/1024);
  }
  if(easing==AnimationEasing::BounceOut){
    int32_t t=int32_t(q),base=0;
    if(q>=930){t-=977;base=1008;}
    else if(q>=744){t-=838;base=960;}
    else if(q>=372){t-=558;base=768;}
    const uint32_t value=uint32_t(base+121*t*t/16384);
    return value>1024?1024:value;
  }
  return q;
}
inline uint8_t alpha(int32_t value) {return uint8_t(value<0?0:value>255?255:value);}
}
// 10-bit progress, integer arithmetic: keep in sync with html-ui/animation.js.
inline AnimationSample sampleAnimation(const AnimationDesc &a,uint32_t elapsed) {
  AnimationSample s{0,0,255,false};
  if(a.effect==AnimationEffect::None||!a.duration)return s;
  const uint32_t time=elapsed<a.delay?0:elapsed-a.delay;
  const bool finished=elapsed>=a.delay&&a.repeat&&time>=uint32_t(a.duration)*a.repeat;
  // Valid durations are <=60000: the scaled remainder fits in 26 bits.
  const uint32_t progress=elapsed<a.delay?0:finished?1024:((time%a.duration)<<10)/a.duration;
  const int32_t q=int32_t(animation_detail::easeQ10(progress,a.easing));
  // distance <=1024 also bounds every effect numerator to abs(value) <=2^30.
  // Keep negative divisions signed to preserve truncation toward zero.
  const int32_t triangle=q<512?2*q:2048-2*q,d=a.distance;
  switch(a.effect) {
    case AnimationEffect::FadeIn:s.opacity=animation_detail::alpha(255*q/1024);break;
    case AnimationEffect::FadeOut:s.opacity=animation_detail::alpha(255-255*q/1024);break;
    case AnimationEffect::SlideLeft:s.x=int32_t(-d*(1024-q)/1024);break;
    case AnimationEffect::SlideRight:s.x=int32_t(d*(1024-q)/1024);break;
    case AnimationEffect::SlideUp:s.y=int32_t(d*(1024-q)/1024);break;
    case AnimationEffect::SlideDown:s.y=int32_t(-d*(1024-q)/1024);break;
    case AnimationEffect::Float:s.y=int32_t(-d*triangle/1024);break;
    case AnimationEffect::Bounce:s.y=int32_t(-d*4*q*(1024-q)/1048576);break;
    case AnimationEffect::Shake:{const int32_t phase=q*4%1024,wave=phase<256?phase*4:phase<768?2048-phase*4:phase*4-4096;s.x=d*wave*(1024-q)/1048576;break;}
    case AnimationEffect::Pulse:s.opacity=animation_detail::alpha(255-166*triangle/1024);break;
    case AnimationEffect::Blink:s.opacity=q>=256&&q<768?0:255;break;
    case AnimationEffect::SlideOutLeft:s.x=-d*q/1024;break;
    case AnimationEffect::SlideOutRight:s.x=d*q/1024;break;
    case AnimationEffect::SlideOutUp:s.y=-d*q/1024;break;
    case AnimationEffect::SlideOutDown:s.y=d*q/1024;break;
    default:break;
  }
  s.active=!finished;return s;
}
struct MotionSample {
  int32_t x, y;
  uint16_t scale;
  int16_t rotation;
  uint16_t reveal;
  uint8_t opacity;
  bool active;
};
inline bool validMotion(const AnimationDesc &a,const MotionDesc *m) {
  if(!m)return true;
  if(!validAnimation(a)||a.duration<1||a.duration>60000||a.delay>60000||a.repeat>1000||m->flags>7||
     m->pivotX<-1000||m->pivotX>1000||m->pivotY<-1000||m->pivotY>1000)return false;
  if(!m->count)return !m->frames&&a.effect!=AnimationEffect::None;
  if(!m->frames||m->count<2||m->count>16||m->frames[0].offset!=0||m->frames[m->count-1].offset!=1024)return false;
  for(uint8_t i=0;i<m->count;++i){const auto &f=m->frames[i];
    if((i&&f.offset<=m->frames[i-1].offset)||f.offset>1024||f.x<-1024||f.x>1024||f.y<-1024||f.y>1024||
       f.scale<128||f.scale>4096||f.rotation<-360||f.rotation>360||f.reveal>1024)return false;
  }
  return true;
}
// The two integer divisions outside the segment search are shared by every
// property. No per-property clocks, float math, allocations or runtime parsing.
inline MotionSample sampleMotion(const AnimationDesc &a,const MotionDesc *m,uint32_t elapsed) {
  if(!m){const auto s=sampleAnimation(a,elapsed);return {s.x,s.y,1024,0,1024,s.opacity,s.active};}
  if(!a.duration)return {0,0,1024,0,1024,255,false};
  const uint32_t time=elapsed<a.delay?0:elapsed-a.delay;
  const bool finished=elapsed>=a.delay&&a.repeat&&time>=uint32_t(a.duration)*a.repeat;
  const uint32_t cycle=finished?a.repeat-1:time/a.duration;
  const bool reverse=(m->flags&MotionAlternate)&&(cycle&1);
  uint32_t progress=elapsed<a.delay?0:finished?1024:((time%a.duration)<<10)/a.duration;
  if(reverse)progress=1024-progress;
  if(!m->count){
    AnimationDesc one=a;one.delay=0;one.repeat=1;
    // Presets sample elapsed in milliseconds; use the unreduced remainder so
    // alternate sampling stays exact even for durations that do not divide Q10.
    uint32_t remainder=elapsed<a.delay?0:finished?a.duration:time%a.duration;
    if(reverse)remainder=a.duration-remainder;
    const auto s=sampleAnimation(one,remainder);
    return {s.x,s.y,1024,0,1024,s.opacity,!finished};
  }
  uint8_t index=1;while(index+1<m->count&&progress>m->frames[index].offset)++index;
  const auto &from=m->frames[index-1],&to=m->frames[index];
  const uint32_t local=(progress-from.offset)*1024/(to.offset-from.offset);
  const int32_t q=int32_t(animation_detail::easeQ10(local,a.easing));
  const auto lerp=[q](int32_t x,int32_t y){return x+(y-x)*q/1024;};
  const int32_t scale=lerp(from.scale,to.scale),reveal=lerp(from.reveal,to.reveal),rotation=lerp(from.rotation,to.rotation);
  return {lerp(from.x,to.x),lerp(from.y,to.y),uint16_t(scale<128?128:scale>4096?4096:scale),
    int16_t(rotation<-360?-360:rotation>360?360:rotation),uint16_t(reveal<0?0:reveal>1024?1024:reveal),
    animation_detail::alpha(lerp(from.opacity,to.opacity)),!finished};
}
} }
