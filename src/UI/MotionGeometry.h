#pragma once
#include "Compiled.h"
#include <stdint.h>

namespace nice_gfx { namespace htmlui { namespace motion_geometry {
// One flash table, quarter-wave symmetry. Trigonometry is never evaluated on
// the device; the inner raster loop uses additions of inverse Q16 coefficients.
inline int32_t sine(int angle) {
  static const int16_t values[91] = {0,286,572,857,1143,1428,1713,1997,2280,2563,2845,3126,3406,3686,3964,4240,4516,4790,5063,5334,5604,5872,6138,6402,6664,6924,7182,7438,7692,7943,8192,8438,8682,8923,9162,9397,9630,9860,10087,10311,10531,10749,10963,11174,11381,11585,11786,11982,12176,12365,12551,12733,12911,13085,13255,13421,13583,13741,13894,14044,14189,14330,14466,14598,14726,14849,14968,15082,15191,15296,15396,15491,15582,15668,15749,15826,15897,15964,16026,16083,16135,16182,16225,16262,16294,16322,16344,16362,16374,16382,16384};
  angle%=360;if(angle<0)angle+=360;
  if(angle<=90)return values[angle];
  if(angle<=180)return values[180-angle];
  if(angle<=270)return -values[angle-180];
  return -values[360-angle];
}
inline int64_t floorDivide(int64_t numerator,int64_t denominator) {
  return numerator>=0?numerator/denominator:-((-numerator+denominator-1)/denominator);
}
inline int64_t ceilDivide(int64_t numerator,int64_t denominator) {
  return -floorDivide(-numerator,denominator);
}
inline Rect bounds(Rect box,uint16_t scale,int16_t rotation) {
  if((scale==1024&&rotation%360==0)||box.width<=0||box.height<=0)return box;
  // Bound the actual quantized inverse, not an independently rounded forward
  // matrix. Q14 table rounding otherwise loses edge pixels on large boxes.
  const int32_t s=sine(rotation)*4096/scale,c=sine(rotation+90)*4096/scale;
  const int64_t denominator=int64_t(c)*c+int64_t(s)*s;
  const int64_t halfX=(int64_t(c<0?-c:c)*box.width+int64_t(s<0?-s:s)*box.height)*32768;
  const int64_t halfY=(int64_t(s<0?-s:s)*box.width+int64_t(c<0?-c:c)*box.height)*32768;
  const int64_t cx=int64_t(box.width)*denominator,cy=int64_t(box.height)*denominator;
  const int32_t left=box.x+int32_t(floorDivide(cx-2*halfX,2*denominator)),top=box.y+int32_t(floorDivide(cy-2*halfY,2*denominator));
  const int32_t right=box.x+int32_t(ceilDivide(cx+2*halfX,2*denominator)),bottom=box.y+int32_t(ceilDivide(cy+2*halfY,2*denominator));
  return {left,top,right-left,bottom-top};
}
struct Inverse {
  int32_t xx,xy,yx,yy;
  explicit Inverse(uint16_t scale,int16_t rotation) {
    const int32_t c=sine(rotation+90)*4096/scale,s=sine(rotation)*4096/scale;
    xx=yy=c;xy=s;yx=-s;
  }
  void point(Rect box,int32_t x,int32_t y,int64_t &u,int64_t &v) const {
    const int64_t dx=int64_t(x)*2+1-(int64_t(box.x)*2+box.width),dy=int64_t(y)*2+1-(int64_t(box.y)*2+box.height);
    u=(int64_t(box.width)<<15)+floorDivide(dx*xx+dy*xy,2);
    v=(int64_t(box.height)<<15)+floorDivide(dx*yx+dy*yy,2);
  }
};
inline Rect reveal(Rect box,uint16_t fraction,uint8_t flags) {
  if(fraction>=1024)return box;
  const uint8_t direction=(flags>>1)&3;
  if(direction<2){const int32_t width=int32_t(int64_t(box.width)*fraction/1024);if(direction==1)box.x+=box.width-width;box.width=width;}
  else {const int32_t height=int32_t(int64_t(box.height)*fraction/1024);if(direction==3)box.y+=box.height-height;box.height=height;}
  return box;
}
inline bool contains(Rect box,uint16_t scale,int16_t rotation,int32_t x,int32_t y,uint16_t fraction=1024,uint8_t flags=0) {
  int64_t u,v;Inverse(scale,rotation).point(box,x,y,u,v);
  const Rect mask=reveal({0,0,box.width,box.height},fraction,flags);
  return u>=int64_t(mask.x)*65536&&v>=int64_t(mask.y)*65536&&u<int64_t(mask.x+mask.width)*65536&&v<int64_t(mask.y+mask.height)*65536;
}
}}}
