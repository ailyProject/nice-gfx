#pragma once
#include "Compiled.h"

namespace nice_gfx { namespace htmlui {
enum class InteractionVersion : uint8_t { Legacy = 1, Coordinated = 2 };
enum class GestureAxis : uint8_t { None = 0, X = 1, Y = 2, Both = 3 };
enum class GestureOwnership : uint8_t { Auto, Local, Parent };
enum class EventScope : uint8_t { Subtree, Self, Component };
// Sparse overrides, copied by View. Native controls derive their own axes.
// Modal descriptors belong to the visible panel, not its always-visible opener.
struct GesturePolicy {
  GestureAxis axis;
  GestureOwnership ownership;
  bool navigationChain, pointerEvents, modal;
  EventScope eventScope;
  constexpr GesturePolicy(GestureAxis axes=GestureAxis::Both,GestureOwnership owner=GestureOwnership::Auto,
      bool chain=false,bool hit=true,bool layer=false,EventScope scope=EventScope::Subtree)
    :axis(axes),ownership(owner),navigationChain(chain),pointerEvents(hit),modal(layer),eventScope(scope){}
};
struct InteractionDesc { Id id; GesturePolicy policy; };
namespace detail {
enum class GestureDecision : uint8_t { Pending, X, Y, Reject };
inline GestureDecision decideGesture(int32_t dx, int32_t dy) {
  const int64_t ax=dx<0?-int64_t(dx):dx, ay=dy<0?-int64_t(dy):dy;
  if(ax<8&&ay<8)return GestureDecision::Pending;
  if(ax>=8&&ax*100>=ay*135)return GestureDecision::X;
  if(ay>=8&&ay*100>=ax*135)return GestureDecision::Y;
  return ax>=16||ay>=16?GestureDecision::Reject:GestureDecision::Pending;
}
inline bool acceptsAxis(GestureAxis axes,bool horizontal) {
  return (uint8_t(axes)&uint8_t(horizontal?GestureAxis::X:GestureAxis::Y))!=0;
}
}
} }
