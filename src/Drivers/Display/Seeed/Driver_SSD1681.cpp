// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "Driver_SSD1681.h"
#include "SeeedHal.h"
#include <stdlib.h>
#include <string.h>
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_SSD1681.cpp
 * @brief  SSD1681 ePaper display driver implementation
 */




Driver_SSD1681::Driver_SSD1681(uint16_t w, uint16_t h, int8_t busyPin)
    : _init_width(w), _init_height(h), _busyPin(busyPin) {
    _width = w; _height = h;
}

Driver_SSD1681::~Driver_SSD1681() {
    protocolFree(_oldCache);
    _oldCache = nullptr;
}

void Driver_SSD1681::busyWait() {
    if (_busyPin < 0) return;
    (void)waitForReadyPin(_busyPin, false);
}

bool Driver_SSD1681::init(IBus& bus) {
    _bus = &bus;
    // Partial-session reset point (all wake paths call init()); runs before the
    // waveform-profile early return. _oldCache is kept: truer than a white baseline.
    if (_cacheUpdatePending) invalidateOldCache();
    _partialActive = false;
    _lastPartialTimedOut = false;
    _fastFallbackFull = false;
    if (_busyPin >= 0) pinMode(_busyPin, NGFX_GXEPD2_INPUT);
    hardwareReset(10, 10);
    // EPD_INIT sequence
    _bus->writeCommand(0x12); // Software reset
    busyWait();
    if (lastOperationError() != DriverOperationError::None) return false;
    if (applyWaveformProfile(EPaperWaveformMode::Full, _busyPin, false)) {
        setRotation(0);
        return lastOperationError() == DriverOperationError::None;
    }
    if (lastOperationError() != DriverOperationError::None) return false;
    _bus->writeCommand(0x01); // Driver output control
    _bus->writeData((_init_height - 1) % 256);
    _bus->writeData((_init_height - 1) / 256);
    _bus->writeData(0x00);
    _bus->writeCommand(0x11); // Data entry mode
    _bus->writeData(0x03);
    _bus->writeCommand(0x44); // RAM X range
    _bus->writeData(0x00);
    _bus->writeData(((_init_width + 7U) / 8U) - 1U);
    _bus->writeCommand(0x45); // RAM Y range
    _bus->writeData(0x00);
    _bus->writeData(0x00);
    _bus->writeData((_init_height - 1) % 256);
    _bus->writeData((_init_height - 1) / 256);
    _bus->writeCommand(0x3C); // Border waveform
    _bus->writeData(0x05);
    _bus->writeCommand(0x18); // Temperature sensor
    _bus->writeData(0x80);
    _bus->writeCommand(0x4E); // RAM X counter
    _bus->writeData(0x00);
    _bus->writeCommand(0x4F); // RAM Y counter
    _bus->writeData(0x00);
    _bus->writeData(0x00);
    busyWait();
    setRotation(0);
    return lastOperationError() == DriverOperationError::None;
}

void Driver_SSD1681::setRotation(uint8_t m) {
    _rotation = m % 4;
    _bus->writeCommand(0x11);
    switch (_rotation) {
        case 0:
            // X+ Y+ — matches the init profile and pushColors' top-to-bottom
            // order (row0=Y0).
            _bus->writeData(0x03);
            _width = _init_width; _height = _init_height;
            break;
        case 2:
            _bus->writeData(0x02);
            _width = _init_width; _height = _init_height;
            break;
        case 1:
        case 3:
            _bus->writeData(0x01); // Default, no 90/270 support
            _width = _init_width; _height = _init_height;
            break;
    }
}

void Driver_SSD1681::invertDisplay(bool) {}

void Driver_SSD1681::displayOn()  { update(); }
void Driver_SSD1681::displayOff() {}

