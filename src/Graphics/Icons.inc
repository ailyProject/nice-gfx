#include "../Fonts/Text.h"

// Icons and text consume the same coverage reconstruction and target sink.
// The descriptor keeps row padding explicit and never allocates a framebuffer.
static bool gfxIconGlyph(const nice_gfx::MonoIcon &icon, uint8_t scale,
                         nice_gfx::TextGlyph &glyph) {
  const bool gray = icon.bitsPerPixel == 4;
  const uint16_t stride = gray ? (icon.width + 1) / 2 : (icon.width + 7) / 8;
  if (!icon.bitmap || !icon.width || !icon.height || icon.width > 128 || icon.height > 128 ||
      (icon.bitsPerPixel != 0 && icon.bitsPerPixel != 1 && !gray) ||
      icon.bytes != uint32_t(stride) * icon.height || !scale || scale > 8) return false;
  glyph = {icon.bitmap, icon.width, icon.height, stride,
           gray ? nice_gfx::TextEncoding::Coverage4 : nice_gfx::TextEncoding::Rows};
  return true;
}

void NICE_GFX::drawIcon(int32_t x, int32_t y, const nice_gfx::MonoIcon &icon,
                        uint16_t color, uint8_t scale) {
  nice_gfx::TextGlyph glyph;
  if (!gfxIconGlyph(icon, scale, glyph)) { _runtimeStatus = nice_gfx::Status::InvalidArgument; return; }
  renderTextGlyph(glyph, x, y, color, color, scale, false);
}

void NICE_GFX::drawIcon(int32_t x, int32_t y, const nice_gfx::MonoIcon &icon,
                        uint16_t foreground, uint16_t background, uint8_t scale) {
  nice_gfx::TextGlyph glyph;
  if (!gfxIconGlyph(icon, scale, glyph)) { _runtimeStatus = nice_gfx::Status::InvalidArgument; return; }
  renderTextGlyph(glyph, x, y, foreground, background, scale, true);
}
