#pragma once
#include "Compiled.h"
#define NICE_GFX_UI_BEHAVIORS_VERSION 1

namespace nice_gfx { namespace htmlui {

enum class BehaviorKind : uint8_t { Toast, Modal, Editor, Task, Timer };
enum class DialogMotion : uint8_t { None, Spring, Circular };
enum BehaviorFlags : uint16_t {
  BehaviorAutoStart=1, BehaviorRepeat=2, BehaviorDismissOnInput=4,
  BehaviorLiveEdit=8, BehaviorSystemFeedback=16
};
// Attach bounded lifecycle behavior to an existing authored node. No additional
// widget tree or navigation stack is created. Numeric editors use scaled ints;
// InputField editors also retain their original UTF-8 text for cancellation.
struct BehaviorDesc {
  Id id=0;
  BehaviorKind kind=BehaviorKind::Toast;
  Id target=0,initialFocus=0;
  uint32_t intervalMs=100,durationMs=0;
  uint16_t flags=0;
  uint8_t dim=0;
  DialogMotion motion=DialogMotion::None;
  uint16_t motionMs=180;
};

} }
