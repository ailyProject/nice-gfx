// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include <stddef.h>
#include <stdint.h>
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file seeed_ep.h
 * @brief Public ePaper panel profile declarations.
 *
 * Implementations and the central catalog live in seeed_ep.inl, compiled once
 * through SeeedEPaperProfiles.cpp. This follows the bb_epaper .cpp + .inl
 * organization without importing its GPL code or panel data.
 */
#ifndef NGFX_SEEED_SEEED_GFX_SEEED_EP_H
#define NGFX_SEEED_SEEED_GFX_SEEED_EP_H


class IBus;

enum class EPaperWaveformMode : uint8_t {
    Full = 0,
    Fast,
    Partial,
    Gray,
};

enum EPaperWaveformCapability : uint8_t {
    EPAPER_WAVEFORM_FULL    = 1U << 0,
    EPAPER_WAVEFORM_FAST    = 1U << 1,
    EPAPER_WAVEFORM_PARTIAL = 1U << 2,
    EPAPER_WAVEFORM_GRAY    = 1U << 3,
};

struct EPaperCommandSequence {
    const uint8_t* bytes;
    size_t size;
};

enum class EPaperWaveformStorage : uint8_t {
    BuiltIn = 0,
    CommandSequence,
};

struct EPaperWaveformProfile {
    const char* id;
    const char* displayName;
    const char* controller;
    uint16_t width;
    uint16_t height;
    uint8_t colorDepth;
    uint8_t capabilities;
    EPaperWaveformStorage storage;
    const EPaperCommandSequence* full;
    const EPaperCommandSequence* fast;
    const EPaperCommandSequence* partial;
    const EPaperCommandSequence* gray;
};

enum class EPaperWaveformResult : uint8_t {
    Ok = 0,
    UnknownProfile,
    IncompatibleProfile,
    UnsupportedMode,
    InvalidSequence,
    BusyTimeout,
};

namespace EPaperWaveformToken {
constexpr uint8_t End = 0x00;
constexpr uint8_t WaitBusy = 0xFE;
constexpr uint8_t DelayMs = 0xFD;
constexpr uint8_t Resolution = 0xFC;
constexpr uint8_t TemplateCommand = 0xFB;

enum class TemplateValue : uint8_t {
    Literal = 0,
    WidthHigh,
    WidthLow,
    HeightHigh,
    HeightLow,
    WidthMinusOneHigh,
    WidthMinusOneLow,
    HeightMinusOneHigh,
    HeightMinusOneLow,
    WidthBytesMinusOne,
};
}

extern const EPaperWaveformProfile kEPaperPanelDefs[];
extern const size_t kEPaperPanelDefCount;

const EPaperWaveformProfile* findEPaperWaveformProfile(const char* controller,
                                                        const char* id,
                                                        uint16_t width = 0,
                                                        uint16_t height = 0,
                                                        uint8_t colorDepth = 0);
size_t ePaperWaveformProfileCount(const char* controller,
                                  uint16_t width = 0,
                                  uint16_t height = 0,
                                  uint8_t colorDepth = 0);
const EPaperWaveformProfile* ePaperWaveformProfileAt(const char* controller,
                                                      size_t index,
                                                      uint16_t width = 0,
                                                      uint16_t height = 0,
                                                      uint8_t colorDepth = 0);
bool ePaperWaveformProfileMatches(const EPaperWaveformProfile& profile,
                                  const char* controller, uint16_t width,
                                  uint16_t height, uint8_t colorDepth);
const EPaperCommandSequence* ePaperWaveformSequence(
    const EPaperWaveformProfile& profile, EPaperWaveformMode mode);
EPaperWaveformResult applyEPaperCommandSequence(
    IBus& bus, const EPaperCommandSequence& sequence, uint16_t width,
    uint16_t height, int busyPin = -1, bool busyReadyHigh = true,
    uint32_t busyTimeoutMs = 30000);

#endif // NGFX_SEEED_SEEED_GFX_SEEED_EP_H

} }
