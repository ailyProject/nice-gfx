/***************************************************************************************
** Function name:           push rectangle
** Description:             push 565 pixel colours into a defined area
***************************************************************************************/
void NICE_GFX::pushRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data)
{
  bool swap = _swapBytes; _swapBytes = false;
  pushImage(x, y, w, h, data);
  _swapBytes = swap;
}


/***************************************************************************************
** Function name:           pushImage
** Description:             plot 16-bit colour sprite or image onto TFT
***************************************************************************************/
void NICE_GFX::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data)
{
  if (!_panel || !data) { _runtimeStatus=nice_gfx::Status::InvalidArgument; return; }
  PI_CLIP;
  const auto order=_swapBytes?nice_gfx::PixelOrder::Native:nice_gfx::PixelOrder::Wire;
  _runtimeStatus=_panel->flush(x,y,nice_gfx::PixelView(data+size_t(dy)*w+dx,dw,dh,w,order),false);
}

/***************************************************************************************
** Function name:           pushImage
** Description:             plot 16-bit sprite or image with 1 colour being transparent
***************************************************************************************/
void NICE_GFX::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data, uint16_t transp)
{
  if (!_panel || !data) { _runtimeStatus=nice_gfx::Status::InvalidArgument; return; }
  PI_CLIP;
  if(!_swapBytes)transp=uint16_t(transp>>8|transp<<8);
  const auto order=_swapBytes?nice_gfx::PixelOrder::Native:nice_gfx::PixelOrder::Wire;
  _runtimeStatus=nice_gfx::rasterRuns(clipped,
    [&](int32_t sx,int32_t sy) { return data[size_t(sy)*w+sx]!=transp; },
    [&](int32_t tx,int32_t ty,int32_t sx,int32_t sy,int32_t count) {
      return _panel->flush(tx,ty,nice_gfx::PixelView(data+size_t(sy)*w+sx,count,1,0,order),false);
    });
}


/***************************************************************************************
** Function name:           pushImage - for FLASH (PROGMEM) stored images
** Description:             plot 16-bit image
***************************************************************************************/
void NICE_GFX::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data)
{
#if !defined(__AVR__) && !defined(ESP8266)
  pushImage(x,y,w,h,const_cast<uint16_t*>(data)); // RAM and flash are memory-mapped; source remains immutable.
#else
  if (!_panel || !data) { _runtimeStatus=nice_gfx::Status::InvalidArgument; return; }
  PI_CLIP;
  _runtimeStatus=_panel->window(x,y,x+dw-1,y+dh-1);
  uint16_t buffer[nice_gfx::pixelBufferPixels];
  const auto order=_swapBytes?nice_gfx::PixelOrder::Native:nice_gfx::PixelOrder::Wire;
  for(int32_t row=0;row<dh && _runtimeStatus==nice_gfx::Status::Ok;++row)
    for(int32_t col=0;col<dw && _runtimeStatus==nice_gfx::Status::Ok;col+=nice_gfx::pixelBufferPixels) {
      const int32_t n=dw-col<int32_t(nice_gfx::pixelBufferPixels)?dw-col:int32_t(nice_gfx::pixelBufferPixels);
      for(int32_t i=0;i<n;++i)buffer[i]=pgm_read_word(data+size_t(dy+row)*w+dx+col+i);
      _runtimeStatus=_panel->pixels(nice_gfx::PixelView(buffer,n,1,0,order));
    }
#endif
}

