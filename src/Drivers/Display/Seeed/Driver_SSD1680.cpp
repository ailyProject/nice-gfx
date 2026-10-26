// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "Driver_SSD1680.h"
#include "SeeedHal.h"
#include <stdlib.h>
#include <string.h>
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_SSD1680.cpp
 * @brief  SSD1680 ePaper display driver implementation
 */




Driver_SSD1680::Driver_SSD1680(uint16_t w, uint16_t h)
    : _init_width(w), _init_height(h)
{
    _width  = w;
    _height = h;
}

Driver_SSD1680::~Driver_SSD1680() {
    protocolFree(_oldCache);
    _oldCache = nullptr;
}

// Private helpers

void Driver_SSD1680::checkBusy() {
    if (_busy_pin < 0) return;
    (void)waitForReadyPin(_busy_pin, false);
}

void Driver_SSD1680::reset() {
    if (_rst_pin >= 0) {
        digitalWrite(_rst_pin, NGFX_GXEPD2_LOW);
        delay(10);
        digitalWrite(_rst_pin, NGFX_GXEPD2_HIGH);
        delay(120);
    }

    checkBusy();

    _bus->writeCommand(NGFX_SEEED_SSD1680_SWRESET);  // 0x12
    checkBusy();
}

// Driver-managed partial old-plane cache ------------------------------------
// Controller-space 1bpp data; valid only after a confirmed monochrome
// full/fast refresh, invalidated by any ambiguous refresh failure.
bool Driver_SSD1680::ensureOldCache() {
    if (_oldCache) return true;
    const size_t fb = frameBytes();
    _oldCache = static_cast<uint8_t*>(protocolAllocate(fb));
    if (_oldCache) {
        memset(_oldCache, 0xFF, fb);
    }
    _oldCacheValid = false;
    return _oldCache != nullptr;
}

void Driver_SSD1680::invalidateOldCache() {
    protocolFree(_oldCache);
    _oldCache = nullptr;
    _oldCacheValid = false;
    _cacheUpdatePending = false;
    _partialActive = false;
}

bool Driver_SSD1680::windowGeometry(size_t* rowBytes, size_t* rows,
                                    size_t* total) const {
    // Accept only in-bounds windows that map exactly onto byte-aligned cache
    // rows (8px x-origin and width); anything else would access the cache OOB.
    if (_winX0 > _winX1 || _winY0 > _winY1) return false;
    if (_winX1 >= _init_width || _winY1 >= _init_height) return false;
    if (_winX0 & 7U) return false;
    const size_t w = (size_t)_winX1 - _winX0 + 1U;
    if (w & 7U) return false;
    const size_t rb = w >> 3;
    const size_t stride = cacheStride();
    if (rb == 0 || rb > stride) return false;
    if ((size_t)(_winX0 >> 3) + rb > stride) return false;
    *rowBytes = rb;
    *rows  = (size_t)_winY1 - _winY0 + 1U;
    *total = rb * (*rows);
    return true;
}

void Driver_SSD1680::pushOldWindow(size_t rowBytes, size_t rows) {
    const size_t stride = cacheStride();
    _bus->writeCommand(NGFX_SEEED_SSD1680_RAMRED);  // 0x26
    for (size_t r = 0; r < rows; ++r) {
        _bus->writeData(_oldCache + ((size_t)_winY0 + r) * stride
                                    + (size_t)(_winX0 >> 3),
                        rowBytes);
    }
}

void Driver_SSD1680::cacheWindow(const uint8_t* data, size_t rowBytes,
                                 size_t rows) {
    if (!data || !_oldCache) return;
    const size_t stride = cacheStride();
    for (size_t r = 0; r < rows; ++r) {
        memcpy(_oldCache + ((size_t)_winY0 + r) * stride
                           + (size_t)(_winX0 >> 3),
               data + r * rowBytes, rowBytes);
    }
    _oldCacheValid = false;
    _cacheUpdatePending = true;
}

void Driver_SSD1680::finishRefresh(bool success) {
    if (!success) {
        // BUSY timeout or bus failure leaves the final glass state unknown.
        invalidateOldCache();
        return;
    }
    if (_cacheUpdatePending) {
        _oldCacheValid = true;
        _cacheUpdatePending = false;
    }
}

