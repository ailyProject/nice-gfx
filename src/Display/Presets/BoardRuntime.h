#pragma once
#include "BoardCatalog.h"
#include "../../Core/Platform.h"
#if NICE_GFX_NATIVE_IDF && defined(ESP_PLATFORM)
#include <driver/gpio.h>
#endif

namespace nice_gfx { namespace detail {
inline Status boardOutput(int pin,bool high) {
  if(pin<0)return Status::Ok;
#if NICE_GFX_NATIVE_IDF && defined(ESP_PLATFORM)
  if(!GPIO_IS_VALID_OUTPUT_GPIO(pin))return Status::InvalidArgument;
  if(gpio_set_level(gpio_num_t(pin),high?1:0)!=ESP_OK ||
     gpio_set_direction(gpio_num_t(pin),GPIO_MODE_OUTPUT)!=ESP_OK)return Status::IoError;
#elif !NICE_GFX_NATIVE_IDF
  pinMode(pin,OUTPUT);digitalWrite(pin,high?HIGH:LOW);
#else
  (void)high;return Status::Unsupported;
#endif
  return Status::Ok;
}

// Fixed board ownership adds only startup policy; chip programs stay shared.
template<class Display,BoardPresetId Id> class FixedBoardDisplay : public Display {
 public:
  static constexpr bool available=true;
  static const BoardPresetInfo &preset() { return BoardPresetTraits<Id>::info(); }
  bool begin() { return beginChecked()==Status::Ok; }
  Status beginChecked() {
    if(started_)return boardStatus_=Display::beginChecked();
    boardStatus_=writeBoardPin(preset().pins.backlight,!backlightHigh());
    if(boardStatus_==Status::Ok)boardStatus_=prepareBoard();
    if(boardStatus_==Status::Ok)boardStatus_=Display::beginChecked();
    if(boardStatus_==Status::Ok) {
      this->setRotation(preset().rotation);
      boardStatus_=Display::lastError();
    }
    if(boardStatus_==Status::Ok)boardStatus_=finishBoard();
    if(boardStatus_==Status::Ok)boardStatus_=writeBoardPin(preset().pins.backlight,backlightHigh());
    started_=boardStatus_==Status::Ok;
    return boardStatus_;
  }
  Status lastError() const { return boardStatus_==Status::Ok?Display::lastError():boardStatus_; }
 protected:
  template<class... Args> explicit FixedBoardDisplay(Args&&... args)
    :Display(static_cast<Args&&>(args)...) {}
  virtual Status prepareBoard() { return Status::Ok; }
  virtual Status finishBoard() { return Status::Ok; }
  virtual bool backlightHigh() const { return true; }
  virtual Status writeBoardPin(int pin,bool high) { return boardOutput(pin,high); }
 private:
  Status boardStatus_=Status::NotInitialized;
  bool started_=false;
};
} } // namespace nice_gfx::detail
