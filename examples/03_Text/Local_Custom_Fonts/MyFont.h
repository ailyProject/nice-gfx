#pragma once
#include <NICE_GFX.h>
// A tiny local font with a sorted Unicode index. Each 6x8 glyph stores one
// byte per row, with the six visible bits on the left. These glyphs are drawn
// for this example; add entries in both arrays to extend the character set.
static const uint16_t localCodepoints[] PROGMEM = { 0x003F, 0x0041, 0x00B5, 0x00E4 };
static const uint8_t localBitmap[] PROGMEM = {
  0x70,0x88,0x08,0x10,0x20,0x00,0x20,0x00, // ?
  0x20,0x50,0x88,0x88,0xF8,0x88,0x88,0x00, // A
  0x00,0x00,0x88,0x88,0x88,0xF0,0x80,0x80, // micro sign
  0x50,0x00,0x70,0x08,0x78,0x88,0x78,0x00  // a with diaeresis
};
static const nice_gfx::MonoFont LocalFont = {
  localBitmap, localCodepoints, 4, 6, 8, 7, nullptr
};
