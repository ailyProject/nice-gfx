/***************************************************************************************
** Function name:           NICE_GFX
** Description:             Bind a panel or create an offscreen renderer
***************************************************************************************/
NICE_GFX::NICE_GFX(nice_gfx::Panel &panel) : NICE_GFX(panel.width(), panel.height()) { _panel = &panel; }

NICE_GFX::~NICE_GFX() { if (_panel) _panel->finish(); }

NICE_GFX::NICE_GFX(int16_t w, int16_t h)
{
  _init_width  = _width  = w;
  _init_height = _height = h;

  // Reset the viewport to the whole screen
  resetViewport();

  rotation  = 0;
  cursor_y = cursor_x = 0;
  textsize  = 1;
  textcolor   = bitmap_fg = 0xFFFF; // White
  textbgcolor = bitmap_bg = 0x0000; // Black
  padX        = 0;                  // No padding


  textwrapX  = true;    // Wrap text at end of line when using print stream
  textwrapY  = false;   // Wrap text at bottom of screen when using print stream
  textdatum = TL_DATUM; // Top Left text alignment is default

  _swapBytes = false;   // Do not swap colour bytes by default

  locked = true;           // Transaction mutex lock flag to ensure begin/endTranaction pairing
  inTransaction = false;   // Flag to prevent multiple sequential functions to keep bus access open

  _utf8     = true;     // UTF8 decoding enabled


  _psram_enable = nice_gfx::platform::psramFound();

  _xPivot = 0;
  _yPivot = 0;


}

/***************************************************************************************
** Function name:           begin
** Description:             Initialize the instantiated display
***************************************************************************************/
void NICE_GFX::begin()
{
  beginChecked();
}


/***************************************************************************************
** Function name:           setRotation
** Description:             rotate the screen orientation m = 0-3
***************************************************************************************/
void NICE_GFX::setRotation(uint8_t m)
{
  if (!_panel) return;
  _runtimeStatus = _panel->setRotation(m);
  if (_runtimeStatus != nice_gfx::Status::Ok) return;
  rotation = _panel->rotation(); _width = _panel->width(); _height = _panel->height();
  resetViewport();
}


/***************************************************************************************
** Function name:           getRotation
** Description:             Return the rotation value (as used by setRotation())
***************************************************************************************/
uint8_t NICE_GFX::getRotation(void)
{
  return rotation;
}


/***************************************************************************************
** Function name:           setOrigin
** Description:             Set graphics origin to position x,y wrt to top left corner
***************************************************************************************/
//Note: setRotation, setViewport and resetViewport will revert origin to top left
void NICE_GFX::setOrigin(int32_t x, int32_t y)
{
  _xDatum = x;
  _yDatum = y;
}


/***************************************************************************************
** Function name:           getOriginX
** Description:             Set graphics origin to position x
***************************************************************************************/
int32_t NICE_GFX::getOriginX(void)
{
  return _xDatum;
}


/***************************************************************************************
** Function name:           getOriginY
** Description:             Set graphics origin to position y
***************************************************************************************/
int32_t NICE_GFX::getOriginY(void)
{
  return _yDatum;
}


/***************************************************************************************
** Function name:           commandList, used for FLASH based lists only (e.g. ST7735)
** Description:             Get initialisation commands from FLASH and send to TFT
***************************************************************************************/
void NICE_GFX::commandList (const uint8_t *addr)
{
  uint8_t  numCommands;
  uint8_t  numArgs;
  uint8_t  ms;

  numCommands = pgm_read_byte(addr++);   // Number of commands to follow

  while (numCommands--)                  // For each command...
  {
    const uint8_t command = pgm_read_byte(addr++);
    numArgs = pgm_read_byte(addr++);
    ms = numArgs & 0x80;
    numArgs &= ~0x80;

    uint8_t args[128];
    for (uint8_t i = 0; i < numArgs; ++i) args[i] = pgm_read_byte(addr++);
    writecommand(command, args, numArgs);

    if (ms)
    {
      ms = pgm_read_byte(addr++);        // Read post-command delay time (ms)
      nice_gfx::platform::delayMs(ms == 255 ? 500 : ms);
    }
  }

}



