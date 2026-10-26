// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include <stdint.h>
#include <stddef.h>
#include "Bus.h"
#include "SeeedHal.h"
#include "seeed_ep.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver.h
 * @brief  IDriver abstract interface for Seeed_GFX v2.0
 *
 * The Driver layer encapsulates the behavior of a specific display
 * driver IC: initialization sequence, address window, pixel writing,
 * rotation, and power management.
 * Each driver IC has its own Driver subclass.
 */

#ifndef NGFX_SEEED_SEEED_GFX_DRIVER_H
#define NGFX_SEEED_SEEED_GFX_DRIVER_H





/** Sticky status reported by synchronous driver operations. */
enum class DriverOperationError : uint8_t {
    None = 0,
    BusyTimeout,
    CommunicationFailed,
};

class IDriver {
public:
    virtual ~IDriver() = default;

    // Basic properties

    /** Display width in pixels (current rotation) */
    virtual uint16_t width() const = 0;

    /** Display height in pixels (current rotation) */
    virtual uint16_t height() const = 0;

    /** Physical panel width, independent of the current rotation. */
    virtual uint16_t nativeWidth() const {
        return (_rotation & 1U) ? height() : width();
    }

    /** Physical panel height, independent of the current rotation. */
    virtual uint16_t nativeHeight() const {
        return (_rotation & 1U) ? width() : height();
    }

    /** Color depth in bits per pixel */
    virtual uint8_t colorDepth() const { return 16; }

    /** Human-readable driver name */
    virtual const char* name() const = 0;

    virtual bool supportsReadback() const { return false; }
    virtual bool supportsPartialRefresh() const { return false; }
    /** Required X-coordinate alignment for partial windows, or 0 for default. */
    virtual uint16_t partialXAlignment() const { return 0; }
    /** Return true when updateFast() has a controller-specific implementation. */
    virtual bool supportsFastRefresh() const { return false; }
    /** Return true when the controller has a working grayscale waveform. */
    virtual bool supportsGrayRefresh(uint8_t levels) const {
        (void)levels;
        return false;
    }
    virtual bool supportsColorfulEPaper() const { return false; }
    virtual bool supportsBWRYEPaper() const { return false; }
    /** True only when the concrete driver has a working deep-sleep command. */
    virtual bool supportsDeepSleep() const { return false; }
    /** Return true when setTemperature() changes the controller waveform. */
    virtual bool supportsTemperatureCompensation() const { return false; }

    /** Last sticky error raised by a synchronous driver operation.
     *  Virtual so composite drivers (e.g. auto-detect wrappers) can report
     *  the state of the delegated child driver. */
    virtual DriverOperationError lastOperationError() const {
        return _operationError;
    }

    /** Clear the sticky operation error before starting a new high-level action. */
    virtual void clearOperationError() {
        _operationError = DriverOperationError::None;
    }

    // Initialization

    /** Initialize the driver IC with the given bus
     *  @param bus  The bus to use for communication
     *  @return true on success
     */
    virtual bool init(IBus& bus) = 0;

    // Display control

    /** Set display rotation (0-3) */
    virtual void setRotation(uint8_t rotation) = 0;

    /** Get current rotation */
    virtual uint8_t rotation() const = 0;

    /** Invert the display colors */
    virtual void invertDisplay(bool invert) = 0;

    /** Turn display on */
    virtual void displayOn() = 0;

    /** Turn display off */
    virtual void displayOff() = 0;

    // Address window

    /** Set the pixel address window for subsequent writes
     *  @param xs  X start coordinate
     *  @param ys  Y start coordinate
     *  @param xe  X end coordinate
     *  @param ye  Y end coordinate
     */
    virtual void setAddrWindow(uint16_t xs, uint16_t ys,
                               uint16_t xe, uint16_t ye) = 0;

    // Pixel writing

    /** Write a single pixel color (must be within address window) */
    virtual void writePixel(uint16_t color) = 0;

    /** Write an array of pixels */
    virtual void writePixels(const uint16_t* data, size_t len) = 0;

    /** Fill with a single color */
    virtual void writeFill(uint16_t color, size_t len) = 0;

