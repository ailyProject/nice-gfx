#pragma once
#include "Compiled.h"

namespace nice_gfx { namespace htmlui {

// Sparse per-node behavior; shared MediaDesc resources contain no playback state.
// Install after mount(). Resource ABI 17 and existing aggregate initializers stay intact.
struct MediaPlaybackDesc {
  Id id, scrubTarget;
  bool autoplay, loop, reverse;
  uint16_t initialFrame;
  constexpr MediaPlaybackDesc(Id node=0,Id target=0,bool automatic=true,bool wrap=true,bool backwards=false,uint16_t initial=0)
    :id(node),scrubTarget(target),autoplay(automatic),loop(wrap),reverse(backwards),initialFrame(initial) {}
};

} }