/***************************************************************************************
** Function name:           pushImage - for FLASH (PROGMEM) stored images
** Description:             plot 16-bit image with 1 colour being transparent
***************************************************************************************/
void NICE_GFX::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data, uint16_t transp)
{
#if !defined(__AVR__) && !defined(ESP8266)
  pushImage(x,y,w,h,const_cast<uint16_t*>(data),transp);
#else
  if (!_panel || !data) { _runtimeStatus=nice_gfx::Status::InvalidArgument; return; }
  PI_CLIP;
  if(!_swapBytes)transp=uint16_t(transp>>8|transp<<8);
  const auto order=_swapBytes?nice_gfx::PixelOrder::Native:nice_gfx::PixelOrder::Wire;
  _runtimeStatus=nice_gfx::rasterRuns(clipped,
    [&](int32_t sx,int32_t sy){return pgm_read_word(data+size_t(sy)*w+sx)!=transp;},
    [&](int32_t tx,int32_t ty,int32_t sx,int32_t sy,int32_t count){
      auto s=_panel->window(tx,ty,tx+count-1,ty);
      uint16_t buffer[nice_gfx::pixelBufferPixels];
      for(int32_t col=0;col<count && s==nice_gfx::Status::Ok;col+=nice_gfx::pixelBufferPixels) {
        const int32_t n=count-col<int32_t(nice_gfx::pixelBufferPixels)?count-col:int32_t(nice_gfx::pixelBufferPixels);
        for(int32_t i=0;i<n;++i)buffer[i]=pgm_read_word(data+size_t(sy)*w+sx+col+i);
        s=_panel->pixels(nice_gfx::PixelView(buffer,n,1,0,order));
      }
      return s;
    });
#endif
}

/***************************************************************************************
** Function name:           pushImage
** Description:             plot 8-bit or 4-bit or 1 bit image or sprite using a line buffer
***************************************************************************************/
void NICE_GFX::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *data, bool bpp8,  uint16_t *cmap)
{
  if (!_panel || !data) { _runtimeStatus=nice_gfx::Status::InvalidArgument; return; }
  PI_CLIP;
  _runtimeStatus=_panel->window(x,y,x+dw-1,y+dh-1);
  if(_runtimeStatus!=nice_gfx::Status::Ok)return;
  const auto read=[&](size_t i)->uint8_t { return pgm_read_byte(data+i); };
  const auto send=[&](int32_t,int32_t,const uint16_t *p,size_t n) { return _panel->pixels(nice_gfx::PixelView(p,n)); };
  if(bpp8) _runtimeStatus=nice_gfx::rasterIndexed<8,true>(w,clipped,read,cmap,bitmap_fg,bitmap_bg,-1,send);
  else if(cmap) _runtimeStatus=nice_gfx::rasterIndexed<4,true>(w,clipped,read,cmap,bitmap_fg,bitmap_bg,-1,send);
  else _runtimeStatus=nice_gfx::rasterIndexed<1,true>(w,clipped,read,cmap,bitmap_fg,bitmap_bg,-1,send);
}


/***************************************************************************************
** Function name:           pushImage
** Description:             plot 8-bit or 4-bit or 1 bit image or sprite using a line buffer
***************************************************************************************/
void NICE_GFX::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data, bool bpp8,  uint16_t *cmap)
{
  if (!_panel || !data) { _runtimeStatus=nice_gfx::Status::InvalidArgument; return; }
  PI_CLIP;
  _runtimeStatus=_panel->window(x,y,x+dw-1,y+dh-1);
  if(_runtimeStatus!=nice_gfx::Status::Ok)return;
  const auto read=[&](size_t i)->uint8_t { return data[i]; };
  const auto send=[&](int32_t,int32_t,const uint16_t *p,size_t n) { return _panel->pixels(nice_gfx::PixelView(p,n)); };
  if(bpp8) _runtimeStatus=nice_gfx::rasterIndexed<8,true>(w,clipped,read,cmap,bitmap_fg,bitmap_bg,-1,send);
  else if(cmap) _runtimeStatus=nice_gfx::rasterIndexed<4,true>(w,clipped,read,cmap,bitmap_fg,bitmap_bg,-1,send);
  else _runtimeStatus=nice_gfx::rasterIndexed<1,true>(w,clipped,read,cmap,bitmap_fg,bitmap_bg,-1,send);
}


