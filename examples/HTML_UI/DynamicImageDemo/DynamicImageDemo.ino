#include <NICE_GFX.h>
#include "Hardware.h"
#include "ui.h"
#include "TouchAdapter.h"
#include <cstdio>

using nice_gfx::Status;
using nice_gfx::htmlui::Event;
using nice_gfx::htmlui::ImageView;
nice_gfx::htmlui::View ui(gfx, kTilePixels);
constexpr auto photoId = ui_config::id("photo");
constexpr auto statusId = ui_config::id("status");
constexpr uint16_t imageWidth = 128, imageHeight = 96;
uint16_t pixels[size_t(imageWidth) * imageHeight];
bool uiReady = false, generating = false, alternate = false;
uint8_t request = 0;
uint32_t frame = 0, lastImage = 0;

void onStart(void *, Event &) { request = 1; }
void onResource(void *, Event &) { request = 2; }
void onClear(void *, Event &) { request = 3; }
void onReset(void *, Event &) { request = 4; }

void report(Status result, const char *caption) {
  ui.setText(statusId, result == Status::Ok ? caption : "Image update failed; old image retained");
}

void updateGeneratedImage(uint32_t now) {
  for(uint16_t y = 0; y < imageHeight; ++y) for(uint16_t x = 0; x < imageWidth; ++x) {
    const uint16_t r = uint16_t((x + frame) % 32);
    const uint16_t g = uint16_t((y + frame) % 64);
    const uint16_t b = uint16_t((x / 4 + y / 4 + frame) % 32);
    pixels[size_t(y) * imageWidth + x] = uint16_t((r << 11) | (g << 5) | b);
  }
  const Status result = ui.setImage(photoId, ImageView(pixels, imageWidth, imageHeight, sizeof(pixels)));
  // setImage copied the frame. The source can now be overwritten or freed.
  for(auto &pixel : pixels) pixel = 0;
  char caption[48];
  std::snprintf(caption, sizeof(caption), "Copied frame %lu", static_cast<unsigned long>(frame));
  report(result, caption);
  ++frame;
  lastImage = now;
}

void setup() {
  Serial.begin(115200);
  if(!beginDisplayHardware() || ui_config::load(ui) != Status::Ok) return;
  if(!ui_config::bind_startDynamic(ui, onStart) ||
     !ui_config::bind_switchResource(ui, onResource) ||
     !ui_config::bind_clearPhoto(ui, onClear) ||
     !ui_config::bind_resetPhoto(ui, onReset)) return;
  uiReady = true;
}

void loop() {
  if(!uiReady) { delay(10); return; }
  const uint32_t now = millis();
  const uint8_t action = request;
  request = 0;
  if(action) generating = action == 1;
  if(action == 1) updateGeneratedImage(now);
  else if(action == 2) {
    alternate = !alternate;
    report(ui_config::setMedia(ui, photoId, alternate ? "demo-stripes" : "demo-checker"), "Compiled resource; no pixel copy");
  } else if(action == 3) report(ui.clearImage(photoId), "Blank component");
  else if(action == 4) report(ui.resetImage(photoId), "Authored fallback restored");
  else if(generating && uint32_t(now - lastImage) >= 100) updateGeneratedImage(now);
  uiReady = pollUi(ui, now); // Continue input, animation and tiled DMA transfers.
  delay(1);
}
