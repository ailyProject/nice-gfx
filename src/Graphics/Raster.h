#pragma once
#include "../Core/Geometry.h"
#include "../Core/Pixel.h"

namespace nice_gfx {
inline bool lineBounds(int32_t first,int32_t end,int32_t origin,int32_t &low,int32_t &high) {
  if (end<=first) return false;
  const int32_t last=end-1;
  if ((origin<0 && first>INT32_MAX+origin) || (origin>0 && last<INT32_MIN+origin)) return false;
  low=origin>0 && first<INT32_MIN+origin?INT32_MIN:first-origin;
  high=origin<0 && last>INT32_MAX+origin?INT32_MAX:last-origin;
  return true;
}
// Emit the visible runs of the original Bresenham line. Starting at the
// viewport edge preserves its error phase; clipping the geometric endpoints
// and restarting Bresenham would change pixels. Work is bounded by the visible
// major axis even for a line spanning the entire signed 32-bit coordinate range.
template<class Sink>
void rasterLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                int32_t originX, int32_t originY, int32_t left, int32_t top,
                int32_t right, int32_t bottom, Sink sink) {
  if (right <= left || bottom <= top) return;
  int32_t lx,hx,ly,hy;
  if (!lineBounds(left,right,originX,lx,hx) || !lineBounds(top,bottom,originY,ly,hy)) return;
  const uint32_t deltaX=x1>=x0?uint32_t(x1)-uint32_t(x0):uint32_t(x0)-uint32_t(x1);
  const uint32_t deltaY=y1>=y0?uint32_t(y1)-uint32_t(y0):uint32_t(y0)-uint32_t(y1);
  const bool steep=deltaY>deltaX;
  if (steep) { int32_t t=x0;x0=y0;y0=t; t=x1;x1=y1;y1=t; }
  if (x0>x1) { int32_t t=x0;x0=x1;x1=t; t=y0;y0=y1;y1=t; }
  const int32_t majorMin=steep?ly:lx,majorMax=steep?hy:hx;
  const int32_t minorMin=steep?lx:ly,minorMax=steep?hx:hy;
  const int32_t first=x0>majorMin?x0:majorMin,last=x1<majorMax?x1:majorMax;
  if (first>last) return;
  const uint32_t dx=steep?deltaY:deltaX,dy=steep?deltaX:deltaY;
  const int step=y0<y1?1:-1;
  uint32_t error=dx/2;
  int32_t minor=y0;
  if (dx && first!=x0) {
    // dx and dy are at most UINT32_MAX, so this product and bias fit uint64_t.
    const uint64_t phase=uint64_t(uint32_t(first)-uint32_t(x0))*dy+(dx-1)/2;
    minor=int32_t(int64_t(y0)+step*int64_t(phase/dx));
    error=dx-1-uint32_t(phase%dx);
  }
  auto emit=[&](int32_t start,int32_t end,int32_t position) {
    if (position<minorMin || position>minorMax) return;
    const int32_t length=int32_t(uint32_t(end)-uint32_t(start)+1);
    if (steep) sink(int32_t(int64_t(position)+originX),int32_t(int64_t(start)+originY),1,length);
    else sink(int32_t(int64_t(start)+originX),int32_t(int64_t(position)+originY),length,1);
  };
  if (!dy) {
    emit(first,last,minor);
    return;
  }
  int32_t runStart=first;
  for (int32_t major=first;;++major) {
    if (major==last) { emit(runStart,major,minor);break; }
    // Unsigned error arithmetic covers UINT32_MAX deltas without signed
    // overflow. The visible inner loop needs no 64-bit operations or divides.
    if (error<dy) {
      emit(runStart,major,minor);
      minor+=step;error+=dx-dy;runStart=major+1;
    } else {
      error-=dy;
    }
  }
}

// Scan only visible pixels. Mask padding and clipped-away source rows never
// become destination pixels. Sink receives contiguous opaque runs.
template<class Visible, class Sink>
Status rasterRuns(const ClippedRect &r, Visible visible, Sink sink) {
  for (int32_t row=0; row<r.height; ++row) {
    int32_t col=0;
    while (col<r.width) {
      while(col<r.width && !visible(r.sourceX+col,r.sourceY+row)) ++col;
      const int32_t first=col;
      while(col<r.width && visible(r.sourceX+col,r.sourceY+row)) ++col;
      if(col!=first) {
        const Status s=sink(r.x+first,r.y+row,r.sourceX+first,r.sourceY+row,col-first);
        if(s!=Status::Ok) return s;
      }
    }
  }
  return Status::Ok;
}

