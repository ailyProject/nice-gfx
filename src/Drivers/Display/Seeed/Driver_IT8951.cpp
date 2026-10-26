// Seeed Studio MIT; adapted from a736f1726cedb89692ca9ebcedcc3d2468ef92f5; see LICENSE and upstream-manifest.json.
#include "Driver_IT8951.h"
namespace nice_gfx { namespace seeed_protocol {
/**
 * @file   Driver_IT8951.cpp
 * @brief  IT8951 Tcon ePaper display driver implementation
 *
 * Ported from Seeed_GFX-master Extensions/Tcon.cpp and
 * TFT_Drivers/IT8951_Defines.h.
 *
 * The IT8951 uses a custom 16-bit SPI protocol with preamble words:
 *   0x6000 = Write command
 *   0x0000 = Write data
 *   0x1000 = Read data
 *
 * Each operation is framed by beginWrite/endWrite or beginRead/endRead.
 * The HRDY pin indicates when the IT8951 is ready for the next word.
 */

// Static helper: bit reversal

TWord Driver_IT8951::reverse_bits_16(TWord x) {
    x = (x & 0xAAAA) >> 1 | (x & 0x5555) << 1;
    x = (x & 0xCCCC) >> 2 | (x & 0x3333) << 2;
    x = (x & 0xF0F0) >> 4 | (x & 0x0F0F) << 4;
    x = (x & 0xFF00) >> 8 | (x & 0x00FF) << 8;
    return x;
}

TByte Driver_IT8951::reverse_bits_8(TByte x) {
    x = (x & 0xAA) >> 1 | (x & 0x55) << 1;
    x = (x & 0xCC) >> 2 | (x & 0x33) << 2;
    x = (x & 0xF0) >> 4 | (x & 0x0F) << 4;
    return x;
}

// Constructor

Driver_IT8951::Driver_IT8951(uint16_t w, uint16_t h, int8_t busyPin)
    : _init_width(w), _init_height(h), _busyPin(busyPin)
    , _gulImgBufAddr(0)
{
    _width  = w;
    _height = h;
    memset(&_gstI80DevInfo, 0, sizeof(_gstI80DevInfo));
    memset(&_imgAreaInfo, 0, sizeof(_imgAreaInfo));
    if (_busyPin >= 0) {
        pinMode(_busyPin, NGFX_GXEPD2_INPUT);
    }
}

// IDriver: Initialization

bool Driver_IT8951::init(IBus& bus) {
    _bus = &bus;
    hardwareReset(10, 10);
    if (_busyPin >= 0 && !waitForReadyPin(_busyPin, true)) return false;
    hostTconInit();

    if (_gstI80DevInfo.usPanelW == 0 || _gstI80DevInfo.usPanelH == 0) {
        return false;
    }

    setRotation(0);
    setAddrWindow(0, 0, _init_width - 1, _init_height - 1);
    return true;
}

// IDriver: Rotation

void Driver_IT8951::setRotation(uint8_t m) {
    _rotation = m % 4;
    if (_rotation & 1) {
        _width  = _init_height;
        _height = _init_width;
    } else {
        _width  = _init_width;
        _height = _init_height;
    }
}

// IDriver: Display control

void Driver_IT8951::invertDisplay(bool invert) {
    (void)invert;
    // ePaper does not support inversion in the traditional sense
}

void Driver_IT8951::displayOn() {
    update();
}

void Driver_IT8951::displayOff() {
    // ePaper retains its image; no explicit "off" needed
}

// IDriver: Address window

void Driver_IT8951::setAddrWindow(uint16_t xs, uint16_t ys,
                                   uint16_t xe, uint16_t ye) {
    setTconWindowsData(xs, ys, xe, ye);
}

// IDriver: Pixel writing

void Driver_IT8951::writePixel(uint16_t color) {
    // Write a single pixel using the IT8951 image load protocol
    TByte gray4 = (TByte)((color & 0x0F00) >> 8); // Extract 4-bit gray from 16-bit color
    TWord packed = (TWord)(gray4 << 12) | (gray4 << 8) | (gray4 << 4) | gray4;
    writePixels(&packed, 1);
}

void Driver_IT8951::writePixels(const uint16_t* data, size_t len) {
    if (!_bus || len == 0) return;

    // Load the pixel data into the IT8951 image buffer
    TCONLdImgInfo stLdImgInfo;
    TCONAreaImgInfo stAreaImgInfo;

    stLdImgInfo.ulStartFBAddr    = reinterpret_cast<uintptr_t>(data);
    stLdImgInfo.usEndianType     = NGFX_SEEED_IT8951_LDIMG_L_ENDIAN;
    stLdImgInfo.usPixelFormat    = NGFX_SEEED_IT8951_4BPP;
    stLdImgInfo.usRotate         = NGFX_SEEED_IT8951_ROTATE_0;
    stLdImgInfo.ulImgBufBaseAddr = _gulImgBufAddr;
    stLdImgInfo.usFilp           = 0;

    stAreaImgInfo.usX      = _imgAreaInfo.usX;
    stAreaImgInfo.usY      = _imgAreaInfo.usY;
    stAreaImgInfo.usWidth  = _imgAreaInfo.usWidth;
    stAreaImgInfo.usHeight = _imgAreaInfo.usHeight;

    tconHostAreaPackedPixelWrite(&stLdImgInfo, &stAreaImgInfo);
}

void Driver_IT8951::writeFill(uint16_t color, size_t len) {
    if (!_bus || len == 0) return;

    // Extract 4-bit gray value and pack 4 pixels into one word
    TByte gray4 = (TByte)((color & 0x0F00) >> 8);
    TWord packed = (TWord)(gray4 << 12) | (gray4 << 8) | (gray4 << 4) | gray4;

    // Allocate a buffer and fill it
    TWord* buf = (TWord*)protocolAllocate(len * sizeof(TWord));
    if (!buf) return;

    for (size_t i = 0; i < len; i++) {
        buf[i] = packed;
    }

    writePixels(buf, len);
    protocolFree(buf);
}

// IDriver: Power management

void Driver_IT8951::sleep() {
    tconSleep();
}

void Driver_IT8951::wake() {
    tconWake();
}

// IT8951-specific: Update

void Driver_IT8951::update() {
    tconDisplayArea(0, 0, _init_width, _init_height, NGFX_SEEED_IT8951_MODE_2);
    tconWaitForDisplayReady();
}

void Driver_IT8951::updatePartial(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    tconDisplayArea(x, y, w, h, NGFX_SEEED_IT8951_MODE_1);
    tconWaitForDisplayReady();
}

// Low-level TCon protocol

void Driver_IT8951::tconWaitForReady() {
    _waitForReady();
}

void Driver_IT8951::tconSendWord(TWord data) {
    _sendWord(data);
}

TWord Driver_IT8951::tconReceiveWord() {
    return _recvWord();
}

// Write command

void Driver_IT8951::tconWriteCmdCode(TWord usCmdCode) {
    TWord wPreamble = 0x6000;

    _bus->beginWrite();

    _waitForReady();
    _sendWord(wPreamble);

    _waitForReady();
    _sendWord(usCmdCode);

    _bus->endWrite();
}

// Write data (single word)

void Driver_IT8951::tconWirteData(TWord usData) {
    TWord wPreamble = 0x0000;

    _bus->beginWrite();

    _waitForReady();
    _sendWord(wPreamble);

    _waitForReady();
    _sendWord(usData);

    _bus->endWrite();
}

// Write data (N words, burst)

void Driver_IT8951::tconWirteNData(TWord* pwBuf, TDWord ulSizeWordCnt) {
    TWord  wPreamble = 0x0000;

    _bus->beginWrite();

    _waitForReady();
    _sendWord(wPreamble);   // Send preamble
    _waitForReady();

    // The original Seeed_GFX IT8951 path sends the packed image buffer with
    // pushPixels() and _swapBytes=false.  On ESP32-S3 that places each
    // little-endian TWord on the wire low byte first.  IT8951 image loading is
    // configured as NGFX_SEEED_IT8951_LDIMG_L_ENDIAN and depends on that order.
    // IBus::writeData16() is intentionally MSB-first for commands/registers,
    // so swap only these burst image words before handing them to the bus.
    // Without this conversion every adjacent pair of 8-pixel columns is
    // exchanged, which turns otherwise sharp text into periodic vertical
    // slices while long horizontal/vertical borders remain mostly intact.
    for (TDWord i = 0; i < ulSizeWordCnt; i++) {
        const TWord littleEndianWireWord =
            static_cast<TWord>((pwBuf[i] >> 8) | (pwBuf[i] << 8));
        _bus->writeData16(littleEndianWireWord);
    }

    _bus->endWrite();
}

// Send command with arguments

void Driver_IT8951::tconSendCmdArg(TWord usCmdCode, TWord* pArg, TWord usNumArg) {
    // Send Cmd code
    tconWriteCmdCode(usCmdCode);
    // Send Data
    for (TWord i = 0; i < usNumArg; i++) {
        tconWirteData(pArg[i]);
    }
}

// Read data (single word)

TWord Driver_IT8951::tconReadData() {
    TWord wRData;
    TWord wPreamble = 0x1000;

    // The IT8951 read preamble, dummy word and returned word form one SPI
    // frame.  CS must stay asserted for the entire sequence.  The original
    // Seeed_GFX tconReadData() sends the preamble after spi_begin_read() for
    // exactly this reason; splitting it into write/read transactions makes
    // the controller discard the pending read.
    _bus->beginRead();
    _waitForReady();
    _sendWord(wPreamble);

    _recvWord();  // Dummy read
    _waitForReady();
    wRData = _recvWord();  // Actual data
    _bus->endRead();

    return wRData;
}

// Read data (N words, burst)

void Driver_IT8951::tconReadNData(TWord* pwBuf, TDWord ulSizeWordCnt) {
    TWord wPreamble = 0x1000;

    // Keep CS low from the read preamble through the final returned word.
    // IT8951 treats a CS rising edge as the end of the read request.
    _bus->beginRead();
    _waitForReady();
    _sendWord(wPreamble);

    _waitForReady();
    _recvWord();  // Dummy read
    _waitForReady();

    for (TDWord i = 0; i < ulSizeWordCnt; i++) {
        pwBuf[i] = _recvWord();
    }
    _bus->endRead();
}

// Register read/write

TWord Driver_IT8951::tconReadReg(TWord usRegAddr) {
    TWord usData;
    // I80 Mode: Send Cmd and Register Address
    tconWriteCmdCode(NGFX_SEEED_IT8951_TCON_REG_RD);
    tconWirteData(usRegAddr);
    // Read data from Host Data bus
    usData = tconReadData();
    return usData;
}

void Driver_IT8951::tconWriteReg(TWord usRegAddr, TWord usValue) {
    // I80 Mode: Send Cmd, Register Address and Write Value
    tconWriteCmdCode(NGFX_SEEED_IT8951_TCON_REG_WR);
    tconWirteData(usRegAddr);
    tconWirteData(usValue);
}

// Load image start

void Driver_IT8951::tconLoadImgStart(TCONLdImgInfo* pstLdImgInfo) {
    TWord usArg;
    // Setting Argument for Load image start
    usArg = (pstLdImgInfo->usEndianType << 8)
          | (pstLdImgInfo->usPixelFormat << 4)
          | (pstLdImgInfo->usRotate);
    // Send Cmd
    tconWriteCmdCode(NGFX_SEEED_IT8951_TCON_LD_IMG);
    // Send Arg
    tconWirteData(usArg);
}

// Set image rotation

void Driver_IT8951::tconSetImgRotation(TDWord rotation) {
    TWord arg = (NGFX_SEEED_IT8951_LDIMG_B_ENDIAN << 8) | (NGFX_SEEED_IT8951_8BPP << 4) | rotation;
    tconWriteCmdCode(NGFX_SEEED_IT8951_TCON_LD_IMG);
    tconWirteData(arg);
}

// Load image area start

void Driver_IT8951::tconLoadImgAreaStart(TCONLdImgInfo* pstLdImgInfo,
                                          TCONAreaImgInfo* pstAreaImgInfo) {
    TWord usArg[5];
    // Setting Argument for Load image start
    usArg[0] = (pstLdImgInfo->usEndianType << 8)
             | (pstLdImgInfo->usPixelFormat << 4)
             | (pstLdImgInfo->usRotate);

    usArg[1] = pstAreaImgInfo->usX;
    usArg[2] = pstAreaImgInfo->usY;
    usArg[3] = pstAreaImgInfo->usWidth;
    usArg[4] = pstAreaImgInfo->usHeight;
    // Send Cmd and Args
    tconSendCmdArg(NGFX_SEEED_IT8951_TCON_LD_IMG_AREA, usArg, 5);
}

// Load image end

void Driver_IT8951::tconLoadImgEnd() {
    tconWriteCmdCode(NGFX_SEEED_IT8951_TCON_LD_IMG_END);
}

// Set image buffer base address

void Driver_IT8951::tconSetImgBufBaseAddr(TDWord ulImgBufAddr) {
    TWord usWordH = (TWord)((ulImgBufAddr >> 16) & 0x0000FFFF);
    TWord usWordL = (TWord)( ulImgBufAddr & 0x0000FFFF);
    // Write NGFX_SEEED_LISAR Reg
    tconWriteReg(NGFX_SEEED_LISAR + 2, usWordH);
    tconWriteReg(NGFX_SEEED_LISAR,     usWordL);
}

// Host area packed pixel write

void Driver_IT8951::tconHostAreaPackedPixelWrite(TCONLdImgInfo* pstLdImgInfo,
                                                  TCONAreaImgInfo* pstAreaImgInfo) {
    // Source buffer address of Host
    TWord* pusFrameBuf = reinterpret_cast<TWord*>(pstLdImgInfo->ulStartFBAddr);

    // Set Image buffer (IT8951) Base address
    tconSetImgBufBaseAddr(pstLdImgInfo->ulImgBufBaseAddr);

    // Send Load Image start Cmd
    tconLoadImgAreaStart(pstLdImgInfo, pstAreaImgInfo);

    // Calculate width in words based on pixel format
    uint16_t height = pstAreaImgInfo->usHeight;
    uint16_t width  = pstAreaImgInfo->usWidth;

    if (pstLdImgInfo->usPixelFormat == NGFX_SEEED_IT8951_4BPP) {
        width = (width + 3) / 4;   // 4 pixels per word
    } else if (pstLdImgInfo->usPixelFormat == NGFX_SEEED_IT8951_8BPP) {
        width = (width + 1) / 2;   // 2 pixels per word
    }
    // For 2BPP: 8 pixels per word, for 3BPP: not packed simply

    // Allocate temporary buffer for mirrored data
    TWord* mirroredFrameBuf = (TWord*)protocolAllocate(width * height * sizeof(TWord));
    if (mirroredFrameBuf == NULL) {
        tconLoadImgEnd();
        return;
    }

    for (uint16_t j = 0; j < height; j++) {
        for (uint16_t i = 0; i < width; i++) {
            if (pstLdImgInfo->usFilp) {
                mirroredFrameBuf[j * width + i] =
                    reverse_bits_16(pusFrameBuf[j * width + i]);
            } else {
                mirroredFrameBuf[j * width + i] =
                    pusFrameBuf[j * width + (width - 1 - i)];
            }
        }
    }

    tconWirteNData(mirroredFrameBuf, height * width);
    protocolFree(mirroredFrameBuf);

    // Send Load Img End Command
    tconLoadImgEnd();
}

// Display area

void Driver_IT8951::tconDisplayArea(TWord usX, TWord usY, TWord usW,
                                     TWord usH, TWord usDpyMode) {
    // Send I80 Display Command (User defined command of IT8951)
    tconWriteCmdCode(NGFX_SEEED_USDEF_I80_CMD_DPY_AREA); // 0x0034
    // Write arguments
    tconWirteData(usX);
    tconWirteData(usY);
    tconWirteData(usW);
    tconWirteData(usH);
    tconWirteData(usDpyMode);
}

// Display area 1bpp

void Driver_IT8951::tconDisplayArea1bpp(TWord usX, TWord usY, TWord usW,
                                         TWord usH, TWord usDpyMode,
                                         TByte ucBGGrayVal, TByte ucFGGrayVal) {
    usX = (_gstI80DevInfo.usPanelW - 1) - usX - usW + 1;

    // Set Display mode to 1 bpp mode - Set 0x18001138 Bit[18] to 1
    tconWriteReg(NGFX_SEEED_UP1SR + 2, tconReadReg(NGFX_SEEED_UP1SR + 2) | (1 << 2));

    // Set BitMap color table 0 and 1
    tconWriteReg(NGFX_SEEED_BGVR, (ucBGGrayVal << 8) | ucFGGrayVal);

    // Display
    tconDisplayArea(usX, usY, usW, usH, usDpyMode);

    tconWaitForDisplayReady();

    // Restore to normal mode
    tconWriteReg(NGFX_SEEED_UP1SR + 2, tconReadReg(NGFX_SEEED_UP1SR + 2) & ~(1 << 2));
}

// Load 1bpp image

void Driver_IT8951::tconLoad1bppImage(const TByte* p1bppImgBuf, TWord usX,
                                       TWord usY, TWord usW, TWord usH,
                                       TByte enFilp) {
    usX = (_gstI80DevInfo.usPanelW - 1) - usX - usW + 1;

    TCONLdImgInfo stLdImgInfo;
    TCONAreaImgInfo stAreaImgInfo;

    // Setting Load image information
    stLdImgInfo.ulStartFBAddr    = reinterpret_cast<uintptr_t>(p1bppImgBuf);
    stLdImgInfo.usEndianType     = NGFX_SEEED_IT8951_LDIMG_L_ENDIAN;
    stLdImgInfo.usPixelFormat    = NGFX_SEEED_IT8951_8BPP; // Use 8bpp mode; transfer size reduced to 1/8
    stLdImgInfo.usRotate         = NGFX_SEEED_IT8951_ROTATE_0;
    stLdImgInfo.ulImgBufBaseAddr = _gulImgBufAddr;
    stLdImgInfo.usFilp           = enFilp;

    // Set Load Area (1bpp: width and x are divided by 8)
    stAreaImgInfo.usX      = (usX + 7) / 8;
    stAreaImgInfo.usY      = usY;
    stAreaImgInfo.usWidth  = (usW + 7) / 8;  // 1bpp, transfer size = 1/8 of 8bpp
    stAreaImgInfo.usHeight = usH;

    // Load Image from Host to IT8951 Image Buffer
    tconHostAreaPackedPixelWrite(&stLdImgInfo, &stAreaImgInfo);
}

// Load image (4bpp)

void Driver_IT8951::tconLoadImage(const TByte* pImgBuf, TWord usX, TWord usY,
                                   TWord usW, TWord usH, TByte enFilp) {
    TCONLdImgInfo stLdImgInfo;
    TCONAreaImgInfo stAreaImgInfo;

    // Setting Load image information
    stLdImgInfo.ulStartFBAddr    = reinterpret_cast<uintptr_t>(pImgBuf);
    stLdImgInfo.usEndianType     = NGFX_SEEED_IT8951_LDIMG_L_ENDIAN;
    stLdImgInfo.usPixelFormat    = NGFX_SEEED_IT8951_4BPP;
    stLdImgInfo.usRotate         = NGFX_SEEED_IT8951_ROTATE_0;
    stLdImgInfo.ulImgBufBaseAddr = _gulImgBufAddr;
    stLdImgInfo.usFilp           = enFilp;

    // Set Load Area
    stAreaImgInfo.usX      = usX;
    stAreaImgInfo.usY      = usY;
    stAreaImgInfo.usWidth  = usW;
    stAreaImgInfo.usHeight = usH;

    // Load Image from Host to IT8951 Image Buffer
    tconHostAreaPackedPixelWrite(&stLdImgInfo, &stAreaImgInfo);
}

// Get device info

void Driver_IT8951::getTconInfo(void* pBuf) {
    TWord* pusWord = (TWord*)pBuf;

    tconWriteCmdCode(NGFX_SEEED_USDEF_I80_CMD_GET_DEV_INFO);

    // Burst Read Request for SPI interface only
    tconReadNData(pusWord, sizeof(I80TCONDevInfo) / 2);
}

// Host TCon initialization

void Driver_IT8951::hostTconInit() {
    setTconVcom(1400);      // SET VCOM

    getTconInfo(&_gstI80DevInfo);

    if (_gstI80DevInfo.usPanelW == 0 || _gstI80DevInfo.usPanelH == 0) {
        return;
    }

    _gulImgBufAddr = _gstI80DevInfo.usImgBufAddrL
                   | (_gstI80DevInfo.usImgBufAddrH << 16);

    // Set to Enable I80 Packed mode
    tconWriteReg(NGFX_SEEED_I80CPCR, 0x0001);
}

void Driver_IT8951::hostTconInitFast() {
    getTconInfo(&_gstI80DevInfo);

    if (_gstI80DevInfo.usPanelW == 0 || _gstI80DevInfo.usPanelH == 0) {
        return;
    }

    _gulImgBufAddr = _gstI80DevInfo.usImgBufAddrL
                   | (_gstI80DevInfo.usImgBufAddrH << 16);
}

// Set window data

void Driver_IT8951::setTconWindowsData(TWord x1, TWord y1, TWord x2, TWord y2) {
    _imgAreaInfo.usX      = x1;
    _imgAreaInfo.usY      = y1;
    _imgAreaInfo.usWidth  = x2 - x1 + 1;
    _imgAreaInfo.usHeight = y2 - y1 + 1;
}

// Temperature control

TWord Driver_IT8951::getTconTemp() {
    tconWriteCmdCode(0x0040);
    tconWirteData(0x00);
    return (TByte)tconReadData();
}

void Driver_IT8951::setTconTemp(TWord temp) {
    tconWriteCmdCode(0x0040);
    tconWirteData(0x01);
    tconWirteData(temp);
}

// VCOM control

TWord Driver_IT8951::getTconVcom() {
    tconWriteCmdCode(0x0039);
    tconWirteData(0x00);
    return tconReadData();
}

void Driver_IT8951::setTconVcom(TWord vcom) {
    tconWriteCmdCode(0x0039);
    tconWirteData(0x02);
    tconWirteData(vcom);
}

// Power management

void Driver_IT8951::tconSleep() {
    tconWriteCmdCode(NGFX_SEEED_IT8951_TCON_SLEEP);
}

void Driver_IT8951::tconWake() {
    tconWriteCmdCode(NGFX_SEEED_IT8951_TCON_SYS_RUN);
}

void Driver_IT8951::tconStandby() {
    tconWriteCmdCode(NGFX_SEEED_IT8951_TCON_STANDBY);
}

// Wait for display ready

void Driver_IT8951::tconWaitForDisplayReady() {
    const uint32_t started = millis();
    while (tconReadReg(NGFX_SEEED_LUTAFSR)) {
        if (static_cast<uint32_t>(millis() - started) >= 30000U) {
            setOperationError(DriverOperationError::BusyTimeout);
            break;
        }
        delay(1);
        yield();
    }
}

} }
