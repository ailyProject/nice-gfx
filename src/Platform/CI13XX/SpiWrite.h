#pragma once
#include <stddef.h>
#include <stdint.h>

namespace nice_gfx { namespace detail {

// MODE0, MSB first. Io supplies masked GPIO writes and a wrapping cycle clock.
// Anchor each phase to an actual edge; an interrupt extends a phase instead of
// causing a burst of shortened clocks when execution resumes.
template<class Io>
inline void ci13xxWriteMode0(Io &io,const uint8_t *bytes,size_t count,uint32_t halfCycles) {
  if(!count)return;
  if(!halfCycles)halfCycles=1;
  uint32_t edge=io.cycles();
  for(size_t i=0;i<count;++i){
    for(uint8_t bit=0x80;bit;bit>>=1){
      while(uint32_t(io.cycles()-edge)<halfCycles){}
      io.low((bytes[i]&bit)!=0);
      edge=io.cycles();
      while(uint32_t(io.cycles()-edge)<halfCycles){}
      io.high();
      edge=io.cycles();
    }
    if((i&63)==63){
      while(uint32_t(io.cycles()-edge)<halfCycles){}
      io.idleLow();io.yield();edge=io.cycles();
    }
  }
  while(uint32_t(io.cycles()-edge)<halfCycles){}
  io.idleLow();
}

} }
