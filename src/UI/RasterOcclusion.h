#pragma once
#include "Compiled.h"

namespace nice_gfx { namespace htmlui { namespace raster_occlusion {

// A conservative interior of an opaque rounded fill. A tile's top/bottom
// spans bound every intervening midpoint span, so this rectangle is covered
// in every row. Corner pixels outside it still take the original paint path.
inline uint32_t root(uint32_t value) {
  uint32_t result=0,bit=1u<<30;
  while(bit>value)bit>>=2;
  while(bit){if(value>=result+bit){value-=result+bit;result=(result>>1)+bit;}else result>>=1;bit>>=2;}
  return result;
}
inline int32_t inset(int32_t radius,int32_t row,int32_t height) {
  const uint32_t d=row<radius?uint32_t(radius-row):row>=height-radius?uint32_t(row-(height-radius-1)):0;
  if(!d)return 0;
  const uint32_t square=d*d,remainder=uint32_t(radius)*radius-square;
  uint32_t extent;
  if(square<=remainder){extent=root(remainder);if(remainder-extent*extent>extent)++extent;}
  else extent=root(remainder+d-1);
  return radius-int32_t(extent);
}
inline Rect interior(Rect shape,int32_t radius,Rect clipped) {
  if(shape.width<=0||shape.height<=0||clipped.width<=0||clipped.height<=0)return {0,0,0,0};
  if(radius<0||radius>255||radius>shape.width/2||radius>shape.height/2)return {0,0,0,0};
  const int64_t top=clipped.y>shape.y?clipped.y:shape.y;
  const int64_t bottom=int64_t(clipped.y)+clipped.height<int64_t(shape.y)+shape.height?
    int64_t(clipped.y)+clipped.height:int64_t(shape.y)+shape.height;
  if(bottom<=top)return {0,0,0,0};
  const int32_t a=inset(radius,int32_t(top-shape.y),shape.height);
  const int32_t b=inset(radius,int32_t(bottom-shape.y-1),shape.height);
  const int32_t margin=a>b?a:b;
  const int64_t left=clipped.x>int64_t(shape.x)+margin?clipped.x:int64_t(shape.x)+margin;
  const int64_t right=int64_t(clipped.x)+clipped.width<int64_t(shape.x)+shape.width-margin?
    int64_t(clipped.x)+clipped.width:int64_t(shape.x)+shape.width-margin;
  if(right<=left)return {0,0,0,0};
  return {int32_t(left),int32_t(top),int32_t(right-left),int32_t(bottom-top)};
}
// Partition area minus its contained opaque interior into disjoint rectangles.
// There is no approximation of the visible pixels: excluded pixels will be
// overwritten by the later opaque node in the same frozen paint snapshot.
inline int strips(Rect area,Rect covered,Rect out[4]) {
  if(covered.width<=0||covered.height<=0){out[0]=area;return 1;}
  int count=0;
  const int32_t right=area.x+area.width,bottom=area.y+area.height;
  const int32_t coverRight=covered.x+covered.width,coverBottom=covered.y+covered.height;
  if(covered.y>area.y)out[count++]={area.x,area.y,area.width,covered.y-area.y};
  if(coverBottom<bottom)out[count++]={area.x,coverBottom,area.width,bottom-coverBottom};
  if(covered.x>area.x)out[count++]={area.x,covered.y,covered.x-area.x,covered.height};
  if(coverRight<right)out[count++]={coverRight,covered.y,right-coverRight,covered.height};
  return count;
}

}}} // namespace nice_gfx::htmlui::raster_occlusion
