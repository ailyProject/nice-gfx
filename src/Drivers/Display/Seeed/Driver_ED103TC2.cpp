// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "Driver_ED103TC2.h"
namespace nice_gfx { namespace seeed_protocol {

Driver_ED103TC2::Driver_ED103TC2(uint16_t w, uint16_t h, int8_t busyPin)
    : Driver_IT8951(w, h, busyPin), _panelWidth(w), _panelHeight(h) {
    _window.usX = 0;
    _window.usY = 0;
    _window.usWidth = w;
    _window.usHeight = h;
}

void Driver_ED103TC2::setAddrWindow(uint16_t xs, uint16_t ys,
                                     uint16_t xe, uint16_t ye) {
    Driver_IT8951::setAddrWindow(xs, ys, xe, ye);
    _window.usX = xs;
    _window.usY = ys;
    _window.usWidth = static_cast<uint16_t>(xe - xs + 1);
    _window.usHeight = static_cast<uint16_t>(ye - ys + 1);
}

void Driver_ED103TC2::update() {
    // Panel_EPaper stores monochrome pixels as 1=white and 0=black, so the
    // IT8951 1bpp lookup table must map foreground (bit 1) to full white and
    // background (bit 0) to black.  Use 0xFF rather than 0xF0 for the white
    // endpoint; the latter is only gray level 15 shifted into the high nibble
    // and cannot perform a true white clearing refresh.
    tconDisplayArea1bpp(0, 0, _panelWidth, _panelHeight,
                        NGFX_SEEED_IT8951_MODE_2, 0x00, 0xFF);
}

void Driver_ED103TC2::updatePartial() {
    tconDisplayArea1bpp(_window.usX, _window.usY,
                        _window.usWidth, _window.usHeight,
                        NGFX_SEEED_IT8951_MODE_1, 0x00, 0xFF);
}

void Driver_ED103TC2::updateGray() {
    tconDisplayArea(_window.usX, _window.usY,
                    _window.usWidth, _window.usHeight, NGFX_SEEED_IT8951_MODE_2);
    tconWaitForDisplayReady();
}

void Driver_ED103TC2::pushNewColors(const uint8_t* data, size_t len) {
    (void)len;
    if (!data) return;
    tconLoad1bppImage(data, _window.usX, _window.usY,
                      _window.usWidth, _window.usHeight, 0);
}

void Driver_ED103TC2::pushOldColors(const uint8_t* data, size_t len) {
    // IT8951 owns a single image buffer, so it has no separate old-data plane.
    (void)data;
    (void)len;
}

void Driver_ED103TC2::pushGrayColors(const uint8_t* data, size_t len) {
    (void)len;
    if (!data) return;
    tconLoadImage(data, _window.usX, _window.usY,
                  _window.usWidth, _window.usHeight, 0);
}

void Driver_ED103TC2::setTemperature(int8_t temp) {
    if (_bus) setTconTemp(static_cast<uint8_t>(temp));
}

} }
