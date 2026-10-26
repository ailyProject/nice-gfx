// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "Driver_SSD1677.h"
#include "SeeedHal.h"
#include <stdlib.h>
#include <string.h>
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_SSD1677.cpp
 * @brief  SSD1677 ePaper display driver implementation
 *
 * Ported from TFT_Drivers/SSD1677_Defines.h, SSD1677_Init.h, SSD1677_Rotation.h
 */




Driver_SSD1677::Driver_SSD1677(uint16_t w, uint16_t h, int8_t busyPin)
    : _init_width(w), _init_height(h), _busyPin(busyPin) {
    _width = w; _height = h;
}

Driver_SSD1677::~Driver_SSD1677() {
    if (_partialBaseline) {
        protocolFree(_partialBaseline);
        _partialBaseline = nullptr;
    }
}

void Driver_SSD1677::writeMonoData(const uint8_t* data, size_t len) {
    if (_invertMono) {
        uint8_t out[512];
        for (size_t off = 0; off < len; off += sizeof(out)) {
            size_t n = (len - off < sizeof(out)) ? (len - off) : sizeof(out);
            for (size_t i = 0; i < n; i++) out[i] = data[off + i] ^ 0xFF;
            _bus->writeData(out, n);
        }
    } else {
        _bus->writeData(data, len);
    }
}

void Driver_SSD1677::ensureBaseline() {
    if (_partialBaseline) return;
    const size_t bytes = ((size_t)(_init_width + 7U) / 8U) * _init_height;
    _partialBaseline = static_cast<uint8_t*>(protocolAllocate(bytes));
    if (_partialBaseline) memset(_partialBaseline, 0xFF, bytes);  // all-white raw (1 = white)
}

void Driver_SSD1677::busyWait() {
    if (_busyPin < 0) return;
    (void)waitForReadyPin(_busyPin, false);
}

bool Driver_SSD1677::init(IBus& bus) {
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

    _bus->writeCommand(0x0C); // Booster soft start
    _bus->writeData(0xAE);
    _bus->writeData(0xC7);
    _bus->writeData(0xC3);
    _bus->writeData(0xC0);
    _bus->writeData(0x80);
    _bus->writeCommand(0x3C); // Border waveform
    _bus->writeData(0x05);
    _bus->writeCommand(0x01); // Driver output control
    _bus->writeData((_init_height - 1) % 256);
    _bus->writeData((_init_height - 1) / 256);
    _bus->writeData(0x02);
    _bus->writeCommand(0x11); // Data entry mode
    _bus->writeData(0x03);
    _bus->writeCommand(0x44); // RAM X range
    _bus->writeData(0x00);
    _bus->writeData(0x00);
    _bus->writeData((_init_width - 1) % 256);
    _bus->writeData((_init_width - 1) / 256);
    _bus->writeCommand(0x45); // RAM Y range
    _bus->writeData(0x00);
    _bus->writeData(0x00);
    _bus->writeData((_init_height - 1) % 256);
    _bus->writeData((_init_height - 1) / 256);
    _bus->writeCommand(0x18); // Temperature sensor
    _bus->writeData(0x80);
    _bus->writeCommand(0x4E); // RAM X counter
    _bus->writeData(0x00);
    _bus->writeData(0x00);
    _bus->writeCommand(0x4F); // RAM Y counter
    _bus->writeData(0x00);
    _bus->writeData(0x00);
    busyWait();
    setRotation(0);
    return lastOperationError() == DriverOperationError::None;
}

void Driver_SSD1677::setRotation(uint8_t m) {
    _rotation = m % 4;
    switch (_rotation) {
        case 0:
            _bus->writeCommand(0x11);
            _bus->writeData(0x03);
            _width = _init_width; _height = _init_height;
            break;
        case 1:
            _bus->writeCommand(0x11);
            _bus->writeData(0x01);
            _width = _init_height; _height = _init_width;
            break;
        case 2:
            _bus->writeCommand(0x11);
            _bus->writeData(0x00);
            _width = _init_width; _height = _init_height;
            break;
        case 3:
            _bus->writeCommand(0x11);
            _bus->writeData(0x02);
            _width = _init_height; _height = _init_width;
            break;
    }
}

void Driver_SSD1677::invertDisplay(bool invert) { _invertMono = invert; }

void Driver_SSD1677::displayOn()  { update(); }
void Driver_SSD1677::displayOff() {}

void Driver_SSD1677::setAddrWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    _winX0 = x1; _winY0 = y1; _winX1 = x2; _winY1 = y2;
    _bus->writeCommand(0x44);
    _bus->writeData(x1 & 0xFF);
    _bus->writeData(x1 >> 8);
    _bus->writeData(x2 & 0xFF);
    _bus->writeData(x2 >> 8);
    _bus->writeCommand(0x45);
    _bus->writeData(y1 & 0xFF);
    _bus->writeData(y1 >> 8);
    _bus->writeData(y2 & 0xFF);
    _bus->writeData(y2 >> 8);
    _bus->writeCommand(0x4E);
    _bus->writeData(x1 & 0xFF);
    _bus->writeData(x1 >> 8);
    _bus->writeCommand(0x4F);
    _bus->writeData(y1 & 0xFF);
    _bus->writeData(y1 >> 8);
}

