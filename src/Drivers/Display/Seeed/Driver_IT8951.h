// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "SeeedHal.h"
#include "Driver.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_IT8951.h
 * @brief  IT8951 Tcon (Timing Controller) ePaper driver for Seeed_GFX v2.0
 *
 * Used on large ePaper displays (e.g., reTerminal E1001 series).
 * Ported from Seeed_GFX-master TFT_Drivers/IT8951_Defines.h and
 * Extensions/Tcon.h / Tcon.cpp.
 *
 * The IT8951 uses a custom SPI protocol with 16-bit preamble words
 * (0x6000 = command, 0x0000 = data, 0x1000 = read) and an HRDY busy pin.
 * Communication is via 16-bit word transfers on top of the 8-bit IBus interface.
 */

#ifndef NGFX_SEEED_SEEED_GFX_DRIVER_IT8951_H
#define NGFX_SEEED_SEEED_GFX_DRIVER_IT8951_H



// Type definitions

typedef uint8_t  TByte;   // uint8_t
typedef uint16_t TWord;   // uint16_t
typedef uint32_t TDWord;  // uint32_t

// Built-in I80 Command Codes

#define NGFX_SEEED_IT8951_TCON_SYS_RUN      0x0001
#define NGFX_SEEED_IT8951_TCON_STANDBY      0x0002
#define NGFX_SEEED_IT8951_TCON_SLEEP        0x0003
#define NGFX_SEEED_IT8951_TCON_REG_RD       0x0010
#define NGFX_SEEED_IT8951_TCON_REG_WR       0x0011
#define NGFX_SEEED_IT8951_TCON_MEM_BST_RD_T 0x0012
#define NGFX_SEEED_IT8951_TCON_MEM_BST_RD_S 0x0013
#define NGFX_SEEED_IT8951_TCON_MEM_BST_WR   0x0014
#define NGFX_SEEED_IT8951_TCON_MEM_BST_END  0x0015
#define NGFX_SEEED_IT8951_TCON_LD_IMG       0x0020
#define NGFX_SEEED_IT8951_TCON_LD_IMG_AREA  0x0021
#define NGFX_SEEED_IT8951_TCON_LD_IMG_END   0x0022

// I80 User-defined command codes
#define NGFX_SEEED_USDEF_I80_CMD_DPY_AREA     0x0034
#define NGFX_SEEED_USDEF_I80_CMD_GET_DEV_INFO 0x0302
#define NGFX_SEEED_USDEF_I80_CMD_DPY_BUF_AREA 0x0037

// Panel defaults

#define NGFX_SEEED_IT8951_PANEL_WIDTH   1872
#define NGFX_SEEED_IT8951_PANEL_HEIGHT  1404

// Rotation modes

#define NGFX_SEEED_IT8951_ROTATE_0     0
#define NGFX_SEEED_IT8951_ROTATE_90    1
#define NGFX_SEEED_IT8951_ROTATE_180   2
#define NGFX_SEEED_IT8951_ROTATE_270   3

// Pixel format modes (BPP - Bits Per Pixel)

#define NGFX_SEEED_IT8951_2BPP   0
#define NGFX_SEEED_IT8951_3BPP   1
#define NGFX_SEEED_IT8951_4BPP   2
#define NGFX_SEEED_IT8951_8BPP   3

// Waveform modes

#define NGFX_SEEED_IT8951_MODE_0   0
#define NGFX_SEEED_IT8951_MODE_1   1
#define NGFX_SEEED_IT8951_MODE_2   2
#define NGFX_SEEED_IT8951_MODE_3   3
#define NGFX_SEEED_IT8951_MODE_4   4

// Endian types

#define NGFX_SEEED_IT8951_LDIMG_L_ENDIAN   0
#define NGFX_SEEED_IT8951_LDIMG_B_ENDIAN   1

// Auto LUT

#define NGFX_SEEED_IT8951_DIS_AUTO_LUT   0
#define NGFX_SEEED_IT8951_EN_AUTO_LUT    1

// LUT Engine Status

#define NGFX_SEEED_IT8951_ALL_LUTE_BUSY 0xFFFF

// IT8951 TCon Register Addresses

// Register base address for I80-only RW access
#define NGFX_SEEED_DISPLAY_REG_BASE 0x1000

// Basic LUT Registers
#define NGFX_SEEED_LUT0EWHR  (NGFX_SEEED_DISPLAY_REG_BASE + 0x00)   // LUT0 Engine Width Height Reg
#define NGFX_SEEED_LUT0XYR   (NGFX_SEEED_DISPLAY_REG_BASE + 0x40)   // LUT0 XY Reg
#define NGFX_SEEED_LUT0BADDR (NGFX_SEEED_DISPLAY_REG_BASE + 0x80)   // LUT0 Base Address Reg
#define NGFX_SEEED_LUT0MFN   (NGFX_SEEED_DISPLAY_REG_BASE + 0xC0)   // LUT0 Mode and Frame number Reg
#define NGFX_SEEED_LUT01AF   (NGFX_SEEED_DISPLAY_REG_BASE + 0x114)  // LUT0 and LUT1 Active Flag Reg

