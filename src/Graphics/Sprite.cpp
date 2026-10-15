#include "Sprite.h"
#include "Clip.h"
#include "Raster.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/**************************************************************************************
// The following class creates Sprites in RAM, graphics can then be drawn in the Sprite
// and rendered quickly onto the TFT screen. The class inherits the graphics functions
// from the NICE_GFX class. Some functions are overridden by this class so that the
// graphics are written to the Sprite rather than the TFT.
// Coded by Bodmer, see license file in root folder
***************************************************************************************/
#if defined(ESP32) || NICE_GFX_NATIVE_IDF
#include <esp_heap_caps.h>
#endif

namespace {
constexpr size_t spriteSpanPixels = 128;
// Row copies also support an image backed by this sprite's active framebuffer.
// memmove protects horizontal overlap; descending rows protect vertical overlap.
void spriteCopyRows(uint8_t *dst,size_t dstStride,const uint8_t *src,size_t srcStride,
                    size_t bytes,size_t rows,bool swapWords=false) {
  const uintptr_t from=reinterpret_cast<uintptr_t>(src),to=reinterpret_cast<uintptr_t>(dst);
  const bool backwards=to>from && to-from<(rows-1)*srcStride+bytes;
  for(size_t row=0;row<rows;++row) {
    const size_t i=backwards?rows-1-row:row;
    uint8_t *d=dst+i*dstStride;
    memmove(d,src+i*srcStride,bytes);
    if(swapWords)for(size_t x=0;x<bytes;x+=2){const uint8_t v=d[x];d[x]=d[x+1];d[x+1]=v;}
  }
}
}
/***************************************************************************************
// Color bytes are swapped when writing to RAM, this introduces a small overhead but
// there is a nett performance gain by using swapped bytes.
***************************************************************************************/

/***************************************************************************************
** Function name:           GFX_Sprite
** Description:             Class constructor
***************************************************************************************/
GFX_Sprite::GFX_Sprite(NICE_GFX *gfx) : NICE_GFX(0, 0)
{
  _gfx = gfx;     // Pointer to gfx class so we can call member functions

  _iwidth    = 0; // Initialise width and height to 0 (it does not exist yet)
  _iheight   = 0;
  _bpp = 16;
  _swapBytes = false;   // Do not swap pushImage colour bytes by default

  _created = false;
  _vpOoB   = true;

  _xs = 0;  // window bounds for pushColor
  _ys = 0;
  _xe = 0;
  _ye = 0;

  _xptr = 0; // pushColor coordinate
  _yptr = 0;

  _colorMap = nullptr;
  _img = nullptr;
  _img4 = _img8 = _img8_1 = _img8_2 = nullptr;

  _psram_enable = true;
  
}


/***************************************************************************************
** Function name:           createSprite
** Description:             Create a sprite (bitmap) of defined width and height
***************************************************************************************/
// cast returned value to (uint8_t*) for 8-bit or (uint16_t*) for 16-bit colours
void* GFX_Sprite::createSprite(int16_t w, int16_t h, uint8_t frames)
{

  if ( _created ) return _img8_1;

  if ( w < 1 || h < 1 ) return nullptr;

  _iwidth  = _dwidth  = _bitwidth = w;
  _iheight = _dheight = h;

  cursor_x = 0;
  cursor_y = 0;

  // Default scroll rectangle and gap fill colour
  _sx = 0;
  _sy = 0;
  _sw = w;
  _sh = h;
  _scolor = GFX_BLACK;

  _img8   = (uint8_t*) callocSprite(w, h, frames);
  _img8_1 = _img8;
  _img8_2 = _img8;
  _img    = (uint16_t*) _img8;
  _img4   = _img8;

  if (!_img8) return nullptr;

  // ESP32 only 16bpp check
  //if (esp_ptr_dma_capable(_img8_1)) Serial.println("DMA capable Sprite pointer _img8_1");
  //else Serial.println("Not a DMA capable Sprite pointer _img8_1");
  //if (esp_ptr_dma_capable(_img8_2)) Serial.println("DMA capable Sprite pointer _img8_2");
  //else Serial.println("Not a DMA capable Sprite pointer _img8_2");

  if (frames > 1) {
    size_t frameBytes;
    if (_bpp == 16) frameBytes = (size_t(w) * h + 1) * sizeof(uint16_t);
    else if (_bpp == 8) frameBytes = size_t(w) * h + 1;
    else if (_bpp == 4) frameBytes = size_t(_iwidth / 2) * h + 1;
    else frameBytes = size_t(_bitwidth / 8) * h + 1;
    _img8_2 = _img8 + frameBytes;
  }

  if (_img8)
  {
    _created = true;
    if ( (_bpp == 4) && (_colorMap == nullptr)) createPalette(default_4bit_palette);
    if (_bpp == 4 && !_colorMap) { deleteSprite(); return nullptr; }

    rotation = 0;
    setViewport(0, 0, _dwidth, _dheight);
    setPivot(_iwidth/2, _iheight/2);
    return _img8_1;
  }

  return nullptr;
}


/***************************************************************************************
** Function name:           getPointer
** Description:             Returns pointer to start of sprite memory area
***************************************************************************************/
void* GFX_Sprite::getPointer(void)
{
  if (!_created) return nullptr;
  return _img8_1;
}


/***************************************************************************************
** Function name:           created
** Description:             Returns true if sprite has been created
***************************************************************************************/
bool GFX_Sprite::created(void)
{
  return _created;
}


/***************************************************************************************
** Function name:           ~GFX_Sprite
** Description:             Class destructor
***************************************************************************************/
GFX_Sprite::~GFX_Sprite(void)
{
  deleteSprite();

}


