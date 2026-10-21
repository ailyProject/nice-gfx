// Included inside nice_gfx::htmlui after InstrumentState and instrumentFormat.
// All drawing uses bounded stack temporaries; samples remain in InstrumentState.
inline uint16_t chartBlend(uint16_t fg,uint16_t bg,int alpha) {
  const int r=(((fg>>11)&31)*alpha+((bg>>11)&31)*(100-alpha)+50)/100;
  const int g=(((fg>>5)&63)*alpha+((bg>>5)&63)*(100-alpha)+50)/100;
  const int b=((fg&31)*alpha+(bg&31)*(100-alpha)+50)/100;
  return uint16_t((r<<11)|(g<<5)|b);
}
template<class Canvas> void renderChart(Canvas &c,const NodeDesc &n,const InstrumentState &state,Rect r) {
  static const ChartStyleDesc defaults={1,0,35,0,0,0,2,0,0,3,2,false,true,true,0,nullptr};
  const auto &a=*n.instrument;const auto &o=a.chartStyle?*a.chartStyle:defaults;const auto &s=n.style;
  const int fh=s.font->height*s.fontSize,fw=s.font->width*s.fontSize;
  auto line=[&](int x1,int y1,int x2,int y2,uint16_t color,int width){
    if(width==1){c.drawLine(x1,y1,x2,y2,color);return;}
    const int dx=abs(x2-x1),sx=x1<x2?1:-1,dy=-abs(y2-y1),sy=y1<y2?1:-1;int error=dx+dy;
    for(;;){c.fillCircle(x1,y1,width/2,color);if(x1==x2&&y1==y2)break;const int twice=2*error;if(twice>=dy){error+=dy;x1+=sx;}if(twice<=dx){error+=dx;y1+=sy;}}
  };
  auto label=[&](const char *text,int x,int y,int width,int align){const int pixels=c.textWidth(text);if(pixels>width||y<r.y||y+fh>r.y+r.height)return;c.drawString(text,x+(align==1?(width-pixels)/2:align==2?width-pixels:0),y);};
  auto fraction=[&](int64_t v,int length){return int((std::max<int64_t>(n.minimum,std::min<int64_t>(n.maximum,v))-n.minimum)*length/(int64_t(n.maximum)-n.minimum));};
  const bool legend=o.legend&&(a.seriesCount>1||*a.series[0].name);const int top=r.y+(legend?fh+4:0);
  if(legend)for(int j=0;j<a.seriesCount;j++){const int width=r.width/a.seriesCount,x=r.x+j*width;c.fillRect(x,r.y+fh/2-2,5,5,a.series[j].color);label(a.series[j].name,x+8,r.y,width-8,0);}
  bool hasLabels=o.axes&&a.horizontal;for(int k=0;k<a.labelCount&&o.axes&&!hasLabels;k++)hasLabels=*a.labels[k]!=0;
  char minLabel[96],maxLabel[96];instrumentFormat(minLabel,sizeof(minLabel),n.minimum,a.decimals,o.axisTicks?"":a.unit);instrumentFormat(maxLabel,sizeof(maxLabel),n.maximum,a.decimals,o.axisTicks?"":a.unit);
  const int axisWidth=std::max(fw*(a.decimals?5:3),std::max(int(c.textWidth(minLabel)),int(c.textWidth(maxLabel))))+4;
  const int left=r.x+(o.axes?std::min<int32_t>(r.width/3,axisWidth):0),bottom=r.y+r.height-(hasLabels?fh+4:1),pw=std::max<int32_t>(0,r.x+r.width-left-1),ph=std::max(0,bottom-top);if(!pw||!ph)return;
  int count=0;for(int j=0;j<a.seriesCount;j++)count=std::max(count,int(state.count[j]));
  auto xAt=[&](int k){return left+(count>1?pw*k/(count-1):pw/2);};
  auto area=[&](int x0,int y0,int x1,int y1,uint16_t color){
    const int baseline=bottom-fraction(0,ph),xs[4]={x0,x1,x1,x0},ys[4]={y0,y1,baseline,baseline};
    for(int y=std::min(baseline,std::min(y0,y1));y<std::max(baseline,std::max(y0,y1));y++){
      int hits[4],size=0;for(int edge=0;edge<4;edge++){const int next=(edge+1)%4;if((ys[edge]<=y&&ys[next]>y)||(ys[next]<=y&&ys[edge]>y))hits[size++]=xs[edge]+(y-ys[edge])*(xs[next]-xs[edge])/(ys[next]-ys[edge]);}
      std::sort(hits,hits+size);const int alpha=o.fillMode==2?o.fillOpacity*abs(y-baseline)/std::max(1,ph):o.fillOpacity;
      for(int e=0;e+1<size;e+=2)if(hits[e+1]>hits[e])c.drawFastHLine(hits[e],y,hits[e+1]-hits[e],chartBlend(color,s.background,alpha));
    }
  };
  if(n.kind==Kind::LineChart&&o.fillMode)for(int j=0;j<a.seriesCount;j++)for(int k=1;k<count;k++){const int32_t v=state.value(a,j,k),old=state.value(a,j,k-1);if(v==ChartNone||old==ChartNone)continue;const int y=bottom-fraction(v,ph),oldY=bottom-fraction(old,ph);area(xAt(k-1),oldY,xAt(k),o.lineMode?oldY:y,a.series[j].color);}
  auto grid=[&](int divisions,bool vertical){if(!divisions)return;const int steps=divisions*(o.minorGrid+1);for(int k=0;k<=steps;k++){const uint16_t color=k%(o.minorGrid+1)==0?s.trackColor:chartBlend(s.trackColor,s.background,35);const int v=(vertical?pw:ph)*k/steps;if(vertical)c.drawFastVLine(left+v,top,ph,color);else c.drawFastHLine(left,top+v,pw,color);}};
  grid(a.grid,a.horizontal);grid(o.gridX,!a.horizontal);
  char formatted[96];
  if(o.axes){
    if(a.horizontal){instrumentFormat(formatted,sizeof(formatted),n.minimum,a.decimals,a.unit);label(formatted,left,bottom+4,pw/2,0);instrumentFormat(formatted,sizeof(formatted),n.maximum,a.decimals,a.unit);label(formatted,left+pw/2,bottom+4,pw/2,2);}
    else if(o.axisTicks&&a.grid){for(int k=0;k<=a.grid;k++){const int32_t v=int32_t(int64_t(n.maximum)-(int64_t(n.maximum)-n.minimum)*k/a.grid);const int y=std::max(top,std::min(bottom-fh,top+ph*k/a.grid-fh/2));if(k>0&&k<a.grid&&ph/a.grid<fh+2)continue;instrumentFormat(formatted,sizeof(formatted),v,a.decimals);label(formatted,r.x,y,left-r.x-4,2);}}
    else{instrumentFormat(formatted,sizeof(formatted),n.maximum,a.decimals,a.unit);label(formatted,r.x,top,left-r.x-4,2);instrumentFormat(formatted,sizeof(formatted),n.minimum,a.decimals,a.unit);label(formatted,r.x,bottom-fh,left-r.x-4,2);}
  }
  for(int i=0;i<o.referenceCount;i++){const auto &ref=o.references[i];const int pos=fraction(ref.value,a.horizontal?pw:ph),start=a.horizontal?top:left,end=start+(a.horizontal?ph:pw),dash=ref.dash?ref.dash:std::max(pw,ph)+1;
    for(int p=start;p<end;p+=dash*(ref.dash?2:1)){const int limit=std::min(end,p+dash);if(a.horizontal)line(left+pos,p,left+pos,limit,ref.color,1);else line(p,bottom-pos,limit,bottom-pos,ref.color,1);}
    if(*ref.label)label(ref.label,left+4,a.horizontal?top+2:std::max(top,bottom-pos-fh-2),pw-8,0);
  }
  for(int j=0;j<a.seriesCount;j++){
    int oldX=0,oldY=0,lastX=0,lastY=0;bool previous=false,connected=false,last=false;
    for(int k=0;k<count;k++){
      const int32_t v=state.value(a,j,k);if(v==ChartNone){if(previous&&!connected&&!o.pointMode)c.fillCircle(oldX,oldY,o.pointRadius,a.series[j].color);previous=connected=false;continue;}
      if(n.kind==Kind::LineChart){const int x=xAt(k),y=bottom-fraction(v,ph);if(previous){if(o.lineMode){line(oldX,oldY,x,oldY,a.series[j].color,o.lineWidth);line(x,oldY,x,y,a.series[j].color,o.lineWidth);}else line(oldX,oldY,x,y,a.series[j].color,o.lineWidth);connected=true;}if(o.pointMode==2)c.fillCircle(x,y,o.pointRadius,a.series[j].color);oldX=lastX=x;oldY=lastY=y;previous=last=true;}
      else{
        int64_t base=0,end=v;if(o.barMode){for(int t=0;t<j;t++){const int32_t old=state.value(a,t,k);if(old!=ChartNone&&(old>=0)==(v>=0))base+=old;}end=base+v;}
        const int category=a.horizontal?ph:pw,start=category*k/count,span=category*(k+1)/count-start,gap=std::min<int>(o.barGap,span/3),space=span-2*gap,groups=o.barMode?1:a.seriesCount,group=o.barMode?0:j,offset=start+gap+space*group/groups,bar=space*(group+1)/groups-space*group/groups,zero=fraction(base,a.horizontal?pw:ph),pos=fraction(end,a.horizontal?pw:ph);if(!bar||zero==pos)continue;
        const int x=a.horizontal?left+std::min(zero,pos):left+offset,y=a.horizontal?top+offset:bottom-std::max(zero,pos),w=a.horizontal?abs(pos-zero):std::max(1,bar-1),h=a.horizontal?std::max(1,bar-1):abs(pos-zero);
        if(o.fillMode==2){for(int row=0;row<h;row++)c.drawFastHLine(x,y+row,w,chartBlend(a.series[j].color,s.background,o.fillOpacity*(h-row)/h));}
        else c.fillRoundRect(x,y,w,h,std::min<int>(o.barRadius,std::min(w,h)/2),a.series[j].color);
      }
    }
    if(n.kind==Kind::LineChart&&last&&o.pointMode==1)c.fillCircle(lastX,lastY,o.pointRadius,a.series[j].color);
    if(n.kind==Kind::LineChart&&previous&&!connected&&!o.pointMode)c.fillCircle(oldX,oldY,o.pointRadius,a.series[j].color);
  }
  if(o.axes&&count){int labelWidth=0;for(int k=0;k<a.labelCount;k++)labelWidth=std::max(labelWidth,int(c.textWidth(a.labels[k])));const int stride=std::max(1,(labelWidth*count+pw-1)/pw);
    if(!a.horizontal)for(int k=0;k<std::min(count,int(a.labelCount));k+=stride){const int span=std::min(pw,pw*stride/count),center=n.kind==Kind::LineChart?(count>1?pw*k/(count-1):pw/2):pw*(2*k+1)/(2*count),x=std::max(left,std::min(left+pw-span,left+center-span/2));label(a.labels[k],x,bottom+4,span,1);}
    else for(int k=0;k<std::min(count,int(a.labelCount));k++)label(a.labels[k],r.x,top+k*ph/count,left-r.x-4,2);
  }
}
