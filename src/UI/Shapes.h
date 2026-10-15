#pragma once
#include "Compiled.h"
#include <algorithm>
#include <math.h>

namespace nice_gfx { namespace htmlui {

inline bool validShape(const NodeDesc &node) {
  if(node.kind!=Kind::Shape)return node.shape==nullptr;
  const ShapeDesc *s=node.shape;
  if(!s||uint8_t(s->type)>uint8_t(ShapeType::Polygon)||s->flags>3||s->strokeWidth>64||
     !s->viewWidth||s->viewWidth>4096||!s->viewHeight||s->viewHeight>4096||s->count>64)return false;
  if((s->flags&ShapeStroke)&&!s->strokeWidth)return false;
  if((s->type==ShapeType::Line||s->type==ShapeType::Polyline)&&(s->flags&ShapeFill))return false;
  const bool vertices=s->type==ShapeType::Line||s->type==ShapeType::Polyline||s->type==ShapeType::Polygon;
  if(!vertices)return !s->count&&!s->points;
  if(!s->points||(s->type==ShapeType::Line?s->count!=2:s->count<(s->type==ShapeType::Polygon?3:2)))return false;
  for(uint8_t i=0;i<s->count;++i)if(s->points[i].x>s->viewWidth||s->points[i].y>s->viewHeight)return false;
  return true;
}

namespace shape_raster {
struct Point { float x,y; };
constexpr float PixelEpsilon=1e-4f;
inline Rect intersection(Rect a,Rect b) {
  const int32_t left=std::max(a.x,b.x),top=std::max(a.y,b.y);
  const int32_t right=int32_t(std::min(int64_t(a.x)+a.width,int64_t(b.x)+b.width));
  const int32_t bottom=int32_t(std::min(int64_t(a.y)+a.height,int64_t(b.y)+b.height));
  return {left,top,std::max<int32_t>(0,right-left),std::max<int32_t>(0,bottom-top)};
}
// All scan conversions sample the integer pixel center in original node
// coordinates. Cropping only bounds the loops; it never changes geometry.
template<class Canvas> struct Raster {
  Canvas &canvas;Rect clip;
  void span(int32_t y,float first,float last,uint16_t color) {
    if(first>last||y<clip.y||y>=clip.y+clip.height)return;
    const int32_t left=int32_t(ceilf(std::max(first,float(clip.x))-PixelEpsilon));
    const int32_t right=int32_t(floorf(std::min(last,float(clip.x+clip.width-1))+PixelEpsilon));
    if(left<=right)canvas.fillRect(left,y,right-left+1,1,color);
  }
  bool rows(float top,float bottom,int32_t &first,int32_t &last) const {
    if(top>bottom||bottom<clip.y||top>clip.y+clip.height-1)return false;
    first=int32_t(ceilf(std::max(top,float(clip.y))-PixelEpsilon));
    last=int32_t(floorf(std::min(bottom,float(clip.y+clip.height-1))+PixelEpsilon));
    return first<=last;
  }
  static bool ellipseSpan(float cx,float cy,float rx,float ry,int32_t y,float &left,float &right) {
    if(rx<0||ry<0)return false;
    const float dy=y-cy;
    if(fabsf(dy)>ry)return false;
    const float extent=ry>0?rx*sqrtf(std::max(0.0f,1.0f-dy*dy/(ry*ry))):rx;
    left=cx-extent;right=cx+extent;return true;
  }
  void ellipse(float cx,float cy,float rx,float ry,float stroke,const ShapeDesc &shape) {
    const bool outline=(shape.flags&ShapeStroke)&&stroke>0,fill=(shape.flags&ShapeFill)!=0;
    if(!outline&&!fill)return;
    const float outside=outline?stroke:0;
    int32_t first,last;if(!rows(cy-ry-outside,cy+ry+outside,first,last))return;
    for(int32_t y=first;y<=last;++y) {
      float left,right;
      if(!outline) { if(ellipseSpan(cx,cy,rx,ry,y,left,right))span(y,left,right,shape.fillColor);continue; }
      if(!ellipseSpan(cx,cy,rx+stroke,ry+stroke,y,left,right))continue;
      float innerLeft,innerRight;
      if(!ellipseSpan(cx,cy,rx-stroke,ry-stroke,y,innerLeft,innerRight))span(y,left,right,shape.strokeColor);
      else {
        // The inner boundary belongs to the interior. Integer limits make
        // the two outline spans disjoint without reading background pixels.
        span(y,left,ceilf(innerLeft-PixelEpsilon)-1,shape.strokeColor);
        if(fill)span(y,innerLeft,innerRight,shape.fillColor);
        span(y,floorf(innerRight+PixelEpsilon)+1,right,shape.strokeColor);
      }
    }
  }
  void rectangle(float left,float top,float right,float bottom,float stroke,const ShapeDesc &shape) {
    const bool outline=(shape.flags&ShapeStroke)&&stroke>0,fill=(shape.flags&ShapeFill)!=0;
    if(!outline&&!fill)return;
    const float outside=outline?stroke:0;
    int32_t first,last;if(!rows(top-outside,bottom+outside,first,last))return;
    for(int32_t y=first;y<=last;++y) {
      if(!outline){span(y,left,right,shape.fillColor);continue;}
      if(y<top+stroke||y>bottom-stroke||left+stroke>right-stroke)span(y,left-stroke,right+stroke,shape.strokeColor);
      else {
        span(y,left-stroke,ceilf(left+stroke-PixelEpsilon)-1,shape.strokeColor);
        if(fill)span(y,left+stroke,right-stroke,shape.fillColor);
        span(y,floorf(right-stroke+PixelEpsilon)+1,right+stroke,shape.strokeColor);
      }
    }
  }
  static bool roundedRectangleSpan(float left,float top,float right,float bottom,float radius,int32_t y,float &first,float &last) {
    if(left>right||top>bottom||y<top||y>bottom)return false;
    radius=std::min(radius,std::min((right-left)*0.5f,(bottom-top)*0.5f));
    const float dy=std::max(std::max(top+radius-y,y-(bottom-radius)),0.0f);
    const float inset=dy>0?radius-sqrtf(std::max(0.0f,radius*radius-dy*dy)):0;
    first=left+inset;last=right-inset;return true;
  }
  void roundedRectangle(float left,float top,float right,float bottom,float radius,const ShapeDesc &shape) {
    const bool outline=(shape.flags&ShapeStroke)&&shape.strokeWidth>0,fill=(shape.flags&ShapeFill)!=0;
    if(!outline&&!fill)return;
    const float stroke=outline?shape.strokeWidth:0,innerRadius=std::max(0.0f,radius-stroke);
    int32_t first,last;if(!rows(top,bottom,first,last))return;
    for(int32_t y=first;y<=last;++y) {
      float leftEdge,rightEdge;if(!roundedRectangleSpan(left,top,right,bottom,radius,y,leftEdge,rightEdge))continue;
      if(!outline){span(y,leftEdge,rightEdge,shape.fillColor);continue;}
      float innerLeft,innerRight;
      if(!roundedRectangleSpan(left+stroke,top+stroke,right-stroke,bottom-stroke,innerRadius,y,innerLeft,innerRight))span(y,leftEdge,rightEdge,shape.strokeColor);
      else {
        span(y,leftEdge,ceilf(innerLeft-PixelEpsilon)-1,shape.strokeColor);
        if(fill)span(y,innerLeft,innerRight,shape.fillColor);
        span(y,floorf(innerRight+PixelEpsilon)+1,rightEdge,shape.strokeColor);
      }
    }
  }
  void polygon(const Point *p,int count,uint16_t color) {
    float top=p[0].y,bottom=top;
    for(int i=1;i<count;++i){top=std::min(top,p[i].y);bottom=std::max(bottom,p[i].y);}
    int32_t first,last;if(!rows(top,bottom,first,last))return;
    float crossing[64];
    for(int32_t y=first;y<=last;++y) {
      int used=0;
      for(int i=0,j=count-1;i<count;j=i++) {
        const Point &a=p[j],&b=p[i];
        // Half-open crossings avoid counting a shared vertex twice; flat
        // edges do not toggle parity. Sorting also handles self crossings.
        if((a.y<=y&&b.y>y)||(b.y<=y&&a.y>y))crossing[used++]=a.x+(y-a.y)*(b.x-a.x)/(b.y-a.y);
      }
      std::sort(crossing,crossing+used);
      for(int i=0;i+1<used;i+=2)span(y,crossing[i],ceilf(crossing[i+1]-PixelEpsilon)-1,color);
    }
  }
  void capsule(Point a,Point b,float radius,uint16_t color) {
    if(std::max(a.x,b.x)+radius<clip.x||std::min(a.x,b.x)-radius>clip.x+clip.width-1)return;
    int32_t first,last;if(!rows(std::min(a.y,b.y)-radius,std::max(a.y,b.y)+radius,first,last))return;
    const float dx=b.x-a.x,dy=b.y-a.y,length=sqrtf(dx*dx+dy*dy);
    if(!length) {
      for(int32_t y=first;y<=last;++y){float left,right;if(ellipseSpan(a.x,a.y,radius,radius,y,left,right))span(y,left,right,color);}return;
    }
    const float nx=-dy*radius/length,ny=dx*radius/length;
    const Point corners[4]={{a.x+nx,a.y+ny},{b.x+nx,b.y+ny},{b.x-nx,b.y-ny},{a.x-nx,a.y-ny}};
    for(int32_t y=first;y<=last;++y) {
      float left=0,right=0;bool found=false;
      const auto include=[&](float x){if(!found){left=right=x;found=true;}else{left=std::min(left,x);right=std::max(right,x);}};
      for(int i=0,j=3;i<4;j=i++) {
        const Point &c=corners[j],&d=corners[i];
        if(c.y==d.y){if(y==c.y){include(c.x);include(d.x);}}
        else if(y>=std::min(c.y,d.y)&&y<=std::max(c.y,d.y))include(c.x+(y-c.y)*(d.x-c.x)/(d.y-c.y));
      }
      for(const Point &end:{a,b}){float l,r;if(ellipseSpan(end.x,end.y,radius,radius,y,l,r)){include(l);include(r);}}
      if(found)span(y,left,right,color);
    }
  }
};
} // namespace shape_raster

// Visible rows only, fixed-size vertex/crossing storage, no heap or framebuffer.
// The caller supplies the actual tile/ancestor intersection rather than the
// canvas datum getters, which intentionally retain full-screen coordinates.
template<class Canvas> void renderShape(Canvas &canvas,const ShapeDesc &shape,Rect box,Rect clipped,uint8_t radius=0) {
  clipped=shape_raster::intersection(box,clipped);if(clipped.width<=0||clipped.height<=0)return;
  shape_raster::Raster<Canvas> raster{canvas,clipped};
  const float stroke=(shape.flags&ShapeStroke)?shape.strokeWidth*0.5f:0;
  const float insetX=std::min(stroke,(box.width-1)*0.5f),insetY=std::min(stroke,(box.height-1)*0.5f);
  const float left=box.x+insetX,top=box.y+insetY,width=std::max(0.0f,box.width-1-2*insetX),height=std::max(0.0f,box.height-1-2*insetY);
  if(shape.type==ShapeType::Circle||shape.type==ShapeType::Ellipse) {
    const float rx=(shape.type==ShapeType::Circle?std::min(width,height):width)*0.5f;
    const float ry=shape.type==ShapeType::Circle?rx:height*0.5f;
    raster.ellipse(box.x+(box.width-1)*0.5f,box.y+(box.height-1)*0.5f,rx,ry,stroke,shape);return;
  }
  if(shape.type==ShapeType::Rect){
    const float outerRadius=std::min(float(radius),std::min((box.width-1)*0.5f,(box.height-1)*0.5f));
    // Keep radius-zero raster coverage unchanged. Rounded geometry is defined
    // on the outer box, so its full-width stroke never protrudes past corners.
    if(outerRadius>0)raster.roundedRectangle(box.x,box.y,box.x+box.width-1,box.y+box.height-1,outerRadius,shape);
    else raster.rectangle(left,top,left+width,top+height,stroke,shape);
    return;
  }
  shape_raster::Point points[64];
  for(int i=0;i<shape.count;++i)points[i]={left+float(shape.points[i].x)*width/shape.viewWidth,top+float(shape.points[i].y)*height/shape.viewHeight};
  if(shape.type==ShapeType::Polygon&&(shape.flags&ShapeFill))raster.polygon(points,shape.count,shape.fillColor);
  if(stroke<=0)return;
  for(int i=1;i<shape.count;++i)raster.capsule(points[i-1],points[i],stroke,shape.strokeColor);
  if(shape.type==ShapeType::Polygon)raster.capsule(points[shape.count-1],points[0],stroke,shape.strokeColor);
}

}} // namespace nice_gfx::htmlui
