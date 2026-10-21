#pragma once
#include "Compiled.h"
#include <algorithm>
namespace nice_gfx { namespace htmlui { namespace detail {
struct RangeGeometry {
 Rect track,fill,thumb;int32_t inset,length,position;bool vertical;
 int32_t coordinate(const Rect &r,int32_t x,int32_t y,int32_t grab=0) const {
  const int32_t offset=vertical?r.y+r.height-y+grab:x-r.x-grab;
  return std::max<int32_t>(0,std::min<int32_t>(length,offset-inset));
 }
};
inline RangeGeometry rangeGeometry(Rect r,const Style &s,bool vertical,bool hasThumb,int32_t value,int32_t minimum,int32_t maximum){
 RangeGeometry g{};g.vertical=vertical;
 const int32_t tw=std::min<int32_t>(r.width,s.thumbWidth),th=std::min<int32_t>(r.height,s.thumbHeight);
 g.inset=hasThumb?(vertical?th:tw)/2:0;
 g.length=std::max<int32_t>(1,(vertical?r.height:r.width)-2*g.inset);
 const int32_t track=std::min<int32_t>(vertical?r.width:r.height,s.trackSize);
 value=std::max(minimum,std::min(maximum,value));
 g.position=maximum>minimum?int32_t((int64_t(value)-minimum)*g.length/(int64_t(maximum)-minimum)):0;
 if(vertical){
  g.track={r.x+(r.width-track)/2,r.y+g.inset,track,g.length};
  g.fill={g.track.x,r.y+r.height-g.inset-g.position,track,g.position};
  g.thumb={r.x+(r.width-tw)/2,r.y+r.height-g.inset-g.position-th/2,tw,th};
 }else{
  g.track={r.x+g.inset,r.y+(r.height-track)/2,g.length,track};
  g.fill={g.track.x,g.track.y,g.position,track};
  g.thumb={r.x+g.inset+g.position-tw/2,r.y+(r.height-th)/2,tw,th};
 }
 return g;
}
}}}
