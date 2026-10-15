#include "../Fonts/Text.h"

/***************************************************************************************
** Function name:           setCursor
** Description:             Set the text cursor x,y position
***************************************************************************************/
void NICE_GFX::setCursor(int16_t x, int16_t y)
{
  cursor_x = x;
  cursor_y = y;
}





/***************************************************************************************
** Function name:           getCursorX
** Description:             Get the text cursor x position
***************************************************************************************/
int16_t NICE_GFX::getCursorX(void)
{
  return cursor_x;
}

/***************************************************************************************
** Function name:           getCursorY
** Description:             Get the text cursor y position
***************************************************************************************/
int16_t NICE_GFX::getCursorY(void)
{
  return cursor_y;
}


/***************************************************************************************
** Function name:           setTextSize
** Description:             Set the text size multiplier
***************************************************************************************/
void NICE_GFX::setTextSize(uint8_t s)
{
  if (s>7) s = 7; // Limit the maximum size multiplier so byte variables can be used for rendering
  textsize = (s > 0) ? s : 1; // Don't allow font size 0
}


/***************************************************************************************
** Function name:           setTextColor
** Description:             Set the font foreground colour (background is transparent)
***************************************************************************************/
void NICE_GFX::setTextColor(uint16_t c)
{
  // For 'transparent' background, we'll set the bg
  // to the same as fg instead of using a flag
  textcolor = textbgcolor = c;
}


/***************************************************************************************
** Function name:           setTextColor
** Description:             Set the font foreground and background colour
***************************************************************************************/
void NICE_GFX::setTextColor(uint16_t c, uint16_t b)
{
  textcolor   = c;
  textbgcolor = b;
}


/***************************************************************************************
** Function name:           setPivot
** Description:             Set the pivot point on the TFT
*************************************************************************************x*/
void NICE_GFX::setPivot(int16_t x, int16_t y)
{
  _xPivot = x;
  _yPivot = y;
}


/***************************************************************************************
** Function name:           getPivotX
** Description:             Get the x pivot position
***************************************************************************************/
int16_t NICE_GFX::getPivotX(void)
{
  return _xPivot;
}


/***************************************************************************************
** Function name:           getPivotY
** Description:             Get the y pivot position
***************************************************************************************/
int16_t NICE_GFX::getPivotY(void)
{
  return _yPivot;
}


/***************************************************************************************
** Function name:           setBitmapColor
** Description:             Set the foreground foreground and background colour
***************************************************************************************/
void NICE_GFX::setBitmapColor(uint16_t c, uint16_t b)
{
  if (c == b) b = ~c;
  bitmap_fg = c;
  bitmap_bg = b;
}


/***************************************************************************************
** Function name:           setTextWrap
** Description:             Define if text should wrap at end of line
***************************************************************************************/
void NICE_GFX::setTextWrap(bool wrapX, bool wrapY)
{
  textwrapX = wrapX;
  textwrapY = wrapY;
}


/***************************************************************************************
** Function name:           setTextDatum
** Description:             Set the text position reference datum
***************************************************************************************/
void NICE_GFX::setTextDatum(uint8_t d)
{
  textdatum = d;
}


/***************************************************************************************
** Function name:           setTextPadding
** Description:             Define padding width (aids erasing old text and numbers)
***************************************************************************************/
void NICE_GFX::setTextPadding(uint16_t x_width)
{
  padX = x_width;
}

/***************************************************************************************
** Function name:           getTextPadding
** Description:             Return the padding width (as used by setTextPadding)
***************************************************************************************/
uint16_t NICE_GFX::getTextPadding(void)
{
  return padX;
}

/***************************************************************************************
** Function name:           getTextDatum
** Description:             Return the text datum value (as used by setTextDatum())
***************************************************************************************/
uint8_t NICE_GFX::getTextDatum(void)
{
  return textdatum;
}


/***************************************************************************************
** Function name:           width
** Description:             Return the pixel width of display (per current rotation)
***************************************************************************************/
// Return the size of the display (per current rotation)
int16_t NICE_GFX::width(void)
{
  if (_vpDatum) return _xWidth;
  return _width;
}


/***************************************************************************************
** Function name:           height
** Description:             Return the pixel height of display (per current rotation)
***************************************************************************************/
int16_t NICE_GFX::height(void)
{
  if (_vpDatum) return _yHeight;
  return _height;
}


// Measurement uses local UTF-8 decoding so a partially received print()
// character is not consumed by textWidth().
#if NICE_GFX_NATIVE_IDF
int16_t NICE_GFX::textWidth(const std::string &string) { return textWidth(string.c_str()); }
#else
int16_t NICE_GFX::textWidth(const String &string) { return textWidth(string.c_str()); }
#endif
int16_t NICE_GFX::textWidth(const char *string)
{
  if (!string || !_monoFont) return 0;
  int32_t width = 0;
  uint16_t n = 0;
  const uint16_t length = strlen(string);
  while (n < length) {
    const uint16_t code = decodeUTF8((uint8_t *)string, &n, length - n);
    if (code >= 32 && code != 127) width += nice_gfx::detail::monoAdvance(*_monoFont, code);
  }
  return width * textsize;
}
int16_t NICE_GFX::fontHeight(void) { return _monoFont ? _monoFont->height * textsize : 0; }