/***************************************************************************************
** Function name:           callocSprite
** Description:             Allocate a memory area for the Sprite and return pointer
***************************************************************************************/
void* GFX_Sprite::callocSprite(int16_t w, int16_t h, uint8_t frames)
{
  if(w<1 || h<1) return nullptr;
  frames=frames>2?2:frames<1?1:frames;
  size_t rowBytes=size_t(w), guardBytes=1;
  if(_bpp==16) {rowBytes*=2;guardBytes=2;}
  else if(_bpp==4) {_iwidth=(int32_t(w)+1)&~1;rowBytes=size_t(_iwidth)/2;}
  else if(_bpp==1) {_bitwidth=_iwidth=(int32_t(w)+7)&~7;rowBytes=size_t(_bitwidth)/8;}
  if(rowBytes>(SIZE_MAX-guardBytes)/size_t(h)) return nullptr;
  const size_t frameBytes=rowBytes*size_t(h)+guardBytes;
  if(frameBytes>SIZE_MAX/frames) return nullptr;
  const size_t bytes=frameBytes*frames;
  if(nice_gfx::platform::psramFound() && _psram_enable && (_bpp!=16 || !_gfx || !_gfx->DMA_Enabled))
    return nice_gfx::platform::callocPsram(bytes);
#if defined(ESP32) || NICE_GFX_NATIVE_IDF
  // calloc() can still select external RAM when ESP-IDF's PSRAM malloc policy
  // is enabled. Honor PSRAM_ENABLE=false (and the 16-bit DMA requirement)
  // explicitly, so the transport can use these pixels directly with DMA.
  if (!_psram_enable || (_bpp == 16 && _gfx && _gfx->DMA_Enabled))
    return heap_caps_calloc(bytes, 1, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#endif
  return calloc(bytes,1);
}


/***************************************************************************************
** Function name:           createPalette (from RAM array)
** Description:             Set a palette for a 4-bit per pixel sprite
***************************************************************************************/
void GFX_Sprite::createPalette(uint16_t colorMap[], uint8_t colors)
{
  if (!_created) return;

  if (colorMap == nullptr)
  {
    // Create a color map using the default FLASH map
    createPalette(default_4bit_palette);
    return;
  }

  // Allocate and clear memory for 16 color map
  if (_colorMap == nullptr) _colorMap = (uint16_t *)calloc(16, sizeof(uint16_t));
  if (!_colorMap) return;

  if (colors > 16) colors = 16;

  // Copy map colors
  for (uint8_t i = 0; i < colors; i++)
  {
    _colorMap[i] = colorMap[i];
  }
}


/***************************************************************************************
** Function name:           createPalette (from FLASH array)
** Description:             Set a palette for a 4-bit per pixel sprite
***************************************************************************************/
void GFX_Sprite::createPalette(const uint16_t colorMap[], uint8_t colors)
{
  if (!_created) return;

  if (colorMap == nullptr)
  {
    // Create a color map using the default FLASH map
    colorMap = default_4bit_palette;
  }

  // Allocate and clear memory for 16 color map
  if (_colorMap == nullptr) _colorMap = (uint16_t *)calloc(16, sizeof(uint16_t));
  if (!_colorMap) return;

  if (colors > 16) colors = 16;

  // Copy map colors
  for (uint8_t i = 0; i < colors; i++)
  {
    _colorMap[i] = pgm_read_word(colorMap++);
  }
}


/***************************************************************************************
** Function name:           frameBuffer
** Description:             For 1 bpp Sprites, select the frame used for graphics
***************************************************************************************/
// Frames are numbered 1 and 2
void* GFX_Sprite::frameBuffer(int8_t f)
{
  if (!_created) return nullptr;

  if ( f == 2 ) _img8 = _img8_2;
  else          _img8 = _img8_1;

  if (_bpp == 16) _img = (uint16_t*)_img8;

  //if (_bpp == 8) _img8 = _img8;

  if (_bpp == 4) _img4 = _img8;

  return _img8;
}


/***************************************************************************************
** Function name:           setColorDepth
** Description:             Set bits per pixel for colour (1, 8 or 16)
***************************************************************************************/
void* GFX_Sprite::setColorDepth(int8_t b)
{
  // Do not re-create the sprite if the colour depth does not change
  if (_bpp == b) return _img8_1;

  // Validate the new colour depth
  if ( b > 8 ) _bpp = 16;  // Bytes per pixel
  else if ( b > 4 ) _bpp = 8;
  else if ( b > 1 ) _bpp = 4;
  else _bpp = 1;

  // Can't change an existing sprite's colour depth so delete and create a new one
  if (_created) {
    deleteSprite();
    return createSprite(_dwidth, _dheight);
  }

  return nullptr;
}


/***************************************************************************************
** Function name:           getColorDepth
** Description:             Get bits per pixel for colour (1, 8 or 16)
***************************************************************************************/
int8_t GFX_Sprite::getColorDepth(void)
{
  if (_created) return _bpp;
  else return 0;
}


/***************************************************************************************
** Function name:           setBitmapColor
** Description:             Set the 1bpp foreground foreground and background colour
***************************************************************************************/
void GFX_Sprite::setBitmapColor(uint16_t c, uint16_t b)
{
  if (c == b) b = ~c;
  bitmap_fg = c; bitmap_bg = b;
  if (_gfx) { _gfx->bitmap_fg = c; _gfx->bitmap_bg = b; }
}


/***************************************************************************************
** Function name:           setPaletteColor
** Description:             Set the 4bpp palette color at the given index
***************************************************************************************/
void GFX_Sprite::setPaletteColor(uint8_t index, uint16_t color)
{
  if (_colorMap == nullptr || index > 15) return; // out of bounds

  _colorMap[index] = color;
}


/***************************************************************************************
** Function name:           getPaletteColor
** Description:             Return the palette color at 4bpp index, or 0 on error.
***************************************************************************************/
uint16_t GFX_Sprite::getPaletteColor(uint8_t index)
{
  if (_colorMap == nullptr || index > 15) return 0; // out of bounds

  return _colorMap[index];
}


/***************************************************************************************
** Function name:           deleteSprite
** Description:             Delete the sprite to free up memory (RAM)
***************************************************************************************/
void GFX_Sprite::deleteSprite(void)
{
  if (_colorMap != nullptr)
  {
    free(_colorMap);
    _colorMap = nullptr;
  }

  if (_created)
  {
    if (_gfx && _gfx->dmaBusy()) _gfx->dmaWait();
    free(_img8_1);
    _img4 = _img8 = _img8_1 = _img8_2 = nullptr;
    _img = nullptr;
    _created = false;
    _vpOoB   = true;  // NICE_GFX class write() uses this to check for valid sprite
  }
}


/***************************************************************************************
** Function name:           pushRotated - Fast fixed point integer maths version
** Description:             Push rotated Sprite to TFT screen
***************************************************************************************/
#define FP_SCALE 10
bool GFX_Sprite::pushRotated(int16_t angle, uint32_t transp)
{
  if ( !_created || !_gfx || _gfx->_vpOoB) return false;

  // Bounding box parameters
  int16_t min_x;
  int16_t min_y;
  int16_t max_x;
  int16_t max_y;

  // Get the bounding box of this rotated source Sprite relative to Sprite pivot
  if ( !getRotatedBounds(angle, &min_x, &min_y, &max_x, &max_y) ) return false;

  uint16_t sline_buffer[spriteSpanPixels];

  int32_t xt = min_x - _gfx->_xPivot;
  int32_t yt = min_y - _gfx->_yPivot;
  uint32_t xe = _dwidth << FP_SCALE;
  uint32_t ye = _dheight << FP_SCALE;
  uint16_t tpcolor = (uint16_t)transp;

  if (transp != 0x00FFFFFF) {
    if (_bpp == 4) tpcolor = _colorMap[transp & 0x0F];
    tpcolor = tpcolor>>8 | tpcolor<<8; // Working with swapped color bytes
  }
  const bool oldSwapBytes=_gfx->getSwapBytes();
  _gfx->setSwapBytes(false);
  _gfx->startWrite(); // Avoid transaction overhead for every gfx pixel

  // Scan destination bounding box and fetch transformed pixels from source Sprite
  for (int32_t y = min_y; y < max_y; y++, yt++) {
    int32_t x = min_x;
    uint32_t xs = uint32_t(int64_t(_cosra) * xt - int64_t(_sinra) * yt + int64_t(_xPivot) * (1 << FP_SCALE) + (1 << (FP_SCALE - 1)));
    uint32_t ys = uint32_t(int64_t(_sinra) * xt + int64_t(_cosra) * yt + int64_t(_yPivot) * (1 << FP_SCALE) + (1 << (FP_SCALE - 1)));

    while ((xs >= xe || ys >= ye) && x < max_x) { x++; xs += _cosra; ys += _sinra; }
    if (x == max_x) continue;

    uint32_t pixel_count = 0;
    do {
      uint32_t rp;
      int32_t xp = xs >> FP_SCALE;
      int32_t yp = ys >> FP_SCALE;
      if (_bpp == 16) {rp = _img[xp + yp * _iwidth]; }
      else { rp = readPixel(xp, yp); rp = (uint16_t)(rp>>8 | rp<<8); }
      if (transp != 0x00FFFFFF && tpcolor == rp) {
        if (pixel_count) {
          // TFT window is already clipped, so this is faster than pushImage()
          _gfx->setWindow(x - pixel_count, y, x - 1, y);
          _gfx->pushPixels(sline_buffer, pixel_count);
          pixel_count = 0;
        }
      }
      else {
        sline_buffer[pixel_count++] = rp;
        if(pixel_count==spriteSpanPixels) {
          _gfx->setWindow(x + 1 - pixel_count,y,x,y);
          _gfx->pushPixels(sline_buffer,pixel_count);
          pixel_count=0;
        }
      }
    } while (++x < max_x && (xs += _cosra) < xe && (ys += _sinra) < ye);
    if (pixel_count) {
      // TFT window is already clipped, so this is faster than pushImage()
      _gfx->setWindow(x - pixel_count, y, x - 1, y);
      _gfx->pushPixels(sline_buffer, pixel_count);
    }
  }

  _gfx->endWrite(); // End transaction
  _gfx->setSwapBytes(oldSwapBytes);

  return true;
}


/***************************************************************************************
** Function name:           pushRotated - Fast fixed point integer maths version
** Description:             Push a rotated copy of the Sprite to another Sprite
***************************************************************************************/
// Not compatible with 4bpp
bool GFX_Sprite::pushRotated(GFX_Sprite *spr, int16_t angle, uint32_t transp)
{
  if ( !_created  || _bpp == 4) return false; // Check this Sprite is created
  if ( !spr || !spr->_created || spr==this || spr->_bpp == 4) return false;

  // Bounding box parameters
  int16_t min_x;
  int16_t min_y;
  int16_t max_x;
  int16_t max_y;

  // Get the bounding box of this rotated source Sprite
  if ( !getRotatedBounds(spr, angle, &min_x, &min_y, &max_x, &max_y) ) return false;

  uint16_t sline_buffer[spriteSpanPixels];

  int32_t xt = min_x - spr->_xPivot;
  int32_t yt = min_y - spr->_yPivot;
  uint32_t xe = _dwidth << FP_SCALE;
  uint32_t ye = _dheight << FP_SCALE;
  uint16_t tpcolor = (uint16_t)transp;
  
  if (transp != 0x00FFFFFF) {
    if (_bpp == 4) tpcolor = _colorMap[transp & 0x0F];
    tpcolor = tpcolor>>8 | tpcolor<<8; // Working with swapped color bytes
  }

  bool oldSwapBytes = spr->getSwapBytes();
  spr->setSwapBytes(false);

  // Scan destination bounding box and fetch transformed pixels from source Sprite
  for (int32_t y = min_y; y < max_y; y++, yt++) {
    int32_t x = min_x;
    uint32_t xs = uint32_t(int64_t(_cosra) * xt - int64_t(_sinra) * yt + int64_t(_xPivot) * (1 << FP_SCALE) + (1 << (FP_SCALE - 1)));
    uint32_t ys = uint32_t(int64_t(_sinra) * xt + int64_t(_cosra) * yt + int64_t(_yPivot) * (1 << FP_SCALE) + (1 << (FP_SCALE - 1)));

    while ((xs >= xe || ys >= ye) && x < max_x) { x++; xs += _cosra; ys += _sinra; }
    if (x == max_x) continue;

    uint32_t pixel_count = 0;
    do {
      uint32_t rp;
      int32_t xp = xs >> FP_SCALE;
      int32_t yp = ys >> FP_SCALE;
      if (_bpp == 16) rp = _img[xp + yp * _iwidth];
      else { rp = readPixel(xp, yp); rp = (uint16_t)(rp>>8 | rp<<8); }
      if (transp != 0x00FFFFFF && tpcolor == rp) {
        if (pixel_count) {
          spr->pushImage(x - pixel_count, y, pixel_count, 1, sline_buffer);
          pixel_count = 0;
        }
      }
      else if(spr->_bpp==1) {
        spr->drawPixel(x,y,_bpp==1?readPixelValue(xp,yp):(rp!=0));
      }
      else {
        sline_buffer[pixel_count++] = rp;
        if(pixel_count==spriteSpanPixels) {
          spr->pushImage(x + 1 - pixel_count,y,pixel_count,1,sline_buffer);
          pixel_count=0;
        }
      }
    } while (++x < max_x && (xs += _cosra) < xe && (ys += _sinra) < ye);
    if (pixel_count) spr->pushImage(x - pixel_count, y, pixel_count, 1, sline_buffer);
  }
  spr->setSwapBytes(oldSwapBytes);
  return true;
}


/***************************************************************************************
** Function name:           getRotatedBounds
** Description:             Get TFT bounding box of a rotated Sprite wrt pivot
***************************************************************************************/
bool GFX_Sprite::getRotatedBounds(int16_t angle, int16_t *min_x, int16_t *min_y,
                                                  int16_t *max_x, int16_t *max_y)
{
  if(!_created || !_gfx || !min_x || !min_y || !max_x || !max_y) return false;
  // Get the bounding box of this rotated source Sprite relative to Sprite pivot
  getRotatedBounds(angle, width(), height(), _xPivot, _yPivot, min_x, min_y, max_x, max_y);

  // Move bounding box so source Sprite pivot coincides with TFT pivot
  *min_x += _gfx->_xPivot;
  *max_x += _gfx->_xPivot;
  *min_y += _gfx->_yPivot;
  *max_y += _gfx->_yPivot;

  // Return if bounding box is outside of TFT viewport
  if (*min_x >= _gfx->_vpW) return false;
  if (*min_y >= _gfx->_vpH) return false;
  if (*max_x <= _gfx->_vpX) return false;
  if (*max_y <= _gfx->_vpY) return false;

  // Clip bounding box to be within TFT viewport
  if (*min_x < _gfx->_vpX) *min_x = _gfx->_vpX;
  if (*min_y < _gfx->_vpY) *min_y = _gfx->_vpY;
  if (*max_x > _gfx->_vpW) *max_x = _gfx->_vpW;
  if (*max_y > _gfx->_vpH) *max_y = _gfx->_vpH;

  return true;
}


/***************************************************************************************
** Function name:           getRotatedBounds
** Description:             Get destination Sprite bounding box of a rotated Sprite wrt pivot
***************************************************************************************/
bool GFX_Sprite::getRotatedBounds(GFX_Sprite *spr, int16_t angle, int16_t *min_x, int16_t *min_y,
                                                                    int16_t *max_x, int16_t *max_y)
{
  if(!_created || !spr || !spr->_created || !min_x || !min_y || !max_x || !max_y) return false;
  // Get the bounding box of this rotated source Sprite relative to Sprite pivot
  getRotatedBounds(angle, width(), height(), _xPivot, _yPivot, min_x, min_y, max_x, max_y);

  // Move bounding box so source Sprite pivot coincides with destination Sprite pivot
  *min_x += spr->_xPivot;
  *max_x += spr->_xPivot;
  *min_y += spr->_yPivot;
  *max_y += spr->_yPivot;

  // Test only to show bounding box
  // spr->fillSprite(GFX_BLACK);
  // spr->drawRect(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1, GFX_GREEN);

  // Return if bounding box is completely outside of destination Sprite
  if (*min_x >= spr->width()) return false;
  if (*min_y >= spr->height()) return false;
  if (*max_x <= 0) return false;
  if (*max_y <= 0) return false;

  // Clip bounding box to Sprite boundaries
  // Clipping to a viewport will be done by destination Sprite pushImage function
  if (*min_x < 0) *min_x = 0;
  if (*min_y < 0) *min_y = 0;
  if (*max_x > spr->width())  *max_x = spr->width();
  if (*max_y > spr->height()) *max_y = spr->height();

  return true;
}


/***************************************************************************************
** Function name:           rotatedBounds
** Description:             Get bounding box of a rotated Sprite wrt pivot
***************************************************************************************/
void GFX_Sprite::getRotatedBounds(int16_t angle, int16_t w, int16_t h, int16_t xp, int16_t yp,
                                   int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y)
{
  // Trig values for the rotation
  float radAngle = -angle * 0.0174532925; // Convert degrees to radians
  float sina = sin(radAngle);
  float cosa = cos(radAngle);

  w -= xp; // w is now right edge coordinate relative to xp
  h -= yp; // h is now bottom edge coordinate relative to yp

  // Calculate new corner coordinates
  int16_t x0 = -xp * cosa - yp * sina;
  int16_t y0 =  xp * sina - yp * cosa;

  int16_t x1 =  w * cosa - yp * sina;
  int16_t y1 = -w * sina - yp * cosa;

  int16_t x2 =  h * sina + w * cosa;
  int16_t y2 =  h * cosa - w * sina;

  int16_t x3 =  h * sina - xp * cosa;
  int16_t y3 =  h * cosa + xp * sina;

  // Find bounding box extremes, enlarge box to accomodate rounding errors
  *min_x = x0-2;
  if (x1 < *min_x) *min_x = x1-2;
  if (x2 < *min_x) *min_x = x2-2;
  if (x3 < *min_x) *min_x = x3-2;

  *max_x = x0+2;
  if (x1 > *max_x) *max_x = x1+2;
  if (x2 > *max_x) *max_x = x2+2;
  if (x3 > *max_x) *max_x = x3+2;

  *min_y = y0-2;
  if (y1 < *min_y) *min_y = y1-2;
  if (y2 < *min_y) *min_y = y2-2;
  if (y3 < *min_y) *min_y = y3-2;

  *max_y = y0+2;
  if (y1 > *max_y) *max_y = y1+2;
  if (y2 > *max_y) *max_y = y2+2;
  if (y3 > *max_y) *max_y = y3+2;

  _sinra = round(sina * (1<<FP_SCALE));
  _cosra = round(cosa * (1<<FP_SCALE));
}


/***************************************************************************************
** Function name:           pushSprite
** Description:             Push the sprite to the TFT at x, y
***************************************************************************************/
void GFX_Sprite::pushSprite(int32_t x, int32_t y)
{
  if (!_created || !_gfx) return;

  if (_bpp == 16)
  {
    bool oldSwapBytes = _gfx->getSwapBytes();
    _gfx->setSwapBytes(false);
    _gfx->pushImage(x, y, _dwidth, _dheight, _img );
    _gfx->setSwapBytes(oldSwapBytes);
  }
  else if (_bpp == 4)
  {
    _gfx->pushImage(x, y, _dwidth, _dheight, _img4, false, _colorMap);
  }
  else _gfx->pushImage(x, y, _dwidth, _dheight, _img8, (bool)(_bpp == 8));
}


/***************************************************************************************
** Function name:           pushSprite
** Description:             Push the sprite to the TFT at x, y with transparent colour
***************************************************************************************/
void GFX_Sprite::pushSprite(int32_t x, int32_t y, uint16_t transp)
{
  if (!_created || !_gfx) return;

  if (_bpp == 16)
  {
    bool oldSwapBytes = _gfx->getSwapBytes();
    _gfx->setSwapBytes(false);
    _gfx->pushImage(x, y, _dwidth, _dheight, _img, transp );
    _gfx->setSwapBytes(oldSwapBytes);
  }
  else if (_bpp == 8)
  {
    transp = (uint8_t)((transp & 0xE000)>>8 | (transp & 0x0700)>>6 | (transp & 0x0018)>>3);
    _gfx->pushImage(x, y, _dwidth, _dheight, _img8, (uint8_t)transp, (bool)true);
  }
  else if (_bpp == 4)
  {
    _gfx->pushImage(x, y, _dwidth, _dheight, _img4, (uint8_t)(transp & 0x0F), false, _colorMap);
  }
  else _gfx->pushImage(x, y, _dwidth, _dheight, _img8, 0, (bool)false);
}


/***************************************************************************************
** Function name:           pushToSprite
** Description:             Push the sprite to another sprite at x, y
***************************************************************************************/
// Note: The following sprite to sprite colour depths are currently supported:
//    Source    Destination
//    16bpp  -> 16bpp
//    16bpp  ->  8bpp
//     8bpp  ->  8bpp
//     4bpp  ->  4bpp (note: color translation depends on the 2 sprites palette colors)
//     1bpp  ->  1bpp (note: color translation depends on the 2 sprites bitmap colors)

bool GFX_Sprite::pushToSprite(GFX_Sprite *dspr, int32_t x, int32_t y)
{
  if (!_created || !dspr) return false;
  if (!dspr->created()) return false;

  // Check destination sprite compatibility
  int8_t ds_bpp = dspr->getColorDepth();
  if (_bpp == 16 && ds_bpp != 16 && ds_bpp !=  8) return false;
  if (_bpp ==  8 && ds_bpp !=  8) return false;
  if (_bpp ==  4 && ds_bpp !=  4) return false;
  if (_bpp ==  1 && ds_bpp !=  1) return false;

  bool oldSwapBytes = dspr->getSwapBytes();
  dspr->setSwapBytes(false);
  dspr->pushImage(x, y, _dwidth, _dheight, _img, _bpp);
  dspr->setSwapBytes(oldSwapBytes);

  return true;
}


/***************************************************************************************
** Function name:           pushToSprite
** Description:             Push the sprite to another sprite at x, y with transparent colour
***************************************************************************************/
// Note: The following sprite to sprite colour depths are currently supported:
//    Source    Destination
//    16bpp  -> 16bpp
//    16bpp  ->  8bpp
//     8bpp  ->  8bpp
//     1bpp  ->  1bpp

bool GFX_Sprite::pushToSprite(GFX_Sprite *dspr, int32_t x, int32_t y, uint16_t transp)
{
  if ( !_created || !dspr || !dspr->_created || dspr==this) return false;

  // Check destination sprite compatibility
  int8_t ds_bpp = dspr->getColorDepth();
  if (_bpp == 16 && ds_bpp != 16 && ds_bpp !=  8) return false;
  if (_bpp ==  8 && ds_bpp !=  8) return false;
  if (_bpp ==  4 || ds_bpp ==  4) return false;
  if (_bpp ==  1 && ds_bpp !=  1) return false;

  bool oldSwapBytes = dspr->getSwapBytes();
  dspr->setSwapBytes(false);
  uint16_t sline_buffer[spriteSpanPixels];

  transp = transp>>8 | transp<<8;

  // Scan destination bounding box and fetch transformed pixels from source Sprite
  for (int32_t ys = 0; ys < height(); ys++) {
    int32_t ox = x;
    uint32_t pixel_count = 0;

    for (int32_t xs = 0; xs < width(); xs++) {
      uint16_t rp = 0;
      if (_bpp == 16) rp = _img[xs + ys * width()];
      else { rp = readPixel(xs, ys); rp = rp>>8 | rp<<8; }
      //dspr->drawPixel(xs, ys, rp);

      if (transp == rp) {
        if (pixel_count) {
          dspr->pushImage(ox, y, pixel_count, 1, sline_buffer);
          ox += pixel_count;
          pixel_count = 0;
        }
        ox++;
      }
      else if(_bpp==1) {
        dspr->drawPixel(ox++,y,readPixelValue(xs,ys));
      }
      else {
        sline_buffer[pixel_count++] = rp;
        if(pixel_count==spriteSpanPixels) {
          dspr->pushImage(ox,y,pixel_count,1,sline_buffer,16);
          ox+=pixel_count;
          pixel_count=0;
        }
      }
    }
    if (pixel_count) dspr->pushImage(ox, y, pixel_count, 1, sline_buffer);
    y++;
  }
  dspr->setSwapBytes(oldSwapBytes);
  return true;
}


/***************************************************************************************
** Function name:           pushSprite
** Description:             Push a cropped sprite to the TFT at tx, ty
***************************************************************************************/
bool GFX_Sprite::pushSprite(int32_t tx, int32_t ty, int32_t sx, int32_t sy, int32_t sw, int32_t sh)
{
  if (!_created || !_gfx) return false;
  nice_gfx::ClippedRect source;
  if(!nice_gfx::clipRect(sx,sy,sw,sh,0,0,0,0,_dwidth,_dheight,source)) return false;
  if(int64_t(tx)+source.width>INT32_MAX || int64_t(ty)+source.height>INT32_MAX) return false;

  // Perform window boundary checks and crop if needed
  setWindow(source.x,source.y,source.x+source.width-1,source.y+source.height-1);

  /* These global variables are now populated for the sprite
  _xs = x start coordinate
  _ys = y start coordinate
  _xe = x end coordinate (inclusive)
  _ye = y end coordinate (inclusive)
  */

  // Calculate new sprite window bounding box width and height
  sw = _xe - _xs + 1;
  sh = _ye - _ys + 1;

  if (_ys >= _iheight) return false;

  if (_bpp == 16)
  {
    bool oldSwapBytes = _gfx->getSwapBytes();
    _gfx->setSwapBytes(false);

    // Check if a faster block copy to screen is possible
    if ( sx == 0 && sw == _dwidth)
      _gfx->pushImage(tx, ty, sw, sh, _img + _iwidth * _ys );
    else // Render line by line
      while (sh--)
        _gfx->pushImage(tx, ty++, sw, 1, _img + _xs + _iwidth * _ys++ );

    _gfx->setSwapBytes(oldSwapBytes);
  }
  else if (_bpp == 8)
  {
    // Check if a faster block copy to screen is possible
    if ( sx == 0 && sw == _dwidth)
      _gfx->pushImage(tx, ty, sw, sh, _img8 + _iwidth * _ys, (bool)true );
    else // Render line by line
    while (sh--)
      _gfx->pushImage(tx, ty++, sw, 1, _img8 + _xs + _iwidth * _ys++, (bool)true );
  }
  else if (_bpp == 4)
  {
    // Check if a faster block copy to screen is possible
    if ( sx == 0 && sw == _dwidth)
      _gfx->pushImage(tx, ty, sw, sh, _img4 + (_iwidth>>1) * _ys, false, _colorMap );
    else // Render line by line
    {
      int32_t ds = _xs&1; // Odd x start pixel

      int32_t de = 0;     // Odd x end pixel
      if ((sw > ds) && (_xe&1)) de = 1;

      uint32_t dm = 0;     // Midsection pixel count
      if (sw > (ds+de)) dm = sw - ds - de;
      sw--;

      uint32_t yp = (_xs + ds + _iwidth * _ys)>>1;
      _gfx->startWrite();
      while (sh--)
      {
        if (ds) _gfx->drawPixel(tx, ty, readPixel(_xs, _ys) );
        if (dm) _gfx->pushImage(tx + ds, ty, dm, 1, _img4 + yp, false, _colorMap );
        if (de) _gfx->drawPixel(tx + sw, ty, readPixel(_xe, _ys) );
        _ys++;
        ty++;
        yp += (_iwidth>>1);
      }
      _gfx->endWrite();
    }
  }
  else // 1bpp
  {
    // Check if a faster block copy to screen is possible
    if ( _xs == 0 && sw == _dwidth)
      _gfx->pushImage(tx, ty, sw, sh, _img8 + (_bitwidth>>3) * _ys, (bool)false );
    else // Render line by line
    {
      _gfx->startWrite();
      uint16_t span[spriteSpanPixels];
      const bool oldSwapBytes=_gfx->getSwapBytes();
      _gfx->setSwapBytes(true);
      while (sh--) {
        for(int32_t col=0;col<sw;col+=int32_t(spriteSpanPixels)) {
          const int32_t count=sw-col<int32_t(spriteSpanPixels)?sw-col:int32_t(spriteSpanPixels);
          for(int32_t i=0;i<count;++i)span[i]=readPixel(_xs+col+i,_ys);
          _gfx->pushImage(tx+col,ty,count,1,span);
        }
        ++ty;++_ys;
      }
      _gfx->setSwapBytes(oldSwapBytes);
      _gfx->endWrite();
    }
  }

  return true;
}


/***************************************************************************************
** Function name:           readPixelValue
** Description:             Read the color map index of a pixel at defined coordinates
***************************************************************************************/
uint16_t GFX_Sprite::readPixelValue(int32_t x, int32_t y)
{
  if (_vpOoB  || !_created) return 0xFF;

  x+= _xDatum;
  y+= _yDatum;

  // Range checking
  if ((x < _vpX) || (y < _vpY) ||(x >= _vpW) || (y >= _vpH)) return 0xFF;

  if (_bpp == 16)
  {
    // Return the pixel colour
    return readPixel(x - _xDatum, y - _yDatum);
  }

  if (_bpp == 8)
  {
    // Return the pixel byte value
    return _img8[x + y * _iwidth];
  }

  if (_bpp == 4)
  {
    if (x >= _dwidth) return 0xFF;
    if ((x & 0x01) == 0)
      return _img4[((x+y*_iwidth)>>1)] >> 4;   // even index = bits 7 .. 4
    else
      return _img4[((x+y*_iwidth)>>1)] & 0x0F; // odd index = bits 3 .. 0.
  }

  if (_bpp == 1)
  {
    // Note: _dwidth and _dheight bounds not checked (rounded up -iwidth and _iheight used)
    if (rotation == 1)
    {
      uint16_t tx = x;
      x = _dwidth - y - 1;
      y = tx;
    }
    else if (rotation == 2)
    {
      x = _dwidth - x - 1;
      y = _dheight - y - 1;
    }
    else if (rotation == 3)
    {
      uint16_t tx = x;
      x = y;
      y = _dheight - tx - 1;
    }
    // Return 1 or 0
    return (_img8[(x + y * _bitwidth)>>3] >> (7-(x & 0x7))) & 0x01;
  }

  return 0;
}

/***************************************************************************************
** Function name:           readPixel
** Description:             Read 565 colour of a pixel at defined coordinates
***************************************************************************************/
uint16_t GFX_Sprite::readPixel(int32_t x, int32_t y)
{
  if (_vpOoB  || !_created) return 0xFFFF;

  x+= _xDatum;
  y+= _yDatum;

  // Range checking
  if ((x < _vpX) || (y < _vpY) ||(x >= _vpW) || (y >= _vpH)) return 0xFFFF;

  if (_bpp == 16)
  {
    uint16_t color = _img[x + y * _iwidth];
    return (color >> 8) | (color << 8);
  }

  if (_bpp == 8)
  {
    uint16_t color = _img8[x + y * _iwidth];
    if (color != 0)
    {
    uint8_t  blue[] = {0, 11, 21, 31};
      color =   (color & 0xE0)<<8 | (color & 0xC0)<<5
              | (color & 0x1C)<<6 | (color & 0x1C)<<3
              | blue[color & 0x03];
    }
    return color;
  }

  if (_bpp == 4)
  {
    if (x >= _dwidth) return 0xFFFF;
    uint16_t color;
    if ((x & 0x01) == 0)
      color = _colorMap[_img4[((x+y*_iwidth)>>1)] >> 4];   // even index = bits 7 .. 4
    else
      color = _colorMap[_img4[((x+y*_iwidth)>>1)] & 0x0F]; // odd index = bits 3 .. 0.
    return color;
  }

  // Note: Must be 1bpp
  // _dwidth and _dheight bounds not checked (rounded up -iwidth and _iheight used)
  if (rotation == 1)
  {
    uint16_t tx = x;
    x = _dwidth - y - 1;
    y = tx;
  }
  else if (rotation == 2)
  {
    x = _dwidth - x - 1;
    y = _dheight - y - 1;
  }
  else if (rotation == 3)
  {
    uint16_t tx = x;
    x = y;
    y = _dheight - tx - 1;
  }

  uint16_t color = (_img8[(x + y * _bitwidth)>>3] << (x & 0x7)) & 0x80;

  if (color) return _gfx ? _gfx->bitmap_fg : bitmap_fg;
  else       return _gfx ? _gfx->bitmap_bg : bitmap_bg;
}


/***************************************************************************************
** Function name:           pushImage
** Description:             push image into a defined area of a sprite
***************************************************************************************/
void  GFX_Sprite::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data, uint8_t sbpp)
{
  if (data == nullptr || !_created || w<=0 || h<=0 || size_t(w)>SIZE_MAX/2/size_t(h)) return;

  PI_CLIP;

  if (_bpp == 16) // Plot a 16 bpp image into a 16 bpp Sprite
  {
    // Pointer within original image
    const uint8_t *ptro = reinterpret_cast<const uint8_t *>(data) + (size_t(dx) + size_t(dy) * size_t(w)) * 2;
    // Pointer within sprite image
    uint8_t *ptrs = (uint8_t *)_img + ((x + y * _iwidth) << 1);

    spriteCopyRows(ptrs,size_t(_iwidth)*2,ptro,size_t(w)*2,size_t(dw)*2,size_t(dh),_swapBytes);
  }
  else if (_bpp == 8 && sbpp == 8) // Plot a 8 bpp image into a 8 bpp Sprite
  {
    // Pointer within original image
    const uint8_t *ptro = reinterpret_cast<const uint8_t *>(data) + size_t(dx) + size_t(dy) * size_t(w);
    // Pointer within sprite image
    uint8_t *ptrs = (uint8_t *)_img + (x + y * _iwidth);

    spriteCopyRows(ptrs,size_t(_iwidth),ptro,size_t(w),size_t(dw),size_t(dh));
  }
  else if (_bpp == 8) // Plot a 16 bpp image into a 8 bpp Sprite
  {
    uint16_t lastColor = 0;
    uint8_t  color8    = 0;
    for (int32_t yp = dy; yp < dy + dh; yp++)
    {
      int32_t xyw = x + y * _iwidth;
      size_t dxypw = size_t(dx) + size_t(yp) * size_t(w);
      for (int32_t xp = dx; xp < dx + dw; xp++)
      {
        uint16_t color = data[dxypw++];
        if (color != lastColor) {
          // When data source is a sprite, the bytes are already swapped
          if(!_swapBytes) color8 = (uint8_t)((color & 0xE0) | (color & 0x07)<<2 | (color & 0x1800)>>11);
          else color8 = (uint8_t)((color & 0xE000)>>8 | (color & 0x0700)>>6 | (color & 0x0018)>>3);
        }
        lastColor = color;
        _img8[xyw++] = color8;
      }
      y++;
    }
  }
  else if (_bpp == 4)
  {
    // The image is assumed to be 4-bit, where each byte corresponds to two pixels.
    // much faster when aligned to a byte boundary, because the alternative is slower, requiring
    // tedious bit operations.

    const size_t sWidth = size_t(_iwidth) / 2;
    const size_t sourceStride = (size_t(w)+1)/2;
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(data);

    if ((x & 0x01) == 0 && (dx & 0x01) == 0 && (dw & 0x01) == 0)
    {
      spriteCopyRows(_img4+size_t(x)/2+size_t(y)*sWidth,sWidth,
                     ptr+size_t(dx)/2+size_t(dy)*sourceStride,sourceStride,size_t(dw)/2,size_t(dh));
    }
    else  // not optimized
    {
      for (int32_t yp = dy; yp < dy + dh; yp++)
      {
        int32_t ox = x;
        for (int32_t xp = dx; xp < dx + dw; xp++)
        {
          uint32_t color;
          if ((xp & 0x01) == 0)
            color = (ptr[size_t(yp)*sourceStride+size_t(xp)/2] & 0xF0) >> 4;
          else
            color = ptr[size_t(yp)*sourceStride+size_t(xp)/2] & 0x0F;
          drawPixel(ox - _xDatum, y - _yDatum, color);
          ox++;
        }
        y++;
      }
    }
  }

  else // 1bpp
  {
    // Plot a 1bpp image into a 1bpp Sprite
    const size_t ww = (size_t(w)+7)/8;
    uint8_t *ptr = (uint8_t *)data;
    for (int32_t yp = dy;  yp < dy + dh; yp++)
    {
      const size_t yw = size_t(yp) * ww;
      int32_t ox = x;
      for (int32_t xp = dx; xp < dx + dw; xp++)
      {
        uint16_t readPixel = (ptr[(xp>>3) + yw] & (0x80 >> (xp & 0x7)) );
        drawPixel(ox++ - _xDatum, y - _yDatum, readPixel);
      }
      y++;
    }
  }
}


