#pragma once
#include "Compiled.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <limits.h>

namespace nice_gfx { namespace htmlui {
constexpr int32_t ChartNone = INT32_MIN;
inline bool instrumentKind(Kind k) { return k>=Kind::Gauge&&k<=Kind::Spinbox; }
inline bool chartKind(Kind k) { return k==Kind::LineChart||k==Kind::BarChart; }
struct InstrumentState {
  // Render snapshots share samples. Only a writer with a live snapshot copies.
  std::shared_ptr<std::vector<int32_t>> data;
  uint16_t head[4]={},count[4]={};
  int32_t visual=0,from=0;uint32_t started=0;bool active=false,pending=false;
  void init(const NodeDesc &n) {
    visual=from=n.value;active=pending=false;data.reset();
    for(int s=0;s<4;++s)head[s]=count[s]=0;
    if(!chartKind(n.kind)||!n.instrument)return;
    const auto &a=*n.instrument;data=std::make_shared<std::vector<int32_t>>(size_t(a.capacity)*a.seriesCount,ChartNone);
    for(int s=0;s<a.seriesCount;++s){count[s]=a.series[s].count;for(int j=0;j<count[s];++j)(*data)[size_t(s)*a.capacity+j]=a.series[s].values[j];}
  }
  std::vector<int32_t> &writable() { if(!data.unique())data=std::make_shared<std::vector<int32_t>>(*data);return *data; }
  int32_t value(const InstrumentDesc &a,int s,int index) const {return s<0||s>=a.seriesCount||index<0||index>=count[s]?ChartNone:(*data)[size_t(s)*a.capacity+(head[s]+index)%a.capacity];}
  bool set(const InstrumentDesc &a,int s,int index,int32_t v) {
    if(s<0||s>=a.seriesCount||index<0||index>=a.capacity)return false;
    writable()[size_t(s)*a.capacity+(head[s]+index)%a.capacity]=v;count[s]=std::max<int>(count[s],index+1);return true;
  }
  bool append(const InstrumentDesc &a,int s,int32_t v) {
    if(s<0||s>=a.seriesCount)return false;
    if(count[s]<a.capacity)return set(a,s,count[s],v);
    writable()[size_t(s)*a.capacity+head[s]]=v;head[s]=(head[s]+1)%a.capacity;return true;
  }
  void animate(const NodeDesc &n,int32_t value) {
    if((n.kind!=Kind::Gauge&&n.kind!=Kind::Ring)||!n.instrument->duration){visual=value;active=false;return;}
    from=visual;active=from!=value;pending=active;
  }
  bool advance(const NodeDesc &n,int32_t value,uint32_t now) {
    if(!active)return false;if(pending){started=now;pending=false;}
    const uint32_t elapsed=uint32_t(now-started);const int64_t remain=elapsed>=n.instrument->duration?0:1000-int64_t(elapsed)*1000/n.instrument->duration;
    const int32_t next=int32_t(value+(int64_t(from)-value)*remain*remain*remain/1000000000);
    const bool changed=visual!=next;visual=next;if(!remain)active=false;return changed;
  }
};
inline bool validInstrument(const NodeDesc &n) {
  if(!instrumentKind(n.kind))return n.instrument==nullptr;
  const auto *a=n.instrument;if(!a||n.maximum<=n.minimum||n.step<=0||n.minimum==ChartNone||a->decimals>3||!a->unit||strlen(a->unit)>48||a->start < -360||a->start>360||!a->sweep||a->sweep>360||!a->thickness||a->thickness>64||a->ticks>21||a->duration>2000||a->zoneCount>4||(a->zoneCount&&!a->zones))return false;
  int32_t previous=n.minimum;for(int i=0;i<a->zoneCount;++i){if(a->zones[i].to<=previous||a->zones[i].to>n.maximum)return false;previous=a->zones[i].to;}
  if(chartKind(n.kind)) {
    if(a->capacity<2||a->capacity>512||!a->seriesCount||a->seriesCount>4||!a->series||a->labelCount>a->capacity||(a->labelCount&&!a->labels)||a->grid>10)return false;
    for(int s=0;s<a->seriesCount;++s)if(!a->series[s].name||a->series[s].count>a->capacity||(a->series[s].count&&!a->series[s].values))return false;
    for(int j=0;j<a->labelCount;++j)if(!a->labels[j])return false;
    if(a->chartStyle){const auto &c=*a->chartStyle;
      if(c.lineWidth<1||c.lineWidth>6||c.fillMode>2||c.fillOpacity>100||c.gridX>24||c.minorGrid>5||c.pointMode>2||c.pointRadius<1||c.pointRadius>6||c.lineMode>1||c.barMode>1||c.barRadius>12||c.barGap>12||c.referenceCount>4||(c.referenceCount&&!c.references))return false;
      if(c.fillMode&&((n.flags&Transparent)||n.style.gradient))return false;
      for(int i=0;i<c.referenceCount;i++){const auto &ref=c.references[i];if(ref.value<n.minimum||ref.value>n.maximum||ref.dash>16||!ref.label||strlen(ref.label)>72)return false;}
    }
  } else if(a->seriesCount||a->capacity||a->labelCount)return false;
  return true;
}
inline void instrumentFormat(char *out,size_t size,int32_t value,uint8_t decimals,const char *unit="") {
  int32_t scale=1;for(int d=0;d<decimals;++d)scale*=10;
  const int64_t positive=value<0?-int64_t(value):value;
  if(decimals)snprintf(out,size,"%s%ld.%0*ld%s%s",value<0?"-":"",long(positive/scale),int(decimals),long(positive%scale),*unit?" ":"",unit);
  else snprintf(out,size,"%ld%s%s",long(value),*unit?" ":"",unit);
}
// Q15 lookup keeps trigonometry out of per-tile rendering.
inline int32_t instrumentSin(int angle) { static const int16_t table[] = {0,572,1144,1715,2286,2856,3425,3993,4560,5126,5690,6252,6813,7371,7927,8481,9032,9580,10126,10668,11207,11743,12275,12803,13328,13848,14364,14876,15383,15886,16383,16876,17364,17846,18323,18794,19260,19720,20173,20621,21062,21497,21925,22347,22762,23170,23571,23964,24351,24730,25101,25465,25821,26169,26509,26841,27165,27481,27788,28087,28377,28659,28932,29196,29451,29697,29934,30162,30381,30591,30791,30982,31163,31335,31498,31650,31794,31927,32051,32165,32269,32364,32448,32523,32587,32642,32687,32722,32747,32762,32767,32762,32747,32722,32687,32642,32587,32523,32448,32364,32269,32165,32051,31927,31794,31650,31498,31335,31163,30982,30791,30591,30381,30162,29934,29697,29451,29196,28932,28659,28377,28087,27788,27481,27165,26841,26509,26169,25821,25465,25101,24730,24351,23964,23571,23170,22762,22347,21925,21497,21062,20621,20173,19720,19260,18794,18323,17846,17364,16876,16383,15886,15383,14876,14364,13848,13328,12803,12275,11743,11207,10668,10126,9580,9032,8481,7927,7371,6813,6252,5690,5126,4560,3993,3425,2856,2286,1715,1144,572,0,-572,-1144,-1715,-2286,-2856,-3425,-3993,-4560,-5126,-5690,-6252,-6813,-7371,-7927,-8481,-9032,-9580,-10126,-10668,-11207,-11743,-12275,-12803,-13328,-13848,-14364,-14876,-15383,-15886,-16384,-16876,-17364,-17846,-18323,-18794,-19260,-19720,-20173,-20621,-21062,-21497,-21925,-22347,-22762,-23170,-23571,-23964,-24351,-24730,-25101,-25465,-25821,-26169,-26509,-26841,-27165,-27481,-27788,-28087,-28377,-28659,-28932,-29196,-29451,-29697,-29934,-30162,-30381,-30591,-30791,-30982,-31163,-31335,-31498,-31650,-31794,-31927,-32051,-32165,-32269,-32364,-32448,-32523,-32587,-32642,-32687,-32722,-32747,-32762,-32767,-32762,-32747,-32722,-32687,-32642,-32587,-32523,-32448,-32364,-32269,-32165,-32051,-31927,-31794,-31650,-31498,-31335,-31163,-30982,-30791,-30591,-30381,-30162,-29934,-29697,-29451,-29196,-28932,-28659,-28377,-28087,-27788,-27481,-27165,-26841,-26509,-26169,-25821,-25465,-25101,-24730,-24351,-23964,-23571,-23170,-22762,-22347,-21925,-21497,-21062,-20621,-20173,-19720,-19260,-18794,-18323,-17846,-17364,-16876,-16384,-15886,-15383,-14876,-14364,-13848,-13328,-12803,-12275,-11743,-11207,-10668,-10126,-9580,-9032,-8481,-7927,-7371,-6813,-6252,-5690,-5126,-4560,-3993,-3425,-2856,-2286,-1715,-1144,-572}; angle%=360;if(angle<0)angle+=360;return table[angle]; }
// Normal display radii fit a signed 32-bit Q15 product: 65535 * 32767 is
// below INT32_MAX. Keep signed division's exact truncation toward zero, and
// retain the wide formula for negative or unusually large external radii.
inline int32_t instrumentX(int32_t center,int32_t radius,int angle) {const int32_t sine=instrumentSin(angle+90);return center+(uint32_t(radius)<=65535?radius*sine/32767:int32_t(int64_t(radius)*sine/32767));}
inline int32_t instrumentY(int32_t center,int32_t radius,int angle) {const int32_t sine=instrumentSin(angle);return center+(uint32_t(radius)<=65535?radius*sine/32767:int32_t(int64_t(radius)*sine/32767));}
template<class Canvas> void instrumentArc(Canvas &c,int x,int y,int radius,int width,int start,int sweep,uint16_t color,bool caps=true) {
  width=std::max(1,std::min(radius,width));const int inner=std::max(0,radius-width);
  for(int step=0;step<sweep;step+=3){int a=start+step,b=start+std::min(sweep,step+3);
    int x1=instrumentX(x,radius,a),y1=instrumentY(y,radius,a),x2=instrumentX(x,radius,b),y2=instrumentY(y,radius,b);
    int x3=instrumentX(x,inner,a),y3=instrumentY(y,inner,a),x4=instrumentX(x,inner,b),y4=instrumentY(y,inner,b);
    c.fillTriangle(x1,y1,x2,y2,x3,y3,color);c.fillTriangle(x3,y3,x2,y2,x4,y4,color);
  }
  if(caps&&sweep>0&&sweep<360){const int middle=radius-width/2;c.fillCircle(instrumentX(x,middle,start),instrumentY(y,middle,start),width/2,color);c.fillCircle(instrumentX(x,middle,start+sweep),instrumentY(y,middle,start+sweep),width/2,color);}
}
#include "ChartRendering.inc.h"

template<class Canvas> void renderInstrument(Canvas &c,const NodeDesc &n,const InstrumentState &state,int32_t value,Rect r) {
  const auto &a=*n.instrument;const auto &s=n.style;const int fh=s.font->height*s.fontSize;
  c.setAntialiasing(!(n.flags&NoAntialias));
  c.setFont(s.font);c.setTextSize(s.fontSize);c.setTextColor(s.color);c.setTextDatum(TL_DATUM);c.setTextPadding(0);
  auto label=[&](const char *text,int x,int y,int width,int align){const int pixels=c.textWidth(text);
    if(pixels>width||y<r.y||y+fh>r.y+r.height)return;c.drawString(text,x+(align==1?(width-pixels)/2:align==2?width-pixels:0),y);};
  auto fraction=[&](int32_t v,int length){return int32_t((int64_t(std::max(n.minimum,std::min(n.maximum,v)))-n.minimum)*length/(int64_t(n.maximum)-n.minimum));};
  char formatted[96];instrumentFormat(formatted,sizeof(formatted),value,a.decimals,a.unit);
  if(n.kind==Kind::Spinbox) {
    const int button=std::min(r.height,r.width/3),cy=r.y+r.height/2,half=std::max(2,std::min(6,button/5));
    c.drawFastVLine(r.x+button,r.y+8,std::max<int32_t>(0,r.height-16),s.trackColor);c.drawFastVLine(r.x+r.width-button,r.y+8,std::max<int32_t>(0,r.height-16),s.trackColor);
    c.fillRect(r.x+button/2-half,cy-1,half*2,2,value<=n.minimum?s.trackColor:s.fillColor);
    const uint16_t plus=value>=n.maximum?s.trackColor:s.fillColor;c.fillRect(r.x+r.width-button/2-half,cy-1,half*2,2,plus);c.fillRect(r.x+r.width-button/2-1,cy-half,2,half*2,plus);
    label(formatted,r.x+button,r.y+(r.height-fh)/2,r.width-button*2,1);return;
  }
  if(n.kind==Kind::Gauge||n.kind==Kind::Ring) {
    const int cx=r.x+r.width/2,cy=r.y+r.height/2,radius=std::max<int32_t>(1,std::min(r.width,r.height)/2-3),width=std::min(radius,int(a.thickness));
    instrumentArc(c,cx,cy,radius,width,a.start,a.sweep,s.trackColor);
    if(a.zoneCount){int begin=0,limit=n.kind==Kind::Ring?fraction(state.visual,a.sweep):a.sweep;for(int i=0;i<a.zoneCount;++i){int end=std::min<int32_t>(limit,fraction(a.zones[i].to,a.sweep));if(end>begin)instrumentArc(c,cx,cy,radius,width,a.start+begin,end-begin,a.zones[i].color,false);begin=end;}if(n.kind==Kind::Ring&&limit>begin)instrumentArc(c,cx,cy,radius,width,a.start+begin,limit-begin,s.fillColor,false);}
    const int angle=a.start+fraction(state.visual,a.sweep);
    if(!a.zoneCount)instrumentArc(c,cx,cy,radius,width,a.start,angle-a.start,s.fillColor);
    for(int i=0;i<a.ticks;++i){int deg=a.start+(a.ticks>1?int(a.sweep)*i/(a.ticks-1):0);c.drawLine(instrumentX(cx,radius-width-3,deg),instrumentY(cy,radius-width-3,deg),instrumentX(cx,radius-width-7,deg),instrumentY(cy,radius-width-7,deg),s.color);}
    if(n.kind==Kind::Gauge){const int length=std::max(1,radius-width-10);c.fillTriangle(instrumentX(cx,3,angle-90),instrumentY(cy,3,angle-90),instrumentX(cx,3,angle+90),instrumentY(cy,3,angle+90),instrumentX(cx,length,angle),instrumentY(cy,length,angle),s.thumbColor);c.fillCircle(cx,cy,3,s.thumbColor);}
    instrumentFormat(formatted,sizeof(formatted),state.visual,a.decimals,a.unit);label(formatted,r.x,cy+(n.kind==Kind::Gauge?radius/3:-fh/2),r.width,1);
    if(n.kind==Kind::Gauge){instrumentFormat(formatted,sizeof(formatted),n.minimum,a.decimals);label(formatted,r.x,r.y+r.height-fh,r.width/2,0);instrumentFormat(formatted,sizeof(formatted),n.maximum,a.decimals);label(formatted,r.x+r.width/2,r.y+r.height-fh,r.width/2,2);}return;
  }
  renderChart(c,n,state,r);
}
} }