// IDriver :: init

bool Driver_SSD1680::init(IBus& bus) {
    // Reset cache session state before the normal applyWaveformProfile()
    // early return. _oldCache is kept: bistable glass retains the image.
    if (_cacheUpdatePending) invalidateOldCache();
    _partialActive = false;
    _lastPartialTimedOut = false;
    _fastFallbackFull = false;

    _bus = &bus;
    if (_enable_pin >= 0) {
        pinMode(_enable_pin, NGFX_GXEPD2_OUTPUT);
        digitalWrite(_enable_pin, NGFX_GXEPD2_HIGH);
    }

    if (_busy_pin >= 0) {
        pinMode(_busy_pin, NGFX_GXEPD2_INPUT);
    }

    reset();
    if (lastOperationError() != DriverOperationError::None) return false;

    if (applyWaveformProfile(EPaperWaveformMode::Full, _busy_pin, false)) {
        setRotation(0);
        return lastOperationError() == DriverOperationError::None;
    }
    if (lastOperationError() != DriverOperationError::None) return false;

    // ---- Full init sequence ----

    // Border waveform
    _bus->writeCommand(NGFX_SEEED_SSD1680_PTLIN);  // 0x3C
    _bus->writeData(0x05);

    // Driver output control: gate lines = height
    _bus->writeCommand(NGFX_SEEED_SSD1680_DRVOUT);  // 0x01
    _bus->writeData((_init_height - 1) & 0xFF);       // low byte
    _bus->writeData(((_init_height - 1) >> 8) & 0xFF); // high byte
    _bus->writeData(0x00);

    // Data entry mode
    _bus->writeCommand(NGFX_SEEED_SSD1680_DTM);  // 0x11
    _bus->writeData(0x03);

    // RAM X address range: 0 to (width/8 - 1)
    _bus->writeCommand(NGFX_SEEED_SSD1680_SETX);  // 0x44
    _bus->writeData(0x00);
    _bus->writeData(((_init_width + 7U) / 8U) - 1U);

    // RAM Y address range: 0 to (height - 1)
    _bus->writeCommand(NGFX_SEEED_SSD1680_SETY);  // 0x45
    _bus->writeData(0x00);
    _bus->writeData(0x00);
    _bus->writeData((_init_height - 1) & 0xFF);
    _bus->writeData(((_init_height - 1) >> 8) & 0xFF);

    // Temperature sensor: internal
    _bus->writeCommand(NGFX_SEEED_SSD1680_TEMP);  // 0x18
    _bus->writeData(0x80);

    // RAM X counter start
    _bus->writeCommand(NGFX_SEEED_SSD1680_RAMXCNT);  // 0x4E
    _bus->writeData(0x00);

    // RAM Y counter start
    _bus->writeCommand(NGFX_SEEED_SSD1680_RAMYCNT);  // 0x4F
    _bus->writeData(0x00);
    _bus->writeData(0x00);

    checkBusy();

    // Apply default rotation
    setRotation(0);

    return lastOperationError() == DriverOperationError::None;
}

// IDriver :: setRotation

void Driver_SSD1680::setRotation(uint8_t m) {
    _rotation = m % 4;

    _bus->writeCommand(NGFX_SEEED_SSD1680_DTM);  // 0x11

    switch (_rotation) {
        case 0:  // Portrait, 0 degrees (canonical orientation)
            // X+ Y+ (0x03): matches the init profile and pushColors'
            // top-to-bottom row order; the panel layer owns rotation.
            _bus->writeData(0x03);
            _width  = _init_width;
            _height = _init_height;
            break;

        case 2:  // Inverted portrait, 180 degrees
            _bus->writeData(NGFX_SEEED_SSD1680_DTM_YINC_XDEC);  // 0x02
            _width  = _init_width;
            _height = _init_height;
            break;

        case 1:  // Landscape 90 degrees — swap handled by the panel layer
        case 3:  // Landscape 270 degrees
        default:
            _bus->writeData(0x03);  // X+ Y+ — same as rot 0
            _width  = _init_width;
            _height = _init_height;
            break;
    }
}

