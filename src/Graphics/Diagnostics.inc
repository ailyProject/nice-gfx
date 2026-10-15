nice_gfx::Status NICE_GFX::setPixelClockHz(uint32_t hz) {
  return _panel ? _panel->setPixelClockHz(hz) : nice_gfx::Status::Unsupported;
}
// Return the instantiated device's state without allocating strings or storage.
DisplayInfo NICE_GFX::displayInfo() const {
  DisplayInfo info;
  info.nativeWidth = _init_width;
  info.nativeHeight = _init_height;
  info.width = _width;
  info.height = _height;
  info.rotation = rotation;
  if (_panel) {
    info.controllerId = _panel->id();
    info.resetPin = _panel->resetPin();
    info.initialized = _panel->ready();
    info.asynchronous = _panel->asynchronous();
    info.bus = _panel->transportInfo();
  }
  return info;
}
