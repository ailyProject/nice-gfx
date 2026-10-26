// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "Driver_JD79660.h"
#include "seeed_ep.h"
#include "SeeedHal.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_JD79660.cpp
 * @brief  JD79660 ePaper display driver implementation
 *
 * 4bpp ePaper driver with grayscale support.
 * Ported from TFT_Drivers/JD79660_Defines.h and JD79660_Init.h
 */



Driver_JD79660::Driver_JD79660(uint16_t w, uint16_t h, int8_t busyPin)
    : _init_width(w), _init_height(h), _busyPin(busyPin)
    , _waveformProfile(findEPaperWaveformProfile("JD79660", "default", w, h, 4))
    , _waveformResult(EPaperWaveformResult::Ok)
{
    _width = w;
    _height = h;
}

void Driver_JD79660::busyWait() {
    if (_busyPin < 0) return;
    (void)waitForReadyPin(_busyPin, true, 21000);
}

bool Driver_JD79660::init(IBus& bus) {
    _bus = &bus;

    if (_busyPin >= 0) pinMode(_busyPin, NGFX_GXEPD2_INPUT);
    hardwareReset(50, 100);
    busyWait();
    if (lastOperationError() != DriverOperationError::None) return false;

    if (!applyWaveform(EPaperWaveformMode::Full)) return false;

    return lastOperationError() == DriverOperationError::None;
}

void Driver_JD79660::setRotation(uint8_t rotation) {
    _rotation = rotation % 4;
    if (_rotation & 1U) {
        _width = _init_height;
        _height = _init_width;
    } else {
        _width = _init_width;
        _height = _init_height;
    }
}

void Driver_JD79660::invertDisplay(bool invert) {
    (void)invert;
}

void Driver_JD79660::displayOn() {
    _bus->writeCommand(0x04);
    busyWait();
}

void Driver_JD79660::displayOff() {
    _bus->writeCommand(0x02);
    delay(1);
    busyWait();
}

void Driver_JD79660::setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye) {
    _bus->writeCommand(0x90);
    _bus->writeData(xs / 256);
    _bus->writeData(xs % 256);
    _bus->writeData(xe / 256);
    _bus->writeData(xe % 256);
    _bus->writeData(ys / 256);
    _bus->writeData(ys % 256);
    _bus->writeData(ye / 256);
    _bus->writeData(ye % 256);
    _bus->writeData(0x01);
}

void Driver_JD79660::writePixel(uint16_t color) {
    (void)color;
}

void Driver_JD79660::writePixels(const uint16_t* data, size_t len) {
    (void)data; (void)len;
}

void Driver_JD79660::writeFill(uint16_t color, size_t len) {
    (void)color; (void)len;
}

void Driver_JD79660::sleep() {
    _bus->writeCommand(0x02);   // Power off
    _bus->writeData(0x00);
    delay(100);
    busyWait();
    if (lastOperationError() != DriverOperationError::None) return;
    _bus->writeCommand(0x07);   // Deep sleep
    _bus->writeData(0xA5);
}

void Driver_JD79660::wake() {
    // Deep sleep clears controller state; reset and replay the selected
    // panel sequence so a second refresh is identical to the first.
    (void)init(*_bus);
}

size_t Driver_JD79660::waveformProfileCount() const {
    return ePaperWaveformProfileCount(name(), _init_width, _init_height,
                                      colorDepth());
}

const EPaperWaveformProfile* Driver_JD79660::waveformProfileAt(size_t index) const {
    return ePaperWaveformProfileAt(name(), index, _init_width, _init_height,
                                   colorDepth());
}

bool Driver_JD79660::selectWaveformProfile(const char* id) {
    if (!id) {
        _waveformResult = EPaperWaveformResult::UnknownProfile;
        return false;
    }
    const EPaperWaveformProfile* candidate = findEPaperWaveformProfile(
        name(), id, _init_width, _init_height, colorDepth());
    if (!candidate) {
        _waveformResult = EPaperWaveformResult::UnknownProfile;
        return false;
    }
    if (!ePaperWaveformProfileMatches(*candidate, name(), _init_width,
                                      _init_height, colorDepth())) {
        _waveformResult = EPaperWaveformResult::IncompatibleProfile;
        return false;
    }
    _waveformProfile = candidate;
    _waveformResult = EPaperWaveformResult::Ok;
    return true;
}

bool Driver_JD79660::applyWaveform(EPaperWaveformMode mode) {
    const EPaperCommandSequence* sequence = _waveformProfile
        ? ePaperWaveformSequence(*_waveformProfile, mode) : nullptr;
    if (!sequence) {
        _waveformResult = EPaperWaveformResult::UnsupportedMode;
        return false;
    }
    _waveformResult = applyEPaperCommandSequence(*_bus, *sequence, _init_width,
                                                  _init_height, _busyPin, true, 21000);
    if (_waveformResult == EPaperWaveformResult::BusyTimeout) {
        setOperationError(DriverOperationError::BusyTimeout);
    } else if (_waveformResult == EPaperWaveformResult::InvalidSequence) {
        setOperationError(DriverOperationError::CommunicationFailed);
    }
    return _waveformResult == EPaperWaveformResult::Ok;
}

void Driver_JD79660::update() {
    _bus->writeCommand(0x12);
    _bus->writeData(0x00);
    busyWait();
}

void Driver_JD79660::pushColors(const uint8_t* data, uint16_t w, uint16_t h) {
    if (!data) return;
    const uint16_t bytesPerRow = static_cast<uint16_t>((w + 1U) / 2U);
    _bus->writeCommand(0x10);
    for (uint16_t row = 0; row < h; ++row) {
        for (uint16_t col = 0; col + 1 < bytesPerRow; col += 2) {
            const uint8_t first = data[static_cast<size_t>(row) * bytesPerRow + col];
            const uint8_t second = data[static_cast<size_t>(row) * bytesPerRow + col + 1];
            _bus->writeData(static_cast<uint8_t>(
                (colorGet(first >> 4) << 6) | (colorGet(first) << 4) |
                (colorGet(second >> 4) << 2) | colorGet(second)));
        }
    }
}

void Driver_JD79660::pushColorsFlip(const uint8_t* data, uint16_t w, uint16_t h) {
    if (!data) return;
    const uint16_t bytesPerRow = static_cast<uint16_t>((w + 1U) / 2U);
    _bus->writeCommand(0x10);
    for (uint16_t row = 0; row < h; row++) {
        const size_t start = static_cast<size_t>(row) * bytesPerRow;
        for (uint16_t col = 0; col + 1 < bytesPerRow; col += 2) {
            const uint8_t first = data[start + bytesPerRow - 1U - col];
            const uint8_t second = data[start + bytesPerRow - 2U - col];
            _bus->writeData(static_cast<uint8_t>(
                (colorGet(first) << 6) | (colorGet(first >> 4) << 4) |
                (colorGet(second) << 2) | colorGet(second >> 4)));
        }
    }
}

void Driver_JD79660::pushOldColors(const uint8_t*, uint16_t, uint16_t) {}
void Driver_JD79660::pushOldColorsFlip(const uint8_t*, uint16_t, uint16_t) {}

uint8_t Driver_JD79660::colorGet(uint8_t color) {
    switch (color & 0x0F) {
        case 0x00: return 0x01; // white
        case 0x0B: return 0x02; // yellow
        case 0x06: return 0x03; // red
        case 0x0F: return 0x00; // black
        default: return 0x00;
    }
}

} }