// IDriver :: display control

void Driver_SSD1680::invertDisplay(bool /*invert*/) {
    // ePaper panels do not support hardware color inversion.
}

void Driver_SSD1680::displayOn() {
    // ePaper retains the image; no explicit "display on" command.
}

void Driver_SSD1680::displayOff() {
    // ePaper retains the image; no explicit "display off" command.
}

// IDriver :: address window

void Driver_SSD1680::setAddrWindow(uint16_t xs, uint16_t ys,
                                    uint16_t xe, uint16_t ye) {
    if (lastOperationError() != DriverOperationError::None || !_bus) return;
    // Cache window; re-issued before each RAM plane (pure register writes).
    _winX0 = xs; _winY0 = ys; _winX1 = xe; _winY1 = ye;

    // RAM X start/end (in bytes, not pixels)
    _bus->writeCommand(NGFX_SEEED_SSD1680_SETX);  // 0x44
    _bus->writeData(xs >> 3);
    _bus->writeData(xe >> 3);

    // RAM Y start/end
    _bus->writeCommand(NGFX_SEEED_SSD1680_SETY);  // 0x45
    _bus->writeData(ys & 0xFF);
    _bus->writeData((ys >> 8) & 0xFF);
    _bus->writeData(ye & 0xFF);
    _bus->writeData((ye >> 8) & 0xFF);

    // RAM X counter
    _bus->writeCommand(NGFX_SEEED_SSD1680_RAMXCNT);  // 0x4E
    _bus->writeData(xs >> 3);

    // RAM Y counter
    _bus->writeCommand(NGFX_SEEED_SSD1680_RAMYCNT);  // 0x4F
    _bus->writeData(ys & 0xFF);
    _bus->writeData((ys >> 8) & 0xFF);
}

// IDriver :: pixel writing (handled by Panel layer for ePaper)

void Driver_SSD1680::writePixel(uint16_t /*color*/) {
    // ePaper writes are done via pushNewColors / pushOldColors.
}

void Driver_SSD1680::writePixels(const uint16_t* /*data*/, size_t /*len*/) {
    // ePaper writes are done via pushNewColors / pushOldColors.
}

void Driver_SSD1680::writeFill(uint16_t /*color*/, size_t /*len*/) {
    // ePaper writes are done via pushNewColors / pushOldColors.
}

// IDriver :: power management

void Driver_SSD1680::sleep() {
    if (_cacheUpdatePending) invalidateOldCache();
    _partialActive = false;
    _fastFallbackFull = false;
    if (_lastPartialTimedOut) {
        // A partial refresh may still be driving after a BUSY timeout:
        // skip deep sleep once (cost: standby power until next operation).
        _lastPartialTimedOut = false;
    } else {
        _bus->writeCommand(NGFX_SEEED_SSD1680_SLPIN);  // 0x10
        _bus->writeData(0x01);
    }
    delay(100);
}

void Driver_SSD1680::wake() {
    // Re-run the full init sequence; reset cache session state first (the
    // applyWaveformProfile() early return below is the normal path).
    if (_cacheUpdatePending) invalidateOldCache();
    _partialActive = false;
    _lastPartialTimedOut = false;
    _fastFallbackFull = false;

    reset();
    if (lastOperationError() != DriverOperationError::None) return;
    if (applyWaveformProfile(EPaperWaveformMode::Full, _busy_pin, false)) {
        setRotation(_rotation);
        return;
    }
    if (lastOperationError() != DriverOperationError::None) return;

    _bus->writeCommand(NGFX_SEEED_SSD1680_PTLIN);  // 0x3C
    _bus->writeData(0x05);

    _bus->writeCommand(NGFX_SEEED_SSD1680_DRVOUT);  // 0x01
    _bus->writeData((_init_height - 1) & 0xFF);
    _bus->writeData(((_init_height - 1) >> 8) & 0xFF);
    _bus->writeData(0x00);

    _bus->writeCommand(NGFX_SEEED_SSD1680_DTM);  // 0x11
    _bus->writeData(0x03);

    _bus->writeCommand(NGFX_SEEED_SSD1680_SETX);  // 0x44
    _bus->writeData(0x00);
    _bus->writeData(((_init_width + 7U) / 8U) - 1U);

    _bus->writeCommand(NGFX_SEEED_SSD1680_SETY);  // 0x45
    _bus->writeData(0x00);
    _bus->writeData(0x00);
    _bus->writeData((_init_height - 1) & 0xFF);
    _bus->writeData(((_init_height - 1) >> 8) & 0xFF);

    _bus->writeCommand(NGFX_SEEED_SSD1680_TEMP);  // 0x18
    _bus->writeData(0x80);

    _bus->writeCommand(NGFX_SEEED_SSD1680_RAMXCNT);  // 0x4E
    _bus->writeData(0x00);

    _bus->writeCommand(NGFX_SEEED_SSD1680_RAMYCNT);  // 0x4F
    _bus->writeData(0x00);
    _bus->writeData(0x00);

    checkBusy();

    setRotation(_rotation);
}

