// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "Driver_UC8179.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_UC8179.cpp
 * @brief  UC8179 ePaper display driver implementation
 *
 * Adapted from TFT_Drivers/UC8179_Defines.h, UC8179_Init.h, UC8179_Rotation.h
 *
 * The UC8179 is a 1-bit (BW) / 2-bit (4-level gray) ePaper driver IC.
 * It supports full update, fast update, partial update, and multi-gray modes.
 *
 * LUT (Look-Up Table) waveform data is used to control the voltage waveforms
 * applied to the ePaper pixels during updates. Two sets of LUT tables are provided:
 * one for normal (1-bit) mode and one for 4-level grayscale mode.
 */

// LUT waveform tables for normal (1-bit) mode

static const uint8_t LUT_VCOM[] = {
    0x26, 0x0F, 0x18, 0x18, 0x14, 0x01,
    0x00, 0x0A, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t LUT_WW[] = {
    0x55, 0x06, 0x0C, 0x17, 0x02, 0x01,
    0x2A, 0x02, 0x1C, 0x02, 0x0D, 0x01,
    0x80, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t LUT_KW[] = {
    0x55, 0x06, 0x0C, 0x17, 0x02, 0x01,
    0x2A, 0x02, 0x1C, 0x02, 0x0D, 0x01,
    0x80, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t LUT_WK[] = {
    0xAA, 0x06, 0x0C, 0x17, 0x02, 0x01,
    0x15, 0x02, 0x1C, 0x02, 0x0D, 0x01,
    0x40, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t LUT_KK[] = {
    0xAA, 0x06, 0x0C, 0x17, 0x02, 0x01,
    0x15, 0x02, 0x1C, 0x02, 0x0D, 0x01,
    0x40, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// CMD_USER table for fast (1-bit) mode
static const uint8_t CMD_USER[] = {
    0x17, 0x3F, 0x3F, 0x09, 0x06, 0x16
};

// LUT waveform tables for 4-level grayscale mode

static const uint8_t LUT_VCOM_GRAY[] = {
    0x00, 0x00, 0x06, 0x08, 0x07, 0x01,
    0x00, 0x06, 0x0A, 0x0B, 0x0A, 0x01,
    0x00, 0x03, 0x03, 0x00, 0x00, 0x03,
    0x00, 0x05, 0x09, 0x06, 0x06, 0x01,
    0x00, 0x02, 0x02, 0x0A, 0x0A, 0x01,
    0x00, 0x0A, 0x11, 0x06, 0x07, 0x01,
    0x00, 0x02, 0x01, 0x02, 0x01, 0x01,
};

static const uint8_t LUT_WW_GRAY[] = {
    0x15, 0x00, 0x06, 0x08, 0x07, 0x01,
    0x54, 0x06, 0x0A, 0x0B, 0x0A, 0x01,
    0x90, 0x03, 0x03, 0x00, 0x00, 0x03,
    0x2A, 0x05, 0x09, 0x06, 0x06, 0x01,
    0xAA, 0x02, 0x02, 0x0A, 0x0A, 0x01,
    0x00, 0x0A, 0x11, 0x06, 0x07, 0x01,
    0x28, 0x02, 0x01, 0x02, 0x01, 0x01,
};

static const uint8_t LUT_KW_GRAY[] = {
    0x2A, 0x00, 0x06, 0x08, 0x07, 0x01,
    0x59, 0x06, 0x0A, 0x0B, 0x0A, 0x01,
    0x90, 0x03, 0x03, 0x00, 0x00, 0x03,
    0x5A, 0x05, 0x09, 0x06, 0x06, 0x01,
    0xA8, 0x02, 0x02, 0x0A, 0x0A, 0x01,
    0x45, 0x0A, 0x11, 0x06, 0x07, 0x01,
    0xA8, 0x02, 0x01, 0x02, 0x01, 0x01,
};

static const uint8_t LUT_WK_GRAY[] = {
    0x16, 0x00, 0x06, 0x08, 0x07, 0x01,
    0xA0, 0x06, 0x0A, 0x0B, 0x0A, 0x01,
    0x90, 0x03, 0x03, 0x00, 0x00, 0x03,
    0x99, 0x05, 0x09, 0x06, 0x06, 0x01,
    0xA0, 0x02, 0x02, 0x0A, 0x0A, 0x01,
    0x40, 0x0A, 0x11, 0x06, 0x07, 0x01,
    0x20, 0x02, 0x01, 0x02, 0x01, 0x01,
};

static const uint8_t LUT_KK_GRAY[] = {
    0x26, 0x00, 0x06, 0x08, 0x07, 0x01,
    0x6A, 0x06, 0x0A, 0x0B, 0x0A, 0x01,
    0x90, 0x03, 0x03, 0x00, 0x00, 0x03,
    0x65, 0x05, 0x09, 0x06, 0x06, 0x01,
    0x50, 0x02, 0x02, 0x0A, 0x0A, 0x01,
    0x10, 0x0A, 0x11, 0x06, 0x07, 0x01,
    0x10, 0x02, 0x01, 0x02, 0x01, 0x01,
};

// CMD_USER table for grayscale mode
static const uint8_t CMD_USER_GRAY[] = {
    0x17, 0x3F, 0x3F, 0x07, 0x06, 0x12
};

// LUT array sizes

#define NGFX_SEEED_UC8179_LUT_SIZE  42   // 7 rows x 6 bytes per LUT table

// Constructor

Driver_UC8179::Driver_UC8179(uint16_t w, uint16_t h)
    : _init_width(w)
    , _init_height(h)
    , _busy_pin(-1)
    , _reset_pin(-1)
    , _use_otp_lut(false)
    , _has_checked_otp(false)
    , _write_plane(NGFX_SEEED_UC8179_DTM2)   // default: write to new data plane
    , _invert_display(false)
    , _partialActive(false)
    , _oldCache(nullptr)
    , _winX0(0), _winY0(0), _winX1(0), _winY1(0)
{
    _width  = w;
    _height = h;
}

// Initialization

bool Driver_UC8179::init(IBus& bus) {
    _bus = &bus;
    // bus is already initialized by Panel_EPaper::begin(), don't call begin() again

    // Configure busy pin if set
    if (_busy_pin >= 0) {
        pinMode(_busy_pin, NGFX_GXEPD2_INPUT);
    }

    // Configure reset pin if set
    if (_reset_pin >= 0) {
        pinMode(_reset_pin, NGFX_GXEPD2_OUTPUT);
        digitalWrite(_reset_pin, NGFX_GXEPD2_HIGH);
    }

    reset();
    if (lastOperationError() != DriverOperationError::None) return false;

    // --- Full init sequence from UC8179_Init.h ---
    // POWER SETTING (PWR)
    _bus->writeCommand(0x01);
    _bus->writeData(0x07);    // VGH=20V, VGL=-20V
    _bus->writeData(0x07);
    _bus->writeData(0x3F);    // VDH=15V
    _bus->writeData(0x3F);    // VDL=-15V

    // Booster Soft Start
    _bus->writeCommand(0x06);
    _bus->writeData(0x17);
    _bus->writeData(0x17);
    _bus->writeData(0x28);
    _bus->writeData(0x17);

    // POWER ON
    _bus->writeCommand(0x04);
    delay(100);

    checkBusy();

    // PANEL SETTING
    _bus->writeCommand(NGFX_SEEED_UC8179_PNLSET);
    _bus->writeData(0x1F);    // KW-3f, KWR-2F, BWROTP-0f, BWOTP-1f

    // Resolution
    _bus->writeCommand(NGFX_SEEED_UC8179_TRES);
    _bus->writeData(_init_width >> 8);
    _bus->writeData(_init_width & 0xFF);
    _bus->writeData(_init_height >> 8);
    _bus->writeData(_init_height & 0xFF);

    // Temperature sensor
    _bus->writeCommand(0x15);
    _bus->writeData(0x00);

    // VCOM and data interval
    _bus->writeCommand(NGFX_SEEED_UC8179_VDCS);
    _bus->writeData(0x10);
    _bus->writeData(0x07);

    // TCON setting
    _bus->writeCommand(NGFX_SEEED_UC8179_TCONSET);
    _bus->writeData(0x22);

    setRotation(0);
    return lastOperationError() == DriverOperationError::None;
}

// Rotation

void Driver_UC8179::setRotation(uint8_t m) {
    _rotation = m % 4;

    _bus->writeCommand(NGFX_SEEED_UC8179_PNLSET);

    switch (_rotation) {
        case 0: // Portrait
            _bus->writeData(NGFX_SEEED_UC8179_PNLSET_NORMAL);   // 0x1F
            _width  = _init_width;
            _height = _init_height;
            break;
        case 1: // Landscape (Portrait + 90)
            _bus->writeData(NGFX_SEEED_UC8179_PNLSET_LANDSCAPE); // 0x1B
            _width  = _init_height;
            _height = _init_width;
            break;
        case 2: // Inverted portrait
            _bus->writeData(NGFX_SEEED_UC8179_PNLSET_INVERTED);  // 0x13
            _width  = _init_width;
            _height = _init_height;
            break;
        case 3: // Inverted landscape
            _bus->writeData(NGFX_SEEED_UC8179_PNLSET_INVLAND);   // 0x17
            _width  = _init_height;
            _height = _init_width;
            break;
    }
}

// Display control

void Driver_UC8179::invertDisplay(bool invert) {
    // User-requested black/white swap. The framebuffer now stores 1 = white,
    // matching this glass, so no product config requests it anymore. The flag
    // remains as an explicit escape hatch; streamData() inverts the 1-bit
    // data plane only while it is set.
    _invert_display = invert;
}

void Driver_UC8179::displayOn() {
    _bus->writeCommand(NGFX_SEEED_UC8179_POWERON);
    delay(100);
    checkBusy();
}

void Driver_UC8179::displayOff() {
    _bus->writeCommand(NGFX_SEEED_UC8179_POWEROFF);
    delay(100);
    checkBusy();
}

// Address window (partial update)

void Driver_UC8179::setAddrWindow(uint16_t xs, uint16_t ys,
                                   uint16_t xe, uint16_t ye) {
    // VCOM and data interval for partial update
    _bus->writeCommand(NGFX_SEEED_UC8179_VDCS);
    _bus->writeData(0xA9);
    _bus->writeData(0x07);

    // Partial in
    _bus->writeCommand(NGFX_SEEED_UC8179_PTLIN);

    // Partial window
    _bus->writeCommand(NGFX_SEEED_UC8179_PTLW);
    _bus->writeData(xs >> 8);
    _bus->writeData(xs & 0xFF);
    _bus->writeData(xe >> 8);
    _bus->writeData(xe & 0xFF);
    _bus->writeData(ys >> 8);
    _bus->writeData(ys & 0xFF);
    _bus->writeData(ye >> 8);
    _bus->writeData(ye & 0xFF);
    _bus->writeData(0x01);   // scan mode

    // Remember the window so the partial-refresh old-plane push can read the
    // matching bytes out of _oldCache.
    _winX0 = xs; _winY0 = ys; _winX1 = xe; _winY1 = ye;
}

// Pixel writing

void Driver_UC8179::writePixel(uint16_t color) {
    // For ePaper, write the low byte to the current data plane
    _bus->writeData((uint8_t)(color & 0xFF));
}

void Driver_UC8179::writePixels(const uint16_t* data, size_t len) {
    // For ePaper, write raw bytes to the current data plane
    for (size_t i = 0; i < len; i++) {
        _bus->writeData((uint8_t)(data[i] & 0xFF));
    }
}

void Driver_UC8179::writeFill(uint16_t color, size_t len) {
    uint8_t c = (uint8_t)(color & 0xFF);
    for (size_t i = 0; i < len; i++) {
        _bus->writeData(c);
    }
}

// Power management

void Driver_UC8179::sleep() {
    // Deep sleep sequence
    _bus->writeCommand(0x50);
    _bus->writeData(0xF7);    // VCOM and data interval setting
    _bus->writeCommand(0x02); // Power off
    checkBusy();
    _bus->writeCommand(NGFX_SEEED_UC8179_SLPIN);
    _bus->writeData(0xA5);    // Deep sleep command
}

void Driver_UC8179::wake() {
    // The original Seeed_GFX ordinary refresh path used EPD_INIT_FAST for
    // UC8179. Restore that factory-provided LUT only on the verified
    // 800x480 glass (including reTerminal E1001). The 648x480 variant stays
    // on its conservative full/OTP path because no matching external LUT has
    // been verified for that panel.
    if (supportsFastRefresh()) {
        wakeupFast();
    } else {
        wakeupFull();
    }
}

// ePaper update methods

void Driver_UC8179::update() {
    _bus->writeCommand(NGFX_SEEED_UC8179_DISPLAYREFRESH);
    delay(1);
    checkBusy();
}

void Driver_UC8179::updateGray() {
    update();
}

void Driver_UC8179::updatePartial() {
    _partialActive = false;
    update();
}

// Init sequences

void Driver_UC8179::initFull() {
    // Driver output control
    _bus->writeCommand(0x01);
    _bus->writeData(0x07);
    _bus->writeData(0x07);
    _bus->writeData(0x3F);
    _bus->writeData(0x3F);

    // Booster soft start
    _bus->writeCommand(0x06);
    _bus->writeData(0x17);
    _bus->writeData(0x17);
    _bus->writeData(0x28);
    _bus->writeData(0x17);

    // Power on
    _bus->writeCommand(0x04);
    delay(100);
    checkBusy();

    // Panel setting
    _bus->writeCommand(NGFX_SEEED_UC8179_PNLSET);
    _bus->writeData(0x1F);

    // Resolution
    _bus->writeCommand(NGFX_SEEED_UC8179_TRES);
    _bus->writeData(_init_width >> 8);
    _bus->writeData(_init_width & 0xFF);
    _bus->writeData(_init_height >> 8);
    _bus->writeData(_init_height & 0xFF);

    // VCOM and data interval
    _bus->writeCommand(NGFX_SEEED_UC8179_VDCS);
    _bus->writeData(0x10);
    _bus->writeData(0x07);
}

void Driver_UC8179::initFast() {
    if (!supportsFastRefresh()) {
        initFull();
        return;
    }
    // Driver output control
    _bus->writeCommand(0x01);
    _bus->writeData(0x07);
    _bus->writeData(CMD_USER[0]);   // VGH
    _bus->writeData(CMD_USER[1]);   // VSH1
    _bus->writeData(CMD_USER[2]);   // VSH2
    _bus->writeData(CMD_USER[3]);   // VSL

    // PLL control
    _bus->writeCommand(NGFX_SEEED_UC8179_PLL);
    _bus->writeData(CMD_USER[4]);   // frame frequency

    // VCM DC setting
    _bus->writeCommand(NGFX_SEEED_UC8179_VCMDC);
    _bus->writeData(CMD_USER[5]);   // VCOM offset

    // Booster soft start
    _bus->writeCommand(0x06);
    _bus->writeData(0x17);
    _bus->writeData(0x17);
    _bus->writeData(0x28);
    _bus->writeData(0x17);

    // Power on
    _bus->writeCommand(0x04);
    delay(100);
    checkBusy();

    // Panel setting
    _bus->writeCommand(NGFX_SEEED_UC8179_PNLSET);
    _bus->writeData(0x3F);

    // Resolution
    _bus->writeCommand(NGFX_SEEED_UC8179_TRES);
    _bus->writeData(_init_width >> 8);
    _bus->writeData(_init_width & 0xFF);
    _bus->writeData(_init_height >> 8);
    _bus->writeData(_init_height & 0xFF);

    // VCOM and data interval
    _bus->writeCommand(NGFX_SEEED_UC8179_VDCS);
    _bus->writeData(0x10);
    _bus->writeData(0x07);

    // Write the normal LUT tables
    writeLUT();
}

void Driver_UC8179::initGray() {
    if (!supportsGrayRefresh(4)) {
        initFull();
        return;
    }
    // Check OTP support if not yet probed
    if (!_has_checked_otp) {
        probeOtpSupport();
    }

    if (_use_otp_lut) {
        initGrayOTP();
        return;
    }

    // Driver output control
    _bus->writeCommand(0x01);
    _bus->writeData(0x07);
    _bus->writeData(CMD_USER_GRAY[0]);   // VGH
    _bus->writeData(CMD_USER_GRAY[1]);   // VSH1
    _bus->writeData(CMD_USER_GRAY[2]);   // VSH2
    _bus->writeData(CMD_USER_GRAY[3]);   // VSL

    // PLL control
    _bus->writeCommand(NGFX_SEEED_UC8179_PLL);
    _bus->writeData(CMD_USER_GRAY[4]);   // frame frequency

    // VCM DC setting
    _bus->writeCommand(NGFX_SEEED_UC8179_VCMDC);
    _bus->writeData(CMD_USER_GRAY[5]);   // VCOM offset

    // Booster soft start
    _bus->writeCommand(0x06);
    _bus->writeData(0x27);
    _bus->writeData(0x27);
    _bus->writeData(0x28);
    _bus->writeData(0x17);

    // Power on
    _bus->writeCommand(0x04);
    delay(100);
    checkBusy();

    // Panel setting
    _bus->writeCommand(NGFX_SEEED_UC8179_PNLSET);
    _bus->writeData(0x3F);

    // Gate line delay
    _bus->writeCommand(NGFX_SEEED_UC8179_GLD);
    _bus->writeData(0x88);

    // VCOM and data interval
    _bus->writeCommand(NGFX_SEEED_UC8179_VDCS);
    _bus->writeData(0x10);
    _bus->writeData(0x07);

    // Gate scan start
    _bus->writeCommand(NGFX_SEEED_UC8179_GSST);
    _bus->writeData(0x00);

    // Resolution
    _bus->writeCommand(NGFX_SEEED_UC8179_TRES);
    _bus->writeData(_init_width >> 8);
    _bus->writeData(_init_width & 0xFF);
    _bus->writeData(_init_height >> 8);
    _bus->writeData(_init_height & 0xFF);

    // Write the grayscale LUT tables
    writeLUTGray();
}

void Driver_UC8179::initPartial() {
    if (!supportsPartialRefresh()) {
        initFull();
        return;
    }
    // Panel setting
    _bus->writeCommand(NGFX_SEEED_UC8179_PNLSET);
    _bus->writeData(0x1F);

    // Power on
    _bus->writeCommand(0x04);
    delay(100);
    checkBusy();

    // Border waveform control
    _bus->writeCommand(NGFX_SEEED_UC8179_REV);
    _bus->writeData(0x02);

    // Border waveform control extended
    _bus->writeCommand(NGFX_SEEED_UC8179_REVE);
    _bus->writeData(0x6E);
}

// Wakeup sequences (reset + init)

void Driver_UC8179::wakeupFull() {
    _partialActive = false;
    reset();
    initFull();
}

void Driver_UC8179::wakeupFast() {
    _partialActive = false;
    reset();
    initFast();
}

void Driver_UC8179::wakeGray() {
    _partialActive = false;
    reset();
    initGray();
}

void Driver_UC8179::wakeupPartial() {
    _partialActive = false;
    reset();
    initPartial();
    // Allocate the old-frame baseline on the first partial refresh; a full
    // refresh before it already populated _oldCache via pushNewColors().
    ensureOldCache();
    _partialActive = true;
}

// Color data push methods

void Driver_UC8179::pushNewColors(const uint8_t* colors, size_t len) {
    if (_partialActive) {
        // Partial session: the differential waveform needs the real "old"
        // window in DTM1 as well as the new window in DTM2. A full refresh
        // pushes both planes; pushing new-only here (as the original library
        // did) ghosts once the controller RAM was lost to deep sleep.
        pushOldWindow();
        _bus->writeCommand(NGFX_SEEED_UC8179_DTM2);   // New data (0x13)
        streamData(colors, len);
        patchOldCache(colors, len);
        return;
    }
    _bus->writeCommand(NGFX_SEEED_UC8179_DTM2);   // New data (0x13)
    streamData(colors, len);
    // A full-frame push becomes the new "previous frame" for the next partial
    // refresh (the glass now shows this content after the update completes).
    if (colors && len >= frameBytes()) {
        ensureOldCache();
        if (_oldCache) memcpy(_oldCache, colors, frameBytes());
    }
}

void Driver_UC8179::pushOldColors(const uint8_t* colors, size_t len) {
    _bus->writeCommand(NGFX_SEEED_UC8179_DTM1);   // Old data (0x10)
    streamData(colors, len);
}

void Driver_UC8179::streamData(const uint8_t* data, size_t len) {
    if (!data || len == 0) return;
    if (_invert_display) {
        for (size_t i = 0; i < len; i++) {
            _bus->writeData(static_cast<uint8_t>(~data[i]));
        }
    } else {
        _bus->writeData(data, len);
    }
}

void Driver_UC8179::ensureOldCache() {
    if (_oldCache) return;
    _oldCache = static_cast<uint8_t*>(protocolAllocate(frameBytes()));
    if (_oldCache) {
        // Fresh baseline is all-white (0xFF in the framebuffer), matching
        // Panel_EPaper's own first-refresh previous-frame assumption.
        memset(_oldCache, 0xFF, frameBytes());
    }
}

void Driver_UC8179::pushOldWindow() {
    ensureOldCache();
    // If the cache could not be allocated there is no trustworthy old plane;
    // fall through to a new-only push (identical to the original behavior).
    if (!_oldCache) return;
    const uint16_t w_bytes = static_cast<uint16_t>(_winX1 - _winX0 + 1U) / 8U;
    const uint16_t h      = static_cast<uint16_t>(_winY1 - _winY0 + 1U);
    const uint16_t stride = _init_width / 8;
    _bus->writeCommand(NGFX_SEEED_UC8179_DTM1);   // Old data (0x10)
    for (uint16_t row = 0; row < h; ++row) {
        streamData(_oldCache + static_cast<size_t>(_winY0 + row) * stride +
                       (_winX0 / 8U),
                   w_bytes);
    }
}

void Driver_UC8179::patchOldCache(const uint8_t* data, size_t len) {
    if (!data || !_oldCache) return;
    const uint16_t w_bytes = static_cast<uint16_t>(_winX1 - _winX0 + 1U) / 8U;
    const uint16_t h      = static_cast<uint16_t>(_winY1 - _winY0 + 1U);
    if (len < static_cast<size_t>(w_bytes) * h) return;
    const uint16_t stride = _init_width / 8;
    for (uint16_t row = 0; row < h; ++row) {
        memcpy(_oldCache + static_cast<size_t>(_winY0 + row) * stride +
                   (_winX0 / 8U),
               data + static_cast<size_t>(row) * w_bytes, w_bytes);
    }
}

void Driver_UC8179::pushNewColorsFlip(const uint8_t* colors, size_t len) {
    // Calculate bytes per row for the current display dimensions
    uint16_t bytes_per_row = _init_width / 8;
    uint16_t num_rows = len / bytes_per_row;

    _bus->writeCommand(NGFX_SEEED_UC8179_DTM2);   // New data (0x13)

    for (uint16_t row = 0; row < num_rows; row++) {
        uint16_t start = row * bytes_per_row;
        for (uint16_t col = 0; col < bytes_per_row; col++) {
            uint8_t b = colors[start + (bytes_per_row - 1 - col)];
            // Reverse the bits within the byte
            b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
            b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
            b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
            _bus->writeData(b);
        }
    }
}

void Driver_UC8179::pushOldColorsFlip(const uint8_t* colors, size_t len) {
    uint16_t bytes_per_row = _init_width / 8;
    uint16_t num_rows = len / bytes_per_row;

    _bus->writeCommand(NGFX_SEEED_UC8179_DTM1);   // Old data (0x10)

    for (uint16_t row = 0; row < num_rows; row++) {
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

void Driver_UC8179::pushGrayColors(const uint8_t* colors, size_t len) {
    // Each iteration processes 4 input bytes = 8 pixels, producing 1 output byte.
    // Total iterations = (width * height) / 8
    // Input buffer size = width * height / 2 bytes (each byte holds 2 x 2-bit pixels)
    uint32_t total_pixels = (uint32_t)_init_width * _init_height;
    const size_t required = (size_t)((total_pixels + 1U) / 2U);
    // The conversion below consumes complete groups of eight pixels.  All
    // currently supported UC8179 panels have a width divisible by eight; do
    // not read past an undersized or incompatible application buffer.
    if (!colors || len < required || (total_pixels & 7U) != 0U) return;
    uint32_t iterations = total_pixels / 8;

    uint8_t temp1, temp2, temp3;
    uint32_t i;
    uint8_t j, k;

    // --- Old data plane (0x10) ---
    _bus->writeCommand(NGFX_SEEED_UC8179_DTM1);   // Old data

    for (i = 0; i < iterations; i++) {
        // Read 4 input bytes = 8 pixels (each byte encodes 2 pixels at 2 bits each)
        uint8_t c0 = colors[i * 4 + 0];
        uint8_t c1 = colors[i * 4 + 1];
        uint8_t c2 = colors[i * 4 + 2];
        uint8_t c3 = colors[i * 4 + 3];

        // Extract 8 pixels from bit5-4 and bit1-0 of each byte
        uint8_t p0 = (c0 >> 4) & 0x03;
        uint8_t p1 = (c0 >> 0) & 0x03;
        uint8_t p2 = (c1 >> 4) & 0x03;
        uint8_t p3 = (c1 >> 0) & 0x03;
        uint8_t p4 = (c2 >> 4) & 0x03;
        uint8_t p5 = (c2 >> 0) & 0x03;
        uint8_t p6 = (c3 >> 4) & 0x03;
        uint8_t p7 = (c3 >> 0) & 0x03;

        // Pack into two bytes (4 pixels per byte, 2 bits per pixel)
        uint8_t packed_byte0 = (p0 << 6) | (p1 << 4) | (p2 << 2) | p3;
        uint8_t packed_byte1 = (p4 << 6) | (p5 << 4) | (p6 << 2) | p7;

        temp3 = 0;
        for (j = 0; j < 2; j++) {
            temp1 = (j == 0) ? packed_byte0 : packed_byte1;
            for (k = 0; k < 4; k++) {
                temp2 = temp1 & 0xC0;
                if (temp2 == 0xC0)
                    temp3 |= 0x01;       // 11 -> 1
                else if (temp2 == 0x00)
                    temp3 |= 0x00;       // 00 -> 0
                else if ((temp2 >= 0x80) && (temp2 < 0xC0))
                    temp3 |= 0x00;       // 10 -> 0
                else if (temp2 == 0x40)
                    temp3 |= 0x01;       // 01 -> 1

                if ((j == 0 && k <= 3) || (j == 1 && k <= 2)) {
                    temp3 <<= 1;
                    temp1 <<= 2;
                }
            }
        }
        _bus->writeData(temp3);
    }

    // --- New data plane (0x13) ---
    _bus->writeCommand(NGFX_SEEED_UC8179_DTM2);   // New data

    for (i = 0; i < iterations; i++) {
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
        for (j = 0; j < 2; j++) {
            temp1 = (j == 0) ? packed_byte0 : packed_byte1;
            for (k = 0; k < 4; k++) {
                temp2 = temp1 & 0xC0;
                if (temp2 == 0xC0)
                    temp3 |= 0x01;       // 11 -> 1
                else if (temp2 == 0x00)
                    temp3 |= 0x00;       // 00 -> 0
                else if ((temp2 >= 0x80) && (temp2 < 0xC0))
                    temp3 |= 0x01;       // 10 -> 1
                else if (temp2 == 0x40)
                    temp3 |= 0x00;       // 01 -> 0

                if ((j == 0 && k <= 3) || (j == 1 && k <= 2)) {
                    temp3 <<= 1;
                    temp1 <<= 2;
                }
            }
        }
        _bus->writeData(temp3);
    }
}

void Driver_UC8179::pushNewGrayColorsFlip(const uint8_t* colors, size_t len) {
    // Re-initialize for grayscale mode, then push flipped monochrome data
    initGray();

    uint16_t bytes_per_row = _init_width / 8;
    uint16_t num_rows = len / bytes_per_row;

    _bus->writeCommand(NGFX_SEEED_UC8179_DTM2);   // New data (0x13)

    for (uint16_t row = 0; row < num_rows; row++) {
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

// Internal helpers

void Driver_UC8179::checkBusy() {
    (void)waitForReadyPin(_busy_pin, true, 30000, 10);
}

void Driver_UC8179::reset() {
    if (_reset_pin < 0) {
        return;
    }
    // Use the cross-platform GPIO adapter; ESP32 keeps raw GPIO semantics.
    gfxDigitalWrite(_reset_pin, false);
    delay(10);
    gfxDigitalWrite(_reset_pin, true);
    delay(10);
    checkBusy();
}

void Driver_UC8179::writeLUT() {
    unsigned int i;

    // VCOM LUT
    checkBusy();
    _bus->writeCommand(NGFX_SEEED_UC8179_LUT_VCOM);
    for (i = 0; i < NGFX_SEEED_UC8179_LUT_SIZE; i++) {
        _bus->writeData(LUT_VCOM[i]);
    }

    // White-to-White LUT
    checkBusy();
    _bus->writeCommand(NGFX_SEEED_UC8179_LUT_WW);
    for (i = 0; i < NGFX_SEEED_UC8179_LUT_SIZE; i++) {
        _bus->writeData(LUT_WW[i]);
    }

    // Black-to-White LUT
    checkBusy();
    _bus->writeCommand(NGFX_SEEED_UC8179_LUT_KW);
    for (i = 0; i < NGFX_SEEED_UC8179_LUT_SIZE; i++) {
        _bus->writeData(LUT_KW[i]);
    }

    // White-to-Black LUT
    checkBusy();
    _bus->writeCommand(NGFX_SEEED_UC8179_LUT_WK);
    for (i = 0; i < NGFX_SEEED_UC8179_LUT_SIZE; i++) {
        _bus->writeData(LUT_WK[i]);
    }

    // Black-to-Black LUT
    _bus->writeCommand(NGFX_SEEED_UC8179_LUT_KK);
    for (i = 0; i < NGFX_SEEED_UC8179_LUT_SIZE; i++) {
        _bus->writeData(LUT_KK[i]);
    }
}

void Driver_UC8179::writeLUTGray() {
    unsigned int i;

    // VCOM LUT
    checkBusy();
    _bus->writeCommand(NGFX_SEEED_UC8179_LUT_VCOM);
    for (i = 0; i < NGFX_SEEED_UC8179_LUT_SIZE; i++) {
        _bus->writeData(LUT_VCOM_GRAY[i]);
    }

    // White-to-White LUT
    checkBusy();
    _bus->writeCommand(NGFX_SEEED_UC8179_LUT_WW);
    for (i = 0; i < NGFX_SEEED_UC8179_LUT_SIZE; i++) {
        _bus->writeData(LUT_WW_GRAY[i]);
    }

    // Black-to-White LUT
    checkBusy();
    _bus->writeCommand(NGFX_SEEED_UC8179_LUT_KW);
    for (i = 0; i < NGFX_SEEED_UC8179_LUT_SIZE; i++) {
        _bus->writeData(LUT_KW_GRAY[i]);
    }

    // White-to-Black LUT
    checkBusy();
    _bus->writeCommand(NGFX_SEEED_UC8179_LUT_WK);
    for (i = 0; i < NGFX_SEEED_UC8179_LUT_SIZE; i++) {
        _bus->writeData(LUT_WK_GRAY[i]);
    }

    // Black-to-Black LUT
    _bus->writeCommand(NGFX_SEEED_UC8179_LUT_KK);
    for (i = 0; i < NGFX_SEEED_UC8179_LUT_SIZE; i++) {
        _bus->writeData(LUT_KK_GRAY[i]);
    }
}

void Driver_UC8179::probeOtpSupport() {
    // Default: do not use internal OTP. The user can enable it
    // via useInternalOTP(true) if their panel supports it.
    // In the original code, this function would probe a register
    // to determine OTP capability. Since the probe mechanism is
    // panel-specific, we default to external LUT.
    _has_checked_otp = true;
    // _use_otp_lut remains as set by the user
}

void Driver_UC8179::initGrayOTP() {
    // Initialize using internal OTP LUT for grayscale mode
    _bus->writeCommand(0x01);
    _bus->writeData(0x07);
    _bus->writeData(0x07);
    _bus->writeData(0x3F);
    _bus->writeData(0x3F);

    _bus->writeCommand(0x06);
    _bus->writeData(0x27);
    _bus->writeData(0x27);
    _bus->writeData(0x18);
    _bus->writeData(0x17);

    _bus->writeCommand(0x04);
    delay(100);
    checkBusy();

    _bus->writeCommand(NGFX_SEEED_UC8179_PNLSET);
    _bus->writeData(0x1F);

    _bus->writeCommand(NGFX_SEEED_UC8179_TRES);
    _bus->writeData(_init_width >> 8);
    _bus->writeData(_init_width & 0xFF);
    _bus->writeData(_init_height >> 8);
    _bus->writeData(_init_height & 0xFF);

    _bus->writeCommand(NGFX_SEEED_UC8179_VDCS);
    _bus->writeData(0x10);
    _bus->writeData(0x07);

    _bus->writeCommand(NGFX_SEEED_UC8179_REV);
    _bus->writeData(0x02);

    _bus->writeCommand(NGFX_SEEED_UC8179_REVE);
    _bus->writeData(0x5F);
}

} }
