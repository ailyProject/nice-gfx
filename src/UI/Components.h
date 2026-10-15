#pragma once
#include "Compiled.h"

namespace nice_gfx { namespace htmlui {

// Authoring controls reuse native nodes. These immutable links provide stable
// public IDs and bounded selection state without introducing new node kinds.
enum class ComponentKind : uint8_t { ImageButton, SettingRow, List, Select, Choice, Tabs, Popup, Loading, ValueSlider, Confirm };
struct ComponentOptionDesc { Id id; const char *label; };
struct ComponentDesc {
  ComponentKind kind;
  Id id, valueId, textId, pageId, expandedId;
  const ComponentOptionDesc *options;
  uint8_t optionCount;
  const char *valueFormat; // One {value}; UTF-8 template at most 128 bytes.
};

} }
