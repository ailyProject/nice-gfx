#pragma once
#include "Gfx.h"
#include "../Fonts/Text.h"
#include "../Core/Geometry.h"
#include "../Display/Refresh.h"
#if (defined(ARDUINO_ARCH_ESP32) || NICE_GFX_NATIVE_IDF) && defined(CONFIG_IDF_TARGET_ESP32S31)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"
#endif

namespace nice_gfx {

// Non-owning RGB565 tile sink. Logical coordinates always refer to the whole
// display, while memory only contains the current clipped tile. All geometry
// and fonts use the normal NICE_GFX rasterizer; no Sprite or allocation needed.
class TileCanvas : public NICE_GFX {
 public:
  TileCanvas(int16_t width, int16_t height) : NICE_GFX(width, height) {}
  void resize(int16_t w, int16_t h) {
    _init_width = _width = w; _init_height = _height = h; detach();
  }
  Status attach(const DirtyRect &area, uint16_t *pixels, size_t capacity,
                size_t stride, PixelOrder order = PixelOrder::Wire) {
    detach();
    if (!pixels || reinterpret_cast<uintptr_t>(pixels) % alignof(uint16_t) ||
        area.x < 0 || area.y < 0 || area.width <= 0 || area.height <= 0 ||
        int64_t(area.x)+area.width > _width || int64_t(area.y)+area.height > _height ||
        stride < size_t(area.width) || size_t(area.width) > capacity ||
        size_t(area.height-1) > (capacity-size_t(area.width))/stride ||
        capacity > SIZE_MAX/sizeof(uint16_t) ||
        reinterpret_cast<uintptr_t>(pixels)>UINTPTR_MAX-capacity*sizeof(uint16_t)) return Status::InvalidArgument;
    data_ = pixels; stride_ = stride; tile_ = area; order_ = order;
    setViewport(area.x, area.y, area.width, area.height, false);
    return Status::Ok;
  }
  void detach() { data_ = nullptr; stream_ = false; resetViewport(); }
  bool attached() const { return data_ != nullptr; }
  // RGB565 images in either byte order, cropped before row copies. Source
  // storage is borrowed for this call and must not overlap the attached tile.
  Status blit(int32_t x, int32_t y, PixelView image) {
    if (!image.valid() || image.width>INT32_MAX || image.height>INT32_MAX) return Status::InvalidArgument;
    if (!data_ || _vpOoB) return Status::Ok;
    const uintptr_t source=reinterpret_cast<uintptr_t>(image.data), target=reinterpret_cast<uintptr_t>(data_);
    const size_t sourceBytes=((image.height-1)*image.stride+image.width)*2;
    const size_t targetBytes=((size_t(tile_.height)-1)*stride_+size_t(tile_.width))*2;
    if (source>UINTPTR_MAX-sourceBytes || target>UINTPTR_MAX-targetBytes ||
        (source<target+targetBytes && target<source+sourceBytes)) return Status::InvalidArgument;
    ClippedRect r;
    const int32_t left=_vpX>tile_.x?_vpX:tile_.x, top=_vpY>tile_.y?_vpY:tile_.y;
    const int32_t right=_vpW<tile_.x+tile_.width?_vpW:tile_.x+tile_.width;
    const int32_t bottom=_vpH<tile_.y+tile_.height?_vpH:tile_.y+tile_.height;
    if (!clipRect(x,y,int32_t(image.width),int32_t(image.height),_xDatum,_yDatum,left,top,right,bottom,r)) return Status::Ok;
    const uint8_t *src=image.data+2*(size_t(r.sourceY)*image.stride+size_t(r.sourceX));
    uint16_t *dst=data_+size_t(r.y-tile_.y)*stride_+size_t(r.x-tile_.x);
    for(int32_t row=0;row<r.height;++row) {
      if (image.order==order_) copyRow(dst,src,size_t(r.width));
      else {
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        swapCopyRow(dst,src,size_t(r.width));
#else
        for(int32_t col=0;col<r.width;++col) {
        uint16_t c;
        if (image.order==PixelOrder::Wire) c=uint16_t(uint16_t(src[2*col])<<8|src[2*col+1]);
        else memcpy(&c,src+2*col,sizeof(c));
        dst[col]=stored(c);
        }
#endif
      }
      if (row+1<r.height) { src+=image.stride*2; dst+=stride_; }
    }
    return Status::Ok;
  }
 protected:
  bool textRenderClip(int32_t &left,int32_t &top,int32_t &right,int32_t &bottom) const override {
    left=_vpX>tile_.x?_vpX:tile_.x;top=_vpY>tile_.y?_vpY:tile_.y;
    right=_vpW<tile_.x+tile_.width?_vpW:tile_.x+tile_.width;
    bottom=_vpH<tile_.y+tile_.height?_vpH:tile_.y+tile_.height;
    return data_ && !_vpOoB && right>left && bottom>top;
  }
  // Validate this complete glyph ROI once. Native or bounded bilinear coverage
  // touches the actual RGB565 underlay without per-run virtual span calls.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((optimize("O3")))
#endif
  bool textRenderGrayGlyph(const TextGlyph &glyph,uint16_t width,uint16_t height,int64_t left,int64_t top,
      const TextRasterClip &clip,uint16_t foreground,uint16_t background,bool opaque) override {
    if(!data_||_vpOoB||!glyph.data||glyph.encoding!=TextEncoding::Coverage4||
       !glyph.width||!glyph.height||!width||!height||glyph.stride<(uint32_t(glyph.width)+1)/2||
       clip.left>=clip.right||clip.top>=clip.bottom||clip.right>width||clip.bottom>height)return false;
    const bool resized=width!=glyph.width||height!=glyph.height;
    if(resized&&(width>256||height>256||glyph.width>256||glyph.height>256))return false;
    const size_t sourceRow=(uint32_t(glyph.width)+1)/2;
    if(size_t(glyph.height-1)>(SIZE_MAX-sourceRow)/glyph.stride)return false;
    const size_t sourceBytes=size_t(glyph.height-1)*glyph.stride+sourceRow;
    const size_t targetBytes=(size_t(tile_.height-1)*stride_+size_t(tile_.width))*2;
    const uintptr_t input=reinterpret_cast<uintptr_t>(glyph.data),output=reinterpret_cast<uintptr_t>(data_);
    if(input>UINTPTR_MAX-sourceBytes||output>UINTPTR_MAX-targetBytes||
       (input<output+targetBytes&&output<input+sourceBytes))return false;
    const int64_t x=left+clip.left,y=top+clip.top,right=left+clip.right,bottom=top+clip.bottom;
    if(x<tile_.x||y<tile_.y||right>int64_t(tile_.x)+tile_.width||bottom>int64_t(tile_.y)+tile_.height||
       x<_vpX||y<_vpY||right>_vpW||bottom>_vpH)return false;
    uint16_t palette[16];const uint16_t ink=stored(foreground);
    if(opaque)for(uint8_t c=0;c<16;++c)palette[c]=stored(textCoverageColor(foreground,background,c));
    uint16_t *target=data_+size_t(y-tile_.y)*stride_+size_t(x-tile_.x);
    if(resized){
      const int32_t dx=int32_t(width)*2,dy=int32_t(height)*2;
      const uint32_t den=uint32_t(dx)*dy;
      const auto floor32=[](int32_t n,int32_t d){return n>=0?n/d:-((-n+d-1)/d);};
      const int32_t nx=(int32_t(clip.left)*2+1)*glyph.width-width;
      const int32_t ny=(int32_t(clip.top)*2+1)*glyph.height-height;
      const int32_t firstSx=floor32(nx,dx);const uint32_t firstFx=uint32_t(nx-firstSx*dx);
      int32_t sy=floor32(ny,dy);uint32_t fy=uint32_t(ny-sy*dy);
      const int32_t stepX=int32_t(glyph.width)*2,stepY=int32_t(glyph.height)*2;
      const int32_t quotientX=stepX/dx,quotientY=stepY/dy;
      const uint32_t remainderX=uint32_t(stepX%dx),remainderY=uint32_t(stepY%dy);
      const auto sample=[&](int32_t sx,int32_t yy)
#if defined(__GNUC__) || defined(__clang__)
          __attribute__((always_inline))
#endif
          -> uint8_t {
        if(sx<0||yy<0||sx>=glyph.width||yy>=glyph.height)return 0;
        const uint8_t p=uint8_t(pgm_read_byte(glyph.data+size_t(yy)*glyph.stride+uint32_t(sx)/2));
        return uint8_t((sx&1)?p&15:p>>4);
      };
      for(uint16_t yy=clip.top;yy<clip.bottom;++yy){
        int32_t sx=firstSx;uint32_t fx=firstFx;uint16_t *dst=target;
        for(uint16_t xx=clip.left;xx<clip.right;++xx,++dst){
          const uint32_t a=uint32_t(sample(sx,sy))*(dx-fx)+uint32_t(sample(sx+1,sy))*fx;
          const uint32_t b=uint32_t(sample(sx,sy+1))*(dx-fx)+uint32_t(sample(sx+1,sy+1))*fx;
          const uint8_t coverage=uint8_t((a*(dy-fy)+b*fy+den/2)/den);
          if(opaque)*dst=palette[coverage];
          else if(coverage==15)*dst=ink;
          else if(coverage){
#if defined(CONFIG_IDF_TARGET_ESP32S31)
            *dst=paletteBlend(foreground,*dst,coverage);
#else
            *dst=stored(textCoverageColor(foreground,stored(*dst),coverage));
#endif
          }
          sx+=quotientX;fx+=remainderX;if(fx>=uint32_t(dx)){fx-=dx;++sx;}
        }
        target+=stride_;sy+=quotientY;fy+=remainderY;if(fy>=uint32_t(dy)){fy-=dy;++sy;}
      }
      return true;
    }
    for(uint16_t sy=clip.top;sy<clip.bottom;++sy){
      const uint8_t *source=glyph.data+size_t(sy)*glyph.stride;
      uint16_t *dst=target;uint8_t packed=0;
      for(uint16_t sx=clip.left;sx<clip.right;++sx,++dst){
        if(!(sx&1)||sx==clip.left)packed=uint8_t(pgm_read_byte(source+sx/2));
        const uint8_t coverage=uint8_t((sx&1)?packed&15:packed>>4);
        if(opaque)*dst=palette[coverage];
        else if(coverage==15)*dst=ink;
        else if(coverage){
#if defined(CONFIG_IDF_TARGET_ESP32S31)
          *dst=paletteBlend(foreground,*dst,coverage);
#else
          *dst=stored(textCoverageColor(foreground,stored(*dst),coverage));
#endif
        }
      }
      target+=stride_;
    }
    return true;
  }
  // Keep the span/palette hot loop optimized when sketches use -Os. The
  // checked clipping and exact coverage arithmetic remain unchanged.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((optimize("O3")))
#endif
  bool textRenderSpan(int32_t x,int32_t y,int32_t w,int32_t h,uint16_t color,uint8_t coverage) override {
    // renderTextGlyph supplies absolute, visible rectangles with a neutral
    // origin. Retain the borrowed tile's bounds even if a future caller uses
    // this hook outside that renderer; no pointer is formed before validation.
    if (!data_ || _vpOoB || w<=0 || h<=0 || x<tile_.x || y<tile_.y ||
        x>=tile_.x+tile_.width || y>=tile_.y+tile_.height ||
        w>tile_.x+tile_.width-x || h>tile_.y+tile_.height-y) return false;
    if (!coverage) return true;
    uint16_t *first=data_+size_t(y-tile_.y)*stride_+size_t(x-tile_.x);
    if (coverage==15) {
      const uint16_t c=stored(color);
      const size_t bytes=size_t(w)*sizeof(uint16_t);
      if (uint8_t(c)==uint8_t(c>>8)) {
        if (size_t(w)==stride_) { memset(first,uint8_t(c),bytes*size_t(h)); return true; }
        for (int32_t row=0;row<h;++row) memset(first+size_t(row)*stride_,uint8_t(c),bytes);
      } else {
        for (int32_t col=0;col<w;++col) first[col]=c;
        for (int32_t row=1;row<h;++row) copyRow(first+size_t(row)*stride_,reinterpret_cast<const uint8_t *>(first),size_t(w));
      }
    } else {
      for (int32_t row=0;row<h;++row) {
        uint16_t *dst=first+size_t(row)*stride_;
        for (int32_t col=0;col<w;++col) {
#if defined(CONFIG_IDF_TARGET_ESP32S31) // Exact AA palette memo.
          if(coverage<15) {dst[col]=paletteBlend(color,dst[col],coverage);continue;}
#endif
          dst[col]=stored(textCoverageColor(color,stored(dst[col]),coverage));
        }
      }
    }
    return true;
  }
 public:
  void begin_nin_write() override {}
  void end_nin_write() override {}
  void setWindow(int32_t x0,int32_t y0,int32_t x1,int32_t y1) override {
    stream_=data_ && x1>=x0 && y1>=y0;
    streamX_=streamLeft_=x0; streamY_=y0; streamRight_=x1; streamBottom_=y1;
  }
  void pushColor(uint16_t color) override {
    if (!stream_) return;
    // setWindow already receives absolute coordinates from clipWindow.
    if (!_vpOoB && streamX_>=_vpX && streamX_<_vpW && streamY_>=_vpY && streamY_<_vpH &&
        streamX_>=tile_.x && streamX_<tile_.x+tile_.width &&
        streamY_>=tile_.y && streamY_<tile_.y+tile_.height)
      data_[size_t(streamY_-tile_.y)*stride_+size_t(streamX_-tile_.x)]=stored(color);
    if (streamX_==streamRight_) {
      streamX_=streamLeft_;
      if (streamY_==streamBottom_) stream_=false; else ++streamY_;
    } else ++streamX_;
  }

