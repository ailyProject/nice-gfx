#pragma once
#include <Graphics/Sprite.h>
#include <Fonts/Builtin.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace benchmark {

enum Scene : uint8_t { Fill, Lines, Shapes, Text, Gradients, Sprite, Widgets, Input, Routes, SceneCount };
static const char *const sceneNames[] = {
  "Fill / RGB565", "Lines / clipping", "Shapes / antialias", "Text / UTF-8",
  "Gradients", "Sprite / animation", "UI / widgets", "UI / pointer input", "UI / transitions"
};
static const uint16_t colors[] = {GFX_CYAN, GFX_ORANGE, GFX_MAGENTA, GFX_GREEN, GFX_YELLOW, GFX_SKYBLUE};

// The compact Latin fonts contain printable ASCII. Add a tiny original degree
// glyph so the UTF-8 sample also exercises the public font fallback mechanism.
inline const nice_gfx::MonoFont &temperatureFont() {
  static const uint16_t index[] PROGMEM = {0x00b0};
  static const uint8_t bitmap[] PROGMEM = {0x00,0x30,0x48,0x48,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  static const nice_gfx::MonoFont symbol = {bitmap, index, 1, 6, 13, 11, nullptr, 1};
  static const nice_gfx::MonoFont font = [] {
    nice_gfx::MonoFont result = nice_gfx::fonts::GfxSansLatin16;
    result.fallback = &symbol;
    return result;
  }();
  return font;
}

// Fixed integer sequence: identical workload on every run, no global random().
inline uint32_t randomStep(uint32_t &state) {
  state ^= state << 13; state ^= state >> 17; state ^= state << 5;
  return state;
}

// Caller installs a viewport at (0,24,width,contentHeight-24). Geometry stays
// inside it, except intentional clipped lines. Direct calls include bus time.
inline uint64_t drawScene(NICE_GFX &target, Scene scene, int w, int h, uint32_t frame) {
  const uint16_t color = colors[(frame / 8) % 6];
  uint32_t rng = 0x13579bdu + frame % 120;
  target.fillRect(0, 0, w, h, scene == Fill ? color : GFX_BLACK);
  if (scene == Fill) return uint64_t(w) * h;
  switch (scene) {
    case Lines:
      for (unsigned i = 0; i < 48; ++i) {
        const int x = int(randomStep(rng) % (w + 48)) - 24;
        const int y = int(randomStep(rng) % (h + 48)) - 24;
        target.drawLine(w / 2, h / 2, x, y, colors[i % 6]);
      }
      target.drawRect(0, 0, w, h, GFX_WHITE);
      break;
    case Shapes:
      for (unsigned i = 0; i < 12; ++i) {
        const int r = 6 + i % 12;
        const int x = r + randomStep(rng) % (w - 2 * r);
        const int y = r + randomStep(rng) % (h - 2 * r);
        if (i % 3 == 0) target.fillSmoothCircle(x, y, r, colors[i % 6], GFX_BLACK);
        else if (i % 3 == 1) target.fillTriangle(x-r, y+r, x+r, y+r, x, y-r, colors[i % 6]);
        else target.drawSmoothRoundRect(x-r, y-r, 5, 3, 2*r, 2*r, colors[i % 6], GFX_BLACK);
      }
      target.drawSmoothArc(w/2, h/2, std::min(w,h)/3, std::min(w,h)/3-5,
                           frame%360, frame%360+140, GFX_WHITE, GFX_BLACK, true);
      break;
    case Text: {
      target.setFont(nice_gfx::fonts::Fixed6x13);
      target.setTextSize(1);
      for (int y = 2; y + 13 < h - 28; y += 17) {
        char line[48];
        std::snprintf(line, sizeof(line), "NICE_GFX %06lu  AaZz 012345", static_cast<unsigned long>(frame));
        target.setTextColor(colors[(y / 17) % 6], GFX_BLACK);
        target.drawString(line, (frame + y) % 9, y);
      }
      target.setFont(temperatureFont());
      target.setTextColor(GFX_WHITE, GFX_BLACK);
      target.drawString("UTF-8: 23\xc2\xb0" "C / 75\xc2\xb0" "F", 4, h - 23);
      break;
    }
    case Gradients:
      target.fillRectHGradient(0, 0, w, h/2, color, GFX_BLACK);
      target.fillRectVGradient(0, h/2, w, h-h/2, GFX_BLACK, colors[(frame/8+2)%6]);
      target.drawCircle((frame*3)%w, h/2, h/4, GFX_WHITE);
      break;
    default: break;
  }
  // Overdraw and AA coverage are not exact transport pixel counts.
  return 0;
}

// Small fixed cap keeps the fixture usable without a full-screen framebuffer.
// Allocation failure is reported as SKIP, never as a fast zero-work frame.
inline bool createAnimation(GFX_Sprite &sprite, int w, int h) {
  sprite.setColorDepth(16);
  sprite.setAttribute(PSRAM_ENABLE, false);
  for (int sw = std::min(w, 192), sh = std::min(h, 120); sw >= 64 && sh >= 48; sw /= 2, sh /= 2) {
    if (sprite.createSprite(sw, sh)) {
      sprite.setFont(nice_gfx::fonts::Fixed6x13);
      return true;
    }
  }
  return false;
}

inline void drawAnimation(GFX_Sprite &sprite, uint32_t elapsedMs) {
  const int w = sprite.width(), h = sprite.height();
  sprite.fillSprite(GFX_BLACK);
  for (int x = 0; x < w; x += 16) sprite.drawFastVLine(x, 0, h, GFX_DARKGREY);
  for (int y = 0; y < h; y += 16) sprite.drawFastHLine(0, y, w, GFX_DARKGREY);
  const float phase = float(elapsedMs % 6000) * 0.00104719755f;
  for (unsigned i = 0; i < 8; ++i) {
    const float angle = phase + i * 0.78539816f;
    const int x = w/2 + int(std::cos(angle) * (w/2-14));
    const int y = h/2 + int(std::sin(angle*2) * (h/2-14));
    sprite.fillSmoothCircle(x, y, 5+i%5, colors[i%6], GFX_BLACK);
  }
  sprite.drawRoundRect(0, 0, w, h, 8, GFX_WHITE);
  sprite.setTextColor(GFX_WHITE, GFX_BLACK);
  sprite.drawString("RGB565 / 8 particles", 4, 4);
}

} // namespace benchmark