void Driver_SSD1677::writePixel(uint16_t) {}
void Driver_SSD1677::writePixels(const uint16_t*, size_t) {}
void Driver_SSD1677::writeFill(uint16_t, size_t) {}

void Driver_SSD1677::sleep() {
    _bus->writeCommand(0x10);
    _bus->writeData(0x01);
    delay(100);
}

void Driver_SSD1677::wake() {
    init(*_bus);
}

void Driver_SSD1677::update() {
    _bus->writeCommand(0x22);
    _bus->writeData(0xF7);
    _bus->writeCommand(0x20);
    busyWait();
}

void Driver_SSD1677::updateGray() {
    _bus->writeCommand(0x22);
    _bus->writeData(0xD7);
    _bus->writeCommand(0x20);
    busyWait();
}

void Driver_SSD1677::updatePartial() {
    _bus->writeCommand(0x22);
    _bus->writeData(0xFF);
    _bus->writeCommand(0x20);
    busyWait();
    _partialActive = false;
}

void Driver_SSD1677::initGray() {
    if (applyWaveformProfile(EPaperWaveformMode::Gray, _busyPin, false)) return;
    if (lastOperationError() != DriverOperationError::None) return;
    _bus->writeCommand(0x12); // Software reset
    busyWait();
    _bus->writeCommand(0x0C);
    _bus->writeData(0xAE);
    _bus->writeData(0xC7);
    _bus->writeData(0xC3);
    _bus->writeData(0xC0);
    _bus->writeData(0x80);
    _bus->writeCommand(0x3C);
    _bus->writeData(0x00);
    _bus->writeCommand(0x01);
    _bus->writeData((_init_height - 1) % 256);
    _bus->writeData((_init_height - 1) / 256);
    _bus->writeData(0x02);
    _bus->writeCommand(0x11);
    _bus->writeData(0x03);
    _bus->writeCommand(0x44);
    _bus->writeData(0x00);
    _bus->writeData(0x00);
    _bus->writeData((_init_width - 1) % 256);
    _bus->writeData((_init_width - 1) / 256);
    _bus->writeCommand(0x45);
    _bus->writeData(0x00);
    _bus->writeData(0x00);
    _bus->writeData((_init_height - 1) % 256);
    _bus->writeData((_init_height - 1) / 256);
    _bus->writeCommand(0x18);
    _bus->writeData(0x80);
    _bus->writeCommand(0x1A);
    _bus->writeData(0x67);
    _bus->writeData(0x00);
    _bus->writeCommand(0x4E);
    _bus->writeData(0x00);
    _bus->writeData(0x00);
    _bus->writeCommand(0x4F);
    _bus->writeData(0x00);
    _bus->writeData(0x00);
    busyWait();
}

void Driver_SSD1677::initPartial() {
    if (applyWaveformProfile(EPaperWaveformMode::Partial, _busyPin, false)) return;
    if (lastOperationError() != DriverOperationError::None) return;
    _bus->writeCommand(0x18);
    _bus->writeData(0x80);
    _bus->writeCommand(0x3C);
    _bus->writeData(0x80);
}

void Driver_SSD1677::wakeGray() {
    // Deep sleep (0x10, 0x01) can only be left through the reset pin.  The
    // gray profile is a complete controller setup, so reset once here and do
    // not replay the normal full-refresh setup first.
    hardwareReset(10, 10);
    initGray();
}

void Driver_SSD1677::wakePartial() {
    init(*_bus);
    initPartial();
    _partialActive = true;
}

void Driver_SSD1677::pushColors(const uint8_t* data, uint16_t w, uint16_t h) {
    _bus->writeCommand(0x24);
    uint32_t count = (uint32_t)w * h / 8;
    for (uint32_t i = 0; i < count; i++) {
        _bus->writeData(data[i]);
    }
}

void Driver_SSD1677::pushColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
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

void Driver_SSD1677::pushOldColors(const uint8_t* data, uint16_t w, uint16_t h) {
    _bus->writeCommand(0x26);
    uint32_t count = (uint32_t)w * h / 8;
    for (uint32_t i = 0; i < count; i++) {
        _bus->writeData(data[i]);
    }
}

