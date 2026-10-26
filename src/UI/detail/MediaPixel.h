#pragma once
#include "../Compiled.h"
#include "../../Core/Platform.h"
#include "../../Core/Pixel.h"
#include <string.h>

namespace nice_gfx { namespace htmlui { namespace detail {
inline uint8_t mediaByte(const uint8_t *p,bool ram) { return ram?*p:pgm_read_byte(p); }
// The caller validates source bounds once. A false return is transparent;
// black and white remain independent colors when an opacity mask is present.
inline bool sampleMediaPixel(const MediaDesc &m,const uint8_t *frame,size_t stride,size_t x,size_t y,
                             bool ram,PixelOrder order,uint16_t &color) {
  const uint8_t *row=frame+y*stride;
  if(mediaPacked(m)) {
    const uint8_t mask=uint8_t(0x80u>>(x&7));
    if(mediaMasked(m)&&!(mediaByte(row+stride*m.height+x/8,ram)&mask))return false;
    color=(mediaByte(row+x/8,ram)&mask)?0xffff:0;return true;
  }
  if(mediaPixelBytes(m)==2){
    const uint8_t *p=row+x*2;
    if(ram&&order==PixelOrder::Native)memcpy(&color,p,2);
    else color=uint16_t(uint16_t(mediaByte(p,ram))<<8|mediaByte(p+1,ram));
    if(mediaColorKeyed(m)){
      const uint16_t key=uint16_t(uint16_t(mediaByte(m.pixels,ram))<<8|mediaByte(m.pixels+1,ram));
      if(color==key)return false;
    }
  }else{
    const uint8_t b=mediaByte(row+x,ram);
    color=uint16_t((((b>>5)*31+3)/7)<<11|(((b>>2)&7)*9)<<5|((b&3)*31+1)/3);
  }
  return true;
}
} } }