void Driver_SSD1680::wakePartial() {
    wake();
    if (lastOperationError() != DriverOperationError::None) return;
    initPartial();
    if (lastOperationError() != DriverOperationError::None) return;
    // Partial is safe only after a confirmed monochrome full/fast refresh.
    if (!ensureOldCache() || !_oldCacheValid) {
        setOperationError(DriverOperationError::CommunicationFailed);
        return;
    }
    _lastPartialTimedOut = false;
    _partialActive = true;
}

void Driver_SSD1680::wakeFast() {
    wake();
    if (lastOperationError() != DriverOperationError::None) return;
    _fastFallbackFull = !_oldCacheValid;
}

void Driver_SSD1680::wakeGray() {
    wake();
}

// ePaper-specific :: update methods

void Driver_SSD1680::update() {
    if (lastOperationError() != DriverOperationError::None || !_bus ||
        _bus->lastError() != 0) {
        finishRefresh(false);
        return;
    }
    _bus->writeCommand(NGFX_SEEED_SSD1680_DISPCTRL);  // 0x22
    _bus->writeData(0xF7);
    _bus->writeCommand(NGFX_SEEED_SSD1680_MASTER);    // 0x20
    checkBusy();
    const bool ok = (lastOperationError() == DriverOperationError::None) &&
                    (!_bus || _bus->lastError() == 0);
    finishRefresh(ok);
}

void Driver_SSD1680::updateFast() {
    if (lastOperationError() != DriverOperationError::None || !_bus ||
        _bus->lastError() != 0) {
        finishRefresh(false);
        _fastFallbackFull = false;
        return;
    }
    // Fast full refresh: 0x1A=0x64 + 0xD7. The first request uses 0xF7 so an
    // unknown power-on frame becomes a trustworthy partial-refresh baseline.
    _bus->writeCommand(0x21);             // Display update control 1
    _bus->writeData(0x00);                // B/W RAM normal
    _bus->writeData(0x80);                // previous-plane option
    if (!_fastFallbackFull) {
        _bus->writeCommand(NGFX_SEEED_SSD1680_LUTOPT);   // 0x1A
        _bus->writeData(0x64);
    }
    _bus->writeCommand(NGFX_SEEED_SSD1680_DISPCTRL);  // 0x22
    _bus->writeData(_fastFallbackFull ? 0xF7 : 0xD7);
    _bus->writeCommand(NGFX_SEEED_SSD1680_MASTER);    // 0x20
    checkBusy();
    const bool ok = (lastOperationError() == DriverOperationError::None) &&
                    (!_bus || _bus->lastError() == 0);
    finishRefresh(ok);
    _fastFallbackFull = false;
}

void Driver_SSD1680::updateGray() {
    if (lastOperationError() != DriverOperationError::None || !_bus ||
        _bus->lastError() != 0) return;
    // Gray trigger (0xC7) differs from the monochrome fast-full waveform.
    _bus->writeCommand(NGFX_SEEED_SSD1680_DISPCTRL);
    _bus->writeData(0xC7);
    _bus->writeCommand(NGFX_SEEED_SSD1680_MASTER);
    checkBusy();
    if (lastOperationError() != DriverOperationError::None ||
        _bus->lastError() != 0) invalidateOldCache();
}

