// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "SeeedHal.h"
#include "Driver.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_SSD1677.h
 * @brief  SSD1677 ePaper display driver for Seeed_GFX v2.0
 *
 * 800x480 1-bit large ePaper driver with partial and grayscale support.
 */

#ifndef NGFX_SEEED_SEEED_GFX_DRIVER_SSD1677_H
#define NGFX_SEEED_SEEED_GFX_DRIVER_SSD1677_H


class Driver_SSD1677 : public IDriver {
public:
    Driver_SSD1677(uint16_t w = 800, uint16_t h = 480, int8_t busyPin = -1);
    ~Driver_SSD1677();

    const char* name() const override { return "SSD1677"; }
    uint16_t width() const override { return _width; }
    uint16_t height() const override { return _height; }
    uint8_t colorDepth() const override { return 1; }
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
    bool supportsPartialRefresh() const override { return true; }
    bool supportsGrayRefresh(uint8_t levels) const override { return levels == 4; }
    void setBusyPin(int pin) override { _busyPin = static_cast<int8_t>(pin); }
    void update() override;
    void updateGray() override;
    void updatePartial() override;
    void initGray();
    void initPartial();
    void wakeGray() override;
    void wakePartial() override;
    void pushColors(const uint8_t* data, uint16_t w, uint16_t h);
    void pushNewColors(const uint8_t* data, size_t len) override;
    void pushOldColors(const uint8_t* data, size_t len) override;
    void setInvertMono(bool invert) { _invertMono = invert; }
    void pushColorsFlip(const uint8_t* data, uint16_t w, uint16_t h);
    void pushOldColors(const uint8_t* data, uint16_t w, uint16_t h);
    void pushOldColorsFlip(const uint8_t* data, uint16_t w, uint16_t h);
    void pushGrayColors(const uint8_t* data, uint16_t w, uint16_t h);
    void pushGrayColors(const uint8_t* data, size_t len) override {
        (void)len;
        pushGrayColors(data, _init_width, _init_height);
    }
    void pushGrayColorsFlip(const uint8_t* data, uint16_t w, uint16_t h);
    void setTemp(uint8_t temp) { (void)temp; }
    void setTemperature(int8_t temp) override { setTemp(static_cast<uint8_t>(temp)); }

private:
    void busyWait();
    void writeMonoData(const uint8_t* data, size_t len);
    void ensureBaseline();
    uint16_t _init_width, _init_height;
    int8_t _busyPin;
    bool _invertMono = false;
    uint8_t* _partialBaseline = nullptr;  // last full frame (native/sent order)
    bool _partialActive = false;          // between wakePartial() and updatePartial()
    uint16_t _winX0 = 0, _winY0 = 0, _winX1 = 0, _winY1 = 0;  // last setAddrWindow()
};

#endif // NGFX_SEEED_SEEED_GFX_DRIVER_SSD1677_H

} }
