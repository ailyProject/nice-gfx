// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "Driver_Sticky_Auto.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_Sticky_Auto.cpp
 * @brief  Auto-detect composite driver for the reTerminal Sticky ePaper
 *
 * Stage 1 aligns with the Sticky product firmware
 * (seeed_epaper/epaper_panel.c resolve_panel_model): reset with the SSD2677
 * timing (low 20 ms / high 10 ms), write command 0x70, read one byte, and
 * treat 0x07 as SSD2677. Power is already on and the bus already begun when
 * Panel_EPaper::begin() calls init(), exactly like the firmware's
 * new_panel -> resolve_panel_model ordering.
 *
 * Stage 2 covers units whose panel SDO is not wired to the shared MISO
 * (observed in the field: probe and temperature reads both come back 0x00).
 * A 0x00/0xFF probe byte means "no chip is driving the line", so instead of
 * the firmware's else branch (which mis-resolves those SSD2677 units to
 * SSD1677) the wrapper identifies the chip by its post-reset BUSY waveform:
 * SSD2677 is busy NGFX_GXEPD2_LOW / ready NGFX_GXEPD2_HIGH, SSD1677 busy NGFX_GXEPD2_HIGH / ready NGFX_GXEPD2_LOW
 * (firmware apply_panel_model_config busy_level 0 vs 1).
 */

Driver_Sticky_Auto::Driver_Sticky_Auto(uint16_t w, uint16_t h)
    : _ssd1677(w, h), _ssd2677(w, h) {
    _width = w;
    _height = h;
}

const char* Driver_Sticky_Auto::name() const {
    return _active ? _active->name() : "Sticky-Auto";
}

uint16_t Driver_Sticky_Auto::width() const {
    return _active ? _active->width() : _width;
}

uint16_t Driver_Sticky_Auto::height() const {
    return _active ? _active->height() : _height;
}

uint16_t Driver_Sticky_Auto::nativeWidth() const {
    return _active ? _active->nativeWidth() : _width;
}

uint16_t Driver_Sticky_Auto::nativeHeight() const {
    return _active ? _active->nativeHeight() : _height;
}

uint8_t Driver_Sticky_Auto::colorDepth() const {
    return _active ? _active->colorDepth() : 1;
}

bool Driver_Sticky_Auto::supportsReadback() const {
    return _active && _active->supportsReadback();
}

bool Driver_Sticky_Auto::supportsPartialRefresh() const {
    return _active && _active->supportsPartialRefresh();
}

uint16_t Driver_Sticky_Auto::partialXAlignment() const {
    return _active ? _active->partialXAlignment() : 0;
}

bool Driver_Sticky_Auto::supportsFastRefresh() const {
    return _active && _active->supportsFastRefresh();
}

bool Driver_Sticky_Auto::supportsGrayRefresh(uint8_t levels) const {
    return _active && _active->supportsGrayRefresh(levels);
}

bool Driver_Sticky_Auto::supportsColorfulEPaper() const {
    return _active && _active->supportsColorfulEPaper();
}

bool Driver_Sticky_Auto::supportsBWRYEPaper() const {
    return _active && _active->supportsBWRYEPaper();
}

bool Driver_Sticky_Auto::supportsDeepSleep() const {
    return _active ? _active->supportsDeepSleep() : true;
}

bool Driver_Sticky_Auto::supportsTemperatureCompensation() const {
    return _active && _active->supportsTemperatureCompensation();
}

DriverOperationError Driver_Sticky_Auto::lastOperationError() const {
    return _active ? _active->lastOperationError()
                   : IDriver::lastOperationError();
}

void Driver_Sticky_Auto::clearOperationError() {
    IDriver::clearOperationError();
    _ssd1677.clearOperationError();
    _ssd2677.clearOperationError();
}