// Update Parameter Setting Registers
#define NGFX_SEEED_UP0SR (NGFX_SEEED_DISPLAY_REG_BASE + 0x134)      // Update Parameter0 Setting Reg

#define NGFX_SEEED_UP1SR     (NGFX_SEEED_DISPLAY_REG_BASE + 0x138)  // Update Parameter1 Setting Reg
#define NGFX_SEEED_LUT0ABFRV (NGFX_SEEED_DISPLAY_REG_BASE + 0x13C)  // LUT0 Alpha blend and Fill rectangle Value
#define NGFX_SEEED_UPBBADDR  (NGFX_SEEED_DISPLAY_REG_BASE + 0x17C)  // Update Buffer Base Address
#define NGFX_SEEED_LUT0IMXY  (NGFX_SEEED_DISPLAY_REG_BASE + 0x180)  // LUT0 Image buffer X/Y offset Reg
#define NGFX_SEEED_LUTAFSR   (NGFX_SEEED_DISPLAY_REG_BASE + 0x224)  // LUT Status Reg (status of All LUT Engines)

#define NGFX_SEEED_BGVR      (NGFX_SEEED_DISPLAY_REG_BASE + 0x250)  // Bitmap (1bpp) image color table

// System Registers
#define NGFX_SEEED_SYS_REG_BASE 0x0000

// Address of System Registers
#define NGFX_SEEED_I80CPCR (NGFX_SEEED_SYS_REG_BASE + 0x04)

// Memory Converter Registers
#define NGFX_SEEED_MCSR_BASE_ADDR 0x0200
#define NGFX_SEEED_MCSR  (NGFX_SEEED_MCSR_BASE_ADDR + 0x0000)
#define NGFX_SEEED_LISAR (NGFX_SEEED_MCSR_BASE_ADDR + 0x0008)

// Struct definitions

/** Load image information structure */
typedef struct TCONLdImgInfo {
    TWord  usEndianType;     // Little or Big Endian
    TWord  usPixelFormat;    // BPP
    TWord  usRotate;         // Rotate mode
    TWord  usFilp;           // Flip mode
    uintptr_t ulStartFBAddr; // Host pointer; never transmitted to the controller
    TDWord ulImgBufBaseAddr; // Base address of target image buffer
} TCONLdImgInfo;

/** Area image information structure */
typedef struct TCONAreaImgInfo {
    TWord usX;
    TWord usY;
    TWord usWidth;
    TWord usHeight;
} TCONAreaImgInfo;

/** IT8951 device information structure */
typedef struct I80TCONDevInfo {
    TWord usPanelW;
    TWord usPanelH;
    TWord usImgBufAddrL;
    TWord usImgBufAddrH;
    TWord usFWVersion[8];   // 16 Bytes String
    TWord usLUTVersion[8];  // 16 Bytes String
} I80TCONDevInfo;

// Driver class

class Driver_IT8951 : public IDriver {
public:
    /**
     * @param w       Panel width in pixels (default NGFX_SEEED_IT8951_PANEL_WIDTH)
     * @param h       Panel height in pixels (default NGFX_SEEED_IT8951_PANEL_HEIGHT)
     * @param busyPin HRDY busy pin number (-1 if not connected)
     */
    Driver_IT8951(uint16_t w = NGFX_SEEED_IT8951_PANEL_WIDTH,
                  uint16_t h = NGFX_SEEED_IT8951_PANEL_HEIGHT,
                  int8_t busyPin = -1);

    // IDriver interface

    const char* name() const override { return "IT8951"; }
    uint16_t width() const override { return _width; }
    uint16_t height() const override { return _height; }
    uint8_t colorDepth() const override { return 4; } // 4-bit grayscale
    bool supportsDeepSleep() const override { return true; }
    bool supportsPartialRefresh() const override { return true; }
    bool supportsGrayRefresh(uint8_t levels) const override { return levels == 16; }
    bool supportsTemperatureCompensation() const override { return true; }