    /** Asynchronous pixel submission remains driver-owned so controller
     *  address-window and framing rules cannot be bypassed by the facade. */
    virtual bool supportsAsyncPixelTransfer() const {
        return _bus && _bus->capabilities().asyncPixelTransfer;
    }
    virtual bool enableDMA(bool enable = true) {
        return _bus && _bus->enableDMA(enable);
    }
    virtual bool writePixelsDMA(const uint16_t* data, size_t len) {
        return _bus && _bus->writePixelsDMA(data, len);
    }
    virtual bool dmaBusy() { return _bus && _bus->dmaBusy(); }

    // Pixel reading

    /** Read a pixel at the given coordinates
     *  @return 16-bit color value
     */
    virtual uint16_t readPixel(uint16_t x, uint16_t y) { (void)x; (void)y; return 0; }

    // Power management

    /** Put the display into sleep mode */
    virtual void sleep() = 0;

    /** Wake the display from sleep mode */
    virtual void wake() = 0;

    // ePaper-specific methods (default no-op for TFT/OLED)

    /** Trigger a full display refresh (ePaper only) */
    virtual void update() {}

    /** Trigger a controller-specific fast refresh (ePaper only). */
    virtual void updateFast() { update(); }

    /** Trigger a partial display refresh (ePaper only) */
    virtual void updatePartial() {}

    /** Prepare the controller for a partial refresh. */
    virtual void wakePartial() { wake(); }

    /** Prepare the controller for a fast refresh. */
    virtual void wakeFast() { wake(); }

    /** Prepare the controller for a grayscale refresh. */
    virtual void wakeGray() { wake(); }

    /** Trigger a grayscale display refresh. */
    virtual void updateGray() { update(); }

    /** Push new frame data to the display (ePaper only).
     *  The base class deliberately performs no controller-specific I/O. */
    virtual void pushNewColors(const uint8_t* data, size_t len) {
        (void)data; (void)len;
    }

    /** Push old frame data to the display (ePaper only).
     *  The base class deliberately performs no controller-specific I/O. */
    virtual void pushOldColors(const uint8_t* data, size_t len) {
        (void)data; (void)len;
    }

    /** Push packed grayscale frame data using the controller's gray planes. */
    virtual void pushGrayColors(const uint8_t* data, size_t len) {
        (void)data; (void)len;
    }

    /** Apply a controller-specific temperature compensation value. */
    virtual void setTemperature(int8_t temperatureC) { (void)temperatureC; }

    // ePaper waveform profile selection

    /** Number of panel waveform profiles offered by this driver. */
    virtual size_t waveformProfileCount() const {
        return ePaperWaveformProfileCount(name(), nativeWidth(), nativeHeight(),
                                          colorDepth());
    }

    /** Return a profile descriptor by index, or nullptr when unavailable. */
    virtual const EPaperWaveformProfile* waveformProfileAt(size_t index) const {
        return ePaperWaveformProfileAt(name(), index, nativeWidth(),
                                       nativeHeight(), colorDepth());
    }

    /** Select a profile by stable ID. Built-in profiles are already active;
     *  command-sequence profiles are consumed by supporting driver init paths. */
    virtual bool selectWaveformProfile(const char* id) {
        const EPaperWaveformProfile* candidate = findEPaperWaveformProfile(
            name(), id, nativeWidth(), nativeHeight(), colorDepth());
        if (!candidate) {
            _waveformProfileResult = EPaperWaveformResult::UnknownProfile;
            return false;
        }
        if (!ePaperWaveformProfileMatches(*candidate, name(), nativeWidth(),
                                          nativeHeight(), colorDepth())) {
            _waveformProfileResult = EPaperWaveformResult::IncompatibleProfile;
            return false;
        }
        _selectedWaveformProfile = candidate;
        _waveformProfileResult = EPaperWaveformResult::Ok;
        return true;
    }

    /** Current profile descriptor, or nullptr for non-ePaper drivers. */
    virtual const EPaperWaveformProfile* waveformProfile() const {
        return _selectedWaveformProfile
            ? _selectedWaveformProfile
            : findEPaperWaveformProfile(name(), "default", nativeWidth(),
                                        nativeHeight(), colorDepth());
    }

    /** Result of the most recent waveform profile selection or sequence apply. */
    virtual EPaperWaveformResult waveformProfileResult() const {
        return _waveformProfileResult;
    }

    // Accessors

    /** Get the bus this driver is using */
    virtual IBus& bus() = 0;

    // Pin configuration (for ePaper drivers)

    /** Set the busy pin for ePaper busy-wait polling */
    virtual void setBusyPin(int pin) { (void)pin; }