void Driver_SSD1681::setAddrWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    if (lastOperationError() != DriverOperationError::None || !_bus) return;
    // Remember the window for the partial-refresh old-plane bookkeeping.
    _winX0 = x1; _winY0 = y1; _winX1 = x2; _winY1 = y2;
    _bus->writeCommand(0x44);
    _bus->writeData(x1 >> 3);
    _bus->writeData(x2 >> 3);
    _bus->writeCommand(0x45);
    _bus->writeData(y1 & 0xFF);
    _bus->writeData(y1 >> 8);
    _bus->writeData(y2 & 0xFF);
    _bus->writeData(y2 >> 8);
    _bus->writeCommand(0x4E);
    _bus->writeData(x1 >> 3);
    _bus->writeCommand(0x4F);
    _bus->writeData(y1 & 0xFF);
    _bus->writeData(y1 >> 8);
}

void Driver_SSD1681::writePixel(uint16_t) {}
void Driver_SSD1681::writePixels(const uint16_t*, size_t) {}
void Driver_SSD1681::writeFill(uint16_t, size_t) {}

void Driver_SSD1681::sleep() {
    if (_cacheUpdatePending) invalidateOldCache();
    _partialActive = false;
    _fastFallbackFull = false;
    // One-shot: after a partial-refresh BUSY timeout, skip DSLP once (the
    // controller may still be driving). Cost: standby power until next op.
    if (_lastPartialTimedOut) {
        _lastPartialTimedOut = false;
    } else {
        _bus->writeCommand(0x10);
        _bus->writeData(0x01);
    }
    delay(100);
}

void Driver_SSD1681::wake() {
    init(*_bus);
}

void Driver_SSD1681::update() {
    if (lastOperationError() != DriverOperationError::None || !_bus ||
        _bus->lastError() != 0) {
        finishRefresh(false);
        return;
    }
    _bus->writeCommand(0x22);
    _bus->writeData(0xF7);
    _bus->writeCommand(0x20);
    busyWait();
    const bool ok = lastOperationError() == DriverOperationError::None &&
                    (!_bus || _bus->lastError() == 0);
    finishRefresh(ok);
}

void Driver_SSD1681::updateFast() {
    if (lastOperationError() != DriverOperationError::None || !_bus ||
        _bus->lastError() != 0) {
        finishRefresh(false);
        _fastFallbackFull = false;
        return;
    }
    // The OTP fast waveform needs a fixed temperature index; without it the
    // panel can report completion without changing the glass.
    if (!_fastFallbackFull) {
        _bus->writeCommand(0x1A);
        _bus->writeData(0x64);
    }
    _bus->writeCommand(0x22);
    _bus->writeData(_fastFallbackFull ? 0xF7 : 0xD7);
    _bus->writeCommand(0x20);
    busyWait();
    const bool ok = lastOperationError() == DriverOperationError::None &&
                    (!_bus || _bus->lastError() == 0);
    finishRefresh(ok);
    _fastFallbackFull = false;
}

void Driver_SSD1681::updateGray() {
    if (lastOperationError() != DriverOperationError::None || !_bus ||
        _bus->lastError() != 0) return;
    _bus->writeCommand(0x22);
    _bus->writeData(0xC7);
    _bus->writeCommand(0x20);
    busyWait();
    if (lastOperationError() != DriverOperationError::None ||
        _bus->lastError() != 0) invalidateOldCache();
}

void Driver_SSD1681::updatePartial() {
    if (lastOperationError() != DriverOperationError::None || !_bus ||
        _bus->lastError() != 0 || !_partialActive ||
        !_cacheUpdatePending) {
        if (_cacheUpdatePending) invalidateOldCache();
        _partialActive = false;
        return;
    }
    _bus->writeCommand(0x22);
    _bus->writeData(0xFC);
    _bus->writeCommand(0x20);
    busyWait();
    // Sticky errors are cleared at each operation start, so any error seen
    // here belongs to the current partial session.
    const bool ok = lastOperationError() == DriverOperationError::None &&
                    (!_bus || _bus->lastError() == 0);
    if (!ok && lastOperationError() == DriverOperationError::BusyTimeout)
        _lastPartialTimedOut = true;  // sleep() will skip DSLP once
    finishRefresh(ok);
    _partialActive = false;
}

