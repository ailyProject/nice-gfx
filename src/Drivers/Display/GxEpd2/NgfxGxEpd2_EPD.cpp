// GxEPD2 protocol HAL port; original author Jean-Marc Zingg, GNU GPL v3.
// Source and modifications: NgfxGxEpd2_EPD.h, upstream-manifest.json, LICENSE.
#include "NgfxGxEpd2_EPD.h"
NgfxGxEpd2Spi NgfxGxEpd2_EPD::_ngfxSpi;
NgfxGxEpd2Serial NgfxGxEpd2_EPD::_ngfxSerial;
namespace nice_gfx {
#if defined(ESP32)||defined(ARDUINO_ARCH_ESP32)||defined(ESP_PLATFORM)||(!defined(ARDUINO)&&!defined(__AVR__))
static thread_local GxEpd2Hal *currentHal=nullptr;
#else
static GxEpd2Hal *currentHal=nullptr;
#endif
GxEpd2Hal *gxEpd2CurrentHal(){return currentHal;}
GxEpd2Hal *gxEpd2SetCurrentHal(GxEpd2Hal *next){GxEpd2Hal *old=currentHal;currentHal=next;return old;}
}