    /** Set the reset pin for ePaper hardware reset */
    virtual void setResetPin(int pin) {
        _hardwareResetPin = static_cast<int8_t>(pin);
    }

    /** Raw chip-ID byte read back by an auto-detect probe, or -1 when the
     *  driver does not probe. Lets sketches read a composite driver's probe
     *  result through the plain IDriver interface (Arduino targets build
     *  with -fno-rtti, so dynamic_cast-based downcasts are unavailable). */
    virtual int probedChipId() const { return -1; }

protected:
    /** Record the first failure in a high-level operation. */
    void setOperationError(DriverOperationError error) {
        if (_operationError == DriverOperationError::None) _operationError = error;
    }

    /** Wait for BUSY and preserve timeout information for the Panel layer. */
    bool waitForReadyPin(int pin, bool readyHigh,
                         uint32_t timeoutMs = 30000,
                         uint16_t pollIntervalMs = 1) {
        if (gfxWaitForPin(pin, readyHigh, timeoutMs, pollIntervalMs)) return true;
        setOperationError(DriverOperationError::BusyTimeout);
        return false;
    }

    /** Pulse the panel reset line. Some Spectra panels require two pulses. */
    void hardwareReset(uint16_t lowMs = 10, uint16_t settleMs = 10,
                       uint8_t pulses = 1) {
        if (_hardwareResetPin < 0) return;
        gfxPinModeOutput(_hardwareResetPin);
        gfxDigitalWrite(_hardwareResetPin, true);
        delay(1);
        for (uint8_t pulse = 0; pulse < pulses; ++pulse) {
            gfxDigitalWrite(_hardwareResetPin, false);
            delay(lowMs);
            gfxDigitalWrite(_hardwareResetPin, true);
            delay(settleMs);
        }
    }

    /** Execute the current profile's mode sequence for an 8-bit EPD controller. */
    bool applyWaveformProfile(EPaperWaveformMode mode, int busyPin,
                              bool busyReadyHigh, uint32_t timeoutMs = 30000) {
        const EPaperWaveformProfile* profile = waveformProfile();
        if (!profile || profile->storage != EPaperWaveformStorage::CommandSequence) {
            return false;
        }
        const EPaperCommandSequence* sequence = ePaperWaveformSequence(*profile, mode);
        if (!sequence || !_bus) {
            _waveformProfileResult = EPaperWaveformResult::UnsupportedMode;
            return false;
        }
        _waveformProfileResult = applyEPaperCommandSequence(
            *_bus, *sequence, nativeWidth(), nativeHeight(), busyPin,
            busyReadyHigh, timeoutMs);
        if (_waveformProfileResult == EPaperWaveformResult::Ok) return true;
        if (_waveformProfileResult == EPaperWaveformResult::BusyTimeout) {
            setOperationError(DriverOperationError::BusyTimeout);
        }
        if (_waveformProfileResult == EPaperWaveformResult::InvalidSequence) {
            setOperationError(DriverOperationError::CommunicationFailed);
        }
        return false;
    }

    /** Common RGB RAM-read sequence used by color display controllers. */
    uint16_t readRgb565Pixel(uint16_t x, uint16_t y, uint8_t ramReadCommand,
                             bool st7735Packing = false) {
        if (!_bus || x >= width() || y >= height() ||
            !_bus->capabilities().readable) return 0;
        _bus->beginRead();
        setAddrWindow(x, y, x, y);
        _bus->writeCommand(ramReadCommand);
        (void)_bus->readData();
        uint8_t r = _bus->readData();
        uint8_t g = _bus->readData();
        uint8_t b = _bus->readData();
        _bus->endRead();
        if (st7735Packing) {
            r = static_cast<uint8_t>(r << 1);
            g = static_cast<uint8_t>(g << 1);
            b = static_cast<uint8_t>(b << 1);
        }
        return static_cast<uint16_t>(((r & 0xF8U) << 8) |
                                     ((g & 0xFCU) << 3) | (b >> 3));
    }

    uint16_t _width  = 240;
    uint16_t _height = 240;
    uint8_t  _rotation = 0;
    IBus*    _bus = nullptr;
    DriverOperationError _operationError = DriverOperationError::None;
    const EPaperWaveformProfile* _selectedWaveformProfile = nullptr;
    EPaperWaveformResult _waveformProfileResult = EPaperWaveformResult::UnknownProfile;
    int8_t _hardwareResetPin = -1;
};

#endif // NGFX_SEEED_SEEED_GFX_DRIVER_H

} }
