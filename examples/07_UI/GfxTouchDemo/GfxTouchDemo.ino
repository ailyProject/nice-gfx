#include <NICE_GFX.h>
#include "Hardware.h"
#include "ui.h"
#include "TouchAdapter.h"
#include <cstdio>

nice_gfx::htmlui::View ui(gfx, kTilePixels);
bool uiReady = false;

struct AppData {
  int32_t brightness = 60;
  bool notificationsEnabled = true;
};
AppData appData;

// Setters affect the active page only. Data persists in appData across routes.
// Text is copied, and setters do not emit input/change recursively.
void refreshUi(const AppData &data) {
  char caption[24];
  snprintf(caption, sizeof(caption), "亮度 %ld%%", static_cast<long>(data.brightness));
  ui.setText(ui_config::id("title"), caption);
  ui.setValue(ui_config::id("brightness"), data.brightness);
  ui.setText(ui_config::id("enable-label"), data.notificationsEnabled ? "通知已开" : "通知已关");
  ui.setValue(ui_config::id("enabled"), data.notificationsEnabled ? 1 : 0);
}

void on_brightnessChanged(void *context, nice_gfx::htmlui::Event &event) {
  auto &data = *static_cast<AppData *>(context);
  data.brightness = event.value;
  refreshUi(data); // Display only: no PWM or external actuator is changed.
}

void on_notificationsChanged(void *context, nice_gfx::htmlui::Event &event) {
  auto &data = *static_cast<AppData *>(context);
  data.notificationsEnabled = event.value != 0;
  refreshUi(data);
}

void on_routeChanged(void *context, nice_gfx::htmlui::Event &) {
  refreshUi(*static_cast<AppData *>(context));
}

void setup() {
  Serial.begin(115200);
  if (!beginDisplayHardware()) return;
  if (ui_config::load(ui) != nice_gfx::Status::Ok) { Serial.println("UI mount failed"); return; }
  if (!ui_config::bind_brightnessChanged(ui, on_brightnessChanged, &appData) ||
      !ui_config::bind_notificationsChanged(ui, on_notificationsChanged, &appData) ||
      !ui.on(0, nice_gfx::htmlui::EventType::RouteChange, on_routeChanged, &appData)) {
    Serial.println("UI event binding failed"); return;
  }
  refreshUi(appData);
  uiReady = true;
}

void loop() {
  if (!uiReady) { delay(10); return; }
  // Read sensors and update appData here without blocking touch input.
  uiReady = pollUi(ui, millis());
  delay(1);
}
