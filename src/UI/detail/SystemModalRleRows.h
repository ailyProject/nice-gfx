#pragma once
#include "BehaviorBackdrop.h"
#include "SystemModalRleSeek.h"
#include <algorithm>

namespace nice_gfx { namespace htmlui { namespace detail {

// All pointers and complete footprints have been proved by the UI owner.
// The stream, raw Wire pixels, damage boxes and LUT stay immutable until join.
struct SystemModalRleRowsInput {
  const uint16_t *encoded=nullptr;
  const uint16_t *rawWire=nullptr;
  uint16_t *output=nullptr;
  size_t width=0,outputStride=0;
  Rect roi{};
  const Rect *textDamage=nullptr;
  size_t textDamageCount=0;
  const BackdropFloorLut *lut=nullptr;
};
struct SystemModalRleRowsResult {
  size_t cursorWords=0,rawPixels=0,rlePixels=0,drawnRuns=0;
};

#if defined(__GNUC__)
__attribute__((always_inline))
#endif
inline void writeSystemModalRleSolid(uint16_t *output,size_t count,uint16_t color){
  if(!count)return;
  if(count==1){*output=color;return;}
#if defined(__GNUC__)
  typedef uint32_t Word __attribute__((__may_alias__));
  if(reinterpret_cast<uintptr_t>(output)&3u){*output++=color;--count;}
  auto *pairs=reinterpret_cast<Word*>(output);
  const uint32_t pair=uint32_t(color)|(uint32_t(color)<<16);
  while(count>=16){
    pairs[0]=pair;pairs[1]=pair;pairs[2]=pair;pairs[3]=pair;
    pairs[4]=pair;pairs[5]=pair;pairs[6]=pair;pairs[7]=pair;
    pairs+=8;count-=16;
  }
  while(count>=2){*pairs++=pair;count-=2;}
  if(count)*reinterpret_cast<uint16_t*>(pairs)=color;
#else
  while(count--)*output++=color;
#endif
}

// The one-row pixel loop from the existing fused copy uses the shared LUT.
// The owner already proved its bounds and aliasing, so no cache API is called.
inline void writeSystemModalRleRaw(uint16_t *output,const uint16_t *input,size_t count,
                                   const BackdropFloorLut &lut){
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  if(!((reinterpret_cast<uintptr_t>(output)^reinterpret_cast<uintptr_t>(input))&3u)){
    if((reinterpret_cast<uintptr_t>(output)&3u)&&count){
      const uint16_t native=rgb565_blend::fromWire(*input++);
      *output++=uint16_t(lut.red[native>>11]|lut.green[(native>>5)&63]|lut.blue[native&31]);--count;
    }
    typedef uint32_t Word __attribute__((__may_alias__));
    auto *to=reinterpret_cast<Word*>(output);const auto *from=reinterpret_cast<const Word*>(input);
    uint32_t previous=0,previousResult=0;bool cached=false;
    size_t pairs=count/2;
    while(pairs--){
      const uint32_t sample=*from++;
      if(!cached||sample!=previous){
        const uint16_t first=uint16_t(sample),second=uint16_t(sample>>16);
        const uint16_t nativeFirst=rgb565_blend::fromWire(first),nativeSecond=rgb565_blend::fromWire(second);
        const uint16_t a=uint16_t(lut.red[nativeFirst>>11]|lut.green[(nativeFirst>>5)&63]|lut.blue[nativeFirst&31]);
        const uint16_t b=first==second?a:uint16_t(lut.red[nativeSecond>>11]|lut.green[(nativeSecond>>5)&63]|lut.blue[nativeSecond&31]);
        previousResult=uint32_t(a)|(uint32_t(b)<<16);previous=sample;cached=true;
      }
      *to++=previousResult;
    }
    output=reinterpret_cast<uint16_t*>(to);input=reinterpret_cast<const uint16_t*>(from);count&=1u;
  }
#endif
  while(count--){
    const uint16_t native=rgb565_blend::fromWire(*input++);
    *output++=uint16_t(lut.red[native>>11]|lut.green[(native>>5)&63]|lut.blue[native&31]);
  }
}

// Pure scalar kernel. Each invocation copies the optional validated seed and
// advances only its own cursor. cursorWords counts actual words loaded after
// that initial cursor, including any further prefix traversal by the lower half.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("O3")))
#endif
inline SystemModalRleRowsResult drawSystemModalRleRows(const SystemModalRleRowsInput &input,
    const rle565::ValidatedSeekCursor *seed=nullptr)noexcept{
  SystemModalRleRowsResult result;
  auto cursor=seed?*seed:rle565::ValidatedSeekCursor{};
  if(size_t(input.roi.y)*input.width+size_t(input.roi.x)<cursor.position)cursor={};
  size_t streamIndex=cursor.index,position=cursor.position,runRemaining=cursor.remaining;
  const size_t initialIndex=streamIndex;
  uint16_t wireColor=cursor.color,previousWireColor=0,nativeColor=0;
  bool colorCached=false;
  const int32_t right=input.roi.x+input.roi.width,bottom=input.roi.y+input.roi.height;
  for(int32_t y=input.roi.y;y<bottom;++y){int32_t x=input.roi.x;
    while(x<right){int32_t coveredEnd=x,clearEnd=right;
      for(size_t at=0;at<input.textDamageCount;++at){const auto &box=input.textDamage[at];
        if(y<box.y||y>=box.y+box.height)continue;
        const int32_t end=box.x+box.width;
        if(box.x<=x){if(end>x&&end>coveredEnd)coveredEnd=std::min(end,right);}
        else if(box.x<clearEnd)clearEnd=box.x;
      }
      if(coveredEnd>x){
        const size_t out=size_t(y)*input.outputStride+size_t(x),in=size_t(y)*input.width+size_t(x),count=size_t(coveredEnd-x);
        writeSystemModalRleRaw(input.output+out,input.rawWire+in,count,*input.lut);
        result.rawPixels+=count;x=coveredEnd;continue;
      }
      size_t remaining=size_t(clearEnd-x);auto *output=input.output+size_t(y)*input.outputStride+size_t(x);
      size_t skip=size_t(y)*input.width+size_t(x)-position;
      while(skip){
        if(!runRemaining){runRemaining=input.encoded[streamIndex];wireColor=input.encoded[streamIndex+1];streamIndex+=2;}
        const size_t count=std::min(skip,runRemaining);skip-=count;runRemaining-=count;position+=count;
      }
      while(remaining){
        if(!runRemaining){runRemaining=input.encoded[streamIndex];wireColor=input.encoded[streamIndex+1];streamIndex+=2;}
        const size_t count=std::min(remaining,runRemaining);
        if(!colorCached||wireColor!=previousWireColor){
          const uint16_t native=rgb565_blend::fromWire(wireColor);
          nativeColor=uint16_t(input.lut->red[native>>11]|input.lut->green[(native>>5)&63]|input.lut->blue[native&31]);
          previousWireColor=wireColor;colorCached=true;
        }
        writeSystemModalRleSolid(output,count,nativeColor);++result.drawnRuns;
        output+=count;remaining-=count;runRemaining-=count;position+=count;
      }
      x=clearEnd;
    }
  }
  result.cursorWords=streamIndex-initialIndex;
  result.rlePixels=size_t(input.roi.width)*size_t(input.roi.height)-result.rawPixels;
  return result;
}

}}}
