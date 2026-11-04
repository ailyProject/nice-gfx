#pragma once
#include "../Graphics/TileCanvas.h"
#include "RotationGeometry.h"
#include "detail/Rgb565Blend.h"

namespace nice_gfx { namespace htmlui {

// A rotated sink for the existing node painters. Source spans are inverse
// sampled into the borrowed destination tile, without a container bitmap.
class RotationCanvas : public TileCanvas {
 public:
  using TileCanvas::TileCanvas;
  using NICE_GFX::drawPixel;
  using Mask=bool (*)(void *,double,double);

  Status attach(const DirtyRect &area,uint16_t *pixels,size_t capacity,size_t stride,PixelOrder order=PixelOrder::Wire) {
    active_=false;area_=area;pixels_=pixels;stride_=stride;order_=order;
    return TileCanvas::attach(area,pixels,capacity,stride,order);
  }

  void transform(const RotationMatrix &matrix,Rect source,Mask mask,void *context) {
    matrix_=matrix;inverse_=matrix.inverse();source_=source;mask_=mask;context_=context;active_=true;
    setViewport(source.x,source.y,source.width,source.height,false);
  }

  void clearTransform() { active_=false;TileCanvas::setViewport(area_.x,area_.y,area_.width,area_.height,false); }

  void setViewport(int32_t x,int32_t y,int32_t width,int32_t height,bool datum=true) {
    if(!active_){TileCanvas::setViewport(x,y,width,height,datum);return;}
    _vpX=std::max(x,source_.x);_vpY=std::max(y,source_.y);
    _vpW=std::min(x+width,source_.x+source_.width);_vpH=std::min(y+height,source_.y+source_.height);
    _xDatum=datum?x:0;_yDatum=datum?y:0;_vpOoB=_vpW<=_vpX||_vpH<=_vpY;
    _vpDatum=datum;
  }

  void drawPixel(int32_t x,int32_t y,uint32_t color) override {
    if(!active_){TileCanvas::drawPixel(x,y,color);return;}
    fillRect(x,y,1,1,color);
  }

  uint16_t readPixel(int32_t x,int32_t y) override {
    if(!active_)return TileCanvas::readPixel(x,y);
    const auto p=matrix_.point(double(x)+_xDatum+.5,double(y)+_yDatum+.5);
    const int32_t px=int32_t(floor(p.x)),py=int32_t(floor(p.y));
    if(px<area_.x||py<area_.y||px>=area_.x+area_.width||py>=area_.y+area_.height)return 0;
    const uint16_t value=pixels_[size_t(py-area_.y)*stride_+size_t(px-area_.x)];
    return order_==PixelOrder::Wire?rgb565_blend::fromWire(value):value;
  }

  void fillRect(int32_t x,int32_t y,int32_t width,int32_t height,uint32_t color) override {
    if(!active_){TileCanvas::fillRect(x,y,width,height,color);return;}
    span({x+_xDatum,y+_yDatum,width,height},[&](int32_t,int32_t,uint16_t){return uint16_t(color);});
  }

  void fillTriangle(int32_t x0,int32_t y0,int32_t x1,int32_t y1,int32_t x2,int32_t y2,uint32_t color) {
    if(!active_){TileCanvas::fillTriangle(x0,y0,x1,y1,x2,y2,color);return;}
    NICE_GFX::fillTriangle(x0,y0,x1,y1,x2,y2,color);
  }

  void fillRoundRect(int32_t x,int32_t y,int32_t width,int32_t height,int32_t radius,uint32_t color) {
    if(!active_){TileCanvas::fillRoundRect(x,y,width,height,radius,color);return;}
    NICE_GFX::fillRoundRect(x,y,width,height,radius,color);
  }

  Status blit(int32_t x,int32_t y,PixelView image) {
    if(!active_)return TileCanvas::blit(x,y,image);
    if(!image.valid())return Status::InvalidArgument;
    const int32_t left=x+_xDatum,top=y+_yDatum;
    span({left,top,int32_t(image.width),int32_t(image.height)},[&](int32_t px,int32_t py,uint16_t){
      const uint8_t *p=image.data+2*(size_t(py-top)*image.stride+size_t(px-left));
      uint16_t value;memcpy(&value,p,2);
      return image.order==PixelOrder::Wire?rgb565_blend::fromWire(value):value;
    });
    return Status::Ok;
  }

