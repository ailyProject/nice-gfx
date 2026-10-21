// NICE_GFX graphics implementation. See LICENSE-Bodmer.txt for contributor notices.

#include "Gfx.h"
#include "Clip.h"
#include "Raster.h"

// Portable integer formatting, including LONG_MIN. No core-specific ltoa().
static char *niceGfxLongToString(long value, char *buffer, int base)
{
  if (buffer == nullptr || base < 2 || base > 36) return buffer;

  const bool negative = value < 0 && base == 10;
  unsigned long magnitude = negative
                              ? 0UL - static_cast<unsigned long>(value)
                              : static_cast<unsigned long>(value);
  char *cursor = buffer;
  do {
    const unsigned long digit = magnitude % static_cast<unsigned long>(base);
    *cursor++ = static_cast<char>(digit < 10 ? '0' + digit
                                             : 'a' + (digit - 10));
    magnitude /= static_cast<unsigned long>(base);
  } while (magnitude != 0);
  if (negative) *cursor++ = '-';
  *cursor = '\0';

  for (char *left = buffer, *right = cursor - 1; left < right;
       ++left, --right) {
    const char temp = *left;
    *left = *right;
    *right = temp;
  }
  return buffer;
}

#include "Display.inc"
#include "Touch.inc"

#include "Transactions.inc"
#include "Viewport.inc"
#include "Device.inc"
#include "Images.inc"
#include "Icons.inc"
#include "Shapes.inc"
#include "Monospace.inc"
#include "TextLayout.inc"
#include "PixelStream.inc"
#include "Antialias.inc"
#include "Color.inc"
#include "TextRender.inc"
#include "Diagnostics.inc"