bool Driver_Sticky_Auto::init(IBus& bus) {
    _bus = &bus;

    IDriver* selected = nullptr;

    // Stage 1 — firmware resolve_panel_model probe: reset with the SSD2677
    // timing (low 20 ms / high 10 ms), command 0x70, read one byte, 0x07 is
    // SSD2677. A byte actively driven to any other value is an SSD1677
    // answering, matching the firmware's "anything else" rule. 0x00/0xFF is
    // what an undriven line returns; on units whose panel SDO is not wired
    // to the shared MISO the firmware's else branch would mis-resolve
    // SSD2677 glass to SSD1677 and leave the panel blank, so those reads
    // fall through to the BUSY-polarity stage instead.
    if (bus.capabilities().readable) {
        hardwareReset(20, 10);
        bus.writeCommand(0x70);
        bus.beginRead();
        const uint8_t chipId = bus.readData();
        bus.endRead();
        _probedChipId = chipId;
        if (chipId == kSsd2677ChipId) {
            selected = &_ssd2677;
        } else if (chipId != kUnwiredLow && chipId != kUnwiredHigh) {
            selected = &_ssd1677;
        }
    }

    // Stage 2 — BUSY-polarity detection, needs no SPI read at all.
    if (!selected) {
        selected = probeByBusyPolarity() ? static_cast<IDriver*>(&_ssd2677)
                                         : static_cast<IDriver*>(&_ssd1677);
    }

    _active = selected;
    if (_active->init(bus)) return true;

    // Safety net: a mis-resolution surfaces as a busy timeout inside the
    // wrong child's init and a blank panel. Reset and retry the other child
    // before giving up.
    IDriver* other = (_active == &_ssd2677)
                         ? static_cast<IDriver*>(&_ssd1677)
                         : static_cast<IDriver*>(&_ssd2677);
    clearOperationError();
    hardwareReset(20, 10);
    _active = other;
    return _active->init(bus);
}

bool Driver_Sticky_Auto::probeByBusyPolarity() {
    // True result = SSD2677. After a reset each chip is busy in its own
    // polarity and then settles at its ready level (firmware
    // apply_panel_model_config: SSD2677 busy_level=0 / ready NGFX_GXEPD2_HIGH, SSD1677
    // busy_level=1 / ready NGFX_GXEPD2_LOW), so the post-reset BUSY waveform names the
    // chip: a busy -> ready transition decides by its direction, and a line
    // that is already steady is taken at its ready level once stable.
    if (_busyPin < 0) return false; // nothing to observe: product default
    gfxPinModeInput(_busyPin);
    hardwareReset(20, 10); // firmware probe reset timing

    static const uint32_t kWindowMs = 1500;
    static const int kStableNeeded = 100; // 200 ms at the 2 ms poll below

    bool level = gfxDigitalRead(_busyPin);
    int stable = 1;
    const uint32_t start = millis();
    for (;;) {
        delay(2);
        const bool now = gfxDigitalRead(_busyPin);
        if (now != level) return now; // now is the ready level
        if (++stable >= kStableNeeded) break;
        if (millis() - start >= kWindowMs) break;
    }
    return level;
}

void Driver_Sticky_Auto::setRotation(uint8_t rotation) {
    if (_active) _active->setRotation(rotation);
}

uint8_t Driver_Sticky_Auto::rotation() const {
    return _active ? _active->rotation() : 0;
}

void Driver_Sticky_Auto::invertDisplay(bool invert) {
    if (_active) _active->invertDisplay(invert);
}

void Driver_Sticky_Auto::displayOn() {
    if (_active) _active->displayOn();
}

void Driver_Sticky_Auto::displayOff() {
    if (_active) _active->displayOff();
}

void Driver_Sticky_Auto::setAddrWindow(uint16_t xs, uint16_t ys,
                                       uint16_t xe, uint16_t ye) {
    if (_active) _active->setAddrWindow(xs, ys, xe, ye);
}

void Driver_Sticky_Auto::writePixel(uint16_t color) {
    if (_active) _active->writePixel(color);
}