/***************************************************************************************
** Function name:           writecommand
** Description:             Send an 8-bit command to the TFT
***************************************************************************************/
void NICE_GFX::writecommand(uint16_t c)
{
  if (_panel) _runtimeStatus = _panel->command(c);
}

/***************************************************************************************
** Function name:           writecommand
** Description:             Send an 8-bit command and its parameters atomically
***************************************************************************************/
void NICE_GFX::writecommand(uint16_t c, const uint8_t *data, uint32_t len)
{
  if (_panel) _runtimeStatus = _panel->command(c, data, len);
}

/***************************************************************************************
** Function name:           writedata
** Description:             Send a 8-bit data value to the TFT
***************************************************************************************/
void NICE_GFX::writeRegister8(uint16_t address, uint8_t value) { writecommand(address, &value, 1); }
void NICE_GFX::writeRegister16(uint16_t address, uint16_t value) {
  if (_panel) _runtimeStatus = _panel->writeRegister16(address, value);
}
void NICE_GFX::writedata(uint8_t d)
{
  if (_panel) _runtimeStatus = _panel->data(&d, 1);
}


/***************************************************************************************
** Function name:           readcommand8
** Description:             Read a 8-bit data value from an indexed command register
***************************************************************************************/
uint8_t NICE_GFX::readcommand8(uint16_t cmd_function, uint8_t index)
{
  if (!_panel) return 0;
  uint8_t value = 0;
  _runtimeStatus = _panel->readIndexedCommand(cmd_function, index, value);
  return _runtimeStatus == nice_gfx::Status::Ok ? value : 0;
}


/***************************************************************************************
** Function name:           readcommand16
** Description:             Read a 16-bit data value from an indexed command register
***************************************************************************************/
uint16_t NICE_GFX::readcommand16(uint16_t cmd_function, uint8_t index)
{
  uint32_t reg;

  reg  = (readcommand8(cmd_function, index + 0) <<  8);
  reg |= (readcommand8(cmd_function, index + 1) <<  0);

  return reg;
}


/***************************************************************************************
** Function name:           readcommand32
** Description:             Read a 32-bit data value from an indexed command register
***************************************************************************************/
uint32_t NICE_GFX::readcommand32(uint16_t cmd_function, uint8_t index)
{
  uint32_t reg;

  reg  = ((uint32_t)readcommand8(cmd_function, index + 0) << 24);
  reg |= ((uint32_t)readcommand8(cmd_function, index + 1) << 16);
  reg |= ((uint32_t)readcommand8(cmd_function, index + 2) <<  8);
  reg |= ((uint32_t)readcommand8(cmd_function, index + 3) <<  0);

  return reg;
}


/***************************************************************************************
** Function name:           read pixel (for SPI Interface II i.e. IM [3:0] = "1101")
** Description:             Read 565 pixel colours from a pixel
***************************************************************************************/
uint16_t NICE_GFX::readPixel(int32_t x0, int32_t y0)
{
  uint16_t color = 0;
  nice_gfx::ClippedRect r;
  if (!_panel || _vpOoB || !nice_gfx::clipRect(x0,y0,1,1,_xDatum,_yDatum,_vpX,_vpY,_vpW,_vpH,r)) return 0;
  _runtimeStatus = _panel->readPixel(r.x, r.y, color);
  return color;
}




/***************************************************************************************
** Function name:           read rectangle (for SPI Interface II i.e. IM [3:0] = "1101")
** Description:             Read 565 pixel colours from a defined area
***************************************************************************************/
void NICE_GFX::readRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data)
{
  if (!_panel || !data) { _runtimeStatus = nice_gfx::Status::InvalidArgument; return; }
  PI_CLIP;
  for (int32_t row = 0; row < dh; ++row) {
    uint16_t *out = data + size_t(row + dy)*size_t(w) + size_t(dx);
    _runtimeStatus = _panel->readRect(x, y + row, dw, 1, out);
    if (_runtimeStatus != nice_gfx::Status::Ok) return;
    // The established readRect API returns wire-order RGB565 for pushRect.
    for (int32_t i=0; i<dw; ++i) out[i] = uint16_t((out[i] >> 8) | (out[i] << 8));
  }
}
