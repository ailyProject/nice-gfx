#include "../Core/Math.h"

void NICE_GFX::setFont(const nice_gfx::MonoFont *font)
{
  _monoFont = font;
  decoderState = 0;
  decoderBuffer = 0;
}

size_t NICE_GFX::write(uint8_t utf8)
{
  if (_vpOoB || !_monoFont) return 1;
  const uint16_t code = decodeUTF8(utf8);
  if (!code || utf8 == '\r') return 1;
  const int32_t lineHeight = _monoFont->height * textsize;
  if (code == '\n') {
    cursor_x = 0;
    cursor_y += lineHeight;
  } else if (code >= 32 && code != 127) {
    const int32_t advance = nice_gfx::detail::monoAdvance(*_monoFont, code) * textsize;
    if (textwrapX && cursor_x + advance > width()) {
      cursor_x = 0;
      cursor_y += lineHeight;
    }
    if (textwrapY && cursor_y >= height()) cursor_y = 0;
    cursor_x += drawChar(code, cursor_x, cursor_y);
  }
  return 1;
}

// Displays stream an opaque unscaled glyph through one hardware window;
// clipped, transparent and Sprite targets consume the same horizontal runs.
void NICE_GFX::renderTextGlyph(const nice_gfx::TextGlyph &glyph, int32_t x, int32_t y,
    uint32_t foreground, uint32_t background, uint8_t scale, bool opaque,
    uint16_t targetWidth, uint16_t targetHeight)
{
  if (_vpOoB || !scale || !glyph.width || !glyph.height || !glyph.data) return;
  const uint16_t glyphWidth = targetWidth ? targetWidth : glyph.width;
  const uint16_t glyphHeight = targetHeight ? targetHeight : glyph.height;
  const bool gray = glyph.encoding == nice_gfx::TextEncoding::Coverage4;
  const bool blendCoverage = _antialiasing && supportsTextCoverage();
  const bool resample = glyphWidth != glyph.width || glyphHeight != glyph.height;
  const uint16_t rasterWidth = blendCoverage ? uint16_t(glyphWidth * scale) : glyphWidth;
  const uint16_t rasterHeight = blendCoverage ? uint16_t(glyphHeight * scale) : glyphHeight;
  const uint8_t rasterScale = blendCoverage ? 1 : scale;
  const int64_t left = int64_t(x) + _xDatum, top = int64_t(y) + _yDatum;
  const int64_t right = left + uint32_t(glyphWidth) * scale;
  const int64_t bottom = top + uint32_t(glyphHeight) * scale;
  int32_t clipLeft,clipTop,clipRight,clipBottom;
  if (!textRenderClip(clipLeft,clipTop,clipRight,clipBottom) ||
      left >= clipRight || top >= clipBottom || right <= clipLeft || bottom <= clipTop) return;
  // Include partially visible enlarged pixels, then retain exact absolute
  // clipping in emit. Origin cancellation and scale arithmetic stay 64-bit.
  const int64_t visibleLeft=left>clipLeft?left:clipLeft, visibleTop=top>clipTop?top:clipTop;
  const int64_t visibleRight=right<clipRight?right:clipRight, visibleBottom=bottom<clipBottom?bottom:clipBottom;
  const nice_gfx::TextRasterClip rasterClip={
    uint16_t((visibleLeft-left)/rasterScale),uint16_t((visibleTop-top)/rasterScale),
    uint16_t((visibleRight-left+rasterScale-1)/rasterScale),uint16_t((visibleBottom-top+rasterScale-1)/rasterScale)};
  const bool cropped=rasterClip.left || rasterClip.top || rasterClip.right!=rasterWidth || rasterClip.bottom!=rasterHeight;
  WriteScope write(*this);
  const auto read = [](const uint8_t *address) { return uint8_t(pgm_read_byte(address)); };
  const bool stream = _panel && opaque && scale == 1 && !cropped;
  if (stream)
    setWindow(int32_t(left), int32_t(top), int32_t(right - 1), int32_t(bottom - 1));
  // A visible span can have an unrepresentable local coordinate when a large
  // origin cancels x/y. Render clipped absolute spans with a neutral origin;
  // restore the caller's origin even if the output strategy returns early.
  struct AbsoluteTextOrigin {
    int32_t &x, &y;
    const int32_t savedX, savedY;
    const bool active;
    AbsoluteTextOrigin(int32_t &dx, int32_t &dy, bool enabled)
      : x(dx), y(dy), savedX(dx), savedY(dy), active(enabled) {
      if (active) x = y = 0;
    }
    ~AbsoluteTextOrigin() { if (active) { x = savedX; y = savedY; } }
  } origin(_xDatum, _yDatum, !stream);
  uint16_t palette[16];
  if (blendCoverage && opaque) {
    palette[0] = uint16_t(background);
    palette[15] = uint16_t(foreground);
    if (gray) {
      for (uint8_t level = 1; level < 15; ++level)
        palette[level] = nice_gfx::textCoverageColor(uint16_t(foreground), uint16_t(background), level);
    } else {
      // Binary EPX combines four 0/15 samples, producing only 0,4,8,11,15,
      // including integer enlargement and nearest-neighbor fallback resizing.
      palette[4] = nice_gfx::textCoverageColor(uint16_t(foreground), uint16_t(background), 4);
      palette[8] = nice_gfx::textCoverageColor(uint16_t(foreground), uint16_t(background), 8);
      palette[11] = nice_gfx::textCoverageColor(uint16_t(foreground), uint16_t(background), 11);
    }
  }
  bool unavailable = false, failed = false;
  const auto emit = [&](uint32_t position, uint32_t length, uint8_t coverage) {
    if (failed) return;
    if (!blendCoverage) coverage = coverage >= 8 ? 15 : 0;
    const uint16_t color = blendCoverage && opaque ? palette[coverage] : uint16_t(coverage ? foreground : background);
    if (stream) {
      if (_runtimeStatus == nice_gfx::Status::Ok)
        _runtimeStatus = _panel->append(color, length);
      return;
    }
    if (!coverage && !opaque) return;
    while (length) {
      const uint32_t sx = position % rasterWidth, sy = position / rasterWidth;
      uint32_t width = rasterWidth - sx;
      if (width > length) width = length;
      const int64_t spanX = left + int64_t(sx) * rasterScale;
      const int64_t spanY = top + int64_t(sy) * rasterScale;
      const int64_t x0 = spanX < clipLeft ? clipLeft : spanX;
      const int64_t y0 = spanY < clipTop ? clipTop : spanY;
      const int64_t x1 = spanX + width * rasterScale > clipRight ? clipRight : spanX + width * rasterScale;
      const int64_t y1 = spanY + rasterScale > clipBottom ? clipBottom : spanY + rasterScale;
      if (x1 > x0 && y1 > y0) {
        if (textRenderSpan(int32_t(x0), int32_t(y0), int32_t(x1 - x0), int32_t(y1 - y0),
                           color, opaque ? 15 : coverage)) {
          // The target owns both the clipped span and its RGB565 underlay.
        } else if (opaque || coverage == 15) {
          fillRect(int32_t(x0), int32_t(y0), int32_t(x1 - x0), int32_t(y1 - y0), color);
        } else if (unavailable) {
          if (coverage >= 8) fillRect(int32_t(x0), int32_t(y0), int32_t(x1 - x0), int32_t(y1 - y0), foreground);
        } else {
          // Each enlarged pixel blends its own underlay. A gradient or rounded
          // UI background must not be replaced with a guessed flat color.
          for (int32_t py = int32_t(y0); py < int32_t(y1) && !failed && !unavailable; ++py) {
            for (int32_t px = int32_t(x0); px < int32_t(x1); ++px) {
              const uint16_t underlay = readPixel(px, py);
              if (_panel && _runtimeStatus != nice_gfx::Status::Ok) {
                if (_runtimeStatus == nice_gfx::Status::Unsupported) {
                  // Write-only panels cannot blend transparent text. Retain a
                  // crisp 1bpp silhouette; explicit fg/bg enables AA instead.
                  unavailable = true;
                  if (coverage >= 8) fillRect(int32_t(x0), int32_t(y0), int32_t(x1 - x0), int32_t(y1 - y0), foreground);
                } else failed = true;
                break;
              }
              drawPixel(px, py, nice_gfx::textCoverageColor(uint16_t(foreground), underlay, coverage));
            }
          }
        }
      }
      position += width;
      length -= width;
    }
  };
  // Clipped native NoAA masks retain their packed-byte/nibble scanner. The
  // reconstruction sampler's neighbor/bounds machinery is only needed for
  // antialiasing or a fallback with different source dimensions.
  if (!blendCoverage && cropped && !resample) {
    if (gray) nice_gfx::textCoverageRuns(glyph,glyphWidth,glyphHeight,read,emit,&rasterClip,true);
    else nice_gfx::textRuns(glyph,read,[&](uint32_t position,uint32_t length,bool ink){emit(position,length,ink?15:0);},&rasterClip);
  }
  else if (blendCoverage || cropped) nice_gfx::textRasterRuns(glyph, rasterWidth, rasterHeight, blendCoverage, read, emit, glyphWidth, glyphHeight,cropped?&rasterClip:nullptr);
  else if (gray || resample) nice_gfx::textCoverageRuns(glyph, glyphWidth, glyphHeight, read, emit);
  else nice_gfx::textRuns(glyph, read, [&](uint32_t position, uint32_t length, bool ink) {
    emit(position, length, ink ? 15 : 0);
  });
}