void Driver_Sticky_Auto::writePixels(const uint16_t* data, size_t len) {
    if (_active) _active->writePixels(data, len);
}

void Driver_Sticky_Auto::writeFill(uint16_t color, size_t len) {
    if (_active) _active->writeFill(color, len);
}

bool Driver_Sticky_Auto::supportsAsyncPixelTransfer() const {
    return _active && _active->supportsAsyncPixelTransfer();
}

bool Driver_Sticky_Auto::enableDMA(bool enable) {
    return _active && _active->enableDMA(enable);
}

bool Driver_Sticky_Auto::writePixelsDMA(const uint16_t* data, size_t len) {
    return _active && _active->writePixelsDMA(data, len);
}

bool Driver_Sticky_Auto::dmaBusy() {
    return _active && _active->dmaBusy();
}

void Driver_Sticky_Auto::sleep() {
    if (_active) _active->sleep();
}

void Driver_Sticky_Auto::wake() {
    if (_active) _active->wake();
}

void Driver_Sticky_Auto::update() {
    if (_active) _active->update();
}

void Driver_Sticky_Auto::updateFast() {
    if (_active) _active->updateFast();
}

void Driver_Sticky_Auto::updatePartial() {
    if (_active) _active->updatePartial();
}

void Driver_Sticky_Auto::wakePartial() {
    if (_active) _active->wakePartial();
}

void Driver_Sticky_Auto::wakeFast() {
    if (_active) _active->wakeFast();
}

void Driver_Sticky_Auto::wakeGray() {
    if (_active) _active->wakeGray();
}

void Driver_Sticky_Auto::updateGray() {
    if (_active) _active->updateGray();
}

void Driver_Sticky_Auto::pushNewColors(const uint8_t* data, size_t len) {
    if (_active) _active->pushNewColors(data, len);
}

void Driver_Sticky_Auto::pushOldColors(const uint8_t* data, size_t len) {
    if (_active) _active->pushOldColors(data, len);
}

void Driver_Sticky_Auto::pushGrayColors(const uint8_t* data, size_t len) {
    if (_active) _active->pushGrayColors(data, len);
}

void Driver_Sticky_Auto::setTemperature(int8_t temperatureC) {
    if (_active) _active->setTemperature(temperatureC);
}

size_t Driver_Sticky_Auto::waveformProfileCount() const {
    return _active ? _active->waveformProfileCount()
                   : IDriver::waveformProfileCount();
}

const EPaperWaveformProfile* Driver_Sticky_Auto::waveformProfileAt(
    size_t index) const {
    return _active ? _active->waveformProfileAt(index)
                   : IDriver::waveformProfileAt(index);
}

bool Driver_Sticky_Auto::selectWaveformProfile(const char* id) {
    return _active ? _active->selectWaveformProfile(id)
                   : IDriver::selectWaveformProfile(id);
}

const EPaperWaveformProfile* Driver_Sticky_Auto::waveformProfile() const {
    return _active ? _active->waveformProfile() : IDriver::waveformProfile();
}

EPaperWaveformResult Driver_Sticky_Auto::waveformProfileResult() const {
    return _active ? _active->waveformProfileResult()
                   : IDriver::waveformProfileResult();
}

IBus& Driver_Sticky_Auto::bus() {
    return *_bus;
}

void Driver_Sticky_Auto::setBusyPin(int pin) {
    // Each child applies its own BUSY polarity inside busyWait(); the
    // wrapper keeps the pin for the BUSY-polarity stage of init().
    _busyPin = static_cast<int8_t>(pin);
    _ssd1677.setBusyPin(pin);
    _ssd2677.setBusyPin(pin);
}

void Driver_Sticky_Auto::setResetPin(int pin) {
    IDriver::setResetPin(pin); // the probe's own hardwareReset()
    _ssd1677.setResetPin(pin);
    _ssd2677.setResetPin(pin);
}

} }