/***************************************************************************************
** Function name:           pushImage
** Description:             push 565 colour FLASH (PROGMEM) image into a defined area
***************************************************************************************/
void  GFX_Sprite::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data)
{
#if defined(ESP32) || NICE_GFX_NATIVE_IDF
  pushImage(x, y, w, h, (uint16_t*) data);
#else
  // Partitioned memory FLASH processor
  if (data == nullptr || !_created || w<=0 || h<=0 || size_t(w)>SIZE_MAX/2/size_t(h)) return;

  PI_CLIP;

  if (_bpp == 16) // Plot a 16 bpp image into a 16 bpp Sprite
  {
    for (int32_t yp = dy; yp < dy + dh; yp++)
    {
      int32_t ox = x;
      for (int32_t xp = dx; xp < dx + dw; xp++)
      {
        uint16_t color = pgm_read_word(data + size_t(xp) + size_t(yp) * size_t(w));
        if(_swapBytes) color = color<<8 | color>>8;
        _img[ox + y * _iwidth] = color;
        ox++;
      }
      y++;
    }
  }

  else if (_bpp == 8) // Plot a 16 bpp image into a 8 bpp Sprite
  {
    for (int32_t yp = dy; yp < dy + dh; yp++)
    {
      int32_t ox = x;
      for (int32_t xp = dx; xp < dx + dw; xp++)
      {
        uint16_t color = pgm_read_word(data + size_t(xp) + size_t(yp) * size_t(w));
        if(_swapBytes) color = color<<8 | color>>8;
        _img8[ox + y * _iwidth] = (uint8_t)((color & 0xE000)>>8 | (color & 0x0700)>>6 | (color & 0x0018)>>3);
        ox++;
      }
      y++;
    }
  }

  else if (_bpp == 4)
  {
    // Match the RAM overload: high nibble first, each source row byte-aligned.
    // Flash storage must be read through pgm_read_byte even for aligned copies.
    const size_t sourceStride = (size_t(w) + 1) / 2;
    const uint8_t *row = reinterpret_cast<const uint8_t *>(data) + size_t(dy) * sourceStride;
    if ((x & 1) == 0 && (dx & 1) == 0 && (dw & 1) == 0)
    {
      const size_t destinationStride = size_t(_iwidth) / 2;
      uint8_t *destination = _img4 + size_t(y) * destinationStride + size_t(x) / 2;
      for (int32_t yp = 0; yp < dh; ++yp)
      {
        for (size_t byte = 0; byte < size_t(dw) / 2; ++byte)
          destination[byte] = pgm_read_byte(row + size_t(dx) / 2 + byte);
        row += sourceStride;
        destination += destinationStride;
      }
    }
    else
    {
      for (int32_t yp = 0; yp < dh; ++yp)
      {
        int32_t ox = x;
        for (int32_t xp = dx; xp < dx + dw; ++xp)
        {
          const uint8_t packed = pgm_read_byte(row + size_t(xp) / 2);
          const uint8_t color = (xp & 1) ? (packed & 0x0F) : (packed >> 4);
          drawPixel(ox++ - _xDatum, y + yp - _yDatum, color);
        }
        row += sourceStride;
      }
    }
  }

  else // Plot a 1bpp image into a 1bpp Sprite
  {
    x-= _xDatum;   // Remove offsets, drawPixel will add
    y-= _yDatum;
    const size_t bsw = (size_t(w)+7)/8;
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(data) + size_t(dy) * bsw;
    
    while (dh--) {
      int32_t odx = dx;
      int32_t ox  = x;
      while (odx < dx + dw) {
        uint8_t pbyte = pgm_read_byte(ptr + (odx>>3));
        uint8_t mask = 0x80 >> (odx & 7);
        while (mask && odx < dx + dw) {
          uint8_t p = pbyte & mask;
          mask = mask >> 1;
          drawPixel(ox++, y, p);
          odx++;
        }
      }
      ptr += bsw;
      y++;
    }
  }
#endif // if ESP32 check
}


