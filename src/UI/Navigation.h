#pragma once
#include "Compiled.h"

namespace nice_gfx { namespace htmlui {
enum class SwipeDirection : uint8_t { Left, Right, Up, Down };
enum class SwipeEdge : uint8_t { Any, Left, Right, Top, Bottom };
enum class GestureInteraction : uint8_t { Release=0, Follow=1, Instant=2 };
// These immutable descriptors and their strings stay valid until replaced or
// cleared. Indicators overlay the page; author content above their reserved area.
struct NavigationGroupDesc {
  Id id; const char *const *paths; uint16_t count; bool swipe;
  uint16_t indicatorHeight, bottom; uint8_t dotSize, dotGap;
  uint16_t trackColor, fillColor, background;
  uint16_t indicatorInset;
  bool backgroundTransparent; // Omitted trailing field retains the opaque footer.
  // Shared navigation stays outside the moving content. Omitted insets keep
  // legacy full-screen transitions. These fields require navigation version 4.
  uint16_t insetLeft, insetTop, insetRight, insetBottom;
};
struct GestureBindingDesc {
  Id id; const char *source; SwipeDirection direction; SwipeEdge edge;
  RouteAction action; const char *route; uint16_t threshold, edgeSize;
  GestureInteraction interaction; // Omitted trailing fields zero-initialize to Release/null.
  const TransitionDesc *transition;
};
struct NavigationDesc {
  const NavigationGroupDesc *groups; uint16_t groupCount;
  const GestureBindingDesc *gestures; uint16_t gestureCount;
};
} }
