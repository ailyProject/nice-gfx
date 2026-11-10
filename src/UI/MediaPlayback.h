#pragma once
#include "Compiled.h"

namespace nice_gfx { namespace htmlui {

// Sparse per-node behavior; shared MediaDesc resources contain no playback state.
// Install after mount().
struct MediaPlaybackDesc {
  Id id, scrubTarget;
  bool autoplay, loop, reverse;
  uint16_t initialFrame;
  constexpr MediaPlaybackDesc(Id node=0,Id target=0,bool automatic=true,bool wrap=true,bool backwards=false,uint16_t initial=0)
    :id(node),scrubTarget(target),autoplay(automatic),loop(wrap),reverse(backwards),initialFrame(initial) {}
};

// One immutable media source projected onto several routes. Each group has at
// most one node on a page; the current frame/resource handle survives navigation
// between its pages. Frame numbers are zero based and fps=0 uses the source rate.
// This optional policy table does not change the compiled resource ABI.
struct SharedMediaDesc {
  Id groupId, nodeId;
  uint16_t pageIndex, targetFrame, fps;
  constexpr SharedMediaDesc(Id group=0,Id node=0,uint16_t page=0,uint16_t target=0,uint16_t rate=0)
    :groupId(group),nodeId(node),pageIndex(page),targetFrame(target),fps(rate) {}
};

} }