  using NICE_GFX::drawPixel;
  void drawPixel(int32_t x, int32_t y, uint32_t color) override {
    // Hot path for AA. Tile bounds remain authoritative even if the caller
    // resets the viewport inside its renderer.
    const int64_t px = int64_t(x)+_xDatum, py = int64_t(y)+_yDatum;
    if (!data_ || _vpOoB || px<_vpX || py<_vpY || px>=_vpW || py>=_vpH ||
        px<tile_.x || py<tile_.y || px>=tile_.x+tile_.width || py>=tile_.y+tile_.height) return;
    data_[size_t(py-tile_.y)*stride_+size_t(px-tile_.x)] = stored(uint16_t(color));
  }
  uint16_t readPixel(int32_t x, int32_t y) override {
    const int64_t px=int64_t(x)+_xDatum, py=int64_t(y)+_yDatum;
    if (!data_ || _vpOoB || px<_vpX || py<_vpY || px>=_vpW || py>=_vpH ||
        px<tile_.x || py<tile_.y || px>=tile_.x+tile_.width || py>=tile_.y+tile_.height) return 0;
    // Encoding is involutive: the same conversion decodes a stored word.
    return stored(data_[size_t(py-tile_.y)*stride_+size_t(px-tile_.x)]);
  }
  void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) override {
    if (!data_ || _vpOoB) return;
    ClippedRect r;
    const int32_t left=_vpX>tile_.x?_vpX:tile_.x, top=_vpY>tile_.y?_vpY:tile_.y;
    const int32_t right=_vpW<tile_.x+tile_.width?_vpW:tile_.x+tile_.width;
    const int32_t bottom=_vpH<tile_.y+tile_.height?_vpH:tile_.y+tile_.height;
    if (!clipRect(x,y,w,h,_xDatum,_yDatum,left,top,right,bottom,r)) return;
    const uint16_t c=stored(uint16_t(color));
    uint16_t *first=data_+size_t(r.y-tile_.y)*stride_+size_t(r.x-tile_.x);
    const size_t bytes=size_t(r.width)*sizeof(uint16_t);
    if (uint8_t(c)==uint8_t(c>>8)) {
      if (size_t(r.width)==stride_) { memset(first,uint8_t(c),bytes*size_t(r.height)); return; }
      for(int32_t row=0;row<r.height;++row) memset(first+size_t(row)*stride_,uint8_t(c),bytes);
    } else {
      for(int32_t col=0;col<r.width;++col) first[col]=c;
      for(int32_t row=1;row<r.height;++row) copyRow(first+size_t(row)*stride_,reinterpret_cast<const uint8_t *>(first),size_t(r.width));
    }
  }
  // Keep the original scanline raster for every intersecting triangle. Arc
  // segments outside a borrowed tile need no divisions or rejected spans,
  // even when a renderer temporarily enlarges the logical viewport.
  void fillTriangle(int32_t x0,int32_t y0,int32_t x1,int32_t y1,int32_t x2,int32_t y2,uint32_t color) {
    if(!data_ || _vpOoB)return;
    const int32_t top=_vpY>tile_.y?_vpY:tile_.y;
    const int32_t bottom=_vpH<tile_.y+tile_.height?_vpH:tile_.y+tile_.height;
    const int32_t minY=y0<y1?(y0<y2?y0:y2):(y1<y2?y1:y2);
    const int32_t maxY=y0>y1?(y0>y2?y0:y2):(y1>y2?y1:y2);
    if(bottom<=top || int64_t(maxY)+_yDatum<top || int64_t(minY)+_yDatum>=bottom)return;
    const int32_t left=_vpX>tile_.x?_vpX:tile_.x;
    const int32_t right=_vpW<tile_.x+tile_.width?_vpW:tile_.x+tile_.width;
    const int32_t minX=x0<x1?(x0<x2?x0:x2):(x1<x2?x1:x2);
    const int32_t maxX=x0>x1?(x0>x2?x0:x2):(x1>x2?x1:x2);
    if(right<=left || int64_t(maxX)+_xDatum<left || int64_t(minX)+_xDatum>=right)return;
    NICE_GFX::fillTriangle(x0,y0,x1,y1,x2,y2,color);
  }
  // Hide the nonvirtual display implementation: a tile needs only its visible
  // rows, rather than both complete midpoint arcs and hundreds of rejected
  // fillRect calls. The two octants below recover exactly those midpoint spans.
  void fillRoundRect(int32_t x,int32_t y,int32_t w,int32_t h,int32_t radius,uint32_t color) {
    if(!data_ || _vpOoB)return;
    ClippedRect r;
    const int32_t left=_vpX>tile_.x?_vpX:tile_.x,top=_vpY>tile_.y?_vpY:tile_.y;
    const int32_t right=_vpW<tile_.x+tile_.width?_vpW:tile_.x+tile_.width;
    const int32_t bottom=_vpH<tile_.y+tile_.height?_vpH:tile_.y+tile_.height;
    if(!clipRect(x,y,w,h,_xDatum,_yDatum,left,top,right,bottom,r))return;
    if(!radius){fillRect(x,y,w,h,color);return;}
    // Keep the existing display behavior for unsupported corner geometry.
    if(radius<0 || radius>w/2 || radius>h/2){NICE_GFX::fillRoundRect(x,y,w,h,radius,color);return;}
    const uint16_t c=stored(uint16_t(color));
    uint16_t *first=data_+size_t(r.y-tile_.y)*stride_+size_t(r.x-tile_.x);
    for(int32_t row=0;row<r.height;++row) {
      const int32_t sy=r.sourceY+row;
      const uint32_t distance=sy<radius?uint32_t(radius-sy):sy>=h-radius?uint32_t(sy-(h-radius-1)):0;
      const int32_t inset=distance?int32_t(roundInset(uint32_t(radius),distance)):0;
      const int32_t begin=inset>r.sourceX?inset:r.sourceX;
      const int32_t end=w-inset<r.sourceX+r.width?w-inset:r.sourceX+r.width;
      if(end<=begin)continue;
      uint16_t *dst=first+size_t(row)*stride_+size_t(begin-r.sourceX);
      const int32_t count=end-begin;
      if(uint8_t(c)==uint8_t(c>>8))memset(dst,uint8_t(c),size_t(count)*2);
      else for(int32_t col=0;col<count;++col)dst[col]=c;
    }
  }
  // Opaque linear background, using a compiler-generated flash RGB565 palette
  // and Q16 palette coordinates. Geometry must fit signed 32-bit arithmetic
  // over the original rectangle (the compiled UI validates it on mount).
  // No heap, temporary bitmap, trigonometry, division or virtual pixel calls.
  // Rounded coverage exactly matches NICE_GFX::fillRoundRect's midpoint spans.
  void fillLinearGradientRoundRect(int32_t x, int32_t y, int32_t w, int32_t h,
                                  uint8_t radius, const uint16_t *colors,
                                  int32_t origin, int32_t stepX, int32_t stepY,bool dither=false) {
    if (!data_ || _vpOoB || !colors || w<=0 || h<=0) return;
    ClippedRect visible;
    const int32_t left=_vpX>tile_.x?_vpX:tile_.x, top=_vpY>tile_.y?_vpY:tile_.y;
    const int32_t right=_vpW<tile_.x+tile_.width?_vpW:tile_.x+tile_.width;
    const int32_t bottom=_vpH<tile_.y+tile_.height?_vpH:tile_.y+tile_.height;
    if (!clipRect(x,y,w,h,_xDatum,_yDatum,left,top,right,bottom,visible)) return;
    int32_t r=radius;
    if(r>w/2)r=w/2;if(r>h/2)r=h/2;
    if(!r) { gradientRect(x,y,w,h,colors,origin,stepX,stepY,dither,0); return; }
    gradientRect(x,y+r,w,h-2*r,colors,origin+r*stepY,stepX,stepY,dither,uint8_t((r&1)<<1));
    // Most small dirty tiles lie entirely in the full-width central strip.
    // Do not walk either arc when no rounded row intersects this tile.
    if(visible.sourceY>=r&&visible.sourceY+visible.height<=h-r)return;
    const int32_t originalRadius=r;
    int32_t f=1-r,ddX=1,ddY=-2*r,dy=0;
    const auto span=[&](int32_t inset,int32_t row) {
      if(row<visible.sourceY||row>=visible.sourceY+visible.height||
         inset>=visible.sourceX+visible.width||w-inset<=visible.sourceX)return;
      gradientRect(x+inset,y+row,w-2*inset,1,colors,
                   origin+inset*stepX+row*stepY,stepX,stepY,dither,uint8_t((inset&1)|((row&1)<<1)));
    };
    while(dy<r) {
      if(f>=0) {
        span(originalRadius-dy,originalRadius-r);
        span(originalRadius-dy,h-originalRadius-1+r);
        --r;ddY+=2;f+=ddY;
      }
      ++dy;ddX+=2;f+=ddX;
      span(originalRadius-r,originalRadius-dy);
      span(originalRadius-r,h-originalRadius-1+dy);
    }
  }
 private:
  static uint32_t swapCopyPair(uint32_t value) {
    return ((value&0x00ff00ffu)<<8)|((value>>8)&0x00ff00ffu);
  }
  // Cross-order copy streams aligned word stores even when a clipped source
  // begins on the second halfword or an odd byte. No alignment-down read or
  // one-past source fetch is permitted; odd tails use exact two-byte reads.
  static void swapCopyRow(uint16_t *destination,const uint8_t *source,size_t pixels) {
#if defined(__GNUC__)
    typedef uint32_t CopyWord __attribute__((__may_alias__));
    if((reinterpret_cast<uintptr_t>(destination)&2)&&pixels){*destination++=uint16_t(source[0]<<8|source[1]);source+=2;--pixels;}
    auto *out=reinterpret_cast<CopyWord *>(destination);
    if(reinterpret_cast<uintptr_t>(source)&1){
      while(pixels>=2){*out++=uint32_t(source[0])<<8|source[1]|uint32_t(source[2])<<24|uint32_t(source[3])<<16;source+=4;pixels-=2;}
      if(pixels)*reinterpret_cast<uint16_t *>(out)=uint16_t(source[0]<<8|source[1]);return;
    }
    if(!(reinterpret_cast<uintptr_t>(source)&2)){
      const auto *in=reinterpret_cast<const CopyWord *>(source);
      while(pixels>=8){const uint32_t a=in[0],b=in[1],c=in[2],d=in[3];out[0]=swapCopyPair(a);out[1]=swapCopyPair(b);out[2]=swapCopyPair(c);out[3]=swapCopyPair(d);in+=4;out+=4;pixels-=8;}
      while(pixels>=2){*out++=swapCopyPair(*in++);pixels-=2;}
      if(pixels){const auto *last=reinterpret_cast<const uint8_t *>(in);*reinterpret_cast<uint16_t *>(out)=uint16_t(last[0]<<8|last[1]);}return;
    }
    uint16_t carry;__builtin_memcpy(&carry,__builtin_assume_aligned(source,2),2);
    const auto *in=reinterpret_cast<const CopyWord *>(source+2);
    while(pixels>=9){const uint32_t a=in[0],b=in[1],c=in[2],d=in[3];
      out[0]=swapCopyPair(uint32_t(carry)|(a<<16));out[1]=swapCopyPair((a>>16)|(b<<16));
      out[2]=swapCopyPair((b>>16)|(c<<16));out[3]=swapCopyPair((c>>16)|(d<<16));
      carry=uint16_t(d>>16);in+=4;out+=4;pixels-=8;
    }
    while(pixels>=3){const uint32_t value=*in++;*out++=swapCopyPair(uint32_t(carry)|(value<<16));carry=uint16_t(value>>16);pixels-=2;}
    if(pixels){auto *tail=reinterpret_cast<uint16_t *>(out);tail[0]=uint16_t(carry>>8|carry<<8);
      if(pixels==2){const auto *last=reinterpret_cast<const uint8_t *>(in);tail[1]=uint16_t(last[0]<<8|last[1]);}}
#else
    for(size_t i=0;i<pixels;++i)destination[i]=uint16_t(source[2*i]<<8|source[2*i+1]);
#endif
  }
#if (defined(ARDUINO_ARCH_ESP32) || NICE_GFX_NATIVE_IDF) && defined(CONFIG_IDF_TARGET_ESP32S31) && defined(__GNUC__)
  // Core 1 owns S31's PIE coprocessor. These caller-saved vector registers
  // are used only in a normal, yieldable task; IRQ/critical/Core 0 use C++.
  static __attribute__((noinline)) IRAM_ATTR void pieCopy64(uint16_t *&dst,const uint8_t *&src,size_t blocks) {
    asm volatile(
      "1:\n"
      "esp.vld.128.ip q0, %[src], 16\n"
      "esp.vld.128.ip q1, %[src], 16\n"
      "esp.vld.128.ip q2, %[src], 16\n"
      "esp.vld.128.ip q3, %[src], 16\n"
      "esp.vst.128.ip q0, %[dst], 16\n"
      "esp.vst.128.ip q1, %[dst], 16\n"
      "esp.vst.128.ip q2, %[dst], 16\n"
      "esp.vst.128.ip q3, %[dst], 16\n"
      "addi %[blocks], %[blocks], -1\n"
      "bnez %[blocks], 1b\n"
      : [dst] "+&r"(dst), [src] "+&r"(src), [blocks] "+&r"(blocks) : : "memory");
  }
#endif
  static void copyRow(uint16_t *dst,const uint8_t *src,size_t pixels) {
    if(!pixels)return;
#if (defined(ARDUINO_ARCH_ESP32) || NICE_GFX_NATIVE_IDF) && defined(CONFIG_IDF_TARGET_ESP32S31) && defined(__GNUC__)
    if(pixels>=32 && !((reinterpret_cast<uintptr_t>(dst)|reinterpret_cast<uintptr_t>(src))&15) &&
       xPortGetCoreID()==1 && xPortCanYield()) {
      const size_t copied=pixels&~size_t(31);pieCopy64(dst,src,copied/32);pixels-=copied;
      if(!pixels)return;
    }
#endif
#if (defined(ARDUINO_ARCH_ESP32) || NICE_GFX_NATIVE_IDF) && defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    // Page surfaces may be two-byte offset after a slide. ESP's libc memcpy
    // then loses its aligned word path; stream aligned PSRAM words instead.
    // GCC's may_alias type permits RGB565 byte/halfword storage to be copied
    // as words without changing its representation or violating aliasing.
    typedef uint32_t CopyWord __attribute__((__may_alias__));
    if(reinterpret_cast<uintptr_t>(src)&1){memcpy(dst,src,pixels*2);return;}
    if(reinterpret_cast<uintptr_t>(dst)&2) {
      uint16_t first;__builtin_memcpy(&first,__builtin_assume_aligned(src,2),2);
      *dst++=first;src+=2;if(!--pixels)return;
    }
    CopyWord *out=reinterpret_cast<CopyWord *>(dst);
    if(!(reinterpret_cast<uintptr_t>(src)&2)) {
      const CopyWord *in=reinterpret_cast<const CopyWord *>(src);
      while(pixels>=8){out[0]=in[0];out[1]=in[1];out[2]=in[2];out[3]=in[3];out+=4;in+=4;pixels-=8;}
      while(pixels>=2){*out++=*in++;pixels-=2;}
      if(pixels){uint16_t last;__builtin_memcpy(&last,__builtin_assume_aligned(in,2),2);*reinterpret_cast<uint16_t *>(out)=last;}
      return;
    }
    uint16_t carry;__builtin_memcpy(&carry,__builtin_assume_aligned(src,2),2);
    const CopyWord *in=reinterpret_cast<const CopyWord *>(src+2);
    // Each word reads one future pixel, so the nine-pixel bound protects the
    // last row and the three-pixel bound protects its remaining pair loop.
    // Never align the source down or read before the borrowed image.
    while(pixels>=9) {
      const uint32_t a=in[0],b=in[1],c=in[2],d=in[3];
      out[0]=uint32_t(carry)|(a<<16);out[1]=(a>>16)|(b<<16);
      out[2]=(b>>16)|(c<<16);out[3]=(c>>16)|(d<<16);
      carry=uint16_t(d>>16);out+=4;in+=4;pixels-=8;
    }
    while(pixels>=3){const uint32_t word=*in++;*out++=uint32_t(carry)|(word<<16);carry=uint16_t(word>>16);pixels-=2;}
    uint16_t *tail=reinterpret_cast<uint16_t *>(out);tail[0]=carry;
    if(pixels==2){uint16_t last;__builtin_memcpy(&last,__builtin_assume_aligned(in,2),2);tail[1]=last;}
#else
    memcpy(dst,src,pixels*2);
#endif
  }
  template<class UInt> static uint32_t integerRoot(UInt value) {
    UInt root=0,bit=UInt(1)<<(sizeof(UInt)*8-2);
    while(bit>value)bit>>=2;
    while(bit) {
      if(value>=root+bit){value-=root+bit;root=(root>>1)+bit;}
      else root>>=1;
      bit>>=2;
    }
    return uint32_t(root);
  }
  template<class UInt> static uint32_t roundInsetImpl(uint32_t radius,uint32_t distance) {
    const UInt square=UInt(distance)*distance,remainder=UInt(radius)*radius-square;
    uint32_t extent;
    if(square<=remainder) {
      extent=integerRoot(remainder);
      if(remainder-UInt(extent)*extent>extent)++extent; // nearest integer in the side octant
    } else extent=integerRoot(remainder+distance-1); // widest span in the cap octant
    return radius-extent;
  }
  static uint32_t roundInset(uint32_t radius,uint32_t distance) {
    return radius<=65535?roundInsetImpl<uint32_t>(radius,distance):roundInsetImpl<uint64_t>(radius,distance);
  }
  static uint8_t gradientIndex(int32_t coordinate) {
    coordinate+=32768;
    return coordinate<=0?0:coordinate>=(255<<16)?255:uint8_t(coordinate>>16);
  }
  void gradientRect(int32_t x,int32_t y,int32_t w,int32_t h,const uint16_t *colors,
                    int32_t origin,int32_t stepX,int32_t stepY,bool dither,uint8_t phase) {
    ClippedRect r;
    const int32_t left=_vpX>tile_.x?_vpX:tile_.x,top=_vpY>tile_.y?_vpY:tile_.y;
    const int32_t right=_vpW<tile_.x+tile_.width?_vpW:tile_.x+tile_.width;
    const int32_t bottom=_vpH<tile_.y+tile_.height?_vpH:tile_.y+tile_.height;
    if(!clipRect(x,y,w,h,_xDatum,_yDatum,left,top,right,bottom,r))return;
    uint16_t *first=data_+size_t(r.y-tile_.y)*stride_+size_t(r.x-tile_.x);
    int32_t rowCoordinate=origin+r.sourceX*stepX+r.sourceY*stepY;
    phase^=uint8_t((r.sourceX&1)|((r.sourceY&1)<<1));
    // Horizontal gradients evaluate one row (two with ordered dithering),
    // then copy at memory bandwidth. Vertical gradients use one color or one
    // alternating color pair per row. Diagonals use prequantized flash LUTs.
    const int32_t period=dither?2:1,rows=stepY?r.height:(r.height<period?r.height:period);
    for(int32_t row=0;row<rows;++row) {
      uint16_t *dst=first+size_t(row)*stride_;
      if(!stepX) {
        const int index=gradientIndex(rowCoordinate);
        const uint16_t c=stored(pgm_read_word(colors+(dither?index*4+phase:index)));
        if(dither) {
          const uint16_t next=stored(pgm_read_word(colors+index*4+(phase^1)));
          int32_t col=0;
          for(;col+1<r.width;col+=2){dst[col]=c;dst[col+1]=next;}
          if(col<r.width)dst[col]=c;
        } else if(uint8_t(c)==uint8_t(c>>8))memset(dst,uint8_t(c),size_t(r.width)*2);
        else for(int32_t col=0;col<r.width;++col)dst[col]=c;
      } else {
        if(dither) {
          if(order_==PixelOrder::Native)gradientRow<false,true>(dst,r.width,colors,rowCoordinate,stepX,phase);
          else gradientRow<true,true>(dst,r.width,colors,rowCoordinate,stepX,phase);
        } else {
          if(order_==PixelOrder::Native)gradientRow<false,false>(dst,r.width,colors,rowCoordinate,stepX,0);
          else gradientRow<true,false>(dst,r.width,colors,rowCoordinate,stepX,0);
        }
      }
      rowCoordinate+=stepY;
      phase^=2;
    }
    if(!stepY)for(int32_t row=period;row<r.height;++row)
      copyRow(first+size_t(row)*stride_,reinterpret_cast<const uint8_t *>(first+size_t(dither?(row&1):0)*stride_),size_t(r.width));
  }
  template<bool Wire,bool Dither> static void gradientRow(uint16_t *dst,int32_t width,const uint16_t *colors,int32_t coordinate,int32_t step,uint8_t phase) {
    const int32_t rounded=coordinate+32768,last=rounded+(width-1)*step;
    // A linear span's endpoints bound every sample. Hoist clamping and pixel
    // order selection out of the normal loop instead of branching per pixel.
    if(uint32_t(rounded)<(256u<<16)&&uint32_t(last)<(256u<<16)) {
      uint32_t q=uint32_t(rounded);
      for(int32_t col=0;col<width;++col,q+=step) {
        const uint16_t c=pgm_read_word(colors+(Dither?((q>>16)*4+phase):(q>>16)));
        dst[col]=Wire?wireWord(c):c;
        if(Dither)phase^=1;
      }
    } else for(int32_t col=0;col<width;++col,coordinate+=step) {
      const int index=gradientIndex(coordinate);
      const uint16_t c=pgm_read_word(colors+(Dither?index*4+phase:index));
      dst[col]=Wire?wireWord(c):c;
      if(Dither)phase^=1;
    }
  }
  static uint16_t wireWord(uint16_t c) {
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    // ESP32 builds disable builtin memcpy: even a two-byte copy otherwise
    // becomes a libc call for every gradient pixel. Keep the wire bytes exact.
    return uint16_t((c<<8)|(c>>8));
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return c;
#else
    const uint8_t bytes[2]={uint8_t(c>>8),uint8_t(c)};
    uint16_t result;memcpy(&result,bytes,sizeof(result));return result;
#endif
  }
  uint16_t stored(uint16_t c) const {
    if (order_ == PixelOrder::Native) return c;
    // Let the compiler resolve endianness, also on 8-bit targets.
    return wireWord(c);
  }
#if defined(CONFIG_IDF_TARGET_ESP32S31) // Exact AA palette memo.
  // Each pixel still supplies its real stored underlay. Only an identical
  // foreground/background/order tuple can reuse exact RGB565 interpolation.
  uint16_t paletteBlend(uint16_t foreground,uint16_t background,uint8_t coverage) {
    const uint16_t bit=uint16_t(1u<<coverage);
    const bool same=foreground==paletteForeground_&&background==paletteBackground_&&order_==paletteOrder_;
    if(same&&(paletteValid_&bit))return palette_[coverage];
    const uint16_t result=stored(textCoverageColor(foreground,stored(background),coverage));
    if(!same){paletteForeground_=foreground;paletteBackground_=background;paletteOrder_=order_;paletteValid_=0;}
    palette_[coverage]=result;paletteValid_|=bit;return result;
  }
  uint16_t palette_[16]={},paletteForeground_=0,paletteBackground_=0,paletteValid_=0;
  PixelOrder paletteOrder_=PixelOrder::Wire;
#endif
  uint16_t *data_ = nullptr;
  size_t stride_ = 0;
  DirtyRect tile_;
  PixelOrder order_ = PixelOrder::Wire;
  int32_t streamX_=0,streamY_=0,streamLeft_=0,streamRight_=0,streamBottom_=0;
  bool stream_=false;
};

} // namespace nice_gfx