void Driver_SSD1680::updatePartial() {
    if (lastOperationError() != DriverOperationError::None || !_bus ||
        _bus->lastError() != 0 || !_partialActive ||
        !_cacheUpdatePending) {
        if (_cacheUpdatePending) invalidateOldCache();
        _partialActive = false;
        return;
    }
    _bus->writeCommand(NGFX_SEEED_SSD1680_DISPCTRL);  // 0x22
    _bus->writeData(0xFC);
    _bus->writeCommand(NGFX_SEEED_SSD1680_MASTER);    // 0x20
    // Synchronous BUSY wait keeps timeout reporting consistent.
    checkBusy();
    // Commit the staged cache update only after this wait succeeds.
    const bool ok = (lastOperationError() == DriverOperationError::None) &&
                    (!_bus || _bus->lastError() == 0);
    if (!ok && lastOperationError() == DriverOperationError::BusyTimeout) {
        // Glass state unknown; sleep() skips deep sleep once.
        _lastPartialTimedOut = true;
    }
    finishRefresh(ok);
    _partialActive = false;
}

// ePaper-specific :: init sub-modes

void Driver_SSD1680::initPartial() {
    // RAM-plane selection for differential updates; not in the shared profile.
    _bus->writeCommand(0x21);
    _bus->writeData(0x00);
    _bus->writeData(0x80);
    if (applyWaveformProfile(EPaperWaveformMode::Partial, _busy_pin, false)) return;
    if (lastOperationError() != DriverOperationError::None) return;
    _bus->writeCommand(NGFX_SEEED_SSD1680_TEMP);   // 0x18
    _bus->writeData(0x80);
    _bus->writeCommand(NGFX_SEEED_SSD1680_PTLIN);  // 0x3C
    _bus->writeData(0x80);
}

void Driver_SSD1680::initGray() {
    if (applyWaveformProfile(EPaperWaveformMode::Gray, _busy_pin, false)) return;
    if (lastOperationError() != DriverOperationError::None) return;
    _bus->writeCommand(NGFX_SEEED_SSD1680_TEMP);       // 0x18
    _bus->writeData(0x80);

    _bus->writeCommand(NGFX_SEEED_SSD1680_DISPCTRL);   // 0x22
    _bus->writeData(0xB1);
    _bus->writeCommand(NGFX_SEEED_SSD1680_MASTER);     // 0x20
    checkBusy();

    _bus->writeCommand(NGFX_SEEED_SSD1680_LUTOPT);     // 0x1A
    _bus->writeData(0x5A);
    _bus->writeData(0x00);

    _bus->writeCommand(NGFX_SEEED_SSD1680_DISPCTRL);   // 0x22
    _bus->writeData(0x91);
    _bus->writeCommand(NGFX_SEEED_SSD1680_MASTER);     // 0x20
    checkBusy();
}

// ePaper-specific :: push color data

void Driver_SSD1680::pushNewColors(const uint8_t* data, size_t len) {
    if (!_bus || !data ||
        lastOperationError() != DriverOperationError::None) return;

    size_t rowBytes = 0, rows = 0, total = 0;
    if (_partialActive) {
        if (!_oldCache || !_oldCacheValid ||
            !windowGeometry(&rowBytes, &rows, &total) || len != total) {
            setOperationError(DriverOperationError::CommunicationFailed);
            return;
        }
        // Differential waveform needs old (0x26) vs new (0x24) RAM: write the
        // cached old window first, re-issuing the window before each plane.
        setAddrWindow(_winX0, _winY0, _winX1, _winY1);
        pushOldWindow(rowBytes, rows);              // 0x26, strictly first
        setAddrWindow(_winX0, _winY0, _winX1, _winY1);
        _bus->writeCommand(NGFX_SEEED_SSD1680_RAMBW);          // 0x24
        _bus->writeData(data, len);
        // Cache updated in place (no second allocation); stays invalid until
        // BUSY confirms the refresh.
        cacheWindow(data, rowBytes, rows);
        return;
    }

    if (len == frameBytes()) {
        // The cache is required only for later differential partial updates.
        // A cache-allocation failure must not prevent this full-frame transfer
        // from reaching the panel; without a cache, later partial updates stay
        // rejected until another full/fast refresh can establish a baseline.
        _bus->writeCommand(NGFX_SEEED_SSD1680_RAMBW);  // 0x24
        _bus->writeData(data, len);
        _winX0 = 0; _winY0 = 0;
        _winX1 = _init_width - 1; _winY1 = _init_height - 1;
        if (ensureOldCache()) {
            cacheWindow(data, cacheStride(), _init_height);
        }
        return;
    }

    // Non-monochrome compatibility path; never use it as a 1bpp baseline.
    _bus->writeCommand(NGFX_SEEED_SSD1680_RAMBW);
    _bus->writeData(data, len);
}

