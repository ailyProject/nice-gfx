#pragma once
// Font data is selected explicitly with setFont(), without build-time registries.
#if defined(LOAD_GLCD) || defined(LOAD_FONT2) || defined(LOAD_FONT4) || \
    defined(LOAD_FONT6) || defined(LOAD_FONT7) || defined(LOAD_FONT8) || \
    defined(LOAD_FONT8N) || defined(LOAD_GFXFF) || defined(LOAD_RLE) || defined(SMOOTH_FONT) || \
    defined(NICE_GFX_FONT_TOM_THUMB) || defined(NICE_GFX_FONT_PROFONT) || \
    defined(NICE_GFX_FONT_FIXED) || defined(NICE_GFX_FONT_UNIFONT_CN) || defined(NICE_GFX_UNIFONT_CN_HEADER)
#error "Font configuration macros were removed; select a MonoFont with setFont()"
#endif
