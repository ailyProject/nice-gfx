// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "SeeedHal.h"
#include "Driver.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_SSD1680.h
 * @brief  SSD1680 ePaper display driver for Seeed_GFX v2.0
 *
 * Supports: full refresh, fast refresh, partial refresh, and 4-level grayscale.
 */

#ifndef NGFX_SEEED_SEEED_GFX_DRIVER_SSD1680_H
#define NGFX_SEEED_SEEED_GFX_DRIVER_SSD1680_H


// SSD1680 Command Constants
#define NGFX_SEEED_SSD1680_DRVOUT     0x01  // Driver output control
#define NGFX_SEEED_SSD1680_GSCAN      0x0F  // Gate scan start position
#define NGFX_SEEED_SSD1680_SLPIN      0x10  // Deep sleep mode
#define NGFX_SEEED_SSD1680_DTM        0x11  // Data entry mode
#define NGFX_SEEED_SSD1680_SWRESET    0x12  // Software reset
#define NGFX_SEEED_SSD1680_TEMP       0x18  // Temperature sensor control
#define NGFX_SEEED_SSD1680_MASTER     0x20  // Master activation
#define NGFX_SEEED_SSD1680_DISPCTRL   0x22  // Display update control
#define NGFX_SEEED_SSD1680_RAMBW      0x24  // Write B/W RAM
#define NGFX_SEEED_SSD1680_RAMRED     0x26  // Write Red RAM
#define NGFX_SEEED_SSD1680_LUTOPT     0x1A  // LUT option / temperature
#define NGFX_SEEED_SSD1680_PTLIN      0x3C  // Border waveform / partial in
#define NGFX_SEEED_SSD1680_SETX       0x44  // Set RAM X start/end
#define NGFX_SEEED_SSD1680_SETY       0x45  // Set RAM Y start/end
#define NGFX_SEEED_SSD1680_RAMXCNT    0x4E  // Set RAM X counter
#define NGFX_SEEED_SSD1680_RAMYCNT    0x4F  // Set RAM Y counter

// Data entry mode flags for setRotation
#define NGFX_SEEED_SSD1680_DTM_YDEC_XINC  0x01  // Y decrement, X increment (0 deg)
#define NGFX_SEEED_SSD1680_DTM_YINC_XDEC  0x02  // Y increment, X decrement (180 deg)

class Driver_SSD1680 : public IDriver {
public:
    /**
     * @param w  Display width in pixels (default 128 for 2.9" panel)
     * @param h  Display height in pixels (default 296 for 2.9" panel)
     */
    Driver_SSD1680(uint16_t w = 128, uint16_t h = 296);

    /// Frees the driver-managed partial old-plane cache.
    ~Driver_SSD1680() override;

    // ---- IDriver interface ------------------------------------------------
    const char* name() const override { return "SSD1680"; }
    uint16_t width() const override { return _width; }
    uint16_t height() const override { return _height; }
    uint16_t nativeWidth() const override { return _init_width; }
    uint16_t nativeHeight() const override { return _init_height; }
    uint8_t  colorDepth() const override { return 1; }
    bool supportsDeepSleep() const override { return true; }

    bool     init(IBus& bus) override;
    void     setRotation(uint8_t rotation) override;
    uint8_t  rotation() const override { return _rotation; }
    void     invertDisplay(bool invert) override;
    void     displayOn() override;
    void     displayOff() override;
    void     setAddrWindow(uint16_t xs, uint16_t ys,
                           uint16_t xe, uint16_t ye) override;
    void     writePixel(uint16_t color) override;
    void     writePixels(const uint16_t* data, size_t len) override;
    void     writeFill(uint16_t color, size_t len) override;
    void     sleep() override;
    void     wake() override;
    IBus&    bus() override { return *_bus; }

    // ---- Pin configuration (called by Panel/Board before init) -------------
    void setBusyPin(int pin) override   { _busy_pin = static_cast<int8_t>(pin); }
    void setResetPin(int pin) override  { _rst_pin = static_cast<int8_t>(pin); }
    void setEnablePin(int8_t pin) { _enable_pin = pin; }

    // ---- ePaper-specific public API ---------------------------------------

    /// Full display update (slow, lowest ghosting)
    bool supportsPartialRefresh() const override { return true; }
    bool supportsFastRefresh() const override { return true; }
    bool supportsGrayRefresh(uint8_t levels) const override { return levels == 4; }
    void update() override;