/***************************************************************************************
** Function name:           setWindow
** Description:             Set the bounds of a window in the sprite
***************************************************************************************/
// Intentionally not constrained to viewport area, does not manage 1bpp rotations
void GFX_Sprite::setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
  if (x0 > x1) transpose(x0, x1);
  if (y0 > y1) transpose(y0, y1);
  
  int32_t w = width();
  int32_t h = height();

  if ((x0 >= w) || (x1 < 0) || (y0 >= h) || (y1 < 0))
  { // Point to that extra "off screen" pixel
    _xs = 0;
    _ys = _dheight;
    _xe = 0;
    _ye = _dheight;
  }
  else
  {
    if (x0 < 0) x0 = 0;
    if (x1 >= w) x1 = w - 1;
    if (y0 < 0) y0 = 0;
    if (y1 >= h) y1 = h - 1;

    _xs = x0;
    _ys = y0;
    _xe = x1;
    _ye = y1;
  }

  _xptr = _xs;
  _yptr = _ys;
}


/***************************************************************************************
** Function name:           pushColor
** Description:             Send a new pixel to the set window
***************************************************************************************/
void GFX_Sprite::pushColor(uint16_t color)
{
  if (!_created ) return;

  // Write the colour to RAM in set window
  if (_bpp == 16)
    _img [_xptr + _yptr * _iwidth] = (uint16_t) (color >> 8) | (color << 8);

  else  if (_bpp == 8)
    _img8[_xptr + _yptr * _iwidth] = (uint8_t )((color & 0xE000)>>8 | (color & 0x0700)>>6 | (color & 0x0018)>>3);

  else if (_bpp == 4)
  {
    uint8_t c = (uint8_t)color & 0x0F;
    if ((_xptr & 0x01) == 0) {
      _img4[(_xptr + _yptr * _iwidth)>>1] = (c << 4) | (_img4[(_xptr + _yptr * _iwidth)>>1] & 0x0F);  // new color is in bits 7 .. 4
    }
    else {
      _img4[(_xptr + _yptr * _iwidth)>>1] = (_img4[(_xptr + _yptr * _iwidth)>>1] & 0xF0) | c; // new color is the low bits
    }
  }

  else drawPixel(_xptr, _yptr, color);

  // Increment x
  _xptr++;

  // Wrap on x and y to start, increment y if needed
  if (_xptr > _xe)
  {
    _xptr = _xs;
    _yptr++;
    if (_yptr > _ye) _yptr = _ys;
  }

}


