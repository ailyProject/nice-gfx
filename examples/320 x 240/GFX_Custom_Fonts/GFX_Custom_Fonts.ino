// Font descriptors live in Flash and are selected explicitly with setFont().
// The existing example path is retained; these examples need no font files.
#include <NICE_GFX.h>
#include <Display/Presets/ExampleBoard.h>

ExampleDisplay gfx;

struct FontSample { const nice_gfx::MonoFont *font; const char *name; uint8_t scale; };
const FontSample samples[] = {
  { &nice_gfx::fonts::TomThumb4x6, "TomThumb4x6", 1 },
  { &nice_gfx::fonts::ProFont6x12, "ProFont6x12", 1 },
  { &nice_gfx::fonts::Fixed6x13, "Fixed6x13", 1 },
  { &nice_gfx::fonts::Cozette6x13, "Cozette6x13", 1 },
  { &nice_gfx::fonts::GfxTerm8x16, "GfxTerm8x16", 1 },
  { &nice_gfx::fonts::GfxTermBold8x16, "GfxTermBold8x16", 1 },
  { &nice_gfx::fonts::Scientifica5x11, "Scientifica5x11", 1 },
  { &nice_gfx::fonts::Tamzen7x14, "Tamzen7x14", 1 },
  { &nice_gfx::fonts::TamzenBold8x16, "TamzenBold8x16", 1 },
  { &nice_gfx::fonts::Spleen8x16, "Spleen8x16", 1 },
  { &nice_gfx::fonts::Spleen12x24, "Spleen12x24", 1 },
  { &nice_gfx::fonts::GfxSansLatin16, "GfxSansLatin16", 1 },
  { &nice_gfx::fonts::GfxSansLatin24, "GfxSansLatin24", 1 },
  { &nice_gfx::fonts::Spleen12x24, "Spleen12x24 x2", 2 }
};
void setup() {
  Serial.begin(115200);
  gfx.begin();
  gfx.setRotation(1);
  gfx.setFont(nice_gfx::fonts::GfxTerm8x16);
  gfx.setTextSize(1);
}

void loop() {
  for (const FontSample &sample : samples) {
    gfx.fillScreen(GFX_NAVY);
    gfx.setFont(nice_gfx::fonts::GfxTerm8x16);
    gfx.setTextSize(1);
    gfx.setTextColor(GFX_CYAN, GFX_NAVY);
    gfx.setTextDatum(TC_DATUM);
    gfx.drawString(sample.name, gfx.width() / 2, 8);
    gfx.setFont(*sample.font);
    gfx.setTextSize(sample.scale);
    gfx.setTextColor(GFX_WHITE, GFX_NAVY);
    gfx.drawString("ABC abc", gfx.width() / 2, 40);
    gfx.drawString("012345", gfx.width() / 2, 48 + gfx.fontHeight());
    gfx.setTextPadding(gfx.textWidth("99.99"));
    for (int value = 0; value < 20; ++value) {
      gfx.drawFloat(value / 10.0, 2, gfx.width() / 2, gfx.height() - gfx.fontHeight() - 4);
      delay(30);
    }
    gfx.setTextPadding(0);
    delay(1200);
  }
  gfx.setTextSize(1);
  gfx.setFont(nice_gfx::fonts::GfxTerm8x16);
  const uint8_t datums[] = {TL_DATUM, TC_DATUM, TR_DATUM, ML_DATUM, MC_DATUM,
    MR_DATUM, BL_DATUM, BC_DATUM, BR_DATUM, L_BASELINE, C_BASELINE, R_BASELINE};
  for (uint8_t datum : datums) {
    gfx.fillScreen(GFX_NAVY);
    const int x = gfx.width() / 2, y = gfx.height() / 2;
    gfx.drawFastHLine(0, y, gfx.width(), GFX_DARKGREY);
    gfx.drawFastVLine(x, 0, gfx.height(), GFX_DARKGREY);
    gfx.setTextDatum(datum);
    gfx.drawString("Datum", x, y);
    delay(500);
  }
  gfx.setTextDatum(TL_DATUM);
}
