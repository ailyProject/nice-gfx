/***************************************************************************************
** Function name:           beginPanelWrite
** Description:             Start SPI transaction for writes and select TFT
***************************************************************************************/
inline void NICE_GFX::beginPanelWrite(void){
  if (locked) _runtimeStatus = _panel && _panel->ready() ? nice_gfx::Status::Ok : nice_gfx::Status::NotInitialized;
  locked = false;
}

// Non-inlined version to permit override
void NICE_GFX::begin_nin_write(void){
  beginPanelWrite();
}

/***************************************************************************************
** Function name:           endPanelWrite
** Description:             End transaction for write and deselect TFT
***************************************************************************************/
inline void NICE_GFX::endPanelWrite(void){
  if (!inTransaction) {
    if (_panel) {
      const auto status = _panel->finish();
      if (_runtimeStatus == nice_gfx::Status::Ok) _runtimeStatus = status;
    }
    locked = true;
  }
}

// Non-inlined version to permit override
void NICE_GFX::end_nin_write(void){
  endPanelWrite();
}