/***************************************************************************************
** Function name:           pushColor
** Description:             Send a "len" new pixels to the set window
***************************************************************************************/
void GFX_Sprite::pushColor(uint16_t color, uint32_t len)
{
  if (!_created ) return;

  uint16_t pixelColor;

  if (_bpp == 16)
    pixelColor = (uint16_t) (color >> 8) | (color << 8);

  else  if (_bpp == 8)
    pixelColor = (color & 0xE000)>>8 | (color & 0x0700)>>6 | (color & 0x0018)>>3;

  else pixelColor = (uint16_t) color; // for 1bpp or 4bpp

  while(len--) writeColor(pixelColor);
}


/***************************************************************************************
** Function name:           writeColor
** Description:             Write a pixel with pre-formatted colour to the set window
***************************************************************************************/
void GFX_Sprite::writeColor(uint16_t color)
{
  if (!_created ) return;

  // Write 16-bit RGB 565 encoded colour to RAM
  if (_bpp == 16) _img [_xptr + _yptr * _iwidth] = color;

  // Write 8-bit RGB 332 encoded colour to RAM
  else if (_bpp == 8) _img8[_xptr + _yptr * _iwidth] = (uint8_t) color;

  else if (_bpp == 4)
  {
    uint8_t c = (uint8_t)color & 0x0F;
    if ((_xptr & 0x01) == 0)
      _img4[(_xptr + _yptr * _iwidth)>>1] = (c << 4) | (_img4[(_xptr + _yptr * _iwidth)>>1] & 0x0F);  // new color is in bits 7 .. 4
    else
      _img4[(_xptr + _yptr * _iwidth)>>1] = (_img4[(_xptr + _yptr * _iwidth)>>1] & 0xF0) | c; // new color is the low bits (x is odd)
  }

  else drawPixel(_xptr, _yptr, color);

  // Increment x
  _xptr++;

  // Wrap on x and y to start, increment y if needed
  if (_xptr > _xe)
  {
    _xptr = _xs;
    _yptr++;
    if (_yptr > _ye) _yptr = _ys;
  }
}


