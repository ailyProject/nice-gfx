#pragma once
#include <cstdint>

namespace nice_gfx { namespace htmlui {
// Device/application state is explicit. Updating generated chrome copies the
// time into retained page state; the caller's string may be temporary.
struct SystemBarState {
  const char *time="09:41";
  uint8_t wifi=3;       // 0 disconnected, 1..3 signal strength.
  uint8_t battery=85;  // 0..100 percent.
  bool charging=false;
  uint16_t notifications=0; // 0..999 unread items.
};
// A canonical source-page element and its generated per-route presentation.
struct SystemPanelLink { uint16_t page; uint32_t clone,source; };
} }
