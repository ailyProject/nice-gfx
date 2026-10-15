#pragma once
#include "Compiled.h"

namespace nice_gfx { namespace htmlui {

// A small explicit layout operation, independent of the offline CSS compiler.
// layoutChildren places non-hidden direct children at the parent's content origin,
// preserves their sizes, and moves their subtrees. resizeContainer changes only
// the selected axis. Container/Scroll support layout; Pager retains its compiled
// stride and may be moved but not resized or given a different content box.
// Direct pager pages retain their authored bounds; move the pager as a whole.
// A hidden parent may be laid out before it is shown; its own visibility does
// not make every direct child disappear from the layout calculation.
enum class LayoutAxis : uint8_t { Horizontal, Vertical };

// Allocated only for nodes that override compiled geometry or appearance.
// State and in-flight frame snapshots share immutable instances; setters copy
// this descriptor before a change. setBounds/layoutBounds use page coordinates
// before scroll and animation, including dynamic movement inherited from parents.
// Moving a parent also moves its descendants; resetting a child restores its
// authored offset relative to its parent's current dynamic position.
// Font/gradient pointers and their data remain borrowed and immutable until all
// referring states AND already submitted frames have been released. setStyle
// copies the Style descriptor, does not alter node flags, and does not reflow.
struct DynamicNode {
  int32_t offsetX=0, offsetY=0, width=0, height=0;
  bool boundsOverride=false;
  Style style{};
  bool styleOverride=false;
};

} }