/***************************************************************************************
** Function name:           setScrollRect
** Description:             Set scroll area within the sprite and the gap fill colour
***************************************************************************************/
// Intentionally not constrained to viewport area
void GFX_Sprite::setScrollRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
  if ((x >= _iwidth) || (y >= _iheight) || !_created ) return;

  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }

  if ((x + w) > _iwidth ) w = _iwidth  - x;
  if ((y + h) > _iheight) h = _iheight - y;

  if ( w < 1 || h < 1) return;

  _sx = x;
  _sy = y;
  _sw = w;
  _sh = h;

  _scolor = color;
}


/***************************************************************************************
** Function name:           scroll
** Description:             Scroll dx,dy pixels, positive right,down, negative left,up
***************************************************************************************/
void GFX_Sprite::scroll(int16_t dx, int16_t dy)
{
  if (abs(dx) >= _sw || abs(dy) >= _sh)
  {
    fillRect (_sx, _sy, _sw, _sh, _scolor);
    return;
  }

  // Fetch the scroll area width and height set by setScrollRect()
  uint32_t w  = _sw - abs(dx); // line width to copy
  uint32_t h  = _sh - abs(dy); // lines to copy
  int32_t iw  = _iwidth;       // rounded up width of sprite

  // Fetch the x,y origin set by setScrollRect()
  uint32_t tx = _sx; // to x
  uint32_t fx = _sx; // from x
  uint32_t ty = _sy; // to y
  uint32_t fy = _sy; // from y

  // Adjust for x delta
  if (dx <= 0) fx -= dx;
  else tx += dx;

  // Adjust for y delta
  if (dy <= 0) fy -= dy;
  else
  { // Scrolling down so start copy from bottom
    ty = ty + _sh - 1; // "To" pointer
    iw = -iw;          // Pointer moves backwards
    fy = ty - dy;      // "From" pointer
  }

  // Calculate "from y" and "to y" pointers in RAM
  uint32_t fyp = fx + fy * _iwidth;
  uint32_t typ = tx + ty * _iwidth;

  // Now move the pixels in RAM
  if (_bpp == 16)
  {
    while (h--)
    { // move pixel lines (to, from, byte count)
      memmove( _img + typ, _img + fyp, w<<1);
      typ += iw;
      fyp += iw;
    }
  }
  else if (_bpp == 8)
  {
    while (h--)
    { // move pixel lines (to, from, byte count)
      memmove( _img8 + typ, _img8 + fyp, w);
      typ += iw;
      fyp += iw;
    }
  }
  else if (_bpp == 4)
  {
    // could optimize for scrolling by even # pixels using memove (later)
    if (dx >  0) { tx += w - 1; fx += w - 1; } // Last pixel inside each span
    while (h--)
    { // move pixels one by one
      for (uint16_t xp = 0; xp < w; xp++)
      {
        if (dx <= 0) drawPixel(tx + xp, ty, readPixelValue(fx + xp, fy));
        if (dx >  0) drawPixel(tx - xp, ty, readPixelValue(fx - xp, fy));
      }
      if (dy <= 0)  { ty++; fy++; }
      else  { ty--; fy--; }
    }
  }
  else if (_bpp == 1 )
  {
    if (dx >  0) { tx += w - 1; fx += w - 1; } // Last pixel inside each span
    while (h--)
    { // move pixels one by one
      for (uint16_t xp = 0; xp < w; xp++)
      {
        if (dx <= 0) drawPixel(tx + xp, ty, readPixelValue(fx + xp, fy));
        if (dx >  0) drawPixel(tx - xp, ty, readPixelValue(fx - xp, fy));
      }
      if (dy <= 0)  { ty++; fy++; }
      else  { ty--; fy--; }
    }
  }
  else return; // Not 1, 4, 8 or 16 bpp

  // Fill the gap left by the scrolling
  if (dx > 0) fillRect(_sx, _sy, dx, _sh, _scolor);
  if (dx < 0) fillRect(_sx + _sw + dx, _sy, -dx, _sh, _scolor);
  if (dy > 0) fillRect(_sx, _sy, _sw, dy, _scolor);
  if (dy < 0) fillRect(_sx, _sy + _sh + dy, _sw, -dy, _scolor);
}