    /// Fast display update (uses LUT, more ghosting)
    void updateFast() override;

    /// Prepare a fast full refresh. The first fast request falls back to a
    /// normal full waveform so the next differential refresh has a baseline.
    void wakeFast() override;

    /// Partial update (only refreshes the current window)
    void updatePartial() override;

    /// Grayscale update (4-level gray)
    void updateGray() override;

    /// Initialize for partial update mode
    void initPartial();

    /// Initialize for 4-level grayscale mode
    void initGray();

    /// Wake up and initialize for partial mode
    void wakePartial() override;

    /// Wake up and initialize for grayscale mode
    void wakeGray() override;

    /// Push B/W pixel data to new-color RAM (0x24)
    void pushNewColors(uint16_t w, uint16_t h, const uint8_t* colors);

    /// Push B/W pixel data to new-color RAM (0x24). During a partial session
    /// the cached old window is written to 0x26 first.
    void pushNewColors(const uint8_t* data, size_t len) override;

    /// Push B/W pixel data to old-color RAM (0x26)
    void pushOldColors(uint16_t w, uint16_t h, const uint8_t* colors);
    void pushOldColors(const uint8_t* colors, size_t len) override {
        if (!_bus || !colors) return;
        _bus->writeCommand(NGFX_SEEED_SSD1680_RAMRED); _bus->writeData(colors, len);
    }

    /// Push B/W pixel data to new-color RAM with horizontal bit-flip per byte
    void pushNewColorsFlip(uint16_t w, uint16_t h, const uint8_t* colors);

    /// Push B/W pixel data to old-color RAM with horizontal bit-flip per byte
    void pushOldColorsFlip(uint16_t w, uint16_t h, const uint8_t* colors);

    /// Push 4-level grayscale data to both RAM planes (0x24 + 0x26)
    /// @param colors  2-bit gray per pixel; total size = w * h / 2 bytes
    void pushNewGrayColors(uint16_t w, uint16_t h, const uint8_t* colors);

    /// Push 4-level grayscale data. Invalidates the 1bpp old-plane cache;
    /// a successful monochrome full refresh is required before partial mode.
    void pushGrayColors(const uint8_t* data, size_t len) override;

    /// Push 4-level grayscale data (same as pushNewGrayColors; no flip)
    void pushNewGrayColorsFlip(uint16_t w, uint16_t h, const uint8_t* colors);

private:
    /// Poll the BUSY pin until the controller is ready (or no-op if no pin)
    void checkBusy();

    /// Hardware + software reset sequence
    void reset();

    // ---- Driver-managed partial old-plane cache ----------------------------
    // Controller-space 1bpp, 1 = white, stride = (_init_width + 7) / 8;
    // updated in place (no staging allocation), invalidated on BUSY timeout.

    size_t cacheStride() const { return ((size_t)_init_width + 7U) / 8U; }
    size_t frameBytes() const { return cacheStride() * _init_height; }

    bool ensureOldCache();       ///< lazy allocation; false on OOM
    void invalidateOldCache();   ///< free cache and mark the baseline unknown
    bool windowGeometry(size_t* rowBytes, size_t* rows, size_t* total) const;
    void pushOldWindow(size_t rowBytes, size_t rows);  ///< 0x26 from cache
    void cacheWindow(const uint8_t* data, size_t rowBytes, size_t rows);
    void finishRefresh(bool success);

    uint8_t* _oldCache = nullptr;  ///< full-frame 1bpp snapshot

    uint16_t _winX0 = 0, _winY0 = 0, _winX1 = 0, _winY1 = 0;  ///< setAddrWindow cache

    bool _partialActive       = false;  ///< wakePartial() .. updatePartial()
    bool _lastPartialTimedOut = false;  ///< one-shot sleep() guard
    bool _oldCacheValid       = false;  ///< cache exactly matches the glass
    bool _cacheUpdatePending  = false;  ///< cache changed; refresh not confirmed
    bool _fastFallbackFull    = false;  ///< first fast request uses 0xF7

    uint16_t _init_width  = 128;
    uint16_t _init_height = 296;
    int8_t   _busy_pin    = -1;
    int8_t   _rst_pin     = -1;
    int8_t   _enable_pin  = -1;
};

#endif // NGFX_SEEED_SEEED_GFX_DRIVER_SSD1680_H

} }
