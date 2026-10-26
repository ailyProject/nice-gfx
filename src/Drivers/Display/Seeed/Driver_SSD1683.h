// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "SeeedHal.h"
#include "Driver.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_SSD1683.h
 * @brief  SSD1683 ePaper display driver for Seeed_GFX v2.0
 *
 * 400x300 1-bit ePaper driver with partial and 4-level grayscale support.
 */

#ifndef NGFX_SEEED_SEEED_GFX_DRIVER_SSD1683_H
#define NGFX_SEEED_SEEED_GFX_DRIVER_SSD1683_H


class Driver_SSD1683 : public IDriver {
public:
    Driver_SSD1683(uint16_t w = 400, uint16_t h = 300, int8_t busyPin = -1);
    ~Driver_SSD1683() override;

    const char* name() const override { return "SSD1683"; }
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
    bool supportsFastRefresh() const override { return true; }
    bool supportsGrayRefresh(uint8_t levels) const override { return levels == 4; }
    void setBusyPin(int pin) override { _busyPin = static_cast<int8_t>(pin); }
    void update() override;
    void updateFast() override;
    void wakeFast() override;
    void updateGray() override;
    void updatePartial() override;
    void initGray();
    void initPartial();
    void wakeGray() override;
    void wakePartial() override;
    void pushColors(const uint8_t* data, uint16_t w, uint16_t h);
    /** Push the new plane (0x24). During a partial session the driver also
     *  pushes its cached old window into 0x26 first. */
    void pushNewColors(const uint8_t* data, size_t len) override;
    void pushOldColors(const uint8_t* data, size_t len) override {
        if (!_bus || !data) return;
        _bus->writeCommand(0x26); _bus->writeData(data, len);
    }
    void pushColorsFlip(const uint8_t* data, uint16_t w, uint16_t h);
    void pushOldColors(const uint8_t* data, uint16_t w, uint16_t h);
    void pushOldColorsFlip(const uint8_t* data, uint16_t w, uint16_t h);
    void pushGrayColors(const uint8_t* data, uint16_t w, uint16_t h);
    void pushGrayColors(const uint8_t* data, size_t len) override {
        (void)len;
        // Gray content cannot be represented by the 1bpp old-plane cache. A
        // monochrome full refresh must re-establish the partial baseline.
        invalidateOldCache();
        pushGrayColors(data, _init_width, _init_height);
    }
    void pushGrayColorsFlip(const uint8_t* data, uint16_t w, uint16_t h);
    void setTemp(uint8_t temp) { (void)temp; }
    void setTemperature(int8_t temp) override { setTemp(static_cast<uint8_t>(temp)); }

private:
    void busyWait();

    // --- Driver-local old-plane cache for partial refresh. Updates commit
    // only after a successful busy wait; otherwise the cache is invalidated.
    size_t cacheStride() const { return ((size_t)_init_width + 7U) / 8U; }
    size_t frameBytes() const { return cacheStride() * _init_height; }
    bool ensureOldCache();
    void invalidateOldCache();
    bool windowGeometry(size_t* rowBytes, size_t* rows, size_t* total) const;
    void pushOldWindow(size_t rowBytes, size_t rows);
    void cacheWindow(const uint8_t* data, size_t rowBytes, size_t rows);
    void finishRefresh(bool success);

    uint16_t _init_width, _init_height;
    int8_t _busyPin;

    uint8_t* _oldCache = nullptr;      // full-frame 1bpp snapshot, controller space (1 = white)
    uint16_t _winX0 = 0, _winY0 = 0, _winX1 = 0, _winY1 = 0;  // last setAddrWindow()
    bool _partialActive = false;       // between wakePartial() and updatePartial()/sleep()/init()
    bool _lastPartialTimedOut = false; // one-shot deep-sleep guard after a partial busy timeout
    bool _oldCacheValid = false;       // cache exactly matches the glass
    bool _cacheUpdatePending = false;  // cache changed; refresh not confirmed
    bool _fastFallbackFull = false;    // first fast request uses 0xF7
};

#endif // NGFX_SEEED_SEEED_GFX_DRIVER_SSD1683_H

} }