// UTF-8 decoding belongs to write/drawString; drawChar receives a code point.
int16_t NICE_GFX::drawChar(uint16_t uniCode, int32_t x, int32_t y)
{
  if (!_monoFont) return 0;
  const nice_gfx::MonoFont *mono = _monoFont;
  if (const nice_gfx::MonoFont *fallback = nice_gfx::detail::monoFallback(*mono, uniCode)) {
    // Nearest-neighbor resampling to the primary font's square em keeps CJK
    // text the same height at every integer scale, including small fonts.
    const uint8_t *data = nice_gfx::detail::monoGlyphExact(*fallback, uniCode);
    const nice_gfx::TextGlyph image = {data, fallback->width, fallback->height,
      nice_gfx::detail::monoStride(*fallback), fallback->bitsPerPixel == 4 ?
      nice_gfx::TextEncoding::Coverage4 : nice_gfx::TextEncoding::Rows};
    renderTextGlyph(image, x, y, textcolor, textbgcolor, textsize, textcolor != textbgcolor,
                    mono->height, mono->height);
    return mono->height * textsize;
  }
  const uint8_t *data = nice_gfx::detail::monoGlyph(*mono, uniCode);
  if (!data) return 0;
  const nice_gfx::TextGlyph image = {data, mono->width, mono->height,
    nice_gfx::detail::monoStride(*mono), mono->bitsPerPixel == 4 ?
    nice_gfx::TextEncoding::Coverage4 : nice_gfx::TextEncoding::Rows};
  renderTextGlyph(image, x, y, textcolor, textbgcolor, textsize, textcolor != textbgcolor);
  return mono->width * textsize;
}

