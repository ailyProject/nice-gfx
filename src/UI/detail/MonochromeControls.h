#pragma once
#include "../DisplayPolicy.h"
#include "../CornerRadius.h"
#include "RangeGeometry.h"

namespace nice_gfx { namespace htmlui { namespace detail {
// A final state mark must survive the control's own fill/knob drawing. Keeping
// this on the stack also covers paintNode's early returns and clipped tiles.
template<class Canvas> struct MonoControlState {
 Canvas &canvas;Rect box,clip;uint16_t background;bool disabled,pressed;

 ~MonoControlState(){
  if(box.width<=0||box.height<=0||(!disabled&&!pressed))return;

  canvas.setViewport(clip.x,clip.y,clip.width,clip.height,false);

  const uint16_t ink=DisplayPolicy::foregroundOn(background);
  const int32_t y=box.y+box.height-1;

  if(disabled)for(int32_t x=0;x<box.width;++x)canvas.drawPixel(box.x+x,y,(x&3)<2?ink:background);
  else {canvas.drawFastHLine(box.x,y,box.width,background);canvas.drawFastHLine(box.x,std::max<int32_t>(box.y,y-1),box.width,ink);}
 }
};

inline uint16_t monoContrast(uint16_t color,uint16_t background){
 return DisplayPolicy::foregroundOn(color)==DisplayPolicy::foregroundOn(background)?DisplayPolicy::foregroundOn(background):color;
}

template<class Canvas> void monoRange(Canvas &c,const RangeGeometry &g,const Style &s,bool thumb,Rect clip){
 const uint16_t empty=s.trackColor,ink=DisplayPolicy::foregroundOn(empty),fill=monoContrast(s.fillColor,empty);
 const int32_t cross=g.vertical?g.track.width:g.track.height;
 const int32_t radius=s.trackRadius==65535?cross/2:std::min<int32_t>(s.trackRadius,cross/2);

 c.fillRoundRect(g.track.x,g.track.y,g.track.width,g.track.height,radius,empty);

 // At one/two pixels a hollow rectangle becomes solid; a dotted baseline
 // retains a distinct empty state even when an author requests a hairline.
 if(cross<3){for(int32_t p=0;p<g.length;p+=2)c.drawPixel(g.track.x+(g.vertical?0:p),g.track.y+(g.vertical?p:0),ink);}
 else c.drawRoundRect(g.track.x,g.track.y,g.track.width,g.track.height,radius,ink);

 if(g.position>0){
  const auto paint=[&](int32_t curve){c.fillRoundRect(g.fill.x,g.fill.y,g.fill.width,g.fill.height,curve,fill);};

  if(!thumb&&s.cornerRadii&&g.position<g.length)paintCornerQuadrants(c,g.fill,clip,s,paint);
  else paint(radius);
 }

 if(thumb){const auto &t=g.thumb;const int32_t r=std::min<int32_t>(s.thumbRadius,std::min(t.width,t.height)/2);
  c.fillRoundRect(t.x,t.y,t.width,t.height,r,empty);

  if(t.width>2&&t.height>2)c.fillRoundRect(t.x+1,t.y+1,t.width-2,t.height-2,std::max<int32_t>(0,r-1),s.thumbColor);

  c.drawRoundRect(t.x,t.y,t.width,t.height,r,ink);
 }
}

template<class Canvas> void monoCheckbox(Canvas &c,Rect box,const Style &s,bool checked,uint16_t background,Rect clip){
 const uint16_t ink=DisplayPolicy::foregroundOn(background),fill=monoContrast(s.fillColor,background);
 const int32_t radius=std::min<int32_t>(s.radius,std::min(box.width,box.height)/2);
 const auto paint=[&](int32_t curve){c.fillRoundRect(box.x,box.y,box.width,box.height,curve,checked?fill:background);};

 if(s.cornerRadii)paintCornerQuadrants(c,box,clip,s,paint);
 else paint(radius);

 strokeCornerRect(c,box,clip,s,1,ink);

 if(checked&&box.width>5&&box.height>5){const uint16_t mark=monoContrast(s.thumbColor,fill);
  const int32_t x0=box.x+box.width*25/100,y0=box.y+box.height/2,x1=box.x+box.width*44/100,y1=box.y+box.height*70/100,x2=box.x+box.width*78/100,y2=box.y+box.height*28/100;

  c.drawWideLine(x0,y0,x1,y1,2,mark);c.drawWideLine(x1,y1,x2,y2,2,mark);
 }
}

template<class Canvas> void monoSwitch(Canvas &c,Rect r,const Style &s,int32_t position,uint16_t background){
 const int32_t track=std::min<int32_t>(r.height,s.trackSize),th=std::min<int32_t>(r.height,s.thumbHeight);
 const int32_t tw=std::min<int32_t>(std::min<int32_t>(r.width,s.thumbWidth),std::max<int32_t>(1,r.width/2));
 const int32_t inset=std::min<int32_t>(1,(r.width-tw)/2),travel=std::max<int32_t>(0,r.width-tw-2*inset);
 const uint16_t ink=DisplayPolicy::foregroundOn(background),off=s.trackColor,on=monoContrast(s.fillColor,off),fill=position>=500?on:off;
 const int32_t radius=s.trackRadius==65535?track/2:std::min<int32_t>(s.trackRadius,track/2);

 c.fillRoundRect(r.x,r.y+(r.height-track)/2,r.width,track,radius,fill);
 c.drawRoundRect(r.x,r.y+(r.height-track)/2,r.width,track,radius,ink);

 const int32_t x=r.x+inset+int32_t(int64_t(travel)*position/1000),y=r.y+(r.height-th)/2;

 c.fillRoundRect(x,y,tw,th,std::min<int32_t>(s.thumbRadius,std::min(tw,th)/2),monoContrast(s.thumbColor,fill));
}
}}}
