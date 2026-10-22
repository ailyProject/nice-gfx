// Font styles are selected with setFont(); no build flags or font registry.
// Change this display and its pins to match your hardware before uploading.
#include <NICE_GFX.h>

ILI9341_240x320_SPI gfx(5, 2, 4, 18, 23, 19);

struct FontDemo {
  const nice_gfx::MonoFont *font;
  const char *name;
  bool chinese;
};

// This demo intentionally retains every listed font. Production programs should
// reference just the fonts they need, or generate a small Chinese text subset.
const FontDemo demos[] = {
  { &nice_gfx::fonts::GfxTerm8x16, "GfxTerm8x16", false },
#if !defined(__AVR__)
  { &nice_gfx::fonts::Cozette6x13, "Cozette6x13", false },
  { &nice_gfx::fonts::GfxTermBold8x16, "GfxTermBold8x16", false },
  { &nice_gfx::fonts::GfxSansCN16, "GfxSansCN16", true },
  { &nice_gfx::fonts::GfxSansCN24, "GfxSansCN24", true },
  { &nice_gfx::fonts::GfxSerifCN24, "GfxSerifCN24", true },
#endif
};

bool ready = false;
size_t selected = 0;
uint32_t changedAt = 0;

void showFont() {
  const FontDemo &demo = demos[selected];
  gfx.fillScreen(GFX_BLACK);
  gfx.setTextSize(1);
  gfx.setTextColor(GFX_CYAN, GFX_BLACK);
  gfx.setFont(nice_gfx::fonts::GfxTerm8x16);
  gfx.drawString(demo.name, 8, 12);
  gfx.setFont(*demo.font);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  gfx.setCursor(8, 44);
  gfx.println("ABC abc");
  gfx.println("01234567");
  if (demo.chinese) {
    gfx.println("中文界面");
    gfx.println("温度25℃");
    gfx.println("设备正常");
  }
}

void setup() {
  ready = gfx.begin();
  if (!ready) return;
  showFont();
  changedAt = millis();
}

void loop() {
  if (!ready) { delay(10); return; }
  const uint32_t now = millis();
  if (uint32_t(now - changedAt) >= 3000) {
    selected = (selected + 1) % (sizeof(demos) / sizeof(demos[0]));
    showFont();
    changedAt = now;
  }
  delay(1);
}