void Driver_SSD1680::pushGrayColors(const uint8_t* data, size_t len) {
    (void)len;
    // Gray repaints both planes as 2-bit data the 1bpp cache cannot hold:
    // invalidate here (not in wakeGray(), which may never reach a push).
    invalidateOldCache();
    pushNewGrayColors(_init_width, _init_height, data);
}

void Driver_SSD1680::pushNewColors(uint16_t w, uint16_t h,
                                    const uint8_t* colors) {
    _bus->writeCommand(NGFX_SEEED_SSD1680_RAMBW);  // 0x24
    size_t n = (size_t)w * h / 8;
    _bus->writeData(colors, n);
}

void Driver_SSD1680::pushOldColors(uint16_t w, uint16_t h,
                                    const uint8_t* colors) {
    _bus->writeCommand(NGFX_SEEED_SSD1680_RAMRED);  // 0x26
    size_t n = (size_t)w * h / 8;
    _bus->writeData(colors, n);
}

void Driver_SSD1680::pushNewColorsFlip(uint16_t w, uint16_t h,
                                        const uint8_t* colors) {
    // Per row: reverse byte order and flip bits within each byte, for
    // panels whose column wiring is reversed.

    _bus->writeCommand(NGFX_SEEED_SSD1680_RAMBW);  // 0x24

    uint16_t bytes_per_row = w / 8;

    for (uint16_t row = 0; row < h; row++) {
        uint16_t start = row * bytes_per_row;
        for (uint16_t col = 0; col < bytes_per_row; col++) {
            uint8_t b = colors[start + (bytes_per_row - 1 - col)];

            b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
            b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
            b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);

            _bus->writeData(b);
        }
    }
}

void Driver_SSD1680::pushOldColorsFlip(uint16_t w, uint16_t h,
                                        const uint8_t* colors) {
    // Same flip logic as pushNewColorsFlip, writing to Red RAM (0x26).

    _bus->writeCommand(NGFX_SEEED_SSD1680_RAMRED);  // 0x26

    uint16_t bytes_per_row = w / 8;

    for (uint16_t row = 0; row < h; row++) {
        uint16_t start = row * bytes_per_row;
        for (uint16_t col = 0; col < bytes_per_row; col++) {
            uint8_t b = colors[start + (bytes_per_row - 1 - col)];

            b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
            b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
            b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);

            _bus->writeData(b);
        }
    }
}

// ePaper-specific :: 4-level grayscale push