  void fillLinearGradientRoundRect(int32_t x,int32_t y,int32_t width,int32_t height,uint8_t radius,
      const uint16_t *colors,int32_t origin,int32_t stepX,int32_t stepY,bool dither=false) {
    if(!active_){TileCanvas::fillLinearGradientRoundRect(x,y,width,height,radius,colors,origin,stepX,stepY,dither);return;}
    if(!colors)return;
    const int32_t left=x+_xDatum,top=y+_yDatum,r=std::min<int32_t>(radius,std::min(width,height)/2);
    span({left,top,width,height},[&](int32_t px,int32_t py,uint16_t under){
      const int32_t lx=px-left,ly=py-top;
      if(r){const int32_t dx=lx<r?r-lx:lx>=width-r?lx-(width-r-1):0;
        const int32_t dy=ly<r?r-ly:ly>=height-r?ly-(height-r-1):0;
        if(dx*dx+dy*dy>r*r)return under;}
      const int64_t q=(int64_t(origin)+int64_t(lx)*stepX+int64_t(ly)*stepY+32768)>>16;
      const int32_t index=int32_t(std::max<int64_t>(0,std::min<int64_t>(255,q)));
      return uint16_t(pgm_read_word(colors+(dither?index*4+(lx&1)+((ly&1)<<1):index)));
    });
  }

  void setWindow(int32_t x0,int32_t y0,int32_t x1,int32_t y1) override {
    if(!active_){TileCanvas::setWindow(x0,y0,x1,y1);return;}
    streamX_=streamLeft_=x0;streamY_=y0;streamRight_=x1;streamBottom_=y1;
  }

  void pushColor(uint16_t color) override {
    if(!active_){TileCanvas::pushColor(color);return;}
    if(streamY_>streamBottom_)return;
    fillRect(streamX_-_xDatum,streamY_-_yDatum,1,1,color);
    if(streamX_==streamRight_){streamX_=streamLeft_;++streamY_;}else ++streamX_;
  }

 protected:
  bool textRenderClip(int32_t &left,int32_t &top,int32_t &right,int32_t &bottom) const override {
    if(!active_)return TileCanvas::textRenderClip(left,top,right,bottom);
    left=_vpX;top=_vpY;right=_vpW;bottom=_vpH;return !_vpOoB;
  }

  bool textRenderGrayGlyph(const TextGlyph &glyph,uint16_t width,uint16_t height,int64_t left,int64_t top,
      const TextRasterClip &clip,uint16_t foreground,uint16_t background,bool opaque) override {
    return !active_&&TileCanvas::textRenderGrayGlyph(glyph,width,height,left,top,clip,foreground,background,opaque);
  }

  bool textRenderSpan(int32_t x,int32_t y,int32_t width,int32_t height,uint16_t color,uint8_t coverage) override {
    if(!active_)return TileCanvas::textRenderSpan(x,y,width,height,color,coverage);
    span({x,y,width,height},[&](int32_t,int32_t,uint16_t under){return textCoverageColor(color,under,coverage);});
    return true;
  }

 private:
  static Rect intersection(Rect a,Rect b) {
    const int32_t x=std::max(a.x,b.x),y=std::max(a.y,b.y);
    return {x,y,std::max<int32_t>(0,std::min(a.x+a.width,b.x+b.width)-x),std::max<int32_t>(0,std::min(a.y+a.height,b.y+b.height)-y)};
  }

  template<class Paint> void span(Rect source,Paint paint) {
    source=intersection(source,{_vpX,_vpY,_vpW-_vpX,_vpH-_vpY});
    if(_vpOoB||source.width<=0||source.height<=0)return;
    const Rect target=intersection(matrix_.bounds(source),{area_.x,area_.y,area_.width,area_.height});
    for(int32_t y=target.y;y<target.y+target.height;++y){
      auto p=inverse_.point(double(target.x)+.5,double(y)+.5);
      for(int32_t x=target.x;x<target.x+target.width;++x,p.x+=inverse_.a,p.y+=inverse_.b){
        const int32_t sx=int32_t(floor(p.x)),sy=int32_t(floor(p.y));
        if(sx<source.x||sy<source.y||sx>=source.x+source.width||sy>=source.y+source.height||
            (mask_&&!mask_(context_,double(x)+.5,double(y)+.5)))continue;
        uint16_t &pixel=pixels_[size_t(y-area_.y)*stride_+size_t(x-area_.x)];
        const uint16_t under=order_==PixelOrder::Wire?rgb565_blend::fromWire(pixel):pixel;
        const uint16_t result=paint(sx,sy,under);
        pixel=order_==PixelOrder::Wire?rgb565_blend::toWire(result):result;
      }
    }
  }

  int32_t streamX_=0,streamY_=0,streamLeft_=0,streamRight_=0,streamBottom_=-1;
  bool active_=false;
  RotationMatrix matrix_,inverse_;
  Rect source_{};
  DirtyRect area_;
  uint16_t *pixels_=nullptr;
  size_t stride_=0;
  PixelOrder order_=PixelOrder::Wire;
  Mask mask_=nullptr;
  void *context_=nullptr;
};

} }
