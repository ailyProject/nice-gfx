#ifndef NICE_GFX_UI_RLE565_H
#define NICE_GFX_UI_RLE565_H
#include <stddef.h>
#include <stdint.h>
namespace nice_gfx { namespace htmlui { namespace rle565 {
// Resumable count/encode pass. Each step reads at most budget source pixels;
// a run can cross any step boundary, including the 65535-word run limit.
// A null output performs the count pass. Partial output is never a stream
// until complete() returns true. The caller keeps source pixels immutable.
struct Encoder {
  const uint16_t *source=nullptr;
  uint16_t *target=nullptr;
  size_t pixels=0,capacity=0,at=0,words=0;
  uint16_t count=0,color=0;
  bool failed=false;
  void begin(const uint16_t *input,size_t size,uint16_t *output,size_t limit){
    source=input;target=output;pixels=size;capacity=limit;at=words=0;count=color=0;
    failed=!input||!size||size>SIZE_MAX/2||reinterpret_cast<uintptr_t>(input)>UINTPTR_MAX-size*2;
    if(output){
      const uintptr_t a=reinterpret_cast<uintptr_t>(input),b=reinterpret_cast<uintptr_t>(output);
      if(size>SIZE_MAX/2||limit>SIZE_MAX/2||a>UINTPTR_MAX-size*2||b>UINTPTR_MAX-limit*2||
         !(a+size*2<=b||b+limit*2<=a))failed=true;
    }
  }
  bool emit(){
    if(words>capacity||capacity-words<2){failed=true;return false;}
    if(target){target[words]=count;target[words+1]=color;}
    words+=2;count=0;return true;
  }
  size_t step(size_t budget){
    if(failed||!budget||complete())return 0;
    size_t consumed=0;
    while(at<pixels&&consumed<budget){
      const uint16_t next=source[at];++at;++consumed;
      if(!count){color=next;count=1;}
      else if(next==color&&count<65535)++count;
      else {if(!emit())return consumed;color=next;count=1;}
    }
    if(at==pixels&&count)emit();
    return consumed;
  }
  bool complete()const{return !failed&&source&&at==pixels&&!count;}
};
// Word-preserving run16/color16 streams. Count zero is invalid. Both passes
// are bounded by caller-owned storage; no allocation, headers or color change.
inline size_t encodedWords(const uint16_t *source,size_t pixels,size_t capacity){
  if(!source||!pixels)return 0;
  size_t words=0;
  for(size_t at=0;at<pixels;){
    if(words>capacity||capacity-words<2)return 0;
    const uint16_t color=source[at];size_t count=1;
    while(count<65535&&count<pixels-at&&source[at+count]==color)++count;
    words+=2;at+=count;
  }
  return words;
}
inline bool disjoint(const uint16_t *source,size_t words,const uint16_t *target,size_t pixels){
  if(words>SIZE_MAX/2||pixels>SIZE_MAX/2)return false;
  const uintptr_t a=reinterpret_cast<uintptr_t>(source),b=reinterpret_cast<uintptr_t>(target);
  const size_t inBytes=words*2,outBytes=pixels*2;
  return a<=UINTPTR_MAX-inBytes&&b<=UINTPTR_MAX-outBytes&&(a+inBytes<=b||b+outBytes<=a);
}
inline size_t encode(const uint16_t *source,size_t pixels,uint16_t *target,size_t capacity){
  if(!source||!target||!pixels||!disjoint(source,pixels,target,capacity))return 0;
  const size_t needed=encodedWords(source,pixels,capacity);if(!needed)return 0;
  size_t words=0;
  for(size_t at=0;at<pixels;){
    const uint16_t color=source[at];size_t count=1;
    while(count<65535&&count<pixels-at&&source[at+count]==color)++count;
    target[words++]=uint16_t(count);target[words++]=color;at+=count;
  }
  return words;
}
inline bool validate(const uint16_t *source,size_t words,size_t pixels){
  if(!source||!words||(words&1)||!pixels)return false;
  size_t decoded=0;
  for(size_t at=0;at<words;at+=2){const size_t count=source[at];if(!count||count>pixels-decoded)return false;decoded+=count;}
  return decoded==pixels;
}
// Forward run reader for a previously validated immutable stream. Seeking
// skips whole runs, so clipping does not require a decoded page allocation.
struct Reader {
  const uint16_t *source=nullptr;size_t words=0,index=0,position=0,remaining=0;
  uint16_t color=0;
  void begin(const uint16_t *data,size_t size){source=data;words=size;index=position=remaining=0;color=0;}
  bool ready(){if(remaining)return true;if(index>=words)return false;remaining=source[index];color=source[index+1];index+=2;return remaining!=0;}
  bool skip(size_t count){while(count){if(!ready())return false;const size_t step=count<remaining?count:remaining;count-=step;remaining-=step;position+=step;}return true;}
  bool seek(size_t target){if(target<position){index=position=remaining=0;}return skip(target-position);}
};
inline bool decode(const uint16_t *source,size_t words,uint16_t *target,size_t pixels){
  if(!target||!validate(source,words,pixels)||!disjoint(source,words,target,pixels))return false;
  uint16_t *out=target;
  for(size_t at=0;at<words;at+=2){size_t count=source[at];const uint16_t color=source[at+1];
#if defined(__GNUC__)
    // S31 PSRAM halfword stores are costly. Write aligned pairs and unroll
    // the long background runs; odd run boundaries still preserve every word.
    typedef uint32_t RleWord __attribute__((__may_alias__));
    if((reinterpret_cast<uintptr_t>(out)&3)&&count){*out++=color;--count;}
    auto *pairs=reinterpret_cast<RleWord *>(out);const uint32_t pair=uint32_t(color)|(uint32_t(color)<<16);
    while(count>=8){pairs[0]=pair;pairs[1]=pair;pairs[2]=pair;pairs[3]=pair;pairs+=4;count-=8;}
    while(count>=2){*pairs++=pair;count-=2;}out=reinterpret_cast<uint16_t *>(pairs);
#endif
    while(count--)*out++=color;
  }
  return true;
}
}}}
#endif