void Driver_SSD1681::initGray() {
    if (applyWaveformProfile(EPaperWaveformMode::Gray, _busyPin, false)) return;
    if (lastOperationError() != DriverOperationError::None) return;
    _bus->writeCommand(0x18);
    _bus->writeData(0x80);
    _bus->writeCommand(0x22);
    _bus->writeData(0xB1);
    _bus->writeCommand(0x20);
    busyWait();
    _bus->writeCommand(0x1A);
    _bus->writeData(0x5A);
    _bus->writeData(0x00);
    _bus->writeCommand(0x22);
    _bus->writeData(0x91);
    _bus->writeCommand(0x20);
    busyWait();
}

void Driver_SSD1681::initPartial() {
    if (applyWaveformProfile(EPaperWaveformMode::Partial, _busyPin, false)) return;
    if (lastOperationError() != DriverOperationError::None) return;
    _bus->writeCommand(0x18);
    _bus->writeData(0x80);
    _bus->writeCommand(0x3C);
    _bus->writeData(0x80);
}

void Driver_SSD1681::wakeGray() { wake(); }

void Driver_SSD1681::wakePartial() {
    init(*_bus);
    if (lastOperationError() != DriverOperationError::None) return;
    initPartial();
    if (lastOperationError() != DriverOperationError::None) return;
    if (!ensureOldCache() || !_oldCacheValid) {
        setOperationError(DriverOperationError::CommunicationFailed);
        return;
    }
    _lastPartialTimedOut = false;
    _partialActive = true;
}

void Driver_SSD1681::wakeFast() {
    wake();
    if (lastOperationError() != DriverOperationError::None) return;
    _fastFallbackFull = !_oldCacheValid;
}

void Driver_SSD1681::pushNewColors(const uint8_t* data, size_t len) {
    if (!_bus || !data ||
        lastOperationError() != DriverOperationError::None) return;

    // Partial branch: push the cached old plane (0x26) before the new plane
    // (0x24), else the differential waveform ghosts against a stale reference.
    size_t rowBytes = 0, rows = 0, total = 0;
    if (_partialActive) {
        if (!_oldCache || !_oldCacheValid ||
            !windowGeometry(&rowBytes, &rows, &total) || len != total) {
            setOperationError(DriverOperationError::CommunicationFailed);
            return;
        }
        // Re-issue the window before each plane (pure register writes); never
        // assume the address counter rewinds across planes.
        setAddrWindow(_winX0, _winY0, _winX1, _winY1);
        pushOldWindow(rowBytes, rows);   // 0x26, strictly before the new plane
        setAddrWindow(_winX0, _winY0, _winX1, _winY1);
        _bus->writeCommand(0x24);
        _bus->writeData(data, len);
        cacheWindow(data, rowBytes, rows);
        return;
    }

    // Only exact full-frame 1bpp data updates the cache; other sizes (e.g.
    // 4bpp/colorful) go straight to 0x24 so they can't poison it.
    if (len == frameBytes()) {
        // Cache allocation is best-effort: the full refresh remains usable
        // under memory pressure, while partial refresh stays unavailable until
        // a later full/fast refresh successfully establishes its baseline.
        _bus->writeCommand(0x24);
        _bus->writeData(data, len);
        _winX0 = 0; _winY0 = 0;
        _winX1 = _init_width - 1; _winY1 = _init_height - 1;
        if (ensureOldCache()) {
            cacheWindow(data, cacheStride(), _init_height);
        }
        return;
    }

    _bus->writeCommand(0x24);
    _bus->writeData(data, len);
}

size_t Driver_SSD1681::cacheStride() const {
    return (static_cast<size_t>(_init_width) + 7U) / 8U;
}

size_t Driver_SSD1681::frameBytes() const {
    return cacheStride() * _init_height;
}

bool Driver_SSD1681::ensureOldCache() {
    if (_oldCache) return true;
    _oldCache = static_cast<uint8_t*>(protocolAllocate(frameBytes()));
    if (_oldCache) memset(_oldCache, 0xFF, frameBytes());
    _oldCacheValid = false;
    return _oldCache != nullptr;
}

void Driver_SSD1681::invalidateOldCache() {
    protocolFree(_oldCache);
    _oldCache = nullptr;
    _oldCacheValid = false;
    _cacheUpdatePending = false;
    _partialActive = false;
}

