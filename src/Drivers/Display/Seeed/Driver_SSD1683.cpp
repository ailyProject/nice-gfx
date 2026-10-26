// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "Driver_SSD1683.h"
#include "SeeedHal.h"
#include <stdlib.h>
#include <string.h>
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_SSD1683.cpp
 * @brief  SSD1683 ePaper display driver implementation
 */




Driver_SSD1683::Driver_SSD1683(uint16_t w, uint16_t h, int8_t busyPin)
    : _init_width(w), _init_height(h), _busyPin(busyPin) {
    _width = w; _height = h;
}

Driver_SSD1683::~Driver_SSD1683() {
    protocolFree(_oldCache);
    _oldCache = nullptr;
}

void Driver_SSD1683::busyWait() {
    if (_busyPin < 0) return;
    (void)waitForReadyPin(_busyPin, false);
}

bool Driver_SSD1683::ensureOldCache() {
    if (_oldCache) return true;
    const size_t full = frameBytes();
    _oldCache = static_cast<uint8_t*>(protocolAllocate(full));
    if (_oldCache) memset(_oldCache, 0xFF, full);
    _oldCacheValid = false;
    return _oldCache != nullptr;
}

void Driver_SSD1683::invalidateOldCache() {
    protocolFree(_oldCache);
    _oldCache = nullptr;
    _oldCacheValid = false;
    _cacheUpdatePending = false;
    _partialActive = false;
}

bool Driver_SSD1683::windowGeometry(size_t* rowBytes, size_t* rows, size_t* total) const {
    if (_winX0 > _winX1 || _winY0 > _winY1) return false;
    if (_winX1 >= _init_width || _winY1 >= _init_height) return false;
    if ((_winX0 & 7U) != 0) return false;  // 8px X alignment required
    const size_t w = (size_t)(_winX1 - _winX0) + 1U;
    if ((w & 7U) != 0) return false;
    const size_t rb = w >> 3;
    const size_t stride = cacheStride();
    if (rb < 1U || rb > stride) return false;
    if ((size_t)(_winX0 >> 3) + rb > stride) return false;
    *rowBytes = rb;
    *rows = (size_t)(_winY1 - _winY0) + 1U;
    *total = rb * *rows;
    return true;
}

void Driver_SSD1683::pushOldWindow(size_t rowBytes, size_t rows) {
    _bus->writeCommand(0x26);
    for (size_t r = 0; r < rows; ++r) {
        _bus->writeData(_oldCache + ((size_t)_winY0 + r) * cacheStride() +
                            ((size_t)_winX0 >> 3),
                        rowBytes);
    }
}

void Driver_SSD1683::cacheWindow(const uint8_t* data, size_t rowBytes,
                                 size_t rows) {
    if (!data || !_oldCache) return;
    for (size_t r = 0; r < rows; ++r) {
        memcpy(_oldCache + ((size_t)_winY0 + r) * cacheStride() +
                   ((size_t)_winX0 >> 3),
               data + r * rowBytes, rowBytes);
    }
    _oldCacheValid = false;
    _cacheUpdatePending = true;
}

void Driver_SSD1683::finishRefresh(bool success) {
    if (!success) {
        invalidateOldCache();
        return;
    }
    if (_cacheUpdatePending) {
        _oldCacheValid = true;
        _cacheUpdatePending = false;
    }
}

