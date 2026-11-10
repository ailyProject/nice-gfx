template<class Canvas> auto chartProfileSet(Canvas &c,int phase,int)->decltype(c.chartProfilePhase(phase),void()){c.chartProfilePhase(phase);}
template<class Canvas> void chartProfileSet(Canvas &,int,...){ }
template<class Canvas> struct ChartProfileScope {Canvas &canvas;explicit ChartProfileScope(Canvas &c):canvas(c){chartProfileSet(canvas,0,0);}~ChartProfileScope(){chartProfileSet(canvas,-1,0);}};
#ifndef NICE_GFX_UI_CHART_COLUMN_WITH_FILL
#define NICE_GFX_UI_CHART_COLUMN_WITH_FILL 1
#endif
#ifndef NICE_GFX_UI_CHART_ROW_MAJOR_GRID
#define NICE_GFX_UI_CHART_ROW_MAJOR_GRID 1
#endif
#ifndef NICE_GFX_UI_CHART_SKIP_DEAD_WORK
#define NICE_GFX_UI_CHART_SKIP_DEAD_WORK 1
#endif
#ifndef NICE_GFX_UI_CHART_FAST_FRACTION
#define NICE_GFX_UI_CHART_FAST_FRACTION 1
#endif
// Included inside nice_gfx::htmlui after InstrumentState and instrumentFormat.
// All drawing uses bounded stack temporaries; samples remain in InstrumentState.
inline uint16_t chartBlend(uint16_t fg,uint16_t bg,int alpha) {
  const int r=(((fg>>11)&31)*alpha+((bg>>11)&31)*(100-alpha)+50)/100;
  const int g=(((fg>>5)&63)*alpha+((bg>>5)&63)*(100-alpha)+50)/100;
  const int b=((fg&31)*alpha+(bg&31)*(100-alpha)+50)/100;
  return uint16_t((r<<11)|(g<<5)|b);
}
// The pattern index is the series ordinal, shared with the preview. All
// phases advance along the polyline, so retiled renders have identical marks.
inline bool monoChartPattern(int pattern,int position){
 if(pattern==1)return position%7<4;if(pattern==2)return position%3==0;if(pattern==3){const int p=position%9;return p<4||p==6;}return true;
}
inline bool monoChartTexture(int pattern,int x,int y){return pattern==1?(y%4)<2:pattern==2?((x+y)%4)<2:pattern==3?((x/2+y/2)&1)==0:true;}
template<class Canvas> void monoChartBar(Canvas &c,int x,int y,int w,int h,uint16_t ink,uint16_t background,int pattern){
 if(w<=0||h<=0)return;if(!pattern){c.fillRect(x,y,w,h,ink);return;}c.fillRect(x,y,w,h,background);
 const int firstX=std::max(1,int(c.getViewportX())-x),lastX=std::min(w-1,int(c.getViewportX()+c.getViewportWidth())-x),firstY=std::max(1,int(c.getViewportY())-y),lastY=std::min(h-1,int(c.getViewportY()+c.getViewportHeight())-y);
 for(int row=firstY;row<lastY;row++){int start=-1;for(int col=firstX;col<=lastX;col++){const bool on=col<lastX&&monoChartTexture(pattern,col,row);if(on&&start<0)start=col;if(!on&&start>=0){c.drawFastHLine(x+start,y+row,col-start,ink);start=-1;}}}
 c.drawRect(x,y,w,h,ink);
}
template<class Canvas> auto chartMaySkipUnusedLabels(Canvas &c,int)->decltype(c.chartHasExactOwner()){return c.chartHasExactOwner();}
template<class Canvas> bool chartMaySkipUnusedLabels(Canvas &,long){return false;}
template<class Canvas> auto chartGridBatch(Canvas &c,int32_t top,int32_t height,int32_t *positions,uint16_t *colors,size_t count,int)->decltype(c.chartGridVerticalBatch(top,height,positions,colors,count)){return c.chartGridVerticalBatch(top,height,positions,colors,count);}
template<class Canvas> bool chartGridBatch(Canvas &,int32_t,int32_t,int32_t *,uint16_t *,size_t,long){return false;}
template<class Canvas>
auto chartColumnStroke(Canvas &c,int x1,int y1,int x2,int y2,uint16_t color,int) -> decltype(c.chartStrokeRadiusOne(x1,y1,x2,y2,color)){return c.chartStrokeRadiusOne(x1,y1,x2,y2,color);}
template<class Canvas>
bool chartColumnStroke(Canvas &,int,int,int,int,uint16_t,long){return false;}
template<class Canvas>
#if defined(CONFIG_IDF_TARGET_ESP32P4) && defined(__GNUC__) && !defined(__clang__)
// Keep Bresenham span accumulation and sample mapping optimized in -Os builds.
__attribute__((optimize("O3")))
#endif
NICE_GFX_UI_CHART_IRAM_ATTR void renderChart(Canvas &c,const NodeDesc &n,const InstrumentState &state,Rect r,bool monochrome=false
#if NICE_GFX_UI_CHART_FUSED_BACKDROP_GRID
    ,bool gridPrepared=false
#endif
    ) {
  ChartProfileScope<Canvas> chartProfileScope(c);
  static const ChartStyleDesc defaults={1,0,35,0,0,0,2,0,0,3,2,false,true,true,0,nullptr};
  const auto &a=*n.instrument;const auto &o=a.chartStyle?*a.chartStyle:defaults;const auto &s=n.style;
  const int fh=s.font->height*s.fontSize,fw=s.font->width*s.fontSize;const bool patterned=monochrome&&a.seriesCount>1;
  const auto seriesColor=[&](int j){return monochrome?DisplayPolicy::foregroundOn(s.background):a.series[j].color;};
  // Use the attached tile and viewport intersection in drawing coordinates.
  int areaLeft=0,areaTop=0,areaRight=0,areaBottom=0;
  const bool areaVisible=c.rasterClip(areaLeft,areaTop,areaRight,areaBottom);
  auto line=[&](int x1,int y1,int x2,int y2,uint16_t color,int width,int pattern=0,int *phase=nullptr) NICE_GFX_UI_CHART_IRAM_ATTR {
    if(width==1&&!pattern){c.drawLine(x1,y1,x2,y2,color);return;}
    // Off-clip colored circles cannot contribute pixels. Preserve the exact
    // Bresenham phase (one increment per max(dx,dy) step) for chart callers.
    const int radius=width/2;
    if(!pattern&&(!areaVisible||int64_t(std::max(x1,x2))+radius<areaLeft||
       int64_t(std::min(x1,x2))-radius>=areaRight||int64_t(std::max(y1,y2))+radius<areaTop||
       int64_t(std::min(y1,y2))-radius>=areaBottom)){
      if(phase)*phase+=std::max(abs(x2-x1),abs(y2-y1));return;
    }
    if(!monochrome&&(!o.fillMode||NICE_GFX_UI_CHART_COLUMN_WITH_FILL)&&!pattern&&radius==1&&chartColumnStroke(c,x1,y1,x2,y2,color,0)){if(phase)*phase+=std::max(abs(x2-x1),abs(y2-y1));return;}
    const int dx=abs(x2-x1),sx=x1<x2?1:-1,dy=-abs(y2-y1),sy=y1<y2?1:-1;int error=dx+dy,step=phase?*phase:0;
    // Batch the identical union of radius-one circles into scanline spans.
    if(!pattern&&radius==1){
      struct Span {int row=0,left=0,right=0;bool used=false;};Span spans[3];
      const auto flush=[&](Span &s){if(s.used){c.drawFastHLine(s.left,s.row,s.right-s.left+1,color);s.used=false;}};
      const auto add=[&](int row,int left,int right){
        if(row<areaTop||row>=areaBottom)return;
        for(auto &s:spans)if(s.used&&s.row==row){s.left=std::min(s.left,left);s.right=std::max(s.right,right);return;}
        for(auto &s:spans)if(!s.used){s={row,left,right,true};return;}
      };
      for(;;){
        add(y1-1,x1,x1);add(y1,x1-1,x1+1);add(y1+1,x1,x1);
        if(x1==x2&&y1==y2)break;
        ++step;const int twice=2*error,oldY=y1;
        if(twice>=dy){error+=dy;x1+=sx;}if(twice<=dx){error+=dx;y1+=sy;}
        if(y1!=oldY)for(auto &s:spans)if(s.used&&s.row==oldY-sy)flush(s);
      }
      for(auto &s:spans)flush(s);if(phase)*phase=step;return;
    }
    for(;;){if(monoChartPattern(pattern,step/std::max(1,width))){if(width==1)c.drawPixel(x1,y1,color);else c.fillCircle(x1,y1,width/2,color);}if(x1==x2&&y1==y2)break;++step;const int twice=2*error;if(twice>=dy){error+=dy;x1+=sx;}if(twice<=dx){error+=dx;y1+=sy;}}if(phase)*phase=step;
  };
  auto marker=[&](int x,int y,uint16_t color,int pattern){const int radius=o.pointRadius;if(pattern==1)c.drawCircle(x,y,radius,color);else if(pattern==2){c.drawFastHLine(x-radius,y,2*radius+1,color);c.drawFastVLine(x,y-radius,2*radius+1,color);}else if(pattern==3){c.drawLine(x,y-radius,x+radius,y,color);c.drawLine(x+radius,y,x,y+radius,color);c.drawLine(x,y+radius,x-radius,y,color);c.drawLine(x-radius,y,x,y-radius,color);}else c.fillCircle(x,y,radius,color);};
  auto label=[&](const char *text,int x,int y,int width,int align){const int pixels=c.textWidth(text);if(pixels>width||y<r.y||y+fh>r.y+r.height)return;c.drawString(text,x+(align==1?(width-pixels)/2:align==2?width-pixels:0),y);};
  auto fraction=[&](int64_t v,int length){
    const int64_t product=(std::max<int64_t>(n.minimum,std::min<int64_t>(n.maximum,v))-n.minimum)*length,denominator=int64_t(n.maximum)-n.minimum;
#if NICE_GFX_UI_CHART_FAST_FRACTION
    if(product>=0&&uint64_t(product)<=UINT32_MAX&&denominator>0&&uint64_t(denominator)<=UINT32_MAX)return int(uint32_t(product)/uint32_t(denominator));
#endif
    return int(product/denominator);
  };
  const bool legend=o.legend&&(a.seriesCount>1||*a.series[0].name),compactLegend=monochrome&&o.axes&&legend&&r.height<3*fh+8;const int top=r.y+(legend?fh+4:0);
  // A short chart cannot stack a legend, two full glyph rows and a plot.
  // Keep its complete domain and series key on one row; data remains untrimmed.
  char minLabel[96],maxLabel[96],formatted[96];int legendLeft=r.x,legendWidth=r.width;
  if(compactLegend){instrumentFormat(minLabel,sizeof(minLabel),n.minimum,a.decimals);instrumentFormat(maxLabel,sizeof(maxLabel),n.maximum,a.decimals,a.unit);std::snprintf(formatted,sizeof(formatted),"%s-%s",minLabel,maxLabel);const int width=c.textWidth(formatted);label(formatted,r.x,r.y,r.width,0);legendLeft+=width+6;legendWidth=std::max(0,int(r.width)-width-6);}
  if(legend)for(int j=0;j<a.seriesCount;j++){const int width=legendWidth/a.seriesCount,x=legendLeft+j*width,sample=compactLegend?8:10,offset=patterned?(compactLegend?10:14):8;const auto color=seriesColor(j);if(width<=0)continue;if(patterned){if(n.kind==Kind::BarChart)monoChartBar(c,x,r.y+fh/2-3,sample,7,color,s.background,j);else line(x,r.y+fh/2,x+sample-1,r.y+fh/2,color,1,j);}else c.fillRect(x,r.y+fh/2-2,5,5,color);const char number[]={char('1'+j),0};const char *name=patterned&&c.textWidth(a.series[j].name)>width-offset?number:a.series[j].name;label(name,x+offset,r.y,width-offset,0);}
  const bool axes=o.axes&&!compactLegend;bool hasLabels=axes&&a.horizontal;for(int k=0;k<a.labelCount&&axes&&!hasLabels;k++)hasLabels=*a.labels[k]!=0;
  const int axisWidth=[&](){
#if NICE_GFX_UI_CHART_SKIP_DEAD_WORK
    if(!axes&&chartMaySkipUnusedLabels(c,0))return 0;
#endif
    instrumentFormat(minLabel,sizeof(minLabel),n.minimum,a.decimals,o.axisTicks?"":a.unit);instrumentFormat(maxLabel,sizeof(maxLabel),n.maximum,a.decimals,o.axisTicks?"":a.unit);
    return std::max(fw*(a.decimals?5:3),std::max(int(c.textWidth(minLabel)),int(c.textWidth(maxLabel))))+4;
  }();
  const int left=r.x+(axes?std::min<int32_t>(r.width/3,axisWidth):0),bottom=r.y+r.height-(hasLabels?fh+4:1),pw=std::max<int32_t>(0,r.x+r.width-left-1),ph=std::max(0,bottom-top);if(!pw||!ph)return;
  int count=0;for(int j=0;j<a.seriesCount;j++)count=std::max(count,int(state.count[j]));
  auto xAt=[&](int k){return left+(count>1?pw*k/(count-1):pw/2);};
  auto area=[&](int x0,int y0,int x1,int y1,uint16_t color){
    if(!areaVisible||std::max(x0,x1)<=areaLeft||std::min(x0,x1)>=areaRight)return;
    const int baseline=bottom-fraction(0,ph),xs[4]={x0,x1,x1,x0},ys[4]={y0,y1,baseline,baseline};
    for(int y=std::max(areaTop,std::min(baseline,std::min(y0,y1)));y<std::min(areaBottom,std::max(baseline,std::max(y0,y1)));y++){
      int hits[4],size=0;for(int edge=0;edge<4;edge++){const int next=(edge+1)%4;if((ys[edge]<=y&&ys[next]>y)||(ys[next]<=y&&ys[edge]>y))hits[size++]=xs[edge]+(y-ys[edge])*(xs[next]-xs[edge])/(ys[next]-ys[edge]);}
      std::sort(hits,hits+size);const int alpha=o.fillMode==2?o.fillOpacity*abs(y-baseline)/std::max(1,ph):o.fillOpacity;
      for(int e=0;e+1<size;e+=2)if(hits[e+1]>hits[e])c.drawFastHLine(hits[e],y,hits[e+1]-hits[e],chartBlend(color,s.background,alpha));
    }
  };
  chartProfileSet(c,1,0);
  if(n.kind==Kind::LineChart&&o.fillMode){
    // Preserve exact polygon intersections and colors while combining
    // adjacent opaque spans before submitting each raster row.
    // The bounded path uses 384 bytes; other domains retain the original path.
    const bool bounded=areaVisible&&count>=2&&count<=96&&r.x>=-32767&&r.y>=-32767&&
      int64_t(r.x)+r.width<=32767&&int64_t(r.y)+r.height<=32767;
    if(!bounded){for(int j=0;j<a.seriesCount;j++)for(int k=1;k<count;k++){const int32_t v=state.value(a,j,k),old=state.value(a,j,k-1);if(v==ChartNone||old==ChartNone)continue;const int y=bottom-fraction(v,ph),oldY=bottom-fraction(old,ph);area(xAt(k-1),oldY,xAt(k),o.lineMode?oldY:y,a.series[j].color);}}
    else{
      int16_t px[96],py[96];for(int k=0;k<count;++k)px[k]=int16_t(xAt(k));
      const int baseline=bottom-fraction(0,ph),firstRow=std::max(areaTop,top),lastRow=std::min(areaBottom,bottom);
      for(int j=0;j<a.seriesCount;++j){
        for(int k=0;k<count;++k){const int32_t v=state.value(a,j,k);py[k]=v==ChartNone?INT16_MIN:int16_t(bottom-fraction(v,ph));}
        for(int y=firstRow;y<lastRow;++y){
          const int alpha=o.fillMode==2?o.fillOpacity*abs(y-baseline)/std::max(1,ph):o.fillOpacity;
          const uint16_t color=chartBlend(a.series[j].color,s.background,alpha);
          bool pending=false;int spanLeft=0,spanRight=0;
          const auto flush=[&](){if(pending){c.drawFastHLine(spanLeft,y,spanRight-spanLeft,color);pending=false;}};
          for(int k=1;k<count;++k){
            if(py[k]==INT16_MIN||py[k-1]==INT16_MIN)continue;
            const int x0=px[k-1],x1=px[k],y0=py[k-1],y1=o.lineMode?y0:py[k];
            if(x1<=areaLeft||x0>=areaRight||y<std::min(baseline,std::min(y0,y1))||y>=std::max(baseline,std::max(y0,y1)))continue;
            // Resolve each half-open trapezoid with the original integer
            // edge intersection only for partially covered scanlines.
            int left=0,right=0;
            if(y<baseline){
              if(y0<=y&&y1<=y){left=x0;right=x1;}
              else if(y0>y&&y1>y)continue;
              else{const int cut=x0+(y-y0)*(x1-x0)/(y1-y0);if(y0<=y){left=x0;right=cut;}else{left=cut;right=x1;}}
            }else{
              if(y<y0&&y<y1){left=x0;right=x1;}
              else if(y>=y0&&y>=y1)continue;
              else{const int cut=x0+(y-y0)*(x1-x0)/(y1-y0);if(y>=y0){left=cut;right=x1;}else{left=x0;right=cut;}}
            }
            left=std::max(areaLeft,left);right=std::min(areaRight,right);if(right<=left)continue;
            if(pending&&left<=spanRight)spanRight=std::max(spanRight,right);else{flush();spanLeft=left;spanRight=right;pending=true;}
          }
          flush();
        }
      }
    }
  }
  chartProfileSet(c,2,0);
  auto grid=[&](int divisions,bool vertical){if(!divisions)return;const int steps=divisions*(o.minorGrid+1);
#if NICE_GFX_UI_CHART_ROW_MAJOR_GRID
    if(vertical&&steps>0&&steps<96&&left>=-32760&&left<=32760&&top>=-32760&&top<=32760&&pw<=32760&&ph>0&&ph<4096){
      int32_t positions[96];uint16_t colors[96];
      for(int k=0;k<=steps;++k){positions[k]=left+pw*k/steps;colors[k]=k%(o.minorGrid+1)==0?s.trackColor:chartBlend(s.trackColor,s.background,35);}
      if(chartGridBatch(c,top,ph,positions,colors,size_t(steps+1),0))return;
    }
#endif
    for(int k=0;k<=steps;k++){const uint16_t color=k%(o.minorGrid+1)==0?s.trackColor:chartBlend(s.trackColor,s.background,35);const int v=(vertical?pw:ph)*k/steps;if(vertical)c.drawFastVLine(left+v,top,ph,color);else c.drawFastHLine(left,top+v,pw,color);}};
#if NICE_GFX_UI_CHART_FUSED_BACKDROP_GRID
  if(!gridPrepared)
#endif
  {grid(a.grid,a.horizontal);grid(o.gridX,!a.horizontal);}
  if(axes){
    if(a.horizontal){instrumentFormat(formatted,sizeof(formatted),n.minimum,a.decimals,a.unit);label(formatted,left,bottom+4,pw/2,0);instrumentFormat(formatted,sizeof(formatted),n.maximum,a.decimals,a.unit);label(formatted,left+pw/2,bottom+4,pw/2,2);}
    else if(o.axisTicks&&a.grid){for(int k=0;k<=a.grid;k++){const int32_t v=int32_t(int64_t(n.maximum)-(int64_t(n.maximum)-n.minimum)*k/a.grid);const int y=std::max(top,std::min(bottom-fh,top+ph*k/a.grid-fh/2));if(k>0&&k<a.grid&&ph/a.grid<fh+2)continue;instrumentFormat(formatted,sizeof(formatted),v,a.decimals);label(formatted,r.x,y,left-r.x-4,2);}}
    else{instrumentFormat(formatted,sizeof(formatted),n.maximum,a.decimals,a.unit);label(formatted,r.x,top,left-r.x-4,2);instrumentFormat(formatted,sizeof(formatted),n.minimum,a.decimals,a.unit);label(formatted,r.x,bottom-fh,left-r.x-4,2);}
  }
  for(int i=0;i<o.referenceCount;i++){const auto &ref=o.references[i];const int pos=fraction(ref.value,a.horizontal?pw:ph),start=a.horizontal?top:left,end=start+(a.horizontal?ph:pw),dash=ref.dash?ref.dash:std::max(pw,ph)+1;
    for(int p=start;p<end;p+=dash*(ref.dash?2:1)){const int limit=std::min(end,p+dash);if(a.horizontal)line(left+pos,p,left+pos,limit,ref.color,1);else line(p,bottom-pos,limit,bottom-pos,ref.color,1);}
    if(*ref.label)label(ref.label,left+4,a.horizontal?top+2:std::max(top,bottom-pos-fh-2),pw-8,0);
  }
  chartProfileSet(c,3,0);
  for(int j=0;j<a.seriesCount;j++){
    const auto color=seriesColor(j);const int pattern=patterned?j:0;int phase=0;
    int oldX=0,oldY=0,lastX=0,lastY=0;bool previous=false,connected=false,last=false;
    for(int k=0;k<count;k++){
      const int32_t v=state.value(a,j,k);if(v==ChartNone){if(previous&&!connected&&!o.pointMode)marker(oldX,oldY,color,pattern);previous=connected=false;phase=0;continue;}
      if(n.kind==Kind::LineChart){const int x=xAt(k),y=bottom-fraction(v,ph);if(previous){if(o.lineMode){line(oldX,oldY,x,oldY,color,o.lineWidth,pattern,&phase);line(x,oldY,x,y,color,o.lineWidth,pattern,&phase);}else line(oldX,oldY,x,y,color,o.lineWidth,pattern,&phase);connected=true;}if(o.pointMode==2)marker(x,y,color,pattern);oldX=lastX=x;oldY=lastY=y;previous=last=true;}
      else{
        int64_t base=0,end=v;if(o.barMode){for(int t=0;t<j;t++){const int32_t old=state.value(a,t,k);if(old!=ChartNone&&(old>=0)==(v>=0))base+=old;}end=base+v;}
        const int category=a.horizontal?ph:pw,start=category*k/count,span=category*(k+1)/count-start,gap=std::min<int>(o.barGap,span/3),space=span-2*gap,groups=o.barMode?1:a.seriesCount,group=o.barMode?0:j,offset=start+gap+space*group/groups,bar=space*(group+1)/groups-space*group/groups,zero=fraction(base,a.horizontal?pw:ph),pos=fraction(end,a.horizontal?pw:ph);if(!bar||zero==pos)continue;
        const int x=a.horizontal?left+std::min(zero,pos):left+offset,y=a.horizontal?top+offset:bottom-std::max(zero,pos),w=a.horizontal?abs(pos-zero):std::max(1,bar-1),h=a.horizontal?std::max(1,bar-1):abs(pos-zero);
        if(patterned)monoChartBar(c,x,y,w,h,color,s.background,pattern);
        else if(o.fillMode==2){if(areaVisible&&int64_t(x)+w>areaLeft&&x<areaRight)for(int row=std::max(0,areaTop-y);row<std::min(h,areaBottom-y);row++)c.drawFastHLine(x,y+row,w,chartBlend(color,s.background,o.fillOpacity*(h-row)/h));}
        else c.fillRoundRect(x,y,w,h,std::min<int>(o.barRadius,std::min(w,h)/2),color);
      }
    }
    if(n.kind==Kind::LineChart&&last&&o.pointMode==1)marker(lastX,lastY,color,pattern);
    if(n.kind==Kind::LineChart&&previous&&!connected&&!o.pointMode)marker(oldX,oldY,color,pattern);
  }
  chartProfileSet(c,4,0);
  if(axes&&count){int labelWidth=0;for(int k=0;k<a.labelCount;k++)labelWidth=std::max(labelWidth,int(c.textWidth(a.labels[k])));const int stride=std::max(1,(labelWidth*count+pw-1)/pw);
    if(!a.horizontal)for(int k=0;k<std::min(count,int(a.labelCount));k+=stride){const int span=std::min(pw,pw*stride/count),center=n.kind==Kind::LineChart?(count>1?pw*k/(count-1):pw/2):pw*(2*k+1)/(2*count),x=std::max(left,std::min(left+pw-span,left+center-span/2));label(a.labels[k],x,bottom+4,span,1);}
    else for(int k=0;k<std::min(count,int(a.labelCount));k++)label(a.labels[k],r.x,top+k*ph/count,left-r.x-4,2);
  }
}