inline uint16_t rgb332(uint8_t color) {
  static const uint8_t blue[]={0,11,21,31};
  return uint16_t(((color&0x1C)>>2 | (color&0xC0)>>3 | (color&0xE0))<<8) |
         uint16_t((color&0x1C)<<3 | blue[color&3]);
}

// Expand a clipped row segment. Packed input is read once per byte instead of
// once per pixel, including the partial byte at either edge.
template<unsigned Bits,class Read>
void decodeIndexedRow(size_t row,size_t x,size_t count,Read read,
                      const uint16_t *palette,uint16_t foreground,uint16_t background,
                      uint16_t *out) {
  if (Bits == 8) {
    while (count--) *out++ = rgb332(read(row + x++));
    return;
  }
  const unsigned perByte = 8 / Bits;
  while (count) {
    const uint8_t value = read(row + x / perByte);
    const unsigned first = unsigned(x % perByte);
    const size_t n = count < perByte - first ? count : perByte - first;
    for (unsigned i = first; i < first + n; ++i) {
      const uint8_t index = uint8_t((value >> (8 - Bits * (i + 1))) & ((1U << Bits) - 1));
      *out++ = Bits == 4 ? palette[index] : index ? foreground : background;
    }
    x += n; count -= n;
  }
}

// Constant stack usage at every display size. Read distinguishes RAM/PROGMEM
// at compile time; Sink performs bulk writes and can terminate on an IO error.
// Stream=true allows opaque batches to span rows in an existing display window.
// The default and all transparent batches preserve row-bounded sink coordinates.
template<unsigned Bits,bool Stream=false,class Read,class Sink>
Status rasterIndexed(int32_t sourceWidth,const ClippedRect &r,Read read,
                     const uint16_t *palette,uint16_t foreground,uint16_t background,
                     int transparent,Sink sink) {
  static_assert(Bits==1 || Bits==4 || Bits==8,"indexed pixel depth");
  const size_t stride=Bits==8 ? size_t(sourceWidth) :
                      Bits==4 ? (size_t(sourceWidth)+1)/2 : (size_t(sourceWidth)+7)/8;
  uint16_t buffer[pixelBufferPixels];
  if (Stream && transparent < 0) {
    size_t used = 0;
    int32_t firstX = 0, firstY = 0;
    for (int32_t row = 0; row < r.height; ++row) {
      const size_t offset = size_t(r.sourceY + row) * stride;
      int32_t col = 0;
      while (col < r.width) {
        if (!used) { firstX = r.x + col; firstY = r.y + row; }
        const size_t room = pixelBufferPixels - used;
        const size_t n = size_t(r.width - col) < room ? size_t(r.width - col) : room;
        decodeIndexedRow<Bits>(offset,size_t(r.sourceX + col),n,read,
                              palette,foreground,background,buffer + used);
        used += n; col += int32_t(n);
        if (used == pixelBufferPixels) {
          const Status s = sink(firstX,firstY,buffer,used);
          if (s != Status::Ok) return s;
          used = 0;
        }
      }
    }
    return used ? sink(firstX,firstY,buffer,used) : Status::Ok;
  }
  for(int32_t row=0;row<r.height;++row) {
    size_t used=0; int32_t first=0;
    const size_t offset=size_t(r.sourceY+row)*stride;
    size_t cachedOffset=SIZE_MAX;
    uint8_t cachedByte=0;
    for(int32_t col=0;col<r.width;++col) {
      const size_t sx=size_t(r.sourceX+col);
      const size_t at=offset+sx/(8/Bits);
      if(Bits==8 || at!=cachedOffset) { cachedByte=read(at); cachedOffset=at; }
      uint8_t index=cachedByte;
      if(Bits==1) index=uint8_t((index>>(7-(sx&7)))&1);
      if(Bits==4) index=uint8_t((index>>(sx&1?0:4))&15);
      if(index!=transparent) {
        if(!used)first=col;
        buffer[used++]=Bits==8?rgb332(index):Bits==4?palette[index]:index?foreground:background;
      }
      if(used && (index==transparent || used==pixelBufferPixels || col==r.width-1)) {
        Status s=sink(r.x+first,r.y+row,buffer,used);
        if(s!=Status::Ok)return s;
        used=0;
      }
    }
  }
  return Status::Ok;
}
}
