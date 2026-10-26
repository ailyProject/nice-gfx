// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include <stdint.h>
#include <stddef.h>
namespace nice_gfx { namespace seeed_protocol {
#ifndef NGFX_SEEED_SEEED_GFX_CAPABILITIES_H
#define NGFX_SEEED_SEEED_GFX_CAPABILITIES_H


enum class PixelFormat : uint8_t {
    Unknown = 0,
    Mono1,
    Gray4,
    Indexed4,
    RGB565,
    RGB888,
};

enum class DisplayTechnology : uint8_t {
    Unknown = 0,
    TFT,
    OLED,
    EInk,
};

struct BusCapabilities {
    bool readable = false;
    /** Legacy alias: true only for queued asynchronous pixel transfers. */
    bool dma = false;
    /** Public pixel buffers can be submitted and remain owned until dmaBusy clears. */
    bool asyncPixelTransfer = false;
    /** The transport uses a hardware DMA engine internally. */
    bool hardwareTransfer = false;
    /** A display peripheral continuously scans a framebuffer (for example RGB/DPI). */
    bool continuousScanout = false;
    bool parallel = false;
    bool secondaryChipSelect = false;
    size_t maxTransferBytes = 0;
};

enum class DmaTransferResult : uint8_t {
    Queued = 0,
    SynchronousFallback,
    Unsupported,
    InvalidArgument,
    ClippedOut,
    SubmitFailed,
};

struct DisplayCapabilities {
    DisplayTechnology technology = DisplayTechnology::Unknown;
    PixelFormat nativeFormat = PixelFormat::Unknown;
    bool readback = false;
    bool partialRefresh = false;
    bool fastRefresh = false;
    bool temperatureCompensation = false;
    bool deepSleep = false;
    bool backlight = false;
    uint16_t partialXAlignment = 1;
    uint16_t partialYAlignment = 1;
};

#endif // NGFX_SEEED_SEEED_GFX_CAPABILITIES_H

} }