bool Driver_SSD1681::windowGeometry(size_t* rowBytes, size_t* rows,
                                    size_t* total) const {
    // Accept only in-bounds, 8px-aligned windows; rejecting anything else
    // prevents out-of-bounds writes into _oldCache.
    if (_winX0 > _winX1 || _winY0 > _winY1) return false;
    if (_winX1 >= _init_width || _winY1 >= _init_height) return false;
    if ((_winX0 & 7U) != 0) return false;
    const size_t w = static_cast<size_t>(_winX1) - _winX0 + 1U;
    if ((w & 7U) != 0) return false;
    const size_t rb = w >> 3;
    const size_t stride = cacheStride();
    if (rb < 1U || rb > stride) return false;
    if (static_cast<size_t>(_winX0 >> 3) + rb > stride) return false;
    const size_t r = static_cast<size_t>(_winY1) - _winY0 + 1U;
    if (rowBytes) *rowBytes = rb;
    if (rows) *rows = r;
    if (total) *total = rb * r;
    return true;
}

void Driver_SSD1681::pushOldWindow(size_t rowBytes, size_t rows) {
    if (!_bus || !_oldCache) return;
    const size_t stride = cacheStride();
    _bus->writeCommand(0x26);
    for (size_t r = 0; r < rows; ++r) {
        _bus->writeData(_oldCache +
                            (static_cast<size_t>(_winY0) + r) * stride +
                            (static_cast<size_t>(_winX0) >> 3),
                        rowBytes);
    }
}

void Driver_SSD1681::cacheWindow(const uint8_t* data, size_t rowBytes,
                                 size_t rows) {
    if (!data || !_oldCache) return;
    const size_t stride = cacheStride();
    for (size_t r = 0; r < rows; ++r) {
        memcpy(_oldCache +
                   (static_cast<size_t>(_winY0) + r) * stride +
                   (static_cast<size_t>(_winX0) >> 3),
               data + r * rowBytes, rowBytes);
    }
    _oldCacheValid = false;
    _cacheUpdatePending = true;
}

void Driver_SSD1681::finishRefresh(bool success) {
    // success = both gates clean: lastOperationError()==None and
    // _bus->lastError()==0. Only then is the staged cache patch committed.
    if (!success) {
        invalidateOldCache();
        return;
    }
    if (_cacheUpdatePending) {
        _oldCacheValid = true;
        _cacheUpdatePending = false;
    }
}

void Driver_SSD1681::pushColors(const uint8_t* data, uint16_t w, uint16_t h) {
    _bus->writeCommand(0x24);
    uint32_t count = (uint32_t)w * h / 8;
    for (uint32_t i = 0; i < count; i++) {
        _bus->writeData(data[i]);
    }
}

void Driver_SSD1681::pushColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
    _bus->writeCommand(0x24);
    uint16_t bytes_per_row = w / 8;
    for (uint16_t row = 0; row < h; row++) {
        uint16_t start = row * bytes_per_row;
        for (uint16_t col = 0; col < bytes_per_row; col++) {
            uint8_t b = data[start + (bytes_per_row - 1 - col)];
            b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
            b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
            b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
            _bus->writeData(b);
        }
    }
}

void Driver_SSD1681::pushOldColors(const uint8_t* data, uint16_t w, uint16_t h) {
    _bus->writeCommand(0x26);
    uint32_t count = (uint32_t)w * h / 8;
    for (uint32_t i = 0; i < count; i++) {
        _bus->writeData(data[i]);
    }
}

void Driver_SSD1681::pushOldColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
    _bus->writeCommand(0x26);
    uint16_t bytes_per_row = w / 8;
    for (uint16_t row = 0; row < h; row++) {
        uint16_t start = row * bytes_per_row;
        for (uint16_t col = 0; col < bytes_per_row; col++) {
            uint8_t b = data[start + (bytes_per_row - 1 - col)];
            b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
            b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
            b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
            _bus->writeData(b);
        }
    }
}

