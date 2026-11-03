#pragma once
#include "../MenuMotion.h"
#include <limits.h>
#include <algorithm>

namespace nice_gfx { namespace htmlui { namespace menu_motion {

// Q8 positions; velocity is Q8 pixels/second. Target changes preserve momentum.
// Shared integer equations are mirrored by the browser preview. A bounded 8ms
// integrator remains stable after clock stalls and uint32 clock wraparound.
struct Axis {
  int64_t position=0, target=0, velocity=0;
  int16_t velocityRemainder=0,positionRemainder=0; // absolute value <1000
  int32_t smoothRemainder=0; // absolute value <65536
  void clearRemainders() { velocityRemainder=positionRemainder=smoothRemainder=0; }
  void snap(int32_t pixels) { position=target=int64_t(pixels)*256;velocity=0;clearRemainders(); }
  void aim(int32_t pixels) { target=int64_t(pixels)*256; }
  int32_t pixels() const {
    // Floor division after adding half also rounds negative coordinates.
    const int64_t n=position+128;
    const int64_t value=n>=0?n/256:-((-n+255)/256);
    return int32_t(value<INT32_MIN?INT32_MIN:value>INT32_MAX?INT32_MAX:value);
  }
  bool active() const { return position!=target||velocity!=0; }
  void step(uint32_t elapsed,const MenuMotionConfig &config) {
    step(elapsed,config,MenuDynamics{});
  }
  void step(uint32_t elapsed,const MenuMotionConfig &config,const MenuDynamics &dynamics) {
    if(config.mode==MenuMotionMode::Instant){position=target;velocity=0;clearRemainders();return;}
    elapsed=uint32_t(double(std::min<uint32_t>(elapsed,config.maxDeltaMs))*config.speed);
    const bool custom=dynamics.frequencyHz!=0||dynamics.dampingRatio!=0.75f;
    const double omega=dynamics.frequencyHz?double(dynamics.frequencyHz)*6.283185307179586:4000.0/config.responseMs;
    const int64_t omegaQ10=int64_t(omega*1024+0.5),dampingQ10=int64_t(2*double(dynamics.dampingRatio)*omegaQ10+0.5);
    const uint32_t maxStep=custom?uint32_t(1000/(omega*(1+2*dynamics.dampingRatio))):8;
    const uint32_t stepLimit=maxStep<1?1:maxStep>8?8:maxStep;
    const uint32_t response=custom?uint32_t(4000/omega):config.responseMs;
    while(elapsed){const uint32_t dt=elapsed>stepLimit?stepLimit:elapsed;elapsed-=dt;
      const int64_t delta=target-position;
      if(config.mode==MenuMotionMode::Smooth){
        const int64_t movement=delta*dt*65536/(std::max<uint32_t>(1,response/4)+dt)+smoothRemainder;
        position+=movement/65536;smoothRemainder=movement%65536;velocity=0;
      }else{
        const int64_t response=config.responseMs;
        const int64_t square=response*response;
        const int64_t omegaSquare=omegaQ10*omegaQ10;
        const int64_t acceleration=custom?(delta/1048576)*omegaSquare+(delta%1048576)*omegaSquare/1048576-velocity*dampingQ10/1024:
          (delta/square)*16000000+(delta%square)*16000000/square-velocity*6000/response;
        const int64_t dv=acceleration*dt+velocityRemainder;
        velocity+=dv/1000;velocityRemainder=dv%1000;
        const int64_t dx=velocity*dt+positionRemainder;
        position+=dx/1000;positionRemainder=dx%1000;
      }
      const int64_t distance=target-position;
      if(distance>=-64&&distance<=64&&velocity>=-256&&velocity<=256){position=target;velocity=0;clearRemainders();}
    }
  }
};

} } }
