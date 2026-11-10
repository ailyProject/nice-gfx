#pragma once
#include <stddef.h>
#include <stdint.h>

// Isolated source-only candidate. The current UI/async sources do not include
// this file. The caller proves the complete input footprint before calling.
#ifndef NICE_GFX_UI_SYSTEM_MODAL_RLE_VALIDATED_SEEK
#if defined(CONFIG_IDF_TARGET_ESP32S31)
#define NICE_GFX_UI_SYSTEM_MODAL_RLE_VALIDATED_SEEK 1
#else
#define NICE_GFX_UI_SYSTEM_MODAL_RLE_VALIDATED_SEEK 0
#endif
#endif

namespace nice_gfx { namespace htmlui { namespace rle565 {
struct ValidatedSeekCursor {
  size_t index=0,position=0,remaining=0;
  uint16_t color=0;
};

#if NICE_GFX_UI_SYSTEM_MODAL_RLE_VALIDATED_SEEK
// Validate every run, including the entire tail after the requested cursor.
// Ordered seek phases avoid a seek-found branch in the final tail loop.
// A boundary cursor has remaining==0 and reloads the following run normally.
// Optional second output is distinct and is published only after the same
// complete validation succeeds. Existing callers retain their single seek.
// Output cursor objects are caller-owned and disjoint from the source words.
inline bool validateAndSeek(const uint16_t *source,size_t words,size_t pixels,
                            size_t seek,ValidatedSeekCursor &output,
                            size_t secondSeek=0,ValidatedSeekCursor *secondOutput=nullptr){
  output=ValidatedSeekCursor{};
  if(secondOutput)*secondOutput=ValidatedSeekCursor{};
  if(!source||!words||(words&1u)||!pixels||seek>pixels)return false;
  if(secondOutput&&(secondOutput==&output||secondSeek<seek||secondSeek>pixels))return false;
  ValidatedSeekCursor cursor,secondCursor;
  size_t at=0,decoded=0;
  while(at<words&&decoded<seek){
    const size_t count=source[at];
    if(!count||count>pixels-decoded)return false;
    const size_t end=decoded+count;
    if(end>=seek){
      cursor.index=at+2;cursor.position=seek;cursor.remaining=end-seek;
      if(cursor.remaining)cursor.color=source[at+1];
    }
    decoded=end;at+=2;
  }
  if(secondOutput){
    if(secondSeek<=decoded){
      // Both positions can lie inside the run already loaded for the first
      // cursor. A boundary retains the following-run index and no color.
      secondCursor=cursor;secondCursor.position=secondSeek;
      secondCursor.remaining=decoded-secondSeek;
      if(!secondCursor.remaining)secondCursor.color=0;
    }else while(at<words&&decoded<secondSeek){
      const size_t count=source[at];
      if(!count||count>pixels-decoded)return false;
      const size_t end=decoded+count;
      if(end>=secondSeek){
        secondCursor.index=at+2;secondCursor.position=secondSeek;secondCursor.remaining=end-secondSeek;
        if(secondCursor.remaining)secondCursor.color=source[at+1];
      }
      decoded=end;at+=2;
    }
  }
  for(;at<words;at+=2){
    const size_t count=source[at];
    if(!count||count>pixels-decoded)return false;
    decoded+=count;
  }
  if(decoded!=pixels)return false;
  output=cursor;if(secondOutput)*secondOutput=secondCursor;return true;
}
#endif
}}}
