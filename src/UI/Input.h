#pragma once
#include "View.h"
#include "Keys.h"

namespace nice_gfx { namespace htmlui {

enum class TouchSample : uint8_t { Released, Pressed, Error };

// A short no-contact sample (including CHSC6X IRQ gaps) must not split one
// finger gesture into a swipe followed by a tap. No allocation or delays.
class TouchInput {
 public:
  explicit TouchInput(uint16_t releaseMs = 30) : releaseMs_(releaseMs) {}
  void cancel(View &view) {
    view.pointerCancel(); down_=false; blocked_=true; releasing_=false;
  }
  void update(View &view, TouchSample sample, int32_t x, int32_t y, uint32_t now) {
    if(sample==TouchSample::Error) { cancel(view);return; }
    if(sample==TouchSample::Pressed) {
      releasing_=false;
      if(blocked_)return; // Fault/home-key tails cannot start a new contact.
      view.pausePointerRepeat(false);
      if(!down_)view.pointerDown(x,y,now);
      else if(x!=x_||y!=y_)view.pointerMove(x,y,now);
      x_=x;y_=y;down_=true;return;
    }
    if(!down_&&!blocked_)return;
    // Consumed reports can leave short no-contact gaps during a real hold.
    // Pause immediately, then resume the same hold if contact returns before
    // the existing release debounce completes; never synthesize another down.
    if(!releasing_) { releasing_=true;releasedAt_=now;view.pausePointerRepeat(true); }
    if(uint32_t(now-releasedAt_)<releaseMs_)return;
    if(down_)view.pointerUp(x_,y_,releasedAt_);
    down_=blocked_=releasing_=false;
  }
 private:
  uint16_t releaseMs_;
  bool down_=false, blocked_=false, releasing_=false;
  int32_t x_=0,y_=0;
  uint32_t releasedAt_=0;
};

// One press edge after debounce, no repeats while held, and no boot-held press.
// Pass an already normalized active level; call from the UI task, never an ISR.
class ButtonInput {
 public:
  explicit ButtonInput(uint16_t debounceMs = 30) : debounceMs_(debounceMs) {}
  bool update(bool pressed, uint32_t now) {
    if(!initialized_) { initialized_=true;raw_=stable_=pressed;changedAt_=now;return false; }
    if(pressed!=raw_) { raw_=pressed;changedAt_=now; }
    if(stable_==raw_||uint32_t(now-changedAt_)<debounceMs_)return false;
    stable_=raw_;return stable_;
  }
 private:
  uint16_t debounceMs_;
  bool initialized_=false,raw_=false,stable_=false;
  uint32_t changedAt_=0;
};

} }
