#include "../Core/Utf8.h"

/***************************************************************************************
** Function name:           color565
** Description:             convert three 8-bit RGB levels to a 16-bit colour value
***************************************************************************************/
uint16_t NICE_GFX::color565(uint8_t r, uint8_t g, uint8_t b)
{
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}


/***************************************************************************************
** Function name:           color16to8
** Description:             convert 16-bit colour to an 8-bit 332 RGB colour value
***************************************************************************************/
uint8_t NICE_GFX::color16to8(uint16_t c)
{
  return ((c & 0xE000)>>8) | ((c & 0x0700)>>6) | ((c & 0x0018)>>3);
}


/***************************************************************************************
** Function name:           color8to16
** Description:             convert 8-bit colour to a 16-bit 565 colour value
***************************************************************************************/
uint16_t NICE_GFX::color8to16(uint8_t color)
{
  uint8_t  blue[] = {0, 11, 21, 31}; // blue 2 to 5-bit colour lookup table
  uint16_t color16 = 0;

  //        =====Green=====     ===============Red==============
  color16  = (color & 0x1C)<<6 | (color & 0xC0)<<5 | (color & 0xE0)<<8;
  //        =====Green=====    =======Blue======
  color16 |= (color & 0x1C)<<3 | blue[color & 0x03];

  return color16;
}

/***************************************************************************************
** Function name:           color16to24
** Description:             convert 16-bit colour to a 24-bit 888 colour value
***************************************************************************************/
uint32_t NICE_GFX::color16to24(uint16_t color565)
{
  uint8_t r = (color565 >> 8) & 0xF8; r |= (r >> 5);
  uint8_t g = (color565 >> 3) & 0xFC; g |= (g >> 6);
  uint8_t b = (color565 << 3) & 0xF8; b |= (b >> 5);

  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | ((uint32_t)b << 0);
}

/***************************************************************************************
** Function name:           color24to16
** Description:             convert 24-bit colour to a 16-bit 565 colour value
***************************************************************************************/
uint32_t NICE_GFX::color24to16(uint32_t color888)
{
  uint16_t r = (color888 >> 8) & 0xF800;
  uint16_t g = (color888 >> 5) & 0x07E0;
  uint16_t b = (color888 >> 3) & 0x001F;

  return (r | g | b);
}

/***************************************************************************************
** Function name:           invertDisplay
** Description:             invert the display colours i = 1 invert, i = 0 normal
***************************************************************************************/
void NICE_GFX::invertDisplay(bool i)
{
  if (_panel) _runtimeStatus = _panel->invert(i);
}


/**************************************************************************
** Function name:           setAttribute
** Description:             Sets a control parameter of an attribute
**************************************************************************/
void NICE_GFX::setAttribute(uint8_t attr_id, uint8_t param) {
    switch (attr_id) {
        case UTF8_SWITCH:
            _utf8  = param;
            decoderState = 0;
            break;
        case PSRAM_ENABLE:
            _psram_enable = param && nice_gfx::platform::psramFound();
            break;
    }
}


/**************************************************************************
** Function name:           getAttribute
** Description:             Get value of an attribute (control parameter)
**************************************************************************/
uint8_t NICE_GFX::getAttribute(uint8_t attr_id) {
    switch (attr_id) {
        case UTF8_SWITCH: // ON/OFF control of UTF-8 decoding
            return _utf8;
        case PSRAM_ENABLE:
            return _psram_enable;
    }

    return false;
}

/***************************************************************************************
** Function name:           decodeUTF8
** Description:             Shared UTF-8 decoder; byte mode is explicitly selectable
*************************************************************************************x*/
uint16_t NICE_GFX::decodeUTF8(uint8_t c)
{
  if (!_utf8) return c;
  return nice_gfx::utf8Byte(c, decoderState, decoderBuffer);
}

uint16_t NICE_GFX::decodeUTF8(uint8_t *buf, uint16_t *index, uint16_t remaining)
{
  if (!buf || !index || !remaining) return 0;
  if (!_utf8) return buf[(*index)++];
  return nice_gfx::utf8Next(buf, *index, remaining);
}


/***************************************************************************************
** Function name:           alphaBlend
** Description:             Blend 16bit foreground and background
*************************************************************************************x*/
uint16_t NICE_GFX::alphaBlend(uint8_t alpha, uint16_t fgc, uint16_t bgc)
{
  // Split out and blend 5-bit red and blue channels
  uint32_t rxb = bgc & 0xF81F;
  rxb += ((fgc & 0xF81F) - rxb) * (alpha >> 2) >> 6;
  // Split out and blend 6-bit green channel
  uint32_t xgx = bgc & 0x07E0;
  xgx += ((fgc & 0x07E0) - xgx) * alpha >> 8;
  // Recombine channels
  return (rxb & 0xF81F) | (xgx & 0x07E0);
}

/***************************************************************************************
** Function name:           alphaBlend
** Description:             Blend 16bit foreground and background with dither
*************************************************************************************x*/
uint16_t NICE_GFX::alphaBlend(uint8_t alpha, uint16_t fgc, uint16_t bgc, uint8_t dither)
{
  if (dither) {
    int16_t alphaDither = (int16_t)alpha - dither + nice_gfx::platform::random(2*dither+1); // +/-4 randomised
    alpha = (uint8_t)alphaDither;
    if (alphaDither <  0) alpha = 0;
    if (alphaDither >255) alpha = 255;
  }

  return alphaBlend(alpha, fgc, bgc);
}

/***************************************************************************************
** Function name:           alphaBlend
** Description:             Blend 24bit foreground and background with optional dither
*************************************************************************************x*/
uint32_t NICE_GFX::alphaBlend24(uint8_t alpha, uint32_t fgc, uint32_t bgc, uint8_t dither)
{

  if (dither) {
    int16_t alphaDither = (int16_t)alpha - dither + nice_gfx::platform::random(2*dither+1); // +/-dither randomised
    alpha = (uint8_t)alphaDither;
    if (alphaDither <  0) alpha = 0;
    if (alphaDither >255) alpha = 255;
  }

  uint32_t rxx = bgc & 0xFF0000;
  rxx += ((fgc & 0xFF0000) - rxx) * alpha >> 8;
  uint32_t xgx = bgc & 0x00FF00;
  xgx += ((fgc & 0x00FF00) - xgx) * alpha >> 8;
  uint32_t xxb = bgc & 0x0000FF;
  xxb += ((fgc & 0x0000FF) - xxb) * alpha >> 8;
  return (rxx & 0xFF0000) | (xgx & 0x00FF00) | (xxb & 0x0000FF);
}
