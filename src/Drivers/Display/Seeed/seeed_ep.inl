// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "seeed_ep.h"
#include "Bus.h"
#include "SeeedHal.h"
#include <string.h>
namespace nice_gfx { namespace seeed_protocol {



namespace {
bool sameString(const char* first, const char* second) {
    return first && second && strcmp(first, second) == 0;
}

bool appendTemplateValue(uint8_t encoded, uint16_t width, uint16_t height,
                         uint8_t& value) {
    const EPaperWaveformToken::TemplateValue type =
        static_cast<EPaperWaveformToken::TemplateValue>(encoded);
    switch (type) {
        case EPaperWaveformToken::TemplateValue::WidthHigh: value = static_cast<uint8_t>(width >> 8); return true;
        case EPaperWaveformToken::TemplateValue::WidthLow: value = static_cast<uint8_t>(width); return true;
        case EPaperWaveformToken::TemplateValue::HeightHigh: value = static_cast<uint8_t>(height >> 8); return true;
        case EPaperWaveformToken::TemplateValue::HeightLow: value = static_cast<uint8_t>(height); return true;
        case EPaperWaveformToken::TemplateValue::WidthMinusOneHigh:
            value = static_cast<uint8_t>((width - 1U) >> 8); return true;
        case EPaperWaveformToken::TemplateValue::WidthMinusOneLow:
            value = static_cast<uint8_t>(width - 1U); return true;
        case EPaperWaveformToken::TemplateValue::HeightMinusOneHigh:
            value = static_cast<uint8_t>((height - 1U) >> 8); return true;
        case EPaperWaveformToken::TemplateValue::HeightMinusOneLow:
            value = static_cast<uint8_t>(height - 1U); return true;
        case EPaperWaveformToken::TemplateValue::WidthBytesMinusOne:
            value = static_cast<uint8_t>((width / 8U) - 1U); return width >= 8U;
        case EPaperWaveformToken::TemplateValue::Literal:
            return false;
    }
    return false;
}

bool validateTemplateRecord(const uint8_t* bytes, size_t size, size_t& offset) {
    if (offset + 2 > size) return false;
    ++offset; // command
    const uint8_t dataCount = bytes[offset++];
    for (uint8_t index = 0; index < dataCount; ++index) {
        if (offset >= size) return false;
        const uint8_t kind = bytes[offset++];
        if (kind == static_cast<uint8_t>(EPaperWaveformToken::TemplateValue::Literal)) {
            if (offset >= size) return false;
            ++offset;
        } else if (kind > static_cast<uint8_t>(EPaperWaveformToken::TemplateValue::WidthBytesMinusOne)) {
            return false;
        }
    }
    return true;
}

// Central panel command definitions. Records are [size, command, parameters].
// Add verified manufacturer/batch profiles in the relevant controller section.
const uint8_t kJD79660DefaultFull[] = {
    2, 0x4D, 0x78,                                      // FITI
    3, 0x00, 0x0F, 0x29,                                // PSR
    8, 0x06, 0x0D, 0x12, 0x30, 0x20, 0x19, 0x2A, 0x22, // BTST_P
    2, 0x50, 0x37,                                      // CDI
    EPaperWaveformToken::Resolution, 0x61,               // TRES
    2, 0xE9, 0x01,
    2, 0x30, 0x08,
    1, 0x04,                                            // Power on
    EPaperWaveformToken::WaitBusy,
    EPaperWaveformToken::End,
};

const EPaperCommandSequence kJD79660DefaultFullSequence = {
    kJD79660DefaultFull, sizeof(kJD79660DefaultFull)
};

const uint8_t kJD79667DefaultFull[] = {
    2, 0x4D, 0x78, 3, 0x00, 0x0F, 0x29, 3, 0x01, 0x07, 0x00,
    4, 0x03, 0x10, 0x54, 0x44, 8, 0x06, 0x05, 0x00, 0x3F, 0x0A, 0x25, 0x12, 0x1A,
    2, 0x50, 0x37, 3, 0x60, 0x02, 0x02, EPaperWaveformToken::Resolution, 0x61,
    2, 0xE7, 0x1C, 2, 0xE3, 0x22, 2, 0xB4, 0xD0, 2, 0xB5, 0x03,
    2, 0xE9, 0x01, 2, 0x30, 0x08, 1, 0x04,
    EPaperWaveformToken::WaitBusy, EPaperWaveformToken::End,
};
const EPaperCommandSequence kJD79667DefaultFullSequence = {
    kJD79667DefaultFull, sizeof(kJD79667DefaultFull)
};

const uint8_t kJD79676DefaultFull[] = {
    2, 0x4D, 0x78, 3, 0x00, 0x0F, 0x29, 2, 0x01, 0x07,
    4, 0x03, 0x10, 0x54, 0x44, 8, 0x06, 0x0F, 0x0A, 0x2F, 0x25, 0x22, 0x2E, 0x21,
    2, 0x50, 0x37, 3, 0x60, 0x02, 0x02, EPaperWaveformToken::Resolution, 0x61,
    2, 0xE7, 0x1C, 2, 0xE3, 0x22, 2, 0xB4, 0xD0, 2, 0xB5, 0x03,
    2, 0xE9, 0x01, 2, 0x30, 0x08, 1, 0x04,
    EPaperWaveformToken::WaitBusy, EPaperWaveformToken::End,
};
const EPaperCommandSequence kJD79676DefaultFullSequence = {
    kJD79676DefaultFull, sizeof(kJD79676DefaultFull)
};

const uint8_t kJD79686BDefaultFull[] = {
    // Exact JD79686B_Init.h power-on sequence. These controller-specific
    // registers must run after every hardware reset, including deep-sleep
    // wake-up; keeping only PSR (0x00) leaves the panel power state undefined.
    2, 0x4D, 0x55, 2, 0xA6, 0x38, 2, 0xB4, 0x5D, 2, 0xB6, 0x80,
    2, 0xB7, 0x00, 2, 0xF7, 0x02, 3, 0x00, 0xF7, 0x0D,
    EPaperWaveformToken::End,
};
const EPaperCommandSequence kJD79686BDefaultFullSequence = {
    kJD79686BDefaultFull, sizeof(kJD79686BDefaultFull)
};

const uint8_t kSSD2677DefaultFull[] = {
    // Aligned byte-for-byte with the Sticky product firmware
    // (seeed_epaper/driver/ssd2677.c, ssd2677_init_otp). Deviations from the
    // legacy Seeed_GFX sequence: PSR {0x2F,0x0E} (was 0x2B,0x29), CDI 0x77
    // (was 0x37), 0xE7=0xC1 (was 0xA4), and RES hard-coded to 800x680.
    // The panel scans 680 gate lines even though only 800x480 are visible;
    // writing RES=800x480 here shifts the image up and garbles the bottom.
    // No PON (0x04) here: the firmware powers the panel inside each refresh
    // (Driver_SSD2677::update), not in init. WaitBusy mirrors the firmware's
    // wait_ready() calls (after reset, after PSR, and at the end of init).
    3, 0x00, 0x2F, 0x0E,
    EPaperWaveformToken::WaitBusy,
    5, 0x06, 0x0F, 0x8B, 0x93, 0xC1,
    2, 0xE7, 0xC1, 2, 0x30, 0x08, 2, 0x50, 0x77,
    9, 0x62, 0x76, 0x76, 0x76, 0x5A, 0x9D, 0x8A, 0x76, 0x62,
    5, 0x61, 0x03, 0x20, 0x02, 0xA8,
    2, 0xE0, 0x10, 5, 0x65, 0x00, 0x00, 0x00, 0x00,
    2, 0xE9, 0x01,
    EPaperWaveformToken::WaitBusy,
    EPaperWaveformToken::End,
};
const EPaperCommandSequence kSSD2677DefaultFullSequence = {
    kSSD2677DefaultFull, sizeof(kSSD2677DefaultFull)
};

// GDEP073E01 (800x480), retained from the original Seeed_GFX driver.
const uint8_t kED2208GDEP073E01Full[] = {
    // Each record length includes the command byte. CMDH (0xAA) takes six
    // data bytes, so its record is 7 bytes — not 8. An 8 here consumes the
    // next record's length (0x07) as command data, corrupts the sequence,
    // then leaves a sticky CommunicationFailed result in Driver_ED2208::init.
    7, 0xAA, 0x49, 0x55, 0x20, 0x08, 0x09, 0x18,
    7, 0x01, 0x3F, 0x00, 0x32, 0x2A, 0x0E, 0x2A,
    3, 0x00, 0x5F, 0x69, 5, 0x03, 0x00, 0x54, 0x00, 0x44,
    5, 0x05, 0x40, 0x1F, 0x1F, 0x2C, 5, 0x06, 0x6F, 0x1F, 0x16, 0x25,
    5, 0x08, 0x6F, 0x1F, 0x1F, 0x22, 3, 0x13, 0x00, 0x04,
    2, 0x30, 0x02, 2, 0x41, 0x00, 2, 0x50, 0x3F, 3, 0x60, 0x02, 0x00,
    EPaperWaveformToken::Resolution, 0x61, 2, 0x82, 0x1E, 2, 0x84, 0x01,
    2, 0x86, 0x00, 2, 0xE3, 0x2F, 2, 0xE0, 0x00, 2, 0xE6, 0x00,
    // Preserve ED2208 CHECK_BUSY's 10 ms first-sample guard after PON.
    1, 0x04, EPaperWaveformToken::DelayMs, 10, 0,
    EPaperWaveformToken::WaitBusy, EPaperWaveformToken::End,
};
const EPaperCommandSequence kED2208GDEP073E01FullSequence = {
    kED2208GDEP073E01Full, sizeof(kED2208GDEP073E01Full)
};

// GDEP040E01 (400x600 native orientation), independently transcribed from
// Good Display's A32-GDEP040E01 reference package. Power-on and the second
// BTST2 setting belong immediately before DRF and are emitted by update().
const uint8_t kED2208GDEP040E01Full[] = {
    7, 0xAA, 0x49, 0x55, 0x20, 0x08, 0x09, 0x18,
    2, 0x01, 0x3F,
    3, 0x00, 0x5F, 0x69,
    5, 0x05, 0x40, 0x1F, 0x1F, 0x2C,
    5, 0x08, 0x6F, 0x1F, 0x1F, 0x22,
    5, 0x06, 0x6F, 0x1F, 0x17, 0x17,
    5, 0x03, 0x00, 0x54, 0x00, 0x44,
    3, 0x60, 0x02, 0x00,
    2, 0x30, 0x08,
    2, 0x50, 0x3F,
    EPaperWaveformToken::Resolution, 0x61,
    2, 0xE3, 0x2F,
    2, 0x84, 0x01,
    EPaperWaveformToken::End,
};
const EPaperCommandSequence kED2208GDEP040E01FullSequence = {
    kED2208GDEP040E01Full, sizeof(kED2208GDEP040E01Full)
};

// SSD16xx templates preserve the original runtime panel dimensions.
const uint8_t kSSD1680DefaultFull[] = {
    2, 0x3C, 0x05,
    EPaperWaveformToken::TemplateCommand, 0x01, 3,
        8, 7, 0, 0x00,
    2, 0x11, 0x03,
    EPaperWaveformToken::TemplateCommand, 0x44, 2,
        0, 0x00, 9,
    EPaperWaveformToken::TemplateCommand, 0x45, 4,
        0, 0x00, 0, 0x00, 8, 7,
    2, 0x18, 0x80, 2, 0x4E, 0x00, 3, 0x4F, 0x00, 0x00,
    EPaperWaveformToken::End,
};
const EPaperCommandSequence kSSD1680DefaultFullSequence = {kSSD1680DefaultFull, sizeof(kSSD1680DefaultFull)};
const uint8_t kSSD1680DefaultPartial[] = {2, 0x18, 0x80, 2, 0x3C, 0x80, EPaperWaveformToken::End};
const EPaperCommandSequence kSSD1680DefaultPartialSequence = {kSSD1680DefaultPartial, sizeof(kSSD1680DefaultPartial)};
const uint8_t kSSD1680DefaultGray[] = {
    2, 0x18, 0x80, 2, 0x22, 0xB1, 1, 0x20, EPaperWaveformToken::WaitBusy,
    3, 0x1A, 0x5A, 0x00, 2, 0x22, 0x91, 1, 0x20,
    EPaperWaveformToken::WaitBusy, EPaperWaveformToken::End,
};
const EPaperCommandSequence kSSD1680DefaultGraySequence = {kSSD1680DefaultGray, sizeof(kSSD1680DefaultGray)};

const uint8_t kSSD1681DefaultFull[] = {
    EPaperWaveformToken::TemplateCommand, 0x01, 3, 8, 7, 0, 0x00,
    2, 0x11, 0x03,
    EPaperWaveformToken::TemplateCommand, 0x44, 2, 0, 0x00, 9,
    EPaperWaveformToken::TemplateCommand, 0x45, 4, 0, 0x00, 0, 0x00, 8, 7,
    2, 0x3C, 0x05, 2, 0x18, 0x80,
    2, 0x4E, 0x00, 3, 0x4F, 0x00, 0x00,
    EPaperWaveformToken::End,
};
const EPaperCommandSequence kSSD1681DefaultFullSequence = {kSSD1681DefaultFull, sizeof(kSSD1681DefaultFull)};
const uint8_t kSSD1681DefaultPartial[] = {2, 0x18, 0x80, 2, 0x3C, 0x80, EPaperWaveformToken::End};
const EPaperCommandSequence kSSD1681DefaultPartialSequence = {kSSD1681DefaultPartial, sizeof(kSSD1681DefaultPartial)};
const uint8_t kSSD1681DefaultGray[] = {
    2, 0x18, 0x80, 2, 0x22, 0xB1, 1, 0x20, EPaperWaveformToken::WaitBusy,
    3, 0x1A, 0x5A, 0x00, 2, 0x22, 0x91, 1, 0x20,
    EPaperWaveformToken::WaitBusy, EPaperWaveformToken::End,
};
const EPaperCommandSequence kSSD1681DefaultGraySequence = {kSSD1681DefaultGray, sizeof(kSSD1681DefaultGray)};

const uint8_t kSSD1683DefaultFull[] = {
    EPaperWaveformToken::TemplateCommand, 0x01, 3, 8, 7, 0, 0x00,
    2, 0x11, 0x03, EPaperWaveformToken::TemplateCommand, 0x44, 2, 0, 0x00, 9,
    EPaperWaveformToken::TemplateCommand, 0x45, 4, 0, 0x00, 0, 0x00, 8, 7,
    2, 0x3C, 0x05, 3, 0x21, 0x40, 0x00, 2, 0x18, 0x80,
    2, 0x4E, 0x00, 3, 0x4F, 0x00, 0x00, EPaperWaveformToken::End,
};
const EPaperCommandSequence kSSD1683DefaultFullSequence = {kSSD1683DefaultFull, sizeof(kSSD1683DefaultFull)};
const uint8_t kSSD1683DefaultPartial[] = {3, 0x21, 0x00, 0x00, 2, 0x18, 0x80, 2, 0x3C, 0x80, EPaperWaveformToken::End};
const EPaperCommandSequence kSSD1683DefaultPartialSequence = {kSSD1683DefaultPartial, sizeof(kSSD1683DefaultPartial)};
const uint8_t kSSD1683DefaultGray[] = {
    3, 0x21, 0x00, 0x00, 2, 0x18, 0x80, 2, 0x22, 0xB1, 1, 0x20,
    EPaperWaveformToken::WaitBusy, 3, 0x1A, 0x5A, 0x00, 2, 0x22, 0x91,
    1, 0x20, EPaperWaveformToken::WaitBusy, EPaperWaveformToken::End,
};
const EPaperCommandSequence kSSD1683DefaultGraySequence = {kSSD1683DefaultGray, sizeof(kSSD1683DefaultGray)};

const uint8_t kSSD1677DefaultFull[] = {
    6, 0x0C, 0xAE, 0xC7, 0xC3, 0xC0, 0x80, 2, 0x3C, 0x05,
    EPaperWaveformToken::TemplateCommand, 0x01, 3, 8, 7, 0, 0x02,
    2, 0x11, 0x03,
    EPaperWaveformToken::TemplateCommand, 0x44, 4, 0, 0x00, 0, 0x00, 6, 5,
    EPaperWaveformToken::TemplateCommand, 0x45, 4, 0, 0x00, 0, 0x00, 8, 7,
    2, 0x18, 0x80, 3, 0x4E, 0x00, 0x00, 3, 0x4F, 0x00, 0x00,
    EPaperWaveformToken::End,
};
const EPaperCommandSequence kSSD1677DefaultFullSequence = {kSSD1677DefaultFull, sizeof(kSSD1677DefaultFull)};
const uint8_t kSSD1677DefaultPartial[] = {2, 0x18, 0x80, 2, 0x3C, 0x80, EPaperWaveformToken::End};
const EPaperCommandSequence kSSD1677DefaultPartialSequence = {kSSD1677DefaultPartial, sizeof(kSSD1677DefaultPartial)};
const uint8_t kSSD1677DefaultGray[] = {
    6, 0x0C, 0xAE, 0xC7, 0xC3, 0xC0, 0x80, 2, 0x3C, 0x00,
    EPaperWaveformToken::TemplateCommand, 0x01, 3, 8, 7, 0, 0x02,
    2, 0x11, 0x03,
    EPaperWaveformToken::TemplateCommand, 0x44, 4, 0, 0x00, 0, 0x00, 6, 5,
    EPaperWaveformToken::TemplateCommand, 0x45, 4, 0, 0x00, 0, 0x00, 8, 7,
    2, 0x18, 0x80, 3, 0x1A, 0x67, 0x00, 3, 0x4E, 0x00, 0x00,
    3, 0x4F, 0x00, 0x00, EPaperWaveformToken::End,
};
const EPaperCommandSequence kSSD1677DefaultGraySequence = {kSSD1677DefaultGray, sizeof(kSSD1677DefaultGray)};

// UC8151D flexible monochrome panels -- OTP LUT path. PSR (0x1F) bit7=0
// selects the factory-programmed OTP LUT, so no LUT register writes are
// needed. Records are [size, command, data...] with size including the
// command byte; WaitBusy waits for BUSY high (UC8151D BUSY is NGFX_GXEPD2_LOW while
// busy, NGFX_GXEPD2_HIGH when ready).
//
// GDEW0213I5FD: 104 source x 212 gate (normally rotated to 212x104).
const uint8_t kUC8151D_GDEW0213I5FD_Full[] = {
    4, 0x06, 0x17, 0x17, 0x17,        // BTST (3 data)
    1, 0x04,                            // PON
    EPaperWaveformToken::WaitBusy,      // wait BUSY high
    2, 0x00, 0x1F,                      // PSR (OTP LUT)
    4, 0x61, 0x68, 0x00, 0xD4,          // TRES (104 source x 212 gate)
    2, 0x50, 0x87,                      // CDI (border floating)
    EPaperWaveformToken::End,
};
const EPaperCommandSequence kUC8151D_GDEW0213I5FD_FullSequence = {
    kUC8151D_GDEW0213I5FD_Full, sizeof(kUC8151D_GDEW0213I5FD_Full)
};

// GDEW029I6FD: 128 source x 296 gate.
const uint8_t kUC8151D_GDEW029I6FD_Full[] = {
    4, 0x06, 0x17, 0x17, 0x17,        // BTST (3 data)
    1, 0x04,                            // PON
    EPaperWaveformToken::WaitBusy,      // wait BUSY high
    2, 0x00, 0x1F,                      // PSR (OTP LUT)
    4, 0x61, 0x80, 0x01, 0x28,          // TRES (128 source x 296 gate)
    2, 0x50, 0x87,                      // CDI (border floating)
    EPaperWaveformToken::End,
};
const EPaperCommandSequence kUC8151D_GDEW029I6FD_FullSequence = {
    kUC8151D_GDEW029I6FD_Full, sizeof(kUC8151D_GDEW029I6FD_Full)
};

// UC8179 and T133A01 retain controller-specific LUT/dual-CS workflows.
}

// Equivalent in responsibility to bb_epaper's panelDefs[]; this is an
// independent implementation and contains only Seeed_GFX-owned definitions.
const EPaperWaveformProfile kEPaperPanelDefs[] = {
    {"default", "JD79660 existing default", "JD79660", 0, 0, 4,
     EPAPER_WAVEFORM_FULL, EPaperWaveformStorage::CommandSequence,
     &kJD79660DefaultFullSequence, nullptr, nullptr, nullptr},

    {"default", "JD79667 existing default", "JD79667", 0, 0, 4,
     EPAPER_WAVEFORM_FULL, EPaperWaveformStorage::CommandSequence,
     &kJD79667DefaultFullSequence, nullptr, nullptr, nullptr},
    {"default", "JD79676 existing default", "JD79676", 0, 0, 4,
     EPAPER_WAVEFORM_FULL, EPaperWaveformStorage::CommandSequence,
     &kJD79676DefaultFullSequence, nullptr, nullptr, nullptr},
    {"default", "JD79686B existing default", "JD79686B", 0, 0, 1,
     EPAPER_WAVEFORM_FULL,
     EPaperWaveformStorage::CommandSequence,
     &kJD79686BDefaultFullSequence, nullptr, nullptr, nullptr},
    {"default", "SSD1677 existing default", "SSD1677", 0, 0, 1,
     EPAPER_WAVEFORM_FULL | EPAPER_WAVEFORM_PARTIAL | EPAPER_WAVEFORM_GRAY,
     EPaperWaveformStorage::CommandSequence,
     &kSSD1677DefaultFullSequence, nullptr, &kSSD1677DefaultPartialSequence,
     &kSSD1677DefaultGraySequence},
    {"default", "UC8151D GDEW0213I5FD 2.13 flex", "UC8151D", 104, 212, 1,
     EPAPER_WAVEFORM_FULL, EPaperWaveformStorage::CommandSequence,
     &kUC8151D_GDEW0213I5FD_FullSequence,
     &kUC8151D_GDEW0213I5FD_FullSequence, nullptr, nullptr},
    {"default", "UC8151D GDEW029I6FD 2.9 flex", "UC8151D", 128, 296, 1,
     EPAPER_WAVEFORM_FULL, EPaperWaveformStorage::CommandSequence,
     &kUC8151D_GDEW029I6FD_FullSequence, &kUC8151D_GDEW029I6FD_FullSequence,
     nullptr, nullptr},
    {"default", "SSD1680 existing default", "SSD1680", 0, 0, 1,
     EPAPER_WAVEFORM_FULL | EPAPER_WAVEFORM_FAST | EPAPER_WAVEFORM_PARTIAL |
         EPAPER_WAVEFORM_GRAY,
     EPaperWaveformStorage::CommandSequence,
     &kSSD1680DefaultFullSequence, &kSSD1680DefaultFullSequence,
     &kSSD1680DefaultPartialSequence,
     &kSSD1680DefaultGraySequence},
    {"default", "SSD1681 existing default", "SSD1681", 0, 0, 1,
     EPAPER_WAVEFORM_FULL | EPAPER_WAVEFORM_FAST | EPAPER_WAVEFORM_PARTIAL |
         EPAPER_WAVEFORM_GRAY,
     EPaperWaveformStorage::CommandSequence,
     &kSSD1681DefaultFullSequence, &kSSD1681DefaultFullSequence,
     &kSSD1681DefaultPartialSequence,
     &kSSD1681DefaultGraySequence},
    {"default", "SSD1683 existing default", "SSD1683", 0, 0, 1,
     EPAPER_WAVEFORM_FULL | EPAPER_WAVEFORM_FAST | EPAPER_WAVEFORM_PARTIAL |
         EPAPER_WAVEFORM_GRAY,
     EPaperWaveformStorage::CommandSequence,
     &kSSD1683DefaultFullSequence, &kSSD1683DefaultFullSequence,
     &kSSD1683DefaultPartialSequence,
     &kSSD1683DefaultGraySequence},
    {"default", "SSD2677 800x480", "SSD2677", 800, 480, 1,
     EPAPER_WAVEFORM_FULL, EPaperWaveformStorage::CommandSequence,
     &kSSD2677DefaultFullSequence, nullptr, nullptr, nullptr},
    {"default", "UC8179 648x480 OTP full", "UC8179", 648, 480, 1,
     EPAPER_WAVEFORM_FULL,
     EPaperWaveformStorage::BuiltIn,
     nullptr, nullptr, nullptr, nullptr},
    {"default", "UC8179 800x480 Seeed LUT modes", "UC8179", 800, 480, 1,
     EPAPER_WAVEFORM_FULL | EPAPER_WAVEFORM_FAST |
         EPAPER_WAVEFORM_PARTIAL | EPAPER_WAVEFORM_GRAY,
     EPaperWaveformStorage::BuiltIn,
     nullptr, nullptr, nullptr, nullptr},
    {"default", "ED2208 GDEP040E01", "ED2208", 400, 600, 4,
     EPAPER_WAVEFORM_FULL, EPaperWaveformStorage::CommandSequence,
     &kED2208GDEP040E01FullSequence, nullptr, nullptr, nullptr},
    {"default", "ED2208 GDEP073E01", "ED2208", 800, 480, 4,
     EPAPER_WAVEFORM_FULL, EPaperWaveformStorage::CommandSequence,
     &kED2208GDEP073E01FullSequence, nullptr, nullptr, nullptr},
    {"default", "IT8951 built-in waveform modes", "IT8951", 0, 0, 4,
     EPAPER_WAVEFORM_FULL | EPAPER_WAVEFORM_PARTIAL | EPAPER_WAVEFORM_GRAY,
     EPaperWaveformStorage::BuiltIn,
     nullptr, nullptr, nullptr, nullptr},
    {"default", "ED103TC2 built-in waveform modes", "ED103TC2", 0, 0, 1,
     EPAPER_WAVEFORM_FULL | EPAPER_WAVEFORM_PARTIAL | EPAPER_WAVEFORM_GRAY,
     EPaperWaveformStorage::BuiltIn,
     nullptr, nullptr, nullptr, nullptr},
    {"default", "T133A01 existing default", "T133A01", 0, 0, 4,
     EPAPER_WAVEFORM_FULL, EPaperWaveformStorage::BuiltIn,
     nullptr, nullptr, nullptr, nullptr},
    {"default", "GDEB0709E01 7.09inch Spectra6", "GDEB0709E01", 0, 0, 4,
     EPAPER_WAVEFORM_FULL, EPaperWaveformStorage::BuiltIn,
     nullptr, nullptr, nullptr, nullptr},
};

const size_t kEPaperPanelDefCount = sizeof(kEPaperPanelDefs) / sizeof(kEPaperPanelDefs[0]);

const EPaperWaveformProfile* findEPaperWaveformProfile(const char* controller,
                                                        const char* id,
                                                        uint16_t width,
                                                        uint16_t height,
                                                        uint8_t colorDepth) {
    if (!controller || !id) return nullptr;
    const EPaperWaveformProfile* generic = nullptr;
    for (size_t index = 0; index < kEPaperPanelDefCount; ++index) {
        const EPaperWaveformProfile* profile = &kEPaperPanelDefs[index];
        if (!sameString(profile->controller, controller) ||
            !sameString(profile->id, id)) continue;
        if (width == 0 && height == 0 && colorDepth == 0) return profile;
        if (!ePaperWaveformProfileMatches(*profile, controller, width, height,
                                          colorDepth)) continue;
        if (profile->width == width && profile->height == height) return profile;
        if (!generic) generic = profile;
    }
    return generic;
}

size_t ePaperWaveformProfileCount(const char* controller, uint16_t width,
                                  uint16_t height, uint8_t colorDepth) {
    if (!controller) return 0;
    const bool filterByPanel = width != 0 || height != 0 || colorDepth != 0;
    size_t count = 0;
    for (size_t index = 0; index < kEPaperPanelDefCount; ++index) {
        const EPaperWaveformProfile& profile = kEPaperPanelDefs[index];
        if (!sameString(profile.controller, controller)) continue;
        if (filterByPanel && !ePaperWaveformProfileMatches(
                profile, controller, width, height, colorDepth)) continue;
        ++count;
    }
    return count;
}

const EPaperWaveformProfile* ePaperWaveformProfileAt(const char* controller,
                                                      size_t index,
                                                      uint16_t width,
                                                      uint16_t height,
                                                      uint8_t colorDepth) {
    if (!controller) return nullptr;
    const bool filterByPanel = width != 0 || height != 0 || colorDepth != 0;
    for (size_t current = 0; current < kEPaperPanelDefCount; ++current) {
        const EPaperWaveformProfile* profile = &kEPaperPanelDefs[current];
        if (!sameString(profile->controller, controller)) continue;
        if (filterByPanel && !ePaperWaveformProfileMatches(
                *profile, controller, width, height, colorDepth)) continue;
        if (index-- == 0) return profile;
    }
    return nullptr;
}

bool ePaperWaveformProfileMatches(const EPaperWaveformProfile& profile,
                                  const char* controller, uint16_t width,
                                  uint16_t height, uint8_t colorDepth) {
    return sameString(profile.controller, controller) &&
           (profile.width == 0 || profile.width == width) &&
           (profile.height == 0 || profile.height == height) &&
           (profile.colorDepth == 0 || profile.colorDepth == colorDepth);
}

const EPaperCommandSequence* ePaperWaveformSequence(
    const EPaperWaveformProfile& profile, EPaperWaveformMode mode) {
    switch (mode) {
        case EPaperWaveformMode::Full: return profile.full;
        case EPaperWaveformMode::Fast: return profile.fast;
        case EPaperWaveformMode::Partial: return profile.partial;
        case EPaperWaveformMode::Gray: return profile.gray;
    }
    return nullptr;
}

EPaperWaveformResult applyEPaperCommandSequence(
    IBus& bus, const EPaperCommandSequence& sequence, uint16_t width,
    uint16_t height, int busyPin, bool busyReadyHigh, uint32_t busyTimeoutMs) {
    if (!sequence.bytes || sequence.size == 0) return EPaperWaveformResult::InvalidSequence;

    size_t validationOffset = 0;
    bool terminated = false;
    while (validationOffset < sequence.size) {
        const uint8_t recordLength = sequence.bytes[validationOffset++];
        if (recordLength == EPaperWaveformToken::End) {
            terminated = validationOffset == sequence.size;
            break;
        }
        if (recordLength == EPaperWaveformToken::WaitBusy) continue;
        if (recordLength == EPaperWaveformToken::DelayMs) {
            if (validationOffset + 2 > sequence.size) return EPaperWaveformResult::InvalidSequence;
            validationOffset += 2;
            continue;
        }
        if (recordLength == EPaperWaveformToken::Resolution) {
            if (validationOffset + 1 > sequence.size) return EPaperWaveformResult::InvalidSequence;
            ++validationOffset;
            continue;
        }
        if (recordLength == EPaperWaveformToken::TemplateCommand) {
            if (!validateTemplateRecord(sequence.bytes, sequence.size, validationOffset)) {
                return EPaperWaveformResult::InvalidSequence;
            }
            continue;
        }
        if (recordLength < 1 || validationOffset + recordLength > sequence.size) {
            return EPaperWaveformResult::InvalidSequence;
        }
        validationOffset += recordLength;
    }
    if (!terminated) return EPaperWaveformResult::InvalidSequence;

    size_t offset = 0;
    while (offset < sequence.size) {
        const uint8_t recordLength = sequence.bytes[offset++];
        if (recordLength == EPaperWaveformToken::End) return EPaperWaveformResult::Ok;
        if (recordLength == EPaperWaveformToken::WaitBusy) {
            if (busyPin >= 0 && !gfxWaitForPin(busyPin, busyReadyHigh, busyTimeoutMs, 1)) {
                return EPaperWaveformResult::BusyTimeout;
            }
            continue;
        }
        if (recordLength == EPaperWaveformToken::DelayMs) {
            const uint16_t milliseconds = static_cast<uint16_t>(sequence.bytes[offset]) |
                                          (static_cast<uint16_t>(sequence.bytes[offset + 1]) << 8);
            offset += 2;
            delay(milliseconds);
            continue;
        }
        if (recordLength == EPaperWaveformToken::Resolution) {
            bus.writeCommand(sequence.bytes[offset++]);
            bus.writeData(static_cast<uint8_t>(width >> 8));
            bus.writeData(static_cast<uint8_t>(width));
            bus.writeData(static_cast<uint8_t>(height >> 8));
            bus.writeData(static_cast<uint8_t>(height));
            continue;
        }
        if (recordLength == EPaperWaveformToken::TemplateCommand) {
            const uint8_t command = sequence.bytes[offset++];
            const uint8_t dataCount = sequence.bytes[offset++];
            bus.writeCommand(command);
            for (uint8_t index = 0; index < dataCount; ++index) {
                const uint8_t kind = sequence.bytes[offset++];
                uint8_t value = 0;
                if (kind == static_cast<uint8_t>(EPaperWaveformToken::TemplateValue::Literal)) {
                    value = sequence.bytes[offset++];
                } else if (!appendTemplateValue(kind, width, height, value)) {
                    return EPaperWaveformResult::InvalidSequence;
                }
                bus.writeData(value);
            }
            continue;
        }
        bus.writeCommand(sequence.bytes[offset++]);
        if (recordLength > 1) {
            bus.writeData(sequence.bytes + offset, recordLength - 1);
            offset += recordLength - 1;
        }
    }
    return EPaperWaveformResult::InvalidSequence;
}

} }
