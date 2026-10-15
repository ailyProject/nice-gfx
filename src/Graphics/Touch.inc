// Runtime touch integration; sampling and calibration math live in Touch.h.
uint8_t NICE_GFX::getTouchRaw(uint16_t *x, uint16_t *y) {
  if (!_touch) { _runtimeStatus=nice_gfx::Status::NotInitialized; return 0; }
  const bool pressed=_touch->getTouchRaw(x,y); _runtimeStatus=_touch->status(); return pressed;
}
uint16_t NICE_GFX::getTouchRawZ() {
  if (!_touch) { _runtimeStatus=nice_gfx::Status::NotInitialized; return 0; }
  const uint16_t pressure=_touch->getTouchRawZ(); _runtimeStatus=_touch->status(); return pressure;
}
uint8_t NICE_GFX::getTouch(uint16_t *x,uint16_t *y,uint16_t threshold) {
  if (!_touch) { _runtimeStatus=nice_gfx::Status::NotInitialized; return 0; }
  const bool pressed=_touch->getTouch(x,y,_width,_height,rotation&3,threshold);
  _runtimeStatus=_touch->status(); return pressed;
}
void NICE_GFX::convertRawXY(uint16_t *x,uint16_t *y) {
  if (!_touch) { _runtimeStatus=nice_gfx::Status::NotInitialized; return; }
  if (!x || !y || !_touch->calibration().convert(*x,*y,_width,_height,rotation&3))
    _runtimeStatus=nice_gfx::Status::InvalidArgument;
  else _runtimeStatus=nice_gfx::Status::Ok;
}
void NICE_GFX::setTouch(const uint16_t *data) {
  _runtimeStatus=_touch ? _touch->setCalibration(data,rotation&3) : nice_gfx::Status::NotInitialized;
}
void NICE_GFX::calibrateTouch(uint16_t *data,uint32_t fg,uint32_t bg,uint8_t size) {
  calibrateTouchChecked(data,fg,bg,size);
}
nice_gfx::Status NICE_GFX::calibrateTouchChecked(uint16_t *data,uint32_t fg,uint32_t bg,
                                                   uint8_t size,uint32_t timeoutMs) {
  using nice_gfx::Status;
  if (!_touch || !_touch->ready()) return _runtimeStatus=Status::NotInitialized;
  if (!data || !size || !timeoutMs || _width<4*size || _height<4*size)
    return _runtimeStatus=Status::InvalidArgument;
  nice_gfx::RawTouch corners[4];
  for (uint8_t corner=0;corner<4;++corner) {
    const int32_t x=corner<2 ? 0 : _width-1, y=corner&1 ? _height-1 : 0;
    const int32_t dx=corner<2 ? size : -size, dy=corner&1 ? -size : size;
    fillScreen(bg); drawLine(x,y,x+dx*2,y,fg); drawLine(x,y,x,y+dy*2,fg);
    drawLine(x,y,x+dx,y+dy,fg);
    const uint32_t started=nice_gfx::platform::millis();
    bool released=false; uint8_t samples=0; uint32_t sumX=0,sumY=0;
    while (samples<8) {
      if (uint32_t(nice_gfx::platform::millis()-started)>=timeoutMs) return _runtimeStatus=Status::Busy;
      nice_gfx::RawTouch point;
      const bool pressed=_touch->read(point);
      if (_touch->status()!=Status::Ok) return _runtimeStatus=_touch->status();
      if (!pressed) { released=true; samples=0; sumX=sumY=0; }
      else if (released) { sumX+=point.x; sumY+=point.y; ++samples; }
      nice_gfx::platform::delayMs(10);
    }
    corners[corner].x=uint16_t(sumX/8); corners[corner].y=uint16_t(sumY/8);
  }
  nice_gfx::TouchCalibration calibration;
  Status result=calibration.fromCorners(corners,rotation&3);
  if(result==Status::Ok) result=_touch->setCalibration(calibration);
  if(result==Status::Ok) { calibration.encodeWords(data); fillScreen(bg); }
  return _runtimeStatus=result;
}