#if NICE_GFX_NATIVE_IDF
int16_t NICE_GFX::drawString(const std::string &string, int32_t x, int32_t y)
#else
int16_t NICE_GFX::drawString(const String &string, int32_t x, int32_t y)
#endif
{ return drawString(string.c_str(), x, y); }

int16_t NICE_GFX::drawString(const char *string, int32_t poX, int32_t poY)
{
  if (!string || !_monoFont) return 0;
  int16_t sumX = 0;
  uint8_t padding = 1;
  const int32_t baseline = _monoFont->baseline * textsize;
  const uint16_t cwidth = textWidth(string), cheight = fontHeight();
  if (textdatum || padX) {

    switch(textdatum) {
      case TC_DATUM:
        poX -= cwidth/2;
        padding += 1;
        break;
      case TR_DATUM:
        poX -= cwidth;
        padding += 2;
        break;
      case ML_DATUM:
        poY -= cheight/2;
        //padding += 0;
        break;
      case MC_DATUM:
        poX -= cwidth/2;
        poY -= cheight/2;
        padding += 1;
        break;
      case MR_DATUM:
        poX -= cwidth;
        poY -= cheight/2;
        padding += 2;
        break;
      case BL_DATUM:
        poY -= cheight;
        //padding += 0;
        break;
      case BC_DATUM:
        poX -= cwidth/2;
        poY -= cheight;
        padding += 1;
        break;
      case BR_DATUM:
        poX -= cwidth;
        poY -= cheight;
        padding += 2;
        break;
      case L_BASELINE:
        poY -= baseline;
        //padding += 0;
        break;
      case C_BASELINE:
        poX -= cwidth/2;
        poY -= baseline;
        padding += 1;
        break;
      case R_BASELINE:
        poX -= cwidth;
        poY -= baseline;
        padding += 2;
        break;
    }
  }
  const uint16_t length = strlen(string);
  uint16_t n = 0;
  while (n < length) {
    const uint16_t code = decodeUTF8((uint8_t *)string, &n, length - n);
    sumX += drawChar(code, poX + sumX, poY);
  }
  if (padX > cwidth && textcolor != textbgcolor) {
    int32_t edge = poX + cwidth;
    switch (padding) {
      case 1: fillRect(edge, poY, padX - cwidth, cheight, textbgcolor); break;
      case 2:
        fillRect(edge, poY, (padX - cwidth) >> 1, cheight, textbgcolor);
        fillRect(poX - ((padX - cwidth) >> 1), poY, (padX - cwidth) >> 1, cheight, textbgcolor);
        break;
      case 3:
        if (edge > padX) edge = padX;
        fillRect(poX + cwidth - edge, poY, edge - cwidth, cheight, textbgcolor);
        break;
    }
  }
  return sumX;
}

