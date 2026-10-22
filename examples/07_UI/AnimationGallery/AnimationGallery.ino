#include "Hardware.h"
#include "ui.h"
#include "TouchAdapter.h"


nice_gfx::htmlui::View ui(gfx, kTilePixels);
bool uiReady = false;

// Application-owned state: add fields here and keep this sketch when updating ui.h.
struct AppData {
  // Your sensor values, settings, and other business state.
};
AppData appData;

// Update from callbacks or loop(), for example (use IDs from your own HTML):
// ui.setText(ui_config::id("status-label"), "Ready");
// ui.setValue(ui_config::id("level"), 42);
// Setters do not synthesize input/change events; update your state explicitly.
// UI routes in data-route/data-action work automatically, without a callback.


void setup() {
#if defined(ARDUINO_ARCH_ESP32) && defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT
  // USB can stay connected while its host stops reading. Drop diagnostics when
  // TX is full instead of stalling screen startup or the input/render pump.
  Serial.setTxTimeoutMs(0);
#endif
  Serial.begin(115200);
  if (!beginDisplayHardware()) return;

  if (ui_config::load(ui) != nice_gfx::Status::Ok) { Serial.println("UI mount failed"); return; }

  uiReady = true;
}

void loop() {
  if (!uiReady) { delay(10); return; }
  // Read sensors / update appData and UI here without blocking the input pump.
  uiReady = pollUi(ui, millis());

  delay(1);
}