void Driver_SSD1680::pushNewGrayColors(uint16_t w, uint16_t h,
                                        const uint8_t* colors) {
    // 4-level gray via both planes; each pixel is 2 bits (00=white, 01=gray1,
    // 10=gray2, 11=black), packed 2 pixels/byte; total = w*h/2 bytes.
    initGray();

    uint32_t total_pixels = (uint32_t)w * h;
    uint32_t total_pixels_div8 = total_pixels / 8;

    uint8_t temp1, temp2, temp3;

    // ---- Plane 1: B/W RAM (0x24) ----
    _bus->writeCommand(NGFX_SEEED_SSD1680_RAMBW);  // 0x24

    for (uint32_t i = 0; i < total_pixels_div8; i++) {
        uint8_t c0 = colors[i * 4 + 0];
        uint8_t c1 = colors[i * 4 + 1];
        uint8_t c2 = colors[i * 4 + 2];
        uint8_t c3 = colors[i * 4 + 3];

        uint8_t p0 = (c0 >> 4) & 0x03;
        uint8_t p1 = c0 & 0x03;
        uint8_t p2 = (c1 >> 4) & 0x03;
        uint8_t p3 = c1 & 0x03;
        uint8_t p4 = (c2 >> 4) & 0x03;
        uint8_t p5 = c2 & 0x03;
        uint8_t p6 = (c3 >> 4) & 0x03;
        uint8_t p7 = c3 & 0x03;

        uint8_t packed0 = (p0 << 6) | (p1 << 4) | (p2 << 2) | p3;
        uint8_t packed1 = (p4 << 6) | (p5 << 4) | (p6 << 2) | p7;

        // Convert to B/W plane: bit 0 of each 2-bit value -> output bit
        //   11 (0xC0) -> 1,  10 (0x80) -> 0,  01 (0x40) -> 1,  00 -> 0
        temp3 = 0;
        for (uint8_t j = 0; j < 2; j++) {
            temp1 = (j == 0) ? packed0 : packed1;
            for (uint8_t k = 0; k < 4; k++) {
                temp2 = temp1 & 0xC0;
                if (temp2 == 0xC0)
                    temp3 |= 0x01;
                else if (temp2 == 0x00)
                    temp3 |= 0x00;
                else if (temp2 == 0x40)
                    temp3 |= 0x01;
                else  // 0x80
                    temp3 |= 0x00;

                if ((j == 0 && k <= 3) || (j == 1 && k <= 2)) {
                    temp3 <<= 1;
                    temp1 <<= 2;
                }
            }
        }
        _bus->writeData(~temp3);
    }

    // ---- Plane 2: Red RAM (0x26) ----
    _bus->writeCommand(NGFX_SEEED_SSD1680_RAMRED);  // 0x26

    for (uint32_t i = 0; i < total_pixels_div8; i++) {
        uint8_t c0 = colors[i * 4 + 0];
        uint8_t c1 = colors[i * 4 + 1];
        uint8_t c2 = colors[i * 4 + 2];
        uint8_t c3 = colors[i * 4 + 3];

        uint8_t p0 = (c0 >> 4) & 0x03;
        uint8_t p1 = c0 & 0x03;
        uint8_t p2 = (c1 >> 4) & 0x03;
        uint8_t p3 = c1 & 0x03;
        uint8_t p4 = (c2 >> 4) & 0x03;
        uint8_t p5 = c2 & 0x03;
        uint8_t p6 = (c3 >> 4) & 0x03;
        uint8_t p7 = c3 & 0x03;

        uint8_t packed0 = (p0 << 6) | (p1 << 4) | (p2 << 2) | p3;
        uint8_t packed1 = (p4 << 6) | (p5 << 4) | (p6 << 2) | p7;

        // Convert to Red plane: bit 1 of each 2-bit value -> output bit
        //   11 (0xC0) -> 1,  10 (0x80) -> 1,  01 (0x40) -> 0,  00 -> 0
        temp3 = 0;
        for (uint8_t j = 0; j < 2; j++) {
            temp1 = (j == 0) ? packed0 : packed1;
            for (uint8_t k = 0; k < 4; k++) {
                temp2 = temp1 & 0xC0;
                if (temp2 == 0xC0)
                    temp3 |= 0x01;
                else if (temp2 == 0x00)
                    temp3 |= 0x00;
                else if (temp2 == 0x40)
                    temp3 |= 0x00;
                else  // 0x80
                    temp3 |= 0x01;

                if ((j == 0 && k <= 3) || (j == 1 && k <= 2)) {
                    temp3 <<= 1;
                    temp1 <<= 2;
                }
            }
        }
        _bus->writeData(~temp3);
    }
}

void Driver_SSD1680::pushNewGrayColorsFlip(uint16_t w, uint16_t h,
                                            const uint8_t* colors) {
    // Delegates to the non-flip version.
    pushNewGrayColors(w, h, colors);
}

} }
