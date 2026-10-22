#pragma once

// Seeed SAMD 1.8.6 noTone() resets TC0 unconditionally. Before the first
// tone(), its generic clock is disabled and SWRST never completes. Keep the
// initialization state here so IR and MUTE can safely stop a silent board.
class FieldLabBuzzer {
  bool initialized_=false;
public:
  void play(unsigned pin,unsigned frequency,unsigned duration){
    tone(pin,frequency,duration);initialized_=true;
  }
  void stop(unsigned pin){if(initialized_)noTone(pin);}
};
