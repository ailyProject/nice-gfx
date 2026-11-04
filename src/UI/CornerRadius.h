#pragma once
#include "Compiled.h"
#include <algorithm>

namespace nice_gfx { namespace htmlui {

inline uint8_t cornerRadius(const Style &style,unsigned corner) {
  return style.cornerRadii?style.cornerRadii[corner]:style.radius;
}

inline uint8_t maxCornerRadius(const Style &style) {
  if(!style.cornerRadii)return style.radius;

  uint8_t radius=0;

  for(unsigned corner=0;corner<4;++corner)radius=std::max(radius,cornerRadius(style,corner));

  return radius;
}

inline bool sameCornerRadii(const Style &a,const Style &b) {
  for(unsigned corner=0;corner<4;++corner)if(cornerRadius(a,corner)!=cornerRadius(b,corner))return false;

  return true;
}

// Preserve the existing midpoint rasterizer. Each corner owns one disjoint
// quadrant; viewport clipping keeps fills, gradients and outlines consistent.
template<class Canvas,class Paint> void paintCornerQuadrants(Canvas &canvas,Rect box,Rect clip,const Style &style,Paint paint) {
  const int32_t halfWidth=box.width/2,halfHeight=box.height/2;
  const Rect quadrants[]={
    {box.x,box.y,halfWidth,halfHeight},
    {box.x+halfWidth,box.y,box.width-halfWidth,halfHeight},
    {box.x+halfWidth,box.y+halfHeight,box.width-halfWidth,box.height-halfHeight},
    {box.x,box.y+halfHeight,halfWidth,box.height-halfHeight}
  };

  for(unsigned corner=0;corner<4;++corner){
    const Rect q=quadrants[corner];
    const int32_t x=std::max(q.x,clip.x),y=std::max(q.y,clip.y);
    const int32_t right=std::min(q.x+q.width,clip.x+clip.width),bottom=std::min(q.y+q.height,clip.y+clip.height);

    if(right<=x||bottom<=y)continue;

    canvas.setViewport(x,y,right-x,bottom-y,false);
    paint(std::min<int32_t>(cornerRadius(style,corner),std::min(box.width,box.height)/2));
  }

  canvas.setViewport(clip.x,clip.y,clip.width,clip.height,false);
}

template<class Canvas> void strokeCornerRect(Canvas &canvas,Rect box,Rect clip,const Style &style,int32_t width,uint16_t color) {
  const auto stroke=[&](int32_t radius){
    for(int32_t inset=0;inset<width&&2*inset<std::min(box.width,box.height);++inset)
      canvas.drawRoundRect(box.x+inset,box.y+inset,box.width-2*inset,box.height-2*inset,std::max<int32_t>(0,radius-inset),color);
  };

  if(style.cornerRadii)paintCornerQuadrants(canvas,box,clip,style,stroke);
  else stroke(std::min<int32_t>(style.radius,std::min(box.width,box.height)/2));
}

}}
