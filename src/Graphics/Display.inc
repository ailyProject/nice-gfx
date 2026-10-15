// Included by NICE_GFX.cpp. No global SPI handles or hardware pin macros.
void NICE_GFX::pushBlock(uint16_t color, uint32_t len) {
  if (_panel) _runtimeStatus = _panel->fill(color, len);
}
void NICE_GFX::pushPixels(const void *data, uint32_t len) {
  if (_panel && len) _runtimeStatus = _panel->pixels(nice_gfx::PixelView(data, len, 1, 0,
      _swapBytes ? nice_gfx::PixelOrder::Native : nice_gfx::PixelOrder::Wire));
}
bool NICE_GFX::initDMA() {
  DMA_Enabled = _panel && _panel->ready() && _panel->asynchronous();
  return DMA_Enabled;
}
void NICE_GFX::deInitDMA() { dmaWait(); DMA_Enabled = false; }
bool NICE_GFX::dmaBusy() { return _panel && _panel->busy(); }
void NICE_GFX::dmaWait() {
  if (_panel) { const auto s = _panel->finish(); if (_runtimeStatus == nice_gfx::Status::Ok) _runtimeStatus = s; }
}
void NICE_GFX::pushPixelsDMA(uint16_t *data, uint32_t len) {
  if (_panel && len) _runtimeStatus = _panel->pixels(nice_gfx::PixelView(data, len, 1, 0,
      _swapBytes ? nice_gfx::PixelOrder::Native : nice_gfx::PixelOrder::Wire), DMA_Enabled);
}
void NICE_GFX::pushImageDMA(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data, uint16_t *buffer) {
  if (!_panel || !data || w <= 0 || h <= 0) return;
  // PI_CLIP retains source stride and origin; no source compaction or byte mutation.
  PI_CLIP;
  const uint16_t *source = data + size_t(dy) * size_t(w) + size_t(dx);
  size_t stride = w;
  if (buffer) {
    dmaWait(); // The caller may reuse the previous transfer's staging buffer.
    for (int32_t row=0; row<dh; ++row)
      memmove(buffer + size_t(row)*dw, source + size_t(row)*w, size_t(dw)*2);
    source = buffer; stride = dw;
  }
  _runtimeStatus = _panel->flush(x, y, nice_gfx::PixelView(source, dw, dh, stride,
      _swapBytes ? nice_gfx::PixelOrder::Native : nice_gfx::PixelOrder::Wire), DMA_Enabled);
}
void NICE_GFX::pushImageDMA(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data) {
  // The pixel source is read without modifying it.
  pushImageDMA(x, y, w, h, const_cast<uint16_t *>(data), nullptr);
}
nice_gfx::Status NICE_GFX::beginChecked() {
  if (!_panel) return _runtimeStatus = nice_gfx::Status::NotInitialized;
  _runtimeStatus = _panel->begin();
  if (_runtimeStatus == nice_gfx::Status::Ok) {
    _width = _init_width = _panel->width(); _height = _init_height = _panel->height();
    rotation = _panel->rotation();
    resetViewport();
  }
  return _runtimeStatus;
}
nice_gfx::Status NICE_GFX::flush(int32_t x, int32_t y, nice_gfx::PixelView view, bool async) {
  if (!_panel) return _runtimeStatus = nice_gfx::Status::NotInitialized;
  return _runtimeStatus = _panel->flush(x, y, view, async);
}

void NICE_GFX::appendPixel(uint16_t color) {
  if (_panel && _runtimeStatus == nice_gfx::Status::Ok) {
    _panel->append(color); _runtimeStatus = _panel->status();
  }
}
