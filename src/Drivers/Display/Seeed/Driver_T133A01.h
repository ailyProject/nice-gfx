// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "SeeedHal.h"
#include "Driver.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_T133A01.h
 * @brief  T133A01 dual-controller color ePaper driver API
 *
 * 1200x1600 4-bit six-color ePaper driver with master/slave chip selects.
 */

#ifndef NGFX_SEEED_SEEED_GFX_DRIVER_T133A01_H
#define NGFX_SEEED_SEEED_GFX_DRIVER_T133A01_H


class Driver_T133A01 : public IDriver {
public:
    Driver_T133A01(uint16_t w = 1200, uint16_t h = 1600, int8_t busyPin = -1);

    const char* name() const override { return "T133A01"; }
    uint16_t width() const override { return _width; }
    uint16_t height() const override { return _height; }
    uint8_t colorDepth() const override { return 4; }
    bool supportsColorfulEPaper() const override { return true; }
    bool supportsDeepSleep() const override { return true; }
    bool supportsTemperatureCompensation() const override { return true; }

    bool init(IBus& bus) override;
    void setRotation(uint8_t rotation) override;
    uint8_t rotation() const override { return _rotation; }
    void invertDisplay(bool invert) override;
    void displayOn() override;
    void displayOff() override;
    void setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye) override;
    void writePixel(uint16_t color) override;
    void writePixels(const uint16_t* data, size_t len) override;
    void writeFill(uint16_t color, size_t len) override;
    void sleep() override;
    void wake() override;
    IBus& bus() override { return *_bus; }

    // ePaper-specific
    void setBusyPin(int pin) override { _busyPin = static_cast<int8_t>(pin); }
    void setResetPin(int pin) override { _resetPin = static_cast<int8_t>(pin); }
    void update() override;
    void pushColors(const uint8_t* data, uint16_t w, uint16_t h);
    void pushNewColors(const uint8_t* data, size_t len) override {
        (void)len; pushColors(data, _init_width, _init_height);
    }
    void pushOldColors(const uint8_t* data, size_t len) override {
        (void)len; pushOldColors(data, _init_width, _init_height);
    }
    void pushColorsFlip(const uint8_t* data, uint16_t w, uint16_t h);
    void pushOldColors(const uint8_t* data, uint16_t w, uint16_t h);
    void pushOldColorsFlip(const uint8_t* data, uint16_t w, uint16_t h);
    void setTemp(uint8_t temp);
    void setTemperature(int8_t temp) override { setTemp(static_cast<uint8_t>(temp)); }

private:
    void busyWait();
    uint8_t colorGet(uint8_t color);
    void writeCommandData(ChipSelectTarget target, uint8_t cmd,
                          const uint8_t* data, size_t len);
    uint16_t _init_width, _init_height;
    int8_t _busyPin;
    int8_t _resetPin = -1;
};

#endif // NGFX_SEEED_SEEED_GFX_DRIVER_T133A01_H

} }