int16_t NICE_GFX::drawNumber(long value, int32_t x, int32_t y)
{
  char str[3 * sizeof(long) + 3];
  niceGfxLongToString(value, str, 10);
  return drawString(str, x, y);
}

int16_t NICE_GFX::drawFloat(float floatNumber, uint8_t dp, int32_t poX, int32_t poY)
{
  if (!nice_gfx::detail::finiteValue(floatNumber)) return drawString("...", poX, poY);
  char str[14];               // Array to contain decimal string
  uint8_t ptr = 0;            // Initialise pointer for array
  int8_t  digits = 1;         // Count the digits to avoid array overflow
  float rounding = 0.5;       // Round up down delta
  bool negative = false;

  if (dp > 7) dp = 7; // Limit the size of decimal portion

  // Adjust the rounding value
  for (uint8_t i = 0; i < dp; ++i) rounding /= 10.0;

  if (floatNumber < -rounding) {   // add sign, avoid adding - sign to 0.0!
    str[ptr++] = '-'; // Negative number
    str[ptr] = 0; // Put a null in the array as a precaution
    digits = 0;   // Set digits to 0 to compensate so pointer value can be used later
    floatNumber = -floatNumber; // Make positive
    negative = true;
  }

  floatNumber += rounding; // Round up or down

  if (dp == 0) {
    if (negative) floatNumber = -floatNumber;
    // Use an exclusive power-of-two upper bound: converting LONG_MAX to float
    // can round it upward, making a <= LONG_MAX check incorrectly accept it.
    const double limit = double(~0UL >> 1) + 1.0;
    if (double(floatNumber) >= limit || double(floatNumber) < -limit)
      return drawString("...", poX, poY);
    return drawNumber((long)floatNumber, poX, poY);
  }

  // For error put ... in string and return (all NICE_GFX library fonts contain . character)
  if (floatNumber >= 2147483647) {
    strcpy(str, "...");
    return drawString(str, poX, poY);
  }
  // No chance of overflow from here on

  // Get integer part
  uint32_t temp = (uint32_t)floatNumber;

  // Put integer part into array
  niceGfxLongToString(temp, str + ptr, 10);

  // Find out where the null is to get the digit count loaded
  while ((uint8_t)str[ptr] != 0) ptr++; // Move the pointer along
  digits += ptr;                  // Count the digits

  str[ptr++] = '.'; // Add decimal point
  str[ptr] = '0';   // Add a dummy zero
  str[ptr + 1] = 0; // Add a null but don't increment pointer so it can be overwritten

  // Get the decimal portion
  floatNumber = floatNumber - temp;

  // Get decimal digits one by one and put in array
  // Limit digit count so we don't get a false sense of resolution
  uint8_t i = 0;
  while ((i < dp) && (digits < 9)) { // while (i < dp) for no limit but array size must be increased
    i++;
    floatNumber *= 10;       // for the next decimal
    temp = floatNumber;      // get the decimal
    niceGfxLongToString(temp, str + ptr, 10);
    ptr++; digits++;         // Increment pointer and digits count
    floatNumber -= temp;     // Remove that digit
  }

  // Finally we can plot the string and return pixel length
  return drawString(str, poX, poY);
}
