#ifndef GFX_RUNTIME_TEXT_H
#define GFX_RUNTIME_TEXT_H

#include <stdint.h>
#define NICE_GFX_TEXT_RASTER_ROI_VERSION 1

namespace nice_gfx {

// Font storage is independent of the target: decoders emit row-major runs and
// the target chooses a display stream or clipped memory spans.
enum class TextEncoding : uint8_t { Rows = 2, Coverage4 = 4 };
struct TextGlyph {
  const uint8_t *data;
  uint16_t width, height, stride;
  TextEncoding encoding;
};
// Half-open bounds in the final raster. Sampling neighbors remain relative
// to the complete glyph, so clipping cannot change EPX or bilinear edges.
struct TextRasterClip { uint16_t left, top, right, bottom; };

// MonoFont glyphs are row-major 1bpp data with byte-aligned rows.
template <typename Read, typename Emit>
void textRuns(const TextGlyph &glyph, Read read, Emit emit,
              const TextRasterClip *clip = nullptr) {
  const uint32_t count = uint32_t(glyph.width) * glyph.height;
  if (!count || !glyph.data || glyph.encoding != TextEncoding::Rows) return;
  uint32_t cachedOffset = UINT32_MAX;
  uint8_t cachedByte = 0;
  auto pixel = [&](uint16_t x, uint16_t y) {
    if (x >= uint32_t(glyph.stride) * 8) return false;
    const uint32_t offset = uint32_t(y) * glyph.stride + x / 8;
    if (offset != cachedOffset) {
      cachedByte = read(glyph.data + offset);
      cachedOffset = offset;
    }
    return (cachedByte & (0x80U >> (x & 7))) != 0;
  };
  if (clip) {
    const uint16_t x0 = clip->left < glyph.width ? clip->left : glyph.width;
    const uint16_t y0 = clip->top < glyph.height ? clip->top : glyph.height;
    const uint16_t x1 = clip->right < glyph.width ? clip->right : glyph.width;
    const uint16_t y1 = clip->bottom < glyph.height ? clip->bottom : glyph.height;
    if (x0 >= x1 || y0 >= y1) return;
    for (uint16_t y = y0; y < y1; ++y) {
      uint32_t start = uint32_t(y) * glyph.width + x0;
      bool foreground = pixel(x0, y);
      for (uint16_t x = uint16_t(x0 + 1); x < x1; ++x) {
        const bool next = pixel(x, y);
        if (next != foreground) {
          const uint32_t position = uint32_t(y) * glyph.width + x;
          emit(start, position - start, foreground);
          start = position;
          foreground = next;
        }
      }
      emit(start, uint32_t(y) * glyph.width + x1 - start, foreground);
    }
    return;
  }
  uint32_t start = 0, position = 0;
  bool foreground = pixel(0, 0);
  for (uint16_t y = 0; y < glyph.height; ++y) {
    for (uint16_t x = 0; x < glyph.width; ++x, ++position) {
      if (!position) continue;
      const bool next = pixel(x, y);
      if (next != foreground) {
        emit(start, position - start, foreground);
        start = position;
        foreground = next;
      }
    }
  }
  emit(start, count - start, foreground);
}

// Coverage runs also support one-level fallback resampling into a square em.
// Rows stay byte-aligned even when an odd glyph width leaves a spare nibble.
// The ordinary 1bpp decoder above keeps its established run boundaries.
template <typename Read, typename Emit>
void textCoverageRuns(const TextGlyph &glyph, uint16_t width, uint16_t height,
                      Read read, Emit emit, const TextRasterClip *clip = nullptr,
                      bool threshold = false) {
  const uint32_t count = uint32_t(width) * height;
  if (!count || !glyph.data || !glyph.width || !glyph.height ||
      (glyph.encoding != TextEncoding::Rows && glyph.encoding != TextEncoding::Coverage4)) return;
  const bool gray = glyph.encoding == TextEncoding::Coverage4;
  const bool resample = width != glyph.width || height != glyph.height;
  uint32_t cachedOffset = UINT32_MAX;
  uint8_t cachedByte = 0;
  auto pixel = [&](uint16_t x, uint16_t y) -> uint8_t {
    // Center-sampled nearest neighbor, without doubling a uint16-sized product.
    const uint32_t sx = resample ? (uint32_t(x) * glyph.width + glyph.width / 2) / width : x;
    const uint32_t sy = resample ? (uint32_t(y) * glyph.height + glyph.height / 2) / height : y;
    if (sx >= uint32_t(glyph.stride) * (gray ? 2 : 8)) return 0;
    const uint32_t offset = sy * glyph.stride + (gray ? sx / 2 : sx / 8);
    if (offset != cachedOffset) {
      cachedByte = read(glyph.data + offset);
      cachedOffset = offset;
    }
    const uint8_t coverage = gray ? uint8_t((cachedByte >> ((sx & 1) ? 0 : 4)) & 15) :
                  uint8_t((cachedByte & (0x80U >> (sx & 7))) ? 15 : 0);
    return threshold ? uint8_t(coverage >= 8 ? 15 : 0) : coverage;
  };
  if (clip) {
    const uint16_t x0 = clip->left < width ? clip->left : width;
    const uint16_t y0 = clip->top < height ? clip->top : height;
    const uint16_t x1 = clip->right < width ? clip->right : width;
    const uint16_t y1 = clip->bottom < height ? clip->bottom : height;
    if (x0 >= x1 || y0 >= y1) return;
    for (uint16_t y = y0; y < y1; ++y) {
      uint32_t start = uint32_t(y) * width + x0;
      uint8_t coverage = pixel(x0, y);
      for (uint16_t x = uint16_t(x0 + 1); x < x1; ++x) {
        const uint8_t next = pixel(x, y);
        if (next != coverage) {
          const uint32_t position = uint32_t(y) * width + x;
          emit(start, position - start, coverage);
          start = position;
          coverage = next;
        }
      }
      emit(start, uint32_t(y) * width + x1 - start, coverage);
    }
    return;
  }
  uint32_t start = 0, position = 0;
  uint8_t coverage = pixel(0, 0);
  for (uint16_t y = 0; y < height; ++y) {
    for (uint16_t x = 0; x < width; ++x, ++position) {
      if (!position) continue;
      const uint8_t next = pixel(x, y);
      if (next != coverage) {
        emit(start, position - start, coverage);
        start = position;
        coverage = next;
      }
    }
  }
  emit(start, count - start, coverage);
}

namespace detail {
// Native binary EPX uses a bounded three-row stencil. Rows are fetched once
// and rotated; the visible horizontal interval includes a one-pixel halo.
// Caller validates Rows, width <= 128 and a complete byte-aligned row stride.
template <typename Read, typename Emit>
void textBinaryStencilRuns(const TextGlyph &glyph, Read read, Emit emit,
                           const TextRasterClip *clip) {
  const uint16_t x0=clip?(clip->left<glyph.width?clip->left:glyph.width):0;
  const uint16_t y0=clip?(clip->top<glyph.height?clip->top:glyph.height):0;
  const uint16_t x1=clip&&clip->right<glyph.width?clip->right:glyph.width;
  const uint16_t y1=clip&&clip->bottom<glyph.height?clip->bottom:glyph.height;
  if(x0>=x1||y0>=y1)return;
  const uint16_t firstByte=(x0?x0-1:0)/8;
  const uint16_t lastByte=(x1<glyph.width?x1:glyph.width-1)/8;
  uint8_t storage[3][16]={};
  uint8_t *up=storage[0],*center=storage[1],*down=storage[2];
  const auto load=[&](uint8_t *row,int32_t y){
    for(uint16_t byte=firstByte;byte<=lastByte;++byte)
      row[byte]=y>=0&&y<glyph.height?read(glyph.data+uint32_t(y)*glyph.stride+byte):0;
  };
  load(up,int32_t(y0)-1);load(center,y0);load(down,int32_t(y0)+1);
  const auto sampleBit=[&](const uint8_t *row,int32_t x){
    return x>=0&&x<glyph.width&&(row[x/8]&(0x80U>>(x&7)));
  };
  uint32_t start=uint32_t(y0)*glyph.width+x0;
  uint8_t previous=0;bool first=true;
  for(uint16_t y=y0;y<y1;++y){
    if(clip){start=uint32_t(y)*glyph.width+x0;first=true;}
    for(uint16_t x=x0;x<x1;++x){
      const bool c=sampleBit(center,x),u=sampleBit(up,x),d=sampleBit(down,x);
      uint8_t coverage=c?15:0;
      if(u!=d){
        const bool l=sampleBit(center,int32_t(x)-1),r=sampleBit(center,int32_t(x)+1);
        if(l!=r){
          const uint8_t sum=uint8_t((l==u?l:c)+(u==r?r:c)+(l==d?l:c)+(d==r?r:c));
          coverage=uint8_t((sum*15+2)/4);
        }
      }
      const uint32_t position=uint32_t(y)*glyph.width+x;
      if(first){previous=coverage;first=false;}
      else if(previous!=coverage){emit(start,position-start,previous);start=position;previous=coverage;}
    }
    if(clip)emit(start,uint32_t(y)*glyph.width+x1-start,previous);
    if(y+1<y1){uint8_t *old=up;up=center;center=down;down=old;load(down,int32_t(y)+2);}
  }
  if(!clip)emit(start,uint32_t(glyph.width)*glyph.height-start,previous);
}

// Integer enlargement needs EPX coverage only at each source block's four
// corners. Cache its three native rows and emit the constant block interiors
// as runs, without repeating source reads or output-pixel division. The caller
// validates complete Rows, width <= 128 and integer scales >= 2 on both axes.
template <typename Read, typename Emit>
void textBinaryEnlargedRuns(const TextGlyph &glyph, uint16_t width, uint16_t height,
                            Read read, Emit emit, const TextRasterClip *clip) {
  const uint16_t x0=clip?(clip->left<width?clip->left:width):0;
  const uint16_t y0=clip?(clip->top<height?clip->top:height):0;
  const uint16_t x1=clip&&clip->right<width?clip->right:width;
  const uint16_t y1=clip&&clip->bottom<height?clip->bottom:height;
  if(x0>=x1||y0>=y1)return;
  const uint16_t scaleX=width/glyph.width,scaleY=height/glyph.height;
  const uint16_t sourceX0=x0/scaleX,sourceX1=(x1-1)/scaleX;
  uint16_t sourceY=y0/scaleY,ry=y0%scaleY;
  // A clipped block needs its vertical halo only when its first/last output
  // corner is visible. Interior rows never consult an outside source row.
  const int32_t firstRow=int32_t(sourceY)-(ry==0?1:0);
  const int32_t lastRow=int32_t((y1-1)/scaleY)+((y1-1)%scaleY==scaleY-1?1:0);
  const uint16_t firstByte=(sourceX0?sourceX0-1:0)/8;
  const uint16_t lastByte=(sourceX1+1<glyph.width?sourceX1+1:glyph.width-1)/8;
  uint8_t storage[3][16]={};
  uint8_t *up=storage[0],*center=storage[1],*down=storage[2];
  const auto load=[&](uint8_t *row,int32_t sy){
    const bool readable=sy>=0&&sy<glyph.height&&sy>=firstRow&&sy<=lastRow;
    for(uint16_t byte=firstByte;byte<=lastByte;++byte)
      row[byte]=readable?read(glyph.data+uint32_t(sy)*glyph.stride+byte):0;
  };
  load(up,int32_t(sourceY)-1);load(center,sourceY);load(down,int32_t(sourceY)+1);
  const auto sampleBit=[&](const uint8_t *row,int32_t sx){
    return sx>=0&&sx<glyph.width&&(row[sx/8]&(0x80U>>(sx&7)));
  };
  uint32_t start=uint32_t(y0)*width+x0;uint8_t previous=0;bool first=true;
  const auto append=[&](uint32_t position,uint8_t value){
    if(first){previous=value;first=false;}
    else if(previous!=value){emit(start,position-start,previous);start=position;previous=value;}
  };
  for(uint16_t y=y0;y<y1;++y){
    const uint32_t base=uint32_t(y)*width;
    if(clip){start=base+x0;first=true;}
    const uint8_t *vertical=ry==0?up:(ry==scaleY-1?down:nullptr);
    uint32_t cellX=uint32_t(sourceX0)*scaleX;
    for(uint16_t sx=sourceX0;sx<=sourceX1;++sx,cellX+=scaleX){
      const bool c=sampleBit(center,sx);
      const uint8_t value=c?15:0;
      const uint32_t cellEnd=cellX+scaleX;
      const uint32_t begin=cellX<x0?x0:cellX,end=cellEnd>x1?x1:cellEnd;
      if(!vertical||sampleBit(vertical,sx)==c){append(base+begin,value);continue;}
      if(begin==cellX)
        append(base+begin,sampleBit(center,int32_t(sx)-1)==c?value:uint8_t(c?11:4));
      const uint32_t middleBegin=begin>cellX?begin:cellX+1;
      const uint32_t middleEnd=end<cellEnd?end:cellEnd-1;
      if(middleBegin<middleEnd)append(base+middleBegin,value);
      if(end==cellEnd)
        append(base+cellEnd-1,sampleBit(center,int32_t(sx)+1)==c?value:uint8_t(c?11:4));
    }
    if(clip)emit(start,base+x1-start,previous);
    if(++ry==scaleY&&y+1<y1){
      ry=0;++sourceY;uint8_t *old=up;up=center;center=down;down=old;
      load(down,int32_t(sourceY)+1);
    }
  }
  if(!clip)emit(start,uint32_t(width)*height-start,previous);
}
}

// Reconstruct a glyph at its final output resolution. Binary masks use EPX
// corner coverage, preserving isolated pixels and straight one-pixel stems.
// Native coverage masks retain their samples; resizing uses bilinear coverage.
// The optional nearest dimensions retain the established fallback-then-scale
// layout when antialiasing is disabled or the source is a binary mask.
template <typename Read, typename Emit>
void textRasterRuns(const TextGlyph &glyph, uint16_t width, uint16_t height,
                    bool antialias, Read read, Emit emit,
                    uint16_t nearestWidth = 0, uint16_t nearestHeight = 0,
                    const TextRasterClip *clip = nullptr) {
  const uint32_t count = uint32_t(width) * height;
  if (!count || !glyph.data || !glyph.width || !glyph.height ||
      (glyph.encoding != TextEncoding::Rows && glyph.encoding != TextEncoding::Coverage4)) return;
  const bool gray = glyph.encoding == TextEncoding::Coverage4;
  const uint16_t x0 = clip ? (clip->left < width ? clip->left : width) : 0;
  const uint16_t y0 = clip ? (clip->top < height ? clip->top : height) : 0;
  const uint16_t x1 = clip && clip->right < width ? clip->right : width;
  const uint16_t y1 = clip && clip->bottom < height ? clip->bottom : height;
  if (x0 >= x1 || y0 >= y1) return;
  if (!nearestWidth) nearestWidth = width;
  if (!nearestHeight) nearestHeight = height;
  // Native integer enlargement needs no intermediate fallback coordinates.
  // Different CJK em sizes retain the established two-stage nearest sampler.
  const bool integerNearest = nearestWidth == glyph.width && nearestHeight == glyph.height &&
      width % glyph.width == 0 && height % glyph.height == 0;
  const uint16_t integerScaleX = integerNearest ? uint16_t(width / glyph.width) : 1;
  const uint16_t integerScaleY = integerNearest ? uint16_t(height / glyph.height) : 1;
  if(antialias&&!gray&&integerNearest&&integerScaleX==1&&integerScaleY==1&&
     glyph.width<=128&&glyph.stride>=uint16_t((glyph.width+7)/8)){
    detail::textBinaryStencilRuns(glyph,read,emit,clip);return;
  }
  if(antialias&&!gray&&integerNearest&&integerScaleX>=2&&integerScaleY>=2&&
     glyph.width<=128&&glyph.stride>=uint16_t((glyph.width+7)/8)){
    detail::textBinaryEnlargedRuns(glyph,width,height,read,emit,clip);return;
  }
  uint32_t cachedOffset = UINT32_MAX;
  uint8_t cachedByte = 0;
  const auto source = [&](int32_t sx, int32_t sy) -> uint8_t {
    if (sx < 0 || sy < 0 || sx >= glyph.width || sy >= glyph.height ||
        uint32_t(sx) >= uint32_t(glyph.stride) * (gray ? 2 : 8)) return 0;
    const uint32_t offset = uint32_t(sy) * glyph.stride + (gray ? uint32_t(sx) / 2 : uint32_t(sx) / 8);
    if (offset != cachedOffset) {
      cachedByte = read(glyph.data + offset);
      cachedOffset = offset;
    }
    return gray ? uint8_t((cachedByte >> ((sx & 1) ? 0 : 4)) & 15) :
                  uint8_t((cachedByte & (0x80U >> (sx & 7))) ? 15 : 0);
  };
  const auto nearest = [&](int32_t x, int32_t y) -> uint8_t {
    if (x < 0 || y < 0 || x >= width || y >= height) return 0;
    if (integerNearest) return source(integerScaleX == 1 ? x : x / integerScaleX,
                                      integerScaleY == 1 ? y : y / integerScaleY);
    const uint32_t bx = uint32_t(x) * nearestWidth / width;
    const uint32_t by = uint32_t(y) * nearestHeight / height;
    const uint32_t sx = (bx * glyph.width + glyph.width / 2) / nearestWidth;
    const uint32_t sy = (by * glyph.height + glyph.height / 2) / nearestHeight;
    return source(int32_t(sx), int32_t(sy));
  };
  const auto pixel = [&](int32_t x, int32_t y) -> uint8_t {
    if (!antialias) return nearest(x, y) >= 8 ? 15 : 0;
    if (!gray) {
      if (integerNearest && integerScaleX >= 2 && integerScaleY >= 2) {
        // At an enlarged binary block's interior, opposing neighbors are
        // identical. At a corner only one vertical and one horizontal source
        // neighbor can differ from center; the other three EPX terms retain
        // center. The exact rounded corner coverage is therefore 11 or 4.
        const int32_t sx=x/integerScaleX,sy=y/integerScaleY;
        const int32_t rx=x%integerScaleX,ry=y%integerScaleY;
        const uint8_t center=source(sx,sy);
        if ((rx && rx != integerScaleX-1) || (ry && ry != integerScaleY-1)) return center;
        const uint8_t vertical=source(sx,ry ? sy+1 : sy-1);
        if (vertical==center) return center;
        const uint8_t horizontal=source(rx ? sx+1 : sx-1,sy);
        return horizontal==center ? center : uint8_t(center ? 11 : 4);
      }
      const uint8_t center = nearest(x, y), up = nearest(x, y - 1), down = nearest(x, y + 1);
      const uint8_t left = nearest(x - 1, y), right = nearest(x + 1, y);
      if (up == down || left == right) return center;
      const uint16_t sum = (left == up ? left : center) + (up == right ? right : center) +
                           (left == down ? left : center) + (down == right ? right : center);
      return uint8_t((sum + 2) / 4);
    }
    if (width == glyph.width && height == glyph.height) return source(x, y);
    // Rational center coordinates avoid floating-point differences between
    // the browser preview and MCU. Taps beyond the glyph cell are transparent.
    const int64_t dx = int64_t(width) * 2, dy = int64_t(height) * 2;
    const int64_t nx = (int64_t(x) * 2 + 1) * glyph.width - width;
    const int64_t ny = (int64_t(y) * 2 + 1) * glyph.height - height;
    const int32_t sx = int32_t(nx >= 0 ? nx / dx : -((-nx + dx - 1) / dx));
    const int32_t sy = int32_t(ny >= 0 ? ny / dy : -((-ny + dy - 1) / dy));
    const int64_t fx = nx - int64_t(sx) * dx, fy = ny - int64_t(sy) * dy;
    const int64_t sum = source(sx, sy) * (dx - fx) * (dy - fy) +
        source(sx + 1, sy) * fx * (dy - fy) + source(sx, sy + 1) * (dx - fx) * fy +
        source(sx + 1, sy + 1) * fx * fy;
    return uint8_t((sum + dx * dy / 2) / (dx * dy));
  };
  if (clip) {
    // Absolute positions retain the complete raster's row stride. Splitting
    // rows avoids emitting the invisible gap between successive cropped rows.
    for (uint16_t y = y0; y < y1; ++y) {
      uint32_t start = uint32_t(y) * width + x0;
      uint8_t coverage = pixel(x0, y);
      for (uint16_t x = uint16_t(x0 + 1); x < x1; ++x) {
        const uint8_t next = pixel(x, y);
        if (next != coverage) {
          const uint32_t position = uint32_t(y) * width + x;
          emit(start, position - start, coverage);
          start = position;
          coverage = next;
        }
      }
      emit(start, uint32_t(y) * width + x1 - start, coverage);
    }
    return;
  }
  uint32_t start = 0, position = 0;
  uint8_t coverage = pixel(0, 0);
  for (uint16_t y = 0; y < height; ++y) {
    for (uint16_t x = 0; x < width; ++x, ++position) {
      if (!position) continue;
      const uint8_t next = pixel(x, y);
      if (next != coverage) {
        emit(start, position - start, coverage);
        start = position;
        coverage = next;
      }
    }
  }
  emit(start, count - start, coverage);
}

// Exact coverage endpoints and rounded interpolation of native RGB565 channels.
inline uint16_t textCoverageColor(uint16_t foreground, uint16_t background, uint8_t coverage) {
  const uint32_t inverse = 15 - coverage;
  const uint16_t red = uint16_t(((foreground >> 11) * coverage + (background >> 11) * inverse + 7) / 15);
  const uint16_t green = uint16_t((((foreground >> 5) & 63) * coverage + ((background >> 5) & 63) * inverse + 7) / 15);
  const uint16_t blue = uint16_t(((foreground & 31) * coverage + (background & 31) * inverse + 7) / 15);
  return uint16_t((red << 11) | (green << 5) | blue);
}

// Split only at row edges; a scaled foreground run remains one rectangle.
template <typename Read, typename Span>
void textSpans(const TextGlyph &glyph, bool opaque, Read read, Span span) {
  textRuns(glyph, read, [&](uint32_t position, uint32_t length, bool foreground) {
    if (!foreground && !opaque) return;
    while (length) {
      const uint16_t x = uint16_t(position % glyph.width);
      uint32_t width = glyph.width - x;
      if (width > length) width = length;
      span(x, uint16_t(position / glyph.width), uint16_t(width), foreground);
      position += width;
      length -= width;
    }
  });
}

} // namespace nice_gfx
#endif
