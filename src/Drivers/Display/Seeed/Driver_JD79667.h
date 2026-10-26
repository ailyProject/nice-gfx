// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "SeeedHal.h"
#include "Driver.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_JD79667.h
 * @brief  JD79667 ePaper display driver for Seeed_GFX v2.0
 *
 * 800x480 4-bit (16-level grayscale) ePaper driver (JD796xx family).
 */

#ifndef NGFX_SEEED_SEEED_GFX_DRIVER_JD79667_H
#define NGFX_SEEED_SEEED_GFX_DRIVER_JD79667_H


class Driver_JD79667 : public IDriver {
public:
    Driver_JD79667(uint16_t w = 800, uint16_t h = 480, int8_t busyPin = -1);

    const char* name() const override { return "JD79667"; }
    uint16_t width() const override { return _width; }
    uint16_t height() const override { return _height; }
    uint8_t colorDepth() const override { return 4; }
    bool supportsBWRYEPaper() const override { return true; }
    bool supportsDeepSleep() const override { return true; }

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
    void setTemp(uint8_t temp) { (void)temp; }
    void setTemperature(int8_t temp) override { setTemp(static_cast<uint8_t>(temp)); }

private:
    void busyWait();
    uint8_t colorGet(uint8_t color);
    uint16_t _init_width, _init_height;
    int8_t _busyPin;
};

#endif // NGFX_SEEED_SEEED_GFX_DRIVER_JD79667_H

} }