void Driver_SSD1681::pushGrayColors(const uint8_t* colors, uint16_t w, uint16_t h) {
    initGray();
    uint32_t totalBytes = (uint32_t)w * h / 8;
    uint8_t temp1, temp2, temp3;

    _bus->writeCommand(0x24);
    for (uint32_t i = 0; i < totalBytes; i++) {
        uint8_t c0 = colors[i * 4 + 0];
        uint8_t c1 = colors[i * 4 + 1];
        uint8_t c2 = colors[i * 4 + 2];
        uint8_t c3 = colors[i * 4 + 3];
        uint8_t p0 = (c0 >> 4) & 0x03;
        uint8_t p1 = (c0 >> 0) & 0x03;
        uint8_t p2 = (c1 >> 4) & 0x03;
        uint8_t p3 = (c1 >> 0) & 0x03;
        uint8_t p4 = (c2 >> 4) & 0x03;
        uint8_t p5 = (c2 >> 0) & 0x03;
        uint8_t p6 = (c3 >> 4) & 0x03;
        uint8_t p7 = (c3 >> 0) & 0x03;
        uint8_t packed_byte0 = (p0 << 6) | (p1 << 4) | (p2 << 2) | p3;
        uint8_t packed_byte1 = (p4 << 6) | (p5 << 4) | (p6 << 2) | p7;

        temp3 = 0;
        for (uint8_t j = 0; j < 2; j++) {
            temp1 = (j == 0) ? packed_byte0 : packed_byte1;
            for (uint8_t k = 0; k < 4; k++) {
                temp2 = temp1 & 0xC0;
                if (temp2 == 0xC0)
                    temp3 |= 0x01;
                else if (temp2 == 0x00)
                    temp3 |= 0x00;
                else if ((temp2 >= 0x80) && (temp2 < 0xC0))
                    temp3 |= 0x00;
                else if (temp2 == 0x40)
                    temp3 |= 0x01;
                if ((j == 0 && k <= 3) || (j == 1 && k <= 2)) {
                    temp3 <<= 1;
                    temp1 <<= 2;
                }
            }
        }
        _bus->writeData(~temp3);
    }

    _bus->writeCommand(0x26);
    for (uint32_t i = 0; i < totalBytes; i++) {
        uint8_t c0 = colors[i * 4 + 0];
        uint8_t c1 = colors[i * 4 + 1];
        uint8_t c2 = colors[i * 4 + 2];
        uint8_t c3 = colors[i * 4 + 3];
        uint8_t p0 = (c0 >> 4) & 0x03;
        uint8_t p1 = (c0 >> 0) & 0x03;
        uint8_t p2 = (c1 >> 4) & 0x03;
        uint8_t p3 = (c1 >> 0) & 0x03;
        uint8_t p4 = (c2 >> 4) & 0x03;
        uint8_t p5 = (c2 >> 0) & 0x03;
        uint8_t p6 = (c3 >> 4) & 0x03;
        uint8_t p7 = (c3 >> 0) & 0x03;
        uint8_t packed_byte0 = (p0 << 6) | (p1 << 4) | (p2 << 2) | p3;
        uint8_t packed_byte1 = (p4 << 6) | (p5 << 4) | (p6 << 2) | p7;

        temp3 = 0;
        for (uint8_t j = 0; j < 2; j++) {
            temp1 = (j == 0) ? packed_byte0 : packed_byte1;
            for (uint8_t k = 0; k < 4; k++) {
                temp2 = temp1 & 0xC0;
                if (temp2 == 0xC0)
                    temp3 |= 0x01;
                else if (temp2 == 0x00)
                    temp3 |= 0x00;
                else if ((temp2 >= 0x80) && (temp2 < 0xC0))
                    temp3 |= 0x01;
                else if (temp2 == 0x40)
                    temp3 |= 0x00;
                if ((j == 0 && k <= 3) || (j == 1 && k <= 2)) {
                    temp3 <<= 1;
                    temp1 <<= 2;
                }
            }
        }
        _bus->writeData(~temp3);
    }
}

void Driver_SSD1681::pushGrayColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
    pushGrayColors(data, w, h);
}

} }