bool Driver_SSD1683::init(IBus& bus) {
    // Reset session state before any early return below; _oldCache is kept —
    // on bistable glass it stays closer to the panel than a white baseline.
    if (_cacheUpdatePending) invalidateOldCache();
    _partialActive = false;
    _lastPartialTimedOut = false;
    _fastFallbackFull = false;
    _bus = &bus;
    if (_busyPin >= 0) pinMode(_busyPin, NGFX_GXEPD2_INPUT);
    hardwareReset(10, 10);
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
    _bus->writeCommand(0x21); // Display update control
    _bus->writeData(0x40);
    _bus->writeData(0x00);
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

void Driver_SSD1683::setRotation(uint8_t m) {
    _rotation = m % 4;
    _bus->writeCommand(0x11);
    switch (_rotation) {
        case 0:
            _bus->writeData(0x03);  // X+ Y+: pushColors writes top-to-bottom (row0 = Y0)
            _width = _init_width; _height = _init_height;
            break;
        case 1:
            _bus->writeData(0x03);
            _width = _init_height; _height = _init_width;
            break;
        case 2:
            _bus->writeData(0x00);
            _width = _init_width; _height = _init_height;
            break;
        case 3:
            _bus->writeData(0x02);
            _width = _init_height; _height = _init_width;
            break;
    }
}

void Driver_SSD1683::invertDisplay(bool) {}

void Driver_SSD1683::displayOn()  { update(); }
void Driver_SSD1683::displayOff() {}

void Driver_SSD1683::setAddrWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    if (lastOperationError() != DriverOperationError::None || !_bus) return;
    _winX0 = x1; _winY0 = y1; _winX1 = x2; _winY1 = y2;  // Cache window; re-issued before each RAM plane (pure register writes)
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

void Driver_SSD1683::writePixel(uint16_t) {}
void Driver_SSD1683::writePixels(const uint16_t*, size_t) {}
void Driver_SSD1683::writeFill(uint16_t, size_t) {}

void Driver_SSD1683::sleep() {
    if (_cacheUpdatePending) invalidateOldCache();
    _partialActive = false;
    _fastFallbackFull = false;
    // One-shot: skip deep sleep after a partial-refresh busy timeout — the
    // controller may still be driving. Cost: standby power until next wake.
    if (_lastPartialTimedOut) {
        _lastPartialTimedOut = false;
    } else {
        _bus->writeCommand(0x10);
        _bus->writeData(0x01);
    }
    delay(100);
}

void Driver_SSD1683::wake() {
    init(*_bus);
}

void Driver_SSD1683::update() {
    if (lastOperationError() != DriverOperationError::None || !_bus ||
        _bus->lastError() != 0) {
        finishRefresh(false);
        return;
    }
    _bus->writeCommand(0x22);
    _bus->writeData(0xF7);
    _bus->writeCommand(0x20);
    busyWait();
    const bool ok = (lastOperationError() == DriverOperationError::None) &&
                    (!_bus || _bus->lastError() == 0);
    finishRefresh(ok);
}

void Driver_SSD1683::updateFast() {
    if (lastOperationError() != DriverOperationError::None || !_bus ||
        _bus->lastError() != 0) {
        finishRefresh(false);
        _fastFallbackFull = false;
        return;
    }
    // Restore the monochrome update-control mode before a fast full update.
    _bus->writeCommand(0x21);
    _bus->writeData(0x40);
    _bus->writeData(0x00);
    if (!_fastFallbackFull) {
        // GDEY042T81 2024 uses 0x6E; this value is also compatible with the
        // earlier panel revision. 0xD7 then selects the OTP fast waveform.
        _bus->writeCommand(0x1A);
        _bus->writeData(0x6E);
    }
    _bus->writeCommand(0x22);
    _bus->writeData(_fastFallbackFull ? 0xF7 : 0xD7);
    _bus->writeCommand(0x20);
    busyWait();
    const bool ok = (lastOperationError() == DriverOperationError::None) &&
                    (!_bus || _bus->lastError() == 0);
    finishRefresh(ok);
    _fastFallbackFull = false;
}

void Driver_SSD1683::updateGray() {
    if (lastOperationError() != DriverOperationError::None || !_bus ||
        _bus->lastError() != 0) return;
    _bus->writeCommand(0x22);
    _bus->writeData(0xCF);
    _bus->writeCommand(0x20);
    busyWait();
    if (lastOperationError() != DriverOperationError::None ||
        _bus->lastError() != 0) invalidateOldCache();
}

void Driver_SSD1683::updatePartial() {
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
    const bool ok = (lastOperationError() == DriverOperationError::None) &&
                    (!_bus || _bus->lastError() == 0);
    if (!ok && lastOperationError() == DriverOperationError::BusyTimeout) {
        _lastPartialTimedOut = true;  // arm the one-shot sleep() guard
    }
    finishRefresh(ok);
    _partialActive = false;
}

void Driver_SSD1683::initGray() {
    if (applyWaveformProfile(EPaperWaveformMode::Gray, _busyPin, false)) return;
    if (lastOperationError() != DriverOperationError::None) return;
    _bus->writeCommand(0x21);
    _bus->writeData(0x00);
    _bus->writeData(0x00);
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

void Driver_SSD1683::initPartial() {
    if (applyWaveformProfile(EPaperWaveformMode::Partial, _busyPin, false)) return;
    if (lastOperationError() != DriverOperationError::None) return;
    _bus->writeCommand(0x21);
    _bus->writeData(0x00);
    _bus->writeData(0x00);
    _bus->writeCommand(0x18);
    _bus->writeData(0x80);
    _bus->writeCommand(0x3C);
    _bus->writeData(0x80);
}

void Driver_SSD1683::wakeGray() { wake(); }

void Driver_SSD1683::wakePartial() {
    init(*_bus);
    if (lastOperationError() != DriverOperationError::None) return;
    initPartial();
    if (lastOperationError() != DriverOperationError::None) return;
    if (!ensureOldCache() || !_oldCacheValid) {
        setOperationError(DriverOperationError::CommunicationFailed);
        return;
    }
    _lastPartialTimedOut = false;
    _partialActive = true;  // after init(): init() clears the flag at its top
}

void Driver_SSD1683::wakeFast() {
    wake();
    if (lastOperationError() != DriverOperationError::None) return;
    _fastFallbackFull = !_oldCacheValid;
}

void Driver_SSD1683::pushNewColors(const uint8_t* data, size_t len) {
    if (!_bus || !data ||
        lastOperationError() != DriverOperationError::None) return;

    size_t rowBytes = 0, rows = 0, total = 0;
    if (_partialActive) {
        if (!_oldCache || !_oldCacheValid ||
            !windowGeometry(&rowBytes, &rows, &total) || len != total) {
            setOperationError(DriverOperationError::CommunicationFailed);
            return;
        }
        // Partial push: the controller diffs old (0x26) vs new (0x24). Write
        // the cached old window first; re-issue the window before each plane.
        setAddrWindow(_winX0, _winY0, _winX1, _winY1);
        pushOldWindow(rowBytes, rows);
        setAddrWindow(_winX0, _winY0, _winX1, _winY1);
        _bus->writeCommand(0x24);
        _bus->writeData(data, len);
        cacheWindow(data, rowBytes, rows);
        return;
    }

    if (len == frameBytes()) {
        // Cache allocation is best-effort: never turn memory pressure in the
        // partial-refresh bookkeeping into a failed full-screen refresh.
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

void Driver_SSD1683::pushColors(const uint8_t* data, uint16_t w, uint16_t h) {
    _bus->writeCommand(0x24);
    uint32_t count = (uint32_t)w * h / 8;
    for (uint32_t i = 0; i < count; i++) {
        _bus->writeData(data[i]);
    }
}

void Driver_SSD1683::pushColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
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

void Driver_SSD1683::pushOldColors(const uint8_t* data, uint16_t w, uint16_t h) {
    _bus->writeCommand(0x26);
    uint32_t count = (uint32_t)w * h / 8;
    for (uint32_t i = 0; i < count; i++) {
        _bus->writeData(data[i]);
    }
}

void Driver_SSD1683::pushOldColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
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

void Driver_SSD1683::pushGrayColors(const uint8_t* colors, uint16_t w, uint16_t h) {
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
        _bus->writeData(temp3);
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
        _bus->writeData(temp3);
    }
}

void Driver_SSD1683::pushGrayColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
    pushGrayColors(data, w, h);
}

} }