/***************************************************************************************
** Function name:           fillSprite
** Description:             Fill the whole sprite with defined colour
***************************************************************************************/
void GFX_Sprite::fillSprite(uint32_t color)
{
  if (!_created || _vpOoB) return;
  fillSpriteRect(_vpX,_vpY,_vpW-_vpX,_vpH-_vpY,color);
}


/***************************************************************************************
** Function name:           width
** Description:             Return the width of sprite
***************************************************************************************/
// Return the size of the sprite
int16_t GFX_Sprite::width(void)
{
  if (!_created ) return 0;

  if (_bpp > 1) {
    if (_vpDatum) return _xWidth;
    return _dwidth;
  }

  if (rotation & 1) {
    if (_vpDatum) return _xWidth;
    return _dheight;
  }

  if (_vpDatum) return _xWidth;
  return _dwidth;
}


/***************************************************************************************
** Function name:           height
** Description:             Return the height of sprite
***************************************************************************************/
int16_t GFX_Sprite::height(void)
{
  if (!_created ) return 0;

  if (_bpp > 1) {
    if (_vpDatum) return _yHeight;
    return _dheight;
  }

  if (rotation & 1) {
    if (_vpDatum) return _yHeight;
    return _dwidth;
  }

  if (_vpDatum) return _yHeight;
  return _dheight;
}