    bool init(IBus& bus) override;
    void setRotation(uint8_t rotation) override;
    uint8_t rotation() const override { return _rotation; }
    void invertDisplay(bool invert) override;
    void displayOn() override;
    void displayOff() override;
    void setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye) override;
    void writePixel(uint16_t color) override;
    void writePixels(const uint16_t* data, size_t len) override;
    void writeFill(uint16_t color, size_t len) override;
    void sleep() override;
    void wake() override;
    IBus& bus() override { return *_bus; }

    // IT8951-specific public interface

    /** Trigger a full-screen display update */
    void update() override;

    /** Trigger a partial display update */
    void updatePartial(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void updatePartial() override {
        updatePartial(_imgAreaInfo.usX, _imgAreaInfo.usY,
                      _imgAreaInfo.usWidth, _imgAreaInfo.usHeight);
    }
    void updateGray() override { update(); }
    void pushNewColors(const uint8_t* data, size_t len) override {
        (void)len;
        if (data) tconLoadImage(data, _imgAreaInfo.usX, _imgAreaInfo.usY,
                                _imgAreaInfo.usWidth, _imgAreaInfo.usHeight, 0);
    }
    void pushGrayColors(const uint8_t* data, size_t len) override {
        pushNewColors(data, len);
    }
    void setTemperature(int8_t temp) override {
        if (_bus) setTconTemp(static_cast<uint8_t>(temp));
    }

    /** Set the HRDY busy pin */
    void setBusyPin(int pin) override {
        _busyPin = static_cast<int8_t>(pin);
        if (pin >= 0) pinMode(pin, NGFX_GXEPD2_INPUT);
    }

    /** Get the device information */
    const I80TCONDevInfo& devInfo() const { return _gstI80DevInfo; }

    /** Get the image buffer address */
    TDWord imgBufAddr() const { return _gulImgBufAddr; }

    // TCon protocol methods (public for advanced use)

    void tconWaitForReady();

    void tconSendWord(TWord data);
    TWord tconReceiveWord();

    void tconWriteCmdCode(TWord usCmdCode);
    void tconWirteData(TWord usData);
    void tconWirteNData(TWord* pwBuf, TDWord ulSizeWordCnt);

    void tconSendCmdArg(TWord usCmdCode, TWord* pArg, TWord usNumArg);
    TWord tconReadData();
    void tconReadNData(TWord* pwBuf, TDWord ulSizeWordCnt);

    TWord tconReadReg(TWord usRegAddr);
    void tconWriteReg(TWord usRegAddr, TWord usValue);

    void tconLoadImgStart(TCONLdImgInfo* pstLdImgInfo);
    void tconLoadImgAreaStart(TCONLdImgInfo* pstLdImgInfo, TCONAreaImgInfo* pstAreaImgInfo);
    void tconLoadImgEnd();

    void tconSetImgBufBaseAddr(TDWord ulImgBufAddr);
    void tconSetImgRotation(TDWord rotation);

    void tconHostAreaPackedPixelWrite(TCONLdImgInfo* pstLdImgInfo, TCONAreaImgInfo* pstAreaImgInfo);

    void tconDisplayArea(TWord usX, TWord usY, TWord usW, TWord usH, TWord usDpyMode);
    void tconDisplayArea1bpp(TWord usX, TWord usY, TWord usW, TWord usH,
                             TWord usDpyMode, TByte ucBGGrayVal, TByte ucFGGrayVal);

    void tconLoad1bppImage(const TByte* p1bppImgBuf, TWord usX, TWord usY,
                           TWord usW, TWord usH, TByte enFilp);
    void tconLoadImage(const TByte* pImgBuf, TWord usX, TWord usY,
                       TWord usW, TWord usH, TByte enFilp);

    TWord getTconTemp();
    void setTconTemp(TWord temp);

    TWord getTconVcom();
    void setTconVcom(TWord vcom);

    void getTconInfo(void* pBuf);
    void setTconWindowsData(TWord x1, TWord y1, TWord x2, TWord y2);
    void hostTconInit();
    void hostTconInitFast();

    void tconSleep();
    void tconWake();
    void tconStandby();
    void tconWaitForDisplayReady();

private:
    // Low-level 16-bit word helpers

    /** Send a 16-bit word via two 8-bit writes (MSB first) */
    inline void _sendWord(TWord data) {
        _bus->writeData16(data);
    }

    /** Receive a 16-bit word via two 8-bit reads (MSB first) */
    inline TWord _recvWord() {
        uint8_t hi = _bus->readData();
        uint8_t lo = _bus->readData();
        return ((TWord)hi << 8) | lo;
    }

    /** Wait for HRDY pin to go high */
    inline void _waitForReady() {
        if (_busyPin >= 0) {
            (void)waitForReadyPin(_busyPin, true);
        }
    }

    /** Helper: combine two 8-bit writes into a 16-bit word send */
    inline void _sendWordRaw(TWord data) {
        _bus->writeData((uint8_t)(data >> 8));
        _bus->writeData((uint8_t)(data & 0xFF));
    }

    // Bit reversal helpers

    static TWord reverse_bits_16(TWord x);
    static TByte reverse_bits_8(TByte x);

    // State

    uint16_t _init_width;
    uint16_t _init_height;
    int8_t   _busyPin;

    I80TCONDevInfo _gstI80DevInfo;
    TDWord         _gulImgBufAddr;
    TCONAreaImgInfo _imgAreaInfo;
};

#endif // NGFX_SEEED_SEEED_GFX_DRIVER_IT8951_H

} }