void Driver_SSD1677::pushNewColors(const uint8_t* data, size_t len) {
    if (!_bus || !data) return;

    if (_partialActive) {
        // Partial: the controller diffs the previous (0x26) and current (0x24)
        // planes to decide each pixel's transition. Feeding only the new window
        // left it without an old-frame reference (gray/gradient ghosting), so
        // send the matching previous window from the baseline first, then the
        // new window, then advance the baseline.
        const uint16_t w = static_cast<uint16_t>(_winX1 - _winX0 + 1U);
        const uint16_t h = static_cast<uint16_t>(_winY1 - _winY0 + 1U);
        const size_t stride = ((size_t)(_init_width + 7U) / 8U);
        const uint16_t rowBytes = static_cast<uint16_t>((w + 7U) / 8U);

        ensureBaseline();
        _bus->writeCommand(0x26);  // previous plane
        if (_partialBaseline) {
            for (uint16_t r = 0; r < h; r++) {
                writeMonoData(_partialBaseline +
                                  (size_t)(_winY0 + r) * stride + (_winX0 >> 3),
                              rowBytes);
            }
            for (uint16_t r = 0; r < h; r++) {
                memcpy(_partialBaseline + (size_t)(_winY0 + r) * stride + (_winX0 >> 3),
                       data + (size_t)r * rowBytes, rowBytes);
            }
        }
        _bus->writeCommand(0x24);  // current plane
        writeMonoData(data, len);
    } else {
        // Full frame: remember it as the partial baseline, then emit it.
        const size_t fullBytes = ((size_t)(_init_width + 7U) / 8U) * _init_height;
        if (len >= fullBytes) {
            ensureBaseline();
            if (_partialBaseline) memcpy(_partialBaseline, data, fullBytes);
        }
        _bus->writeCommand(0x24);
        writeMonoData(data, len);
    }
}

void Driver_SSD1677::pushOldColors(const uint8_t* data, size_t len) {
    if (!_bus || !data) return;
    _bus->writeCommand(0x26);
    writeMonoData(data, len);
}

void Driver_SSD1677::pushOldColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
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

void Driver_SSD1677::pushGrayColors(const uint8_t* colors, uint16_t w, uint16_t h) {
    uint16_t blocks_per_row = w / 8;
    uint8_t temp1, temp2, temp3;

    _bus->writeCommand(0x24);
    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < blocks_per_row; col++) {
            uint32_t i = row * blocks_per_row + (blocks_per_row - 1 - col);
            uint8_t c0 = colors[i * 4 + 3];
            uint8_t c1 = colors[i * 4 + 2];
            uint8_t c2 = colors[i * 4 + 1];
            uint8_t c3 = colors[i * 4 + 0];
            uint8_t p0 = (c0 >> 0) & 0x03;
            uint8_t p1 = (c0 >> 4) & 0x03;
            uint8_t p2 = (c1 >> 0) & 0x03;
            uint8_t p3 = (c1 >> 4) & 0x03;
            uint8_t p4 = (c2 >> 0) & 0x03;
            uint8_t p5 = (c2 >> 4) & 0x03;
            uint8_t p6 = (c3 >> 0) & 0x03;
            uint8_t p7 = (c3 >> 4) & 0x03;
            uint8_t packed_byte0 = (p0 << 6) | (p1 << 4) | (p2 << 2) | p3;
            uint8_t packed_byte1 = (p4 << 6) | (p5 << 4) | (p6 << 2) | p7;

            temp3 = 0;
            for (uint8_t j = 0; j < 2; j++) {
                temp1 = (j == 0) ? packed_byte0 : packed_byte1;
                for (uint8_t k = 0; k < 4; k++) {
                    temp2 = temp1 & 0xC0;
                    if (temp2 == 0x00)
                        temp3 |= 0x01;
                    else if (temp2 == 0x40)
                        temp3 |= 0x01;
                    else if (temp2 == 0x80)
                        temp3 |= 0x00;
                    else if (temp2 == 0xC0)
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

    _bus->writeCommand(0x26);
    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < blocks_per_row; col++) {
            uint32_t i = row * blocks_per_row + (blocks_per_row - 1 - col);
            uint8_t c0 = colors[i * 4 + 3];
            uint8_t c1 = colors[i * 4 + 2];
            uint8_t c2 = colors[i * 4 + 1];
            uint8_t c3 = colors[i * 4 + 0];
            uint8_t p0 = (c0 >> 0) & 0x03;
            uint8_t p1 = (c0 >> 4) & 0x03;
            uint8_t p2 = (c1 >> 0) & 0x03;
            uint8_t p3 = (c1 >> 4) & 0x03;
            uint8_t p4 = (c2 >> 0) & 0x03;
            uint8_t p5 = (c2 >> 4) & 0x03;
            uint8_t p6 = (c3 >> 0) & 0x03;
            uint8_t p7 = (c3 >> 4) & 0x03;
            uint8_t packed_byte0 = (p0 << 6) | (p1 << 4) | (p2 << 2) | p3;
            uint8_t packed_byte1 = (p4 << 6) | (p5 << 4) | (p6 << 2) | p7;

            temp3 = 0;
            for (uint8_t j = 0; j < 2; j++) {
                temp1 = (j == 0) ? packed_byte0 : packed_byte1;
                for (uint8_t k = 0; k < 4; k++) {
                    temp2 = temp1 & 0xC0;
                    if (temp2 == 0x00)
                        temp3 |= 0x01;
                    else if (temp2 == 0x40)
                        temp3 |= 0x00;
                    else if (temp2 == 0x80)
                        temp3 |= 0x01;
                    else if (temp2 == 0xC0)
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
}

void Driver_SSD1677::pushGrayColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
    pushGrayColors(data, w, h);
}

} }
