/***************************************************************************************
** Function name:           setAddrWindow
** Description:             define an area to receive a stream of pixels
***************************************************************************************/
// Chip select is high at the end of this function
void NICE_GFX::setAddrWindow(int32_t x0, int32_t y0, int32_t w, int32_t h)
{
  const int64_t right=int64_t(x0)+w-1, bottom=int64_t(y0)+h-1;
  if(w<=0 || h<=0 || right>INT32_MAX || bottom>INT32_MAX) { _runtimeStatus=nice_gfx::Status::InvalidArgument; return; }
  beginPanelWrite(); setWindow(x0,y0,int32_t(right),int32_t(bottom)); endPanelWrite();
}


/***************************************************************************************
** Function name:           setWindow
** Description:             define an area to receive a stream of pixels
***************************************************************************************/
// Chip select stays low, call beginPanelWrite first. Use setAddrWindow() from sketches
void NICE_GFX::setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
  if (_panel) _runtimeStatus = _panel->window(x0, y0, x1, y1);
}

/***************************************************************************************
** Function name:           drawPixel
** Description:             push a single pixel at an arbitrary position
***************************************************************************************/
void NICE_GFX::drawPixel(int32_t x, int32_t y, uint32_t color)
{
  nice_gfx::ClippedRect r;
  if (!_panel || _vpOoB || !nice_gfx::clipRect(x,y,1,1,_xDatum,_yDatum,_vpX,_vpY,_vpW,_vpH,r)) return;
  _runtimeStatus = _panel->window(r.x,r.y,r.x,r.y);
  if (_runtimeStatus == nice_gfx::Status::Ok) _runtimeStatus = _panel->fill(uint16_t(color),1);
}

/***************************************************************************************
** Function name:           pushColor
** Description:             push a single pixel
***************************************************************************************/
void NICE_GFX::pushColor(uint16_t color)
{
  beginPanelWrite();

  appendPixel(color);

  endPanelWrite();
}


/***************************************************************************************
** Function name:           startWrite
** Description:             begin transaction with CS low, MUST later call endWrite
***************************************************************************************/
void NICE_GFX::startWrite(void)
{
  if (_writeDepth == UINT32_MAX) { _runtimeStatus=nice_gfx::Status::InvalidArgument; return; }
  if (_writeDepth++ == 0) begin_nin_write();
  inTransaction = true;
}

/***************************************************************************************
** Function name:           endWrite
** Description:             end transaction with CS high
***************************************************************************************/
void NICE_GFX::endWrite(void)
{
  if (!_writeDepth || --_writeDepth) return;
  inTransaction = false;
  end_nin_write(); // Finishes the panel and drains accepted DMA descriptors.
}