/***************************************************************************************
** Function name:           pushImage
** Description:             plot 8 or 4 or 1 bit image or sprite with a transparent colour
***************************************************************************************/
void NICE_GFX::pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data, uint8_t transp, bool bpp8, uint16_t *cmap)
{
  if (!_panel || !data) { _runtimeStatus=nice_gfx::Status::InvalidArgument; return; }
  PI_CLIP;
  const auto read=[&](size_t i)->uint8_t{return data[i];};
  const auto send=[&](int32_t tx,int32_t ty,const uint16_t *p,size_t n) {
    return _panel->flush(tx,ty,nice_gfx::PixelView(p,n),false);
  };
  if(bpp8) _runtimeStatus=nice_gfx::rasterIndexed<8>(w,clipped,read,cmap,bitmap_fg,bitmap_bg,transp,send);
  else if(cmap) _runtimeStatus=nice_gfx::rasterIndexed<4>(w,clipped,read,cmap,bitmap_fg,bitmap_bg,transp,send);
  else _runtimeStatus=nice_gfx::rasterIndexed<1>(w,clipped,read,cmap,bitmap_fg,bitmap_bg,0,send);
}

/***************************************************************************************
** Function name:           pushMaskedImage
** Description:             Render a 16-bit colour image to TFT with a 1bpp mask
***************************************************************************************/
// Can be used with a 16bpp sprite and a 1bpp sprite for the mask
void NICE_GFX::pushMaskedImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *img, uint8_t *mask)
{
  if (!_panel || !img || !mask) { _runtimeStatus=nice_gfx::Status::InvalidArgument; return; }
  PI_CLIP;
  const size_t stride=(size_t(w)+7)/8;
  const auto order=_swapBytes?nice_gfx::PixelOrder::Native:nice_gfx::PixelOrder::Wire;
  _runtimeStatus=nice_gfx::rasterRuns(clipped,
    [&](int32_t sx,int32_t sy) { return (mask[size_t(sy)*stride+size_t(sx)/8] & (0x80>>(sx&7)))!=0; },
    [&](int32_t tx,int32_t ty,int32_t sx,int32_t sy,int32_t count) {
      return _panel->flush(tx,ty,nice_gfx::PixelView(img+size_t(sy)*w+sx,count,1,0,order),false);
    });
}


/***************************************************************************************
** Function name:           setSwapBytes
** Description:             Used by 16-bit pushImage() to swap byte order in colours
***************************************************************************************/
void NICE_GFX::setSwapBytes(bool swap)
{
  _swapBytes = swap;
}


/***************************************************************************************
** Function name:           getSwapBytes
** Description:             Return the swap byte order for colours
***************************************************************************************/
bool NICE_GFX::getSwapBytes(void)
{
  return _swapBytes;
}


/***************************************************************************************
** Function name:           read rectangle (for SPI Interface II i.e. IM [3:0] = "1101")
** Description:             Read RGB pixel colours from a defined area
***************************************************************************************/
// If w and h are 1, then 1 pixel is read, *data array size must be 3 bytes per pixel
void  NICE_GFX::readRectRGB(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data)
{
  if (!_panel || !data) { _runtimeStatus = nice_gfx::Status::InvalidArgument; return; }
  PI_CLIP;
  uint16_t colors[64];
  for (int32_t row=0; row<dh; ++row) for (int32_t col=0; col<dw; col+=64) {
    const int32_t count = dw-col < 64 ? dw-col : 64;
    _runtimeStatus = _panel->readRect(x+col,y+row,count,1,colors);
    if (_runtimeStatus != nice_gfx::Status::Ok) return;
    uint8_t *out = data + 3*(size_t(row+dy)*size_t(w)+size_t(col+dx));
    for(int32_t i=0;i<count;++i) {
      const uint16_t v=colors[i];
      *out++=uint8_t((v>>8)&0xF8)|uint8_t(v>>13);
      *out++=uint8_t((v>>3)&0xFC)|uint8_t((v>>9)&3);
      *out++=uint8_t((v<<3)&0xF8)|uint8_t((v>>2)&7);
    }
  }
}

