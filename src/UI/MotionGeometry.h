#pragma once
#include "Compiled.h"
#include <stdint.h>

#ifndef NICE_GFX_UI_EXACT_FAST_MOTION_BOUNDS
#define NICE_GFX_UI_EXACT_FAST_MOTION_BOUNDS 1
#endif

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
// Local edge coordinates in hundredths of a pixel. Shapes use pixel-center
// extents so 0/100 percent coincide with their first/last authored pixels.
inline void pivot100(Rect box,int16_t pivotX,int16_t pivotY,bool pixelCenters,int64_t &x,int64_t &y) {
  x=int64_t(box.width-(pixelCenters?1:0))*pivotX+(pixelCenters?50:0);
  y=int64_t(box.height-(pixelCenters?1:0))*pivotY+(pixelCenters?50:0);
}
inline Rect bounds(Rect box,uint16_t scale,int16_t rotation,int16_t pivotX=50,int16_t pivotY=50,bool pixelCenters=false) {
  if((scale==1024&&rotation%360==0)||box.width<=0||box.height<=0)return box;
  // Bound the actual quantized inverse, not an independently rounded forward
  // matrix. Q14 table rounding otherwise loses edge pixels on large boxes.
  const int32_t s=sine(rotation)*4096/scale,c=sine(rotation+90)*4096/scale;
#if NICE_GFX_UI_EXACT_FAST_MOTION_BOUNDS
  // A conservative ordinary-display domain keeps every intermediate exact and
  // bounded. Other pivots/scales/coordinates retain the original algorithm.
  if(pivotX==50&&pivotY==50&&scale>=128&&scale<=4096&&
     box.x>=-32768&&box.x<=32767&&box.y>=-32768&&box.y<=32767&&
     box.width<=4096&&box.height<=4096){
    const uint32_t a=uint32_t(c<0?-c:c),b=uint32_t(s<0?-s:s);
    uint32_t right,bottom;
    if(!b||!a){
      // Cancel the common nonzero inverse coefficient before division. At the
      // admitted limits each rounded numerator is < 2,416,967,680 (<2^32).
      const uint32_t coefficient=a?a:b,denominator=coefficient*2;
      const uint32_t x=a?uint32_t(box.width)*(coefficient+65536u):uint32_t(box.width)*coefficient+uint32_t(box.height)*65536u;
      const uint32_t y=a?uint32_t(box.height)*(coefficient+65536u):uint32_t(box.height)*coefficient+uint32_t(box.width)*65536u;
      right=(x+denominator-1)/denominator;bottom=(y+denominator-1)/denominator;
    }else{
      const uint64_t denominator=(uint64_t(a)*a+uint64_t(b)*b)*2;
      const uint64_t x=uint64_t(box.width)*(denominator/2)+(uint64_t(a)*box.width+uint64_t(b)*box.height)*65536;
      const uint64_t y=uint64_t(box.height)*(denominator/2)+(uint64_t(b)*box.width+uint64_t(a)*box.height)*65536;
      right=uint32_t((x+denominator-1)/denominator);bottom=uint32_t((y+denominator-1)/denominator);
    }
    // floor(width - q) == width - ceil(q): the two rational bounds sum
    // exactly to width (height respectively), including odd pixel extents.
    const int32_t left=box.width-int32_t(right),top=box.height-int32_t(bottom);
    return {box.x+left,box.y+top,int32_t(right)-left,int32_t(bottom)-top};
  }
#endif
  const int64_t denominator=int64_t(c)*c+int64_t(s)*s;
  if(pivotX!=50||pivotY!=50) {
    int64_t px,py;pivot100(box,pivotX,pivotY,pixelCenters,px,py);
    const int64_t baseX=floorDivide(px,100),baseY=floorDivide(py,100);
    const int64_t remainderX=px-baseX*100,remainderY=py-baseY*100;
    int64_t left=INT64_MAX,top=INT64_MAX,right=INT64_MIN,bottom=INT64_MIN;
    for(uint8_t corner=0;corner<4;++corner) {
      const int64_t x=(corner&1?int64_t(box.width)*100:0)-px,y=(corner&2?int64_t(box.height)*100:0)-py;
      const int64_t nx=(int64_t(c)*x-int64_t(s)*y)*65536+remainderX*denominator;
      const int64_t ny=(int64_t(s)*x+int64_t(c)*y)*65536+remainderY*denominator;
      const int64_t loX=box.x+baseX+floorDivide(nx,denominator*100),hiX=box.x+baseX+ceilDivide(nx,denominator*100);
      const int64_t loY=box.y+baseY+floorDivide(ny,denominator*100),hiY=box.y+baseY+ceilDivide(ny,denominator*100);
      if(loX<left)left=loX;if(hiX>right)right=hiX;if(loY<top)top=loY;if(hiY>bottom)bottom=hiY;
    }
    return {int32_t(left),int32_t(top),int32_t(right-left),int32_t(bottom-top)};
  }
  const int64_t halfX=(int64_t(c<0?-c:c)*box.width+int64_t(s<0?-s:s)*box.height)*32768;
  const int64_t halfY=(int64_t(s<0?-s:s)*box.width+int64_t(c<0?-c:c)*box.height)*32768;
  const int64_t cx=int64_t(box.width)*denominator,cy=int64_t(box.height)*denominator;
  const int32_t left=box.x+int32_t(floorDivide(cx-2*halfX,2*denominator)),top=box.y+int32_t(floorDivide(cy-2*halfY,2*denominator));
  const int32_t right=box.x+int32_t(ceilDivide(cx+2*halfX,2*denominator)),bottom=box.y+int32_t(ceilDivide(cy+2*halfY,2*denominator));
  return {left,top,right-left,bottom-top};
}
struct Inverse {
  int32_t xx,xy,yx,yy;
  int16_t pivotX,pivotY;bool pixelCenters;
  explicit Inverse(uint16_t scale,int16_t rotation,int16_t px=50,int16_t py=50,bool centers=false):pivotX(px),pivotY(py),pixelCenters(centers) {
    const int32_t c=sine(rotation+90)*4096/scale,s=sine(rotation)*4096/scale;
    xx=yy=c;xy=s;yx=-s;
  }
  void offset100(Rect box,int32_t x,int32_t y,int64_t &dx,int64_t &dy) const {
    int64_t px,py;pivot100(box,pivotX,pivotY,pixelCenters,px,py);
    dx=(int64_t(x)*2+1-int64_t(box.x)*2)*50-px;
    dy=(int64_t(y)*2+1-int64_t(box.y)*2)*50-py;
  }
  void point(Rect box,int32_t x,int32_t y,int64_t &u,int64_t &v) const {
    int64_t px,py,dx,dy;pivot100(box,pivotX,pivotY,pixelCenters,px,py);offset100(box,x,y,dx,dy);
    u=floorDivide(px*65536+dx*xx+dy*xy,100);
    v=floorDivide(py*65536+dx*yx+dy*yy,100);
  }
};
inline Rect reveal(Rect box,uint16_t fraction,uint8_t flags) {
  if(fraction>=1024)return box;
  const uint8_t direction=(flags>>1)&3;
  if(direction<2){const int32_t width=int32_t(int64_t(box.width)*fraction/1024);if(direction==1)box.x+=box.width-width;box.width=width;}
  else {const int32_t height=int32_t(int64_t(box.height)*fraction/1024);if(direction==3)box.y+=box.height-height;box.height=height;}
  return box;
}
inline bool contains(Rect box,uint16_t scale,int16_t rotation,int32_t x,int32_t y,uint16_t fraction=1024,uint8_t flags=0,int16_t pivotX=50,int16_t pivotY=50,bool pixelCenters=false) {
  int64_t u,v;Inverse(scale,rotation,pivotX,pivotY,pixelCenters).point(box,x,y,u,v);
  const Rect mask=reveal({0,0,box.width,box.height},fraction,flags);
  return u>=int64_t(mask.x)*65536&&v>=int64_t(mask.y)*65536&&u<int64_t(mask.x+mask.width)*65536&&v<int64_t(mask.y+mask.height)*65536;
}
}}}
