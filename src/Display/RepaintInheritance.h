#pragma once
#include "Transport.h"

namespace nice_gfx { namespace detail {
// Exact conservative stale minus guaranteed full repaint. No allocation,
// merging or dropping pixels. Fragment exhaustion rejects the WHOLE plan;
// caller then inherits its original complete stale set. Overlapping stale
// inputs may cause redundant copies, but never a missing preserved pixel.
template<size_t Capacity>
bool subtractRepaint(const DirtyRect *stale,size_t staleCount,
    const DirtyRect *repaint,size_t repaintCount,size_t width,size_t height,
    DirtyRect (&output)[Capacity],size_t &outputCount){
  outputCount=0;
  if(!width||!height||width>INT32_MAX||height>INT32_MAX||
     (staleCount&&!stale)||(repaintCount&&!repaint)||staleCount>Capacity)return false;
  const auto valid=[&](const DirtyRect &r){return r.x>=0&&r.y>=0&&r.width>0&&r.height>0&&
    int64_t(r.x)+r.width<=int64_t(width)&&int64_t(r.y)+r.height<=int64_t(height);};
  for(size_t i=0;i<staleCount;++i)if(!valid(stale[i]))return false;
  for(size_t i=0;i<repaintCount;++i)if(!valid(repaint[i]))return false;
  for(size_t i=0;i<staleCount;++i)output[outputCount++]=stale[i];
  DirtyRect next[Capacity];
  for(size_t c=0;c<repaintCount;++c){size_t count=0;const auto &cut=repaint[c];
    const auto add=[&](int32_t x,int32_t y,int32_t w,int32_t h){if(w<=0||h<=0)return true;if(count==Capacity)return false;next[count++]=DirtyRect(x,y,w,h);return true;};
    for(size_t i=0;i<outputCount;++i){const auto &r=output[i];
      const int32_t l=r.x>cut.x?r.x:cut.x,t=r.y>cut.y?r.y:cut.y;
      const int32_t rr=r.x+r.width<cut.x+cut.width?r.x+r.width:cut.x+cut.width;
      const int32_t b=r.y+r.height<cut.y+cut.height?r.y+r.height:cut.y+cut.height;
      bool ok=true;
      if(l>=rr||t>=b)ok=add(r.x,r.y,r.width,r.height);
      else ok=add(r.x,r.y,r.width,t-r.y)&&add(r.x,b,r.width,r.y+r.height-b)&&
        add(r.x,t,l-r.x,b-t)&&add(rr,t,r.x+r.width-rr,b-t);
      if(!ok){outputCount=0;return false;}
    }
    outputCount=count;for(size_t i=0;i<count;++i)output[i]=next[i];
  }
  return true;
}
}} // namespace nice_gfx::detail
