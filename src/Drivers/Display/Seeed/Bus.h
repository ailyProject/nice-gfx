// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include <stdint.h>
#include <stddef.h>
#include "Capabilities.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Bus.h
 * @brief  IBus abstract interface for Seeed_GFX v2.0
 *
 * The Bus layer encapsulates all low-level communication protocols.
 * Each concrete Bus implementation handles one communication type
 * (SPI, I2C, Parallel 8-bit, Parallel 16-bit).
 */

#ifndef NGFX_SEEED_SEEED_GFX_BUS_H
#define NGFX_SEEED_SEEED_GFX_BUS_H



enum class ChipSelectTarget : uint8_t {
    Primary = 0,
    Secondary,
    Both,
};

class IBus {
public:
    virtual ~IBus() = default;

    // Lifecycle

    /** Initialize the bus hardware */
    virtual bool begin() = 0;

    /** De-initialize the bus hardware */
    virtual void end() = 0;

    // Write transaction control

    /** Begin a write transaction (assert CS, set write mode) */
    virtual void beginWrite() = 0;

    /** End a write transaction (de-assert CS, restore mode) */
    virtual void endWrite() = 0;

    // Command write (DC=0)

    /** Send an 8-bit command to the display */
    virtual void writeCommand(uint8_t cmd) = 0;

    /** Send a 16-bit command to the display */
    virtual void writeCommand16(uint16_t cmd) {
        writeCommand(static_cast<uint8_t>(cmd >> 8));
        writeCommand(static_cast<uint8_t>(cmd & 0xFF));
    }

    // Data write (DC=1)

    /** Send a single byte of data */
    virtual void writeData(uint8_t data) = 0;

    /** Send a 16-bit word of data */
    virtual void writeData16(uint16_t data) {
        writeData(static_cast<uint8_t>(data >> 8));
        writeData(static_cast<uint8_t>(data & 0xFF));
    }

    /** Send a buffer of data bytes */
    virtual void writeData(const uint8_t* data, size_t len) = 0;

    // Pixel write (optimized)

    /** Write an array of 16-bit pixels (with optional byte swap) */
    virtual void writePixels(const uint16_t* data, size_t len) {
        if (!data) return;
        for (size_t i = 0; i < len; ++i) writeData16(data[i]);
    }

    /** Write the same pixel value repeatedly */
    virtual void writeRepeat(uint16_t pixel, size_t len) {
        for (size_t i = 0; i < len; ++i) writeData16(pixel);
    }

    // Read operations

    /** Begin a read transaction */
    virtual void beginRead() = 0;

    /** End a read transaction */
    virtual void endRead() = 0;

    /** Read a single byte of data */
    virtual uint8_t readData() = 0;

    // Query

    /** Returns true if this is a parallel bus */
    virtual bool isParallel() const { return false; }

    /** Maximum transfer size in bytes */
    virtual size_t maxTransferSize() const { return 65535; }

    /** Whether the bus can independently select two display controllers. */
    virtual bool supportsSecondaryChipSelect() const { return false; }

    /** Select the controller(s) asserted by subsequent transactions. */
    virtual void selectChip(ChipSelectTarget target) { (void)target; }

    // DMA support

    /** Returns true if DMA is supported */
    virtual bool supportsDMA() const { return false; }

    /** Enable or disable DMA use. Returns the resulting enabled state. */
    virtual bool enableDMA(bool enable = true) { (void)enable; return false; }

    /** Write pixels using DMA */
    virtual bool writePixelsDMA(const uint16_t* data, size_t len) {
        (void)data; (void)len;
        return false;
    }

    /** Returns true if a DMA transfer is in progress */
    virtual bool dmaBusy() { return false; }

    // Configuration

    /** Set the bus clock frequency */
    virtual void setFrequency(uint32_t freq) = 0;

    /** Get the current bus clock frequency */
    virtual uint32_t frequency() const = 0;

    /** Last backend transport error (0 means no error). */
    virtual int lastError() const { return 0; }

    /**
     * Human-readable detail for the last backend transport error.
     *
     * Implementations must return storage that remains valid after the bus
     * object is destroyed because product-mode begin() tears down a failed
     * display stack before the caller reads Seeed_GFX::lastResult().
     */
    virtual const char* lastErrorMessage() const { return nullptr; }

    virtual BusCapabilities capabilities() const {
        BusCapabilities caps;
        caps.readable = false;
        caps.dma = supportsDMA();
        caps.asyncPixelTransfer = supportsDMA();
        caps.hardwareTransfer = supportsDMA();
        caps.parallel = isParallel();
        caps.maxTransferBytes = maxTransferSize();
        return caps;
    }
};

#endif // NGFX_SEEED_SEEED_GFX_BUS_H

} }
