// All bundled fonts are declared here. Only referenced data enters the firmware.
#include <NICE_GFX.h>

// Change pins for your board.
ILI9341_240x320_SPI gfx(5, 2, 4, 18, 23, 19);
#if !defined(__AVR__)
GFX_Sprite label(&gfx);
#endif

void setup() {
  if (!gfx.begin()) return;
  gfx.fillScreen(GFX_BLACK);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  int y = 8;
  // This application deliberately references all three English fonts.
  // Remove an entry if your application does not use it.
  const nice_gfx::MonoFont *english[] = {
    &nice_gfx::fonts::TomThumb4x6,
    &nice_gfx::fonts::ProFont6x12,
    &nice_gfx::fonts::Fixed6x13
  };
  for (const nice_gfx::MonoFont *font : english) {
    gfx.setFont(font);
    gfx.drawString("ABC abc 0123: iW", 8, y);
    y += gfx.fontHeight() + 8;
  }
#if !defined(__AVR__)
  // Full Chinese needs about 250 KiB Flash. AVR can use a generated subset.
  gfx.setFont(nice_gfx::fonts::UnifontCN16);
  gfx.drawString("中文显示：温度 25℃", 8, y);
  y += gfx.fontHeight() + 8;
  label.setColorDepth(16);
  if (label.createSprite(224, 32)) {
    label.fillSprite(GFX_NAVY);
    label.setFont(nice_gfx::fonts::UnifontCN16);
    label.setTextColor(GFX_WHITE, GFX_NAVY);
    label.setCursor(0, 8);
    label.print("设备运行正常");
    label.pushSprite(8, y);
  }
#endif
}

void loop() {}