/***************************************************************************************
** Function name:           setRotation
** Description:             Rotate coordinate frame for 1bpp sprite
***************************************************************************************/
// Does nothing for 4, 8 and 16 bpp sprites.
void GFX_Sprite::setRotation(uint8_t r)
{
  if (_bpp != 1) return;

  rotation = r & 3;
  
  if (rotation&1) {
    resetViewport();
  }
  else {
    resetViewport();
  }
}


/***************************************************************************************
** Function name:           getRotation
** Description:             Get rotation for 1bpp sprite
***************************************************************************************/
uint8_t GFX_Sprite::getRotation(void)
{
  return rotation;
}


/***************************************************************************************
** Function name:           drawPixel
** Description:             push a single pixel at an arbitrary position
***************************************************************************************/
void GFX_Sprite::drawPixel(int32_t x, int32_t y, uint32_t color)
{
  if (!_created || _vpOoB) return;
  nice_gfx::ClippedRect r;
  if (!nice_gfx::clipRect(x,y,1,1,_xDatum,_yDatum,_vpX,_vpY,_vpW,_vpH,r)) return;
  x=r.x;y=r.y;

  if (_bpp == 16)
  {
    color = (color >> 8) | (color << 8);
    _img[x+y*_iwidth] = (uint16_t) color;
  }
  else if (_bpp == 8)
  {
    _img8[x+y*_iwidth] = (uint8_t)((color & 0xE000)>>8 | (color & 0x0700)>>6 | (color & 0x0018)>>3);
  }
  else if (_bpp == 4)
  {
    uint8_t c = color & 0x0F;
    int index = (x+y*_iwidth)>>1;;
    if ((x & 0x01) == 0) {
      _img4[index] = (uint8_t)((c << 4) | (_img4[index] & 0x0F));
    }
    else {
      _img4[index] =  (uint8_t)(c | (_img4[index] & 0xF0));
    }
  }
  else // 1 bpp
  {
    if (rotation == 1)
    {
      uint16_t tx = x;
      x = _dwidth - y - 1;
      y = tx;
    }
    else if (rotation == 2)
    {
      x = _dwidth - x - 1;
      y = _dheight - y - 1;
    }
    else if (rotation == 3)
    {
      uint16_t tx = x;
      x = y;
      y = _dheight - tx - 1;
    }

    if (color) _img8[(x + y * _bitwidth)>>3] |=  (0x80 >> (x & 0x7));
    else       _img8[(x + y * _bitwidth)>>3] &= ~(0x80 >> (x & 0x7));
  }
}


/***************************************************************************************
** Function name:           fillRect
** Description:             draw a filled rectangle
***************************************************************************************/
void GFX_Sprite::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
  nice_gfx::ClippedRect r;
  if (!_created || _vpOoB || !nice_gfx::clipRect(x,y,w,h,_xDatum,_yDatum,_vpX,_vpY,_vpW,_vpH,r)) return;
  fillSpriteRect(r.x,r.y,r.width,r.height,color);
}

// Coordinates are clipped once in the logical frame. Each memory format has
// a bulk sink; packed edges preserve pixels outside the requested rectangle.
void GFX_Sprite::fillSpriteRect(int32_t x,int32_t y,int32_t w,int32_t h,uint32_t color)
{
  if (w<=0 || h<=0) return;
  if (_bpp==16) {
    const uint16_t wire=uint16_t((color>>8)|(color<<8));
    uint16_t *first=_img+size_t(y)*_iwidth+x;
    if (x==0 && w==_iwidth && uint8_t(wire)==uint8_t(wire>>8)) {
      memset(first,uint8_t(wire),size_t(w)*size_t(h)*2);
      return;
    }
    if (uint8_t(wire)==uint8_t(wire>>8)) memset(first,uint8_t(wire),size_t(w)*2);
    else for (int32_t col=0;col<w;++col) first[col]=wire;
    for (int32_t row=1;row<h;++row) memcpy(first+size_t(row)*_iwidth,first,size_t(w)*2);
    return;
  }
  if (_bpp==8) {
    const uint8_t value=uint8_t((color&0xE000)>>8 | (color&0x0700)>>6 | (color&0x0018)>>3);
    for (int32_t row=0;row<h;++row) memset(_img8+size_t(y+row)*_iwidth+x,value,size_t(w));
    return;
  }
  if (_bpp==4) {
    const uint8_t value=uint8_t(color&15), pair=uint8_t(value*17);
    for (int32_t row=0;row<h;++row) {
      uint8_t *dst=_img4+(size_t(y+row)*_iwidth+x)/2;
      int32_t count=w;
      if (x&1) { *dst=uint8_t((*dst&0xF0)|value);++dst;--count; }
      memset(dst,pair,size_t(count/2));
      if (count&1) dst[count/2]=uint8_t((dst[count/2]&15)|(value<<4));
    }
    return;
  }
  // A rotated mono rectangle is still rectangular in the packed framebuffer.
  if (rotation==1) { const int32_t tx=x;x=_dwidth-y-h;y=tx;transpose(w,h); }
  else if (rotation==2) { x=_dwidth-x-w;y=_dheight-y-h; }
  else if (rotation==3) { const int32_t tx=x;x=y;y=_dheight-tx-w;transpose(w,h); }
  const int32_t end=x+w-1;
  const uint8_t firstMask=uint8_t(0xFFu>>(x&7));
  const uint8_t lastMask=uint8_t(0xFFu<<(7-(end&7)));
  const size_t firstByte=size_t(x)/8,lastByte=size_t(end)/8;
  for (int32_t row=0;row<h;++row) {
    uint8_t *dst=_img8+size_t(y+row)*size_t(_bitwidth/8);
    const uint8_t left=firstByte==lastByte?uint8_t(firstMask&lastMask):firstMask;
    if (color) dst[firstByte]|=left;else dst[firstByte]&=uint8_t(~left);
    if (firstByte==lastByte) continue;
    if (lastByte>firstByte+1) memset(dst+firstByte+1,color?0xFF:0,lastByte-firstByte-1);
    if (color) dst[lastByte]|=lastMask;else dst[lastByte]&=uint8_t(~lastMask);
  }
}


// Character layout, bitmap decoding and span rendering are inherited.
