#pragma once
#include <stdint.h>
#include <stddef.h>
#define NICE_GFX_INPUT_VERSION 1
namespace nice_gfx { namespace htmlui {
enum class KeyGesture : uint8_t { Press, Short, Long, Repeat, None };
enum class InputAction : uint8_t { None, Previous, Next, Up, Down, Left, Right, Confirm, Back, Home, Push, Replace, Activate, Focus, Increment, Decrement, ScrollPrevious, ScrollNext, PagePrevious, PageNext, Callback };
struct KeyBinding { uint32_t key; KeyGesture gesture; InputAction action; uint32_t target; const char *route; const char *page; };
struct FocusDesc { uint32_t id; int16_t order; bool enabled; uint32_t up, down, left, right; };
struct InputPage { const char *path; uint32_t initial; bool spatial; };
struct InputConfig { const KeyBinding *bindings; size_t count; const FocusDesc *focus; size_t focusCount; const InputPage *pages; size_t pageCount; uint16_t color; uint8_t width; };
// Allocation-free, wrap-safe and nonblocking. Call from the UI task.
class KeyInput {
 public:
  KeyInput(uint16_t debounce=30,uint16_t longPress=600,uint16_t repeatDelay=450,uint16_t repeatInterval=120)
    : debounce_(debounce),longMs_(longPress),delay_(repeatDelay),interval_(repeatInterval) {}
  void cancel() { blocked_=true;held_=false; }
  KeyGesture update(bool pressed,uint32_t now,bool allowLong=false,bool allowRepeat=false) {
    if(!initialized_) { initialized_=true;raw_=stable_=pressed;changed_=now;blocked_=pressed;return KeyGesture::None; }
    if(pressed!=raw_) { raw_=pressed;changed_=now; }
    if(stable_!=raw_&&uint32_t(now-changed_)>=debounce_) {
      stable_=raw_;
      if(!stable_) { const bool click=held_&&!blocked_&&!long_&&!repeated_;held_=blocked_=false;if(click)return KeyGesture::Short; }
      else if(!blocked_) { held_=true;long_=repeated_=false;started_=lastRepeat_=now;return KeyGesture::Press; }
    }
    if(!pressed&&!stable_&&uint32_t(now-changed_)>=debounce_)blocked_=false;
    if(held_&&stable_&&raw_&&!blocked_) {
      if(allowLong&&!long_&&uint32_t(now-started_)>=longMs_) { long_=true;return KeyGesture::Long; }
      if(allowRepeat&&!allowLong&&uint32_t(now-started_)>=delay_&&(!repeated_||uint32_t(now-lastRepeat_)>=interval_)) { repeated_=true;lastRepeat_=now;return KeyGesture::Repeat; }
    }
    return KeyGesture::None;
  }
 private:
  uint16_t debounce_,longMs_,delay_,interval_;
  bool initialized_=false,raw_=false,stable_=false,blocked_=false,held_=false,long_=false,repeated_=false;
  uint32_t changed_=0,started_=0,lastRepeat_=0;
};
} }
