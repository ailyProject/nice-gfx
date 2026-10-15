# Font descriptor examples

These paths are retained for existing bookmarks. All sketches now select
Flash-resident `MonoFont` objects with `setFont()`; no font files or build
flags are required.

- Font_Demo_1 and Print_Smooth_Font demonstrate print streams, wrapping,
  colors, backgrounds and integer scaling.
- Font_Demo_2 demonstrates alignment, measurements and padded numeric updates.
- Font_Demo_3 demonstrates reusable Sprites and transparent compositing.
- Font_Demo_4 creates a tightly sized text Sprite, pushes it and releases it.
- Unicode_test demonstrates UTF-8 Chinese text and direct Unicode glyphs.
- Smooth_font_gradient demonstrates transparent text over gradient graphics.
- Smooth_font_reading_Display samples a background color to choose contrast,
  then renders text while preserving the existing graphics.
- SD_Card reads UTF-8 text from `font-demo.txt` on a card. Its font stays in
  Flash. Change `SD_CS` to match your hardware.

Text uses one-bit bitmap glyphs; graphics such as arcs and clock hands can
still use the library's antialiased drawing functions. These examples do not
claim antialiased font rendering.
