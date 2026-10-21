// NICE_GFX: one display object, constructor pins, portable graphics and DMA.
// Contributor notices are preserved in LICENSE-Bodmer.txt.

// Stop fonts etc. being loaded multiple times
#if !defined(_NICE_GFXH_)
#define _NICE_GFXH_

#include "../Core/Version.h"
#define NICE_GFX_ANTIALIASING_VERSION 1

// Bit level feature flags
// Bit 0 set: viewport capability
#define NICE_GFX_FEATURES 1

/***************************************************************************************
**                         Section 1: Load required header files
***************************************************************************************/

#include "../Core/Platform.h"
#include "../Core/TextWriter.h"
#include "../Fonts/FontStorage.h"
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#if NICE_GFX_NATIVE_IDF
#include <string>
#endif
#include "../Fonts/FontOptions.h"
#include "Icon.h"
#include "../Fonts/Monospace.h"
#include "../Display/Panel.h"
#include "../Drivers/Touch/TouchController.h"
#include "../Core/Geometry.h"
#include "../Display/DisplayAccess.h"

namespace nice_gfx { struct TextGlyph; namespace detail { struct DisplayAccess; } }

/***************************************************************************************
**                         Section 5: Font datum enumeration
***************************************************************************************/
//These enumerate the text plotting alignment (reference datum point)
#define TL_DATUM 0 // Top left (default)
#define TC_DATUM 1 // Top centre
#define TR_DATUM 2 // Top right
#define ML_DATUM 3 // Middle left
#define CL_DATUM 3 // Centre left, same as above
#define MC_DATUM 4 // Middle centre
#define CC_DATUM 4 // Centre centre, same as above
#define MR_DATUM 5 // Middle right
#define CR_DATUM 5 // Centre right, same as above
#define BL_DATUM 6 // Bottom left
#define BC_DATUM 7 // Bottom centre
#define BR_DATUM 8 // Bottom right
#define L_BASELINE  9 // Left character baseline (Line the 'A' character would sit on)
#define C_BASELINE 10 // Centre character baseline
#define R_BASELINE 11 // Right character baseline

/***************************************************************************************
**                         Section 6: Colour enumeration
***************************************************************************************/
// Default color definitions
#define GFX_BLACK       0x0000      /*   0,   0,   0 */
#define GFX_NAVY        0x000F      /*   0,   0, 128 */
#define GFX_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define GFX_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define GFX_MAROON      0x7800      /* 128,   0,   0 */
#define GFX_PURPLE      0x780F      /* 128,   0, 128 */
#define GFX_OLIVE       0x7BE0      /* 128, 128,   0 */
#define GFX_LIGHTGREY   0xD69A      /* 211, 211, 211 */
#define GFX_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define GFX_BLUE        0x001F      /*   0,   0, 255 */
#define GFX_GREEN       0x07E0      /*   0, 255,   0 */
#define GFX_CYAN        0x07FF      /*   0, 255, 255 */
#define GFX_RED         0xF800      /* 255,   0,   0 */
#define GFX_MAGENTA     0xF81F      /* 255,   0, 255 */
#define GFX_YELLOW      0xFFE0      /* 255, 255,   0 */
#define GFX_WHITE       0xFFFF      /* 255, 255, 255 */
#define GFX_ORANGE      0xFDA0      /* 255, 180,   0 */
#define GFX_GREENYELLOW 0xB7E0      /* 180, 255,   0 */
#define GFX_PINK        0xFE19      /* 255, 192, 203 */ //Lighter pink, was 0xFC9F
#define GFX_BROWN       0x9A60      /* 150,  75,   0 */
#define GFX_GOLD        0xFEA0      /* 255, 215,   0 */
#define GFX_SILVER      0xC618      /* 192, 192, 192 */
#define GFX_SKYBLUE     0x867D      /* 135, 206, 235 */
#define GFX_VIOLET      0x915C      /* 180,  46, 226 */

// Next is a special 16-bit colour value that encodes to 8 bits
// and will then decode back to the same 16-bit value.
// Convenient for 8-bit and 16-bit transparent sprites.
#define GFX_TRANSPARENT 0x0120 // This is actually a dark green

// Default palette for 4-bit colour sprites
static const uint16_t default_4bit_palette[] PROGMEM = {
  GFX_BLACK,    //  0  ^
  GFX_BROWN,    //  1  |
  GFX_RED,      //  2  |
  GFX_ORANGE,   //  3  |
  GFX_YELLOW,   //  4  Colours 0-9 follow the resistor colour code!
  GFX_GREEN,    //  5  |
  GFX_BLUE,     //  6  |
  GFX_PURPLE,   //  7  |
  GFX_DARKGREY, //  8  |
  GFX_WHITE,    //  9  v
  GFX_CYAN,     // 10  Blue+green mix
  GFX_MAGENTA,  // 11  Blue+red mix
  GFX_MAROON,   // 12  Darker red colour
  GFX_DARKGREEN,// 13  Darker green colour
  GFX_NAVY,     // 14  Darker blue colour
  GFX_PINK      // 15
};

/***************************************************************************************
**                         Section 7: Diagnostic support
***************************************************************************************/
// Snapshot of the current display and its constructor-selected transport.
// GPIOs use the same Arduino pin numbering as the constructor arguments.
struct DisplayInfo {
  const char *version = NICE_GFX_VERSION;
  uint32_t controllerId = 0;
  uint16_t nativeWidth = 0, nativeHeight = 0;
  uint16_t width = 0, height = 0;
  uint8_t rotation = 0;
  int resetPin = -1;
  bool initialized = false, asynchronous = false;
  nice_gfx::TransportInfo bus;
};

/***************************************************************************************
**                         Section 8: Class member and support functions
***************************************************************************************/

// Class functions and variables
class NICE_GFX : public nice_gfx::PrintBase { friend class GFX_Sprite; // Sprite class has access to protected members
  friend struct nice_gfx::detail::DisplayAccess;

 //--------------------------------------- public ------------------------------------//
 public:

  NICE_GFX(const NICE_GFX &) = delete;
  NICE_GFX &operator=(const NICE_GFX &) = delete;
  virtual ~NICE_GFX();

  nice_gfx::Status beginChecked();
  nice_gfx::Status lastError() const { return _runtimeStatus; }
  nice_gfx::Status wait() { dmaWait(); return _runtimeStatus; }
  bool asynchronous() const { return _panel && _panel->asynchronous(); }
  // Absolute panel coordinates; stride and byte order are explicit, independent
  // of viewport/setSwapBytes. Keep data alive and unchanged until dmaWait().
  nice_gfx::Status flush(int32_t x, int32_t y, nice_gfx::PixelView view, bool async = true);

  // Initialize the display; beginChecked() additionally returns an error status.
  // Controller variants (including ST7735 tab) are PanelOptions at construction.
  void     begin();

  // Target sinks specialize physical output and in-memory storage.
  virtual void     drawPixel(int32_t x, int32_t y, uint32_t color),
                   fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

  // Shared geometry dispatches spans to the target sink. Keeping algorithms
  // out of the vtable also lets small MCU builds discard unused primitives.
  void drawLine(int32_t xs, int32_t ys, int32_t xe, int32_t ye, uint32_t color);
  void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color);
  void drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color);
  // 1bpp flash icon; transparent background, tint color, optional integer scale 1–8.
  void drawIcon(int32_t x, int32_t y, const nice_gfx::MonoIcon &icon, uint16_t color, uint8_t scale = 1);
  // An explicit background enables coverage blending on write-only panels.
  void drawIcon(int32_t x, int32_t y, const nice_gfx::MonoIcon &icon,
                uint16_t foreground, uint16_t background, uint8_t scale);
  // Applies to every font and icon. Disable for the original pixel silhouette.
  void setAntialiasing(bool enabled) { _antialiasing = enabled; }
  bool getAntialiasing() const { return _antialiasing; }

  int16_t drawChar(uint16_t uniCode, int32_t x, int32_t y);
  virtual int16_t height(void), width(void);

                   // Read the colour of a pixel at x,y and return value in 565 format
  virtual uint16_t readPixel(int32_t x, int32_t y);

  virtual void     setWindow(int32_t xs, int32_t ys, int32_t xe, int32_t ye);   // Note: start + end coordinates

                   // Push (aka write pixel) colours to the set window
  virtual void     pushColor(uint16_t color);

                   // These are non-inlined to enable override
  virtual void     begin_nin_write();
  virtual void     end_nin_write();

  void     setRotation(uint8_t r); // Set the display image orientation to 0, 1, 2 or 3
  uint8_t  getRotation(void);      // Read the current rotation

  // Change the origin position from the default top left
  // Note: setRotation, setViewport and resetViewport will revert origin to top left corner of screen/sprite
  void     setOrigin(int32_t x, int32_t y);
  int32_t  getOriginX(void);
  int32_t  getOriginY(void);

  void     invertDisplay(bool i);  // Tell TFT to invert all displayed colours


  // The GFX_Sprite class inherits the following functions (not all are useful to Sprite class
  void     setAddrWindow(int32_t xs, int32_t ys, int32_t w, int32_t h); // Note: start coordinates + width and height

  // Viewport commands, see "Viewport_Demo" sketch
  void     setViewport(int32_t x, int32_t y, int32_t w, int32_t h, bool vpDatum = true);
  bool     checkViewport(int32_t x, int32_t y, int32_t w, int32_t h);
  int32_t  getViewportX(void);
  int32_t  getViewportY(void);
  int32_t  getViewportWidth(void);
  int32_t  getViewportHeight(void);
  bool     getViewportDatum(void);
  void     frameViewport(uint16_t color, int32_t w);
  void     resetViewport(void);

           // Clip input window to viewport bounds, return false if whole area is out of bounds
  bool     clipAddrWindow(int32_t* x, int32_t* y, int32_t* w, int32_t* h);
           // Clip input window area to viewport bounds, return false if whole area is out of bounds
  bool     clipWindow(int32_t* xs, int32_t* ys, int32_t* xe, int32_t* ye);

           // Write a solid block of a single colour
  void     pushBlock(uint16_t color, uint32_t len);

           // Write a set of pixels stored in memory, use setSwapBytes(true/false) function to correct endianess
  void     pushPixels(const void * data_in, uint32_t len);

           // Support for half duplex (bi-directional SDA) SPI bus where MOSI must be switched to input


  // Graphics drawing
  void     fillScreen(uint32_t color),
           drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color),
           drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color),
           fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color);

  void     fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2);
  void     fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2);

  void     drawCircle(int32_t x, int32_t y, int32_t r, uint32_t color),
           drawCircleHelper(int32_t x, int32_t y, int32_t r, uint8_t cornername, uint32_t color),
           fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color),
           fillCircleHelper(int32_t x, int32_t y, int32_t r, uint8_t cornername, int32_t delta, uint32_t color),

           drawEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color),
           fillEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color),

           //                 Corner 1               Corner 2               Corner 3
           drawTriangle(int32_t x1,int32_t y1, int32_t x2,int32_t y2, int32_t x3,int32_t y3, uint32_t color),
           fillTriangle(int32_t x1,int32_t y1, int32_t x2,int32_t y2, int32_t x3,int32_t y3, uint32_t color);


  // Smooth (anti-aliased) graphics drawing
           // Draw a pixel blended with the background pixel colour (bg_color) specified,  return blended colour
           // If the bg_color is not specified, the background pixel colour will be read from TFT or sprite
  uint16_t drawPixel(int32_t x, int32_t y, uint32_t color, uint8_t alpha, uint32_t bg_color = 0x00FFFFFF);

           // Draw an anti-aliased (smooth) arc between start and end angles. Arc ends are anti-aliased.
           // By default the arc is drawn with square ends unless the "roundEnds" parameter is included and set true
           // Angle = 0 is at 6 o'clock position, 90 at 9 o'clock etc. The angles must be in range 0-360 or they will be clipped to these limits
           // The start angle may be larger than the end angle. Arcs are always drawn clockwise from the start angle.
  void     drawSmoothArc(int32_t x, int32_t y, int32_t r, int32_t ir, uint32_t startAngle, uint32_t endAngle, uint32_t fg_color, uint32_t bg_color, bool roundEnds = false);

           // As per "drawSmoothArc" except the ends of the arc are NOT anti-aliased, this facilitates dynamic arc length changes with
           // arc segments and ensures clean segment joints. 
           // The sides of the arc are anti-aliased by default. If smoothArc is false sides will NOT be anti-aliased
  void     drawArc(int32_t x, int32_t y, int32_t r, int32_t ir, uint32_t startAngle, uint32_t endAngle, uint32_t fg_color, uint32_t bg_color, bool smoothArc = true);

           // Draw an anti-aliased filled circle at x, y with radius r
           // Note: The thickness of line is 3 pixels to reduce the visible "braiding" effect of anti-aliasing narrow lines
           //       this means the inner anti-alias zone is always at r-1 and the outer zone at r+1
  void     drawSmoothCircle(int32_t x, int32_t y, int32_t r, uint32_t fg_color, uint32_t bg_color);
  
           // Draw an anti-aliased filled circle at x, y with radius r
           // If bg_color is not included the background pixel colour will be read from TFT or sprite
  void     fillSmoothCircle(int32_t x, int32_t y, int32_t r, uint32_t color, uint32_t bg_color = 0x00FFFFFF);

           // Draw a rounded rectangle that has a line thickness of r-ir+1 and bounding box defined by x,y and w,h
           // The outer corner radius is r, inner corner radius is ir
           // The inside and outside of the border are anti-aliased
  void     drawSmoothRoundRect(int32_t x, int32_t y, int32_t r, int32_t ir, int32_t w, int32_t h, uint32_t fg_color, uint32_t bg_color = 0x00FFFFFF, uint8_t quadrants = 0xF);

           // Draw a filled rounded rectangle , corner radius r and bounding box defined by x,y and w,h
  void     fillSmoothRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color, uint32_t bg_color = 0x00FFFFFF);

           // Draw a small anti-aliased filled circle at ax,ay with radius r (uses drawWideLine)
           // If bg_color is not included the background pixel colour will be read from TFT or sprite
  void     drawSpot(float ax, float ay, float r, uint32_t fg_color, uint32_t bg_color = 0x00FFFFFF);

           // Draw an anti-aliased wide line from ax,ay to bx,by width wd with radiused ends (radius is wd/2)
           // If bg_color is not included the background pixel colour will be read from TFT or sprite
  void     drawWideLine(float ax, float ay, float bx, float by, float wd, uint32_t fg_color, uint32_t bg_color = 0x00FFFFFF);

           // Draw an anti-aliased wide line from ax,ay to bx,by with different width at each end aw, bw and with radiused ends
           // If bg_color is not included the background pixel colour will be read from TFT or sprite
  void     drawWedgeLine(float ax, float ay, float bx, float by, float aw, float bw, uint32_t fg_color, uint32_t bg_color = 0x00FFFFFF);


  // Image rendering
           // Swap the byte order for pushImage() and pushPixels() - corrects endianness
  void     setSwapBytes(bool swap);
  bool     getSwapBytes(void);

           // Draw bitmap
  void     drawBitmap( int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t fgcolor),
           drawBitmap( int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t fgcolor, uint16_t bgcolor),
           drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t fgcolor),
           drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t fgcolor, uint16_t bgcolor),
           setBitmapColor(uint16_t fgcolor, uint16_t bgcolor); // Define the 2 colours for 1bpp sprites

           // Set TFT pivot point (use when rendering rotated sprites)
  void     setPivot(int16_t x, int16_t y);
  int16_t  getPivotX(void), // Get pivot x
           getPivotY(void); // Get pivot y

           // The next functions can be used as a pair to copy screen blocks (or horizontal/vertical lines) to another location
           // Read a block of pixels to a data buffer, buffer is 16-bit and the size must be at least w * h
  void     readRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data);
           // Write a block of pixels to the screen which have been read by readRect()
  void     pushRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data);

           // These are used to render images or sprites stored in RAM arrays (used by Sprite class for 16bpp Sprites)
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data);
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data, uint16_t transparent);

           // These are used to render images stored in FLASH (PROGMEM)
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data, uint16_t transparent);
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data);

           // These are used by Sprite class pushSprite() member function for 1, 4 and 8 bits per pixel (bpp) colours
           // They are not intended to be used with user sketches (but could be)
           // Set bpp8 true for 8bpp sprites, false otherwise. The cmap pointer must be specified for 4bpp
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t  *data, bool bpp8 = true, uint16_t *cmap = nullptr);
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t  *data, uint8_t  transparent, bool bpp8 = true, uint16_t *cmap = nullptr);
           // FLASH version
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *data, bool bpp8,  uint16_t *cmap = nullptr);

           // Render a 16-bit colour image with a 1bpp mask
  void     pushMaskedImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *img, uint8_t *mask);

           // This next function has been used successfully to dump the TFT screen to a PC for documentation purposes
           // It reads a screen area and returns the 3 RGB 8-bit colour values of each pixel in the buffer
           // Set w and h to 1 to read 1 pixel's colour. The data buffer must be at least w * h * 3 bytes
  void     readRectRGB(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data);


  // Text uses the MonoFont selected by setFont(); all coordinates are pixels.
  int16_t drawNumber(long value, int32_t x, int32_t y);
  int16_t drawFloat(float value, uint8_t decimal, int32_t x, int32_t y);
  int16_t drawString(const char *string, int32_t x, int32_t y);
#if NICE_GFX_NATIVE_IDF
  int16_t drawString(const std::string &string, int32_t x, int32_t y);
#else
  int16_t drawString(const String &string, int32_t x, int32_t y);
#endif
  void setCursor(int16_t x, int16_t y);
  int16_t getCursorX(void), getCursorY(void);
  void setTextColor(uint16_t color);
  void setTextColor(uint16_t foreground, uint16_t background);
  void setTextSize(uint8_t size);
  void setTextWrap(bool wrapX, bool wrapY = false);
  void setTextDatum(uint8_t datum);
  uint8_t getTextDatum(void);
  void setTextPadding(uint16_t width);
  uint16_t getTextPadding(void);
  // No font is implicit. nullptr clears the selection and disables text drawing.
  void setFont(const nice_gfx::MonoFont *font);
  void setFont(const nice_gfx::MonoFont &font) { setFont(&font); }
  const nice_gfx::MonoFont *getFont() const { return _monoFont; }
  int16_t textWidth(const char *string);
#if NICE_GFX_NATIVE_IDF
  int16_t textWidth(const std::string &string);
#else
  int16_t textWidth(const String &string);
#endif
  int16_t fontHeight(void);
  uint16_t decodeUTF8(uint8_t *buf, uint16_t *index, uint16_t remaining);
  uint16_t decodeUTF8(uint8_t c);
  using nice_gfx::PrintBase::write;
  size_t write(uint8_t) override;

  // Low level read/write
  void     writecommand(uint16_t c);  // Send an 8-bit command, function resets DC/RS high ready for data
  void     writecommand(uint16_t c, const uint8_t *data, uint32_t len); // Send command and parameters atomically
  void     writeRegister8(uint16_t address, uint8_t value);
  void     writeRegister16(uint16_t address, uint16_t value);
  void     writedata(uint8_t d);     // Send data with DC/RS set high

  void     commandList(const uint8_t *addr); // Send a initialisation sequence to TFT stored in FLASH

  uint8_t  readcommand8(uint16_t cmd_function, uint8_t index = 0); // read 8 bits from TFT
  uint16_t readcommand16(uint16_t cmd_function, uint8_t index = 0); // read 16 bits from TFT
  uint32_t readcommand32(uint16_t cmd_function, uint8_t index = 0); // read 32 bits from TFT


  // Colour conversion
           // Convert 8-bit red, green and blue to 16 bits
  uint16_t color565(uint8_t red, uint8_t green, uint8_t blue);

           // Convert 8-bit colour to 16 bits
  uint16_t color8to16(uint8_t color332);
           // Convert 16-bit colour to 8 bits
  uint8_t  color16to8(uint16_t color565);

           // Convert 16-bit colour to/from 24-bit, R+G+B concatenated into LS 24 bits
  uint32_t color16to24(uint16_t color565);
  uint32_t color24to16(uint32_t color888);

           // Alpha blend 2 colours, see generic "alphaBlend_Test" example
           // alpha =   0 = 100% background colour
           // alpha = 255 = 100% foreground colour
  uint16_t alphaBlend(uint8_t alpha, uint16_t fgc, uint16_t bgc);

           // 16-bit colour alphaBlend with alpha dither (dither reduces colour banding)
  uint16_t alphaBlend(uint8_t alpha, uint16_t fgc, uint16_t bgc, uint8_t dither);
           // 24-bit colour alphaBlend with optional alpha dither
  uint32_t alphaBlend24(uint8_t alpha, uint32_t fgc, uint32_t bgc, uint8_t dither = 0);

  // DMA capability belongs to the selected transport. Sources remain immutable
  // and must remain alive and unchanged until dmaWait() or endWrite().
  bool     initDMA();  // Enable asynchronous submissions when supported.
  void     deInitDMA(void);   // Drain pending transfers and disable asynchronous image submissions.

  // Clipping preserves source stride. Optional buffer receives the clipped copy.
  void     pushImageDMA(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t* data, uint16_t* buffer = nullptr);

           // For case where pointer is a const and the image data must not be modified (clipped or byte swapped)
  void     pushImageDMA(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t const* data);

           // Push a block of pixels into a window set up using setAddrWindow()
  void     pushPixelsDMA(uint16_t* image, uint32_t len);

           // Poll completion without blocking; use dmaWait() to wait.
  bool     dmaBusy(void); // returns true if DMA is still in progress
  void     dmaWait(void); // wait until DMA is complete

  bool     DMA_Enabled = false;   // Flag for DMA enabled state

  // Bare metal functions
  void     startWrite(void);                         // Begin SPI transaction
  void     endWrite(void);                           // End SPI transaction

  // Set/get an arbitrary library configuration attribute or option
  //       Use to switch ON/OFF capabilities such as UTF8 decoding - each attribute has a unique ID
  //       id = 0: reserved - may be used in future to reset all attributes to a default state
  //       id = 2: Turn on (a=true) or off (a=false) UTF8 decoding
  //       id = 3: Enable or disable use of ESP32 PSRAM (if available)
           #define UTF8_SWITCH  2
           #define PSRAM_ENABLE 3
  void     setAttribute(uint8_t id = 0, uint8_t a = 0); // Set attribute value
  uint8_t  getAttribute(uint8_t id = 0);                // Get attribute value

  // Controller, dimensions, GPIOs and transport capabilities of this object.
  DisplayInfo displayInfo() const;
  // RGB pixel clock change; other transports return Unsupported.
  nice_gfx::Status setPixelClockHz(uint32_t hz);

  // Global variables
  uint32_t textcolor, textbgcolor;         // Text foreground and background colours

  uint32_t bitmap_fg, bitmap_bg;           // Bitmap foreground (bit=1) and background (bit=0) colours

  uint8_t  textsize,  // Current font size multiplier
           textdatum, // Text reference datum
           rotation;  // Display rotation (0-3)

  uint8_t  decoderState = 0;   // UTF8 decoder state        - not for user access
  uint32_t decoderBuffer = 0;  // Includes non-BMP sequences before replacement

 //--------------------------------------- private ------------------------------------//
 private:
  const nice_gfx::MonoFont *_monoFont = nullptr;
  bool _antialiasing = true;

  nice_gfx::Panel *_panel = nullptr; // Internal reference; the concrete display owns its panel and transport.
  nice_gfx::Status _runtimeStatus = nice_gfx::Status::NotInitialized;
  nice_gfx::TouchController *_touch = nullptr;
  inline void beginPanelWrite();
  inline void endPanelWrite();
  void appendPixel(uint16_t color);

           // Smooth graphics helper
  uint8_t  sqrt_fraction(uint32_t num);

           // Helper function: calculate distance of a point from a finite length line between two points
  float    wedgeLineDistance(float pax, float pay, float bax, float bay, float dr);

  bool     locked, inTransaction;
  uint32_t _writeDepth = 0; // Explicit user batches can nest with Sprite transfers.

 //-------------------------------------- protected ----------------------------------//
 protected:
  // Nesting preserves an enclosing draw operation or the application's batch.
  // A scope also closes on early return, so clipping cannot leave a batch open.
  class WriteScope {
   public:
    explicit WriteScope(NICE_GFX &target) : target_(target), previous_(target.inTransaction) {
      target_.begin_nin_write();
      target_.inTransaction = true;
    }
    ~WriteScope() {
      target_.inTransaction = previous_;
      target_.end_nin_write();
    }
   private:
    NICE_GFX &target_;
    bool previous_;
    WriteScope(const WriteScope &) = delete;
    WriteScope &operator=(const WriteScope &) = delete;
  };
  explicit NICE_GFX(nice_gfx::Panel &panel); // Internal facade binding only.
  NICE_GFX(int16_t width, int16_t height); // Offscreen renderer, used by Sprite.

  // Font decoding is shared; virtual fillRect supplies the memory span target.
  // Packed Sprite colors are indices/bits instead of RGB565 blend colors.
  virtual bool supportsTextCoverage() const { return true; }
  // Absolute output bounds may be smaller than a caller's logical viewport.
  // Tile targets expose their borrowed region before font samples are decoded.
  virtual bool textRenderClip(int32_t &left,int32_t &top,int32_t &right,int32_t &bottom) const {
    left=_vpX;top=_vpY;right=_vpW;bottom=_vpH;
    return !_vpOoB && right>left && bottom>top;
  }
  // Absolute rectangles have already been clipped by the shared glyph
  // renderer. Memory targets can consume a span without repeating viewport
  // clipping or dispatching readPixel/drawPixel for every coverage sample.
  // Coverage 15 stores color; lower coverage blends against each destination.
  // Returning false retains the ordinary display/Sprite fallback, including
  // readback errors and write-only display handling.
  virtual bool textRenderSpan(int32_t, int32_t, int32_t, int32_t, uint16_t, uint8_t) {
    return false;
  }
  void renderTextGlyph(const nice_gfx::TextGlyph &glyph, int32_t x, int32_t y,
      uint32_t foreground, uint32_t background, uint8_t scale, bool opaque,
      uint16_t targetWidth = 0, uint16_t targetHeight = 0);

  int32_t  _init_width, _init_height; // Display w/h as input, used by setRotation()
  int32_t  _width, _height;           // Display w/h as modified by current rotation


  int16_t  _xPivot;   // TFT x pivot point coordinate for rotated Sprites
  int16_t  _yPivot;   // TFT x pivot point coordinate for rotated Sprites

  // Viewport variables
  int32_t  _vpX, _vpY, _vpW, _vpH;    // Note: x start, y start, x end + 1, y end + 1
  int32_t  _xDatum;
  int32_t  _yDatum;
  int32_t  _xWidth;
  int32_t  _yHeight;
  bool     _vpDatum;
  bool     _vpOoB;

  int32_t  cursor_x, cursor_y, padX;       // Text cursor x,y and padding setting



  bool     textwrapX, textwrapY;  // If set, 'wrap' text at right and optionally bottom edge of display
  bool     _swapBytes; // Swap the byte order for TFT pushImage()

                       // User sketch manages these via set/getAttribute()
  bool     _utf8;         // If set, use UTF-8 decoder in print stream 'write()' function (default ON)
  bool     _psram_enable; // Enable PSRAM use for library functions (TBD) and Sprites




/***************************************************************************************
**                         Section 9: NICE_GFX class conditional extensions
***************************************************************************************/
 public:
  void attachTouch(nice_gfx::TouchController &touch) { _touch = &touch; }
  uint8_t getTouchRaw(uint16_t *x, uint16_t *y);
  uint16_t getTouchRawZ();
  uint8_t getTouch(uint16_t *x, uint16_t *y, uint16_t threshold = 600);
  void convertRawXY(uint16_t *x, uint16_t *y);
  void setTouch(const uint16_t *data);
  void calibrateTouch(uint16_t *data, uint32_t foreground, uint32_t background, uint8_t size);
  nice_gfx::Status calibrateTouchChecked(uint16_t *data, uint32_t foreground,
      uint32_t background, uint8_t size = 8, uint32_t timeoutMs = 30000);


}; // End of class NICE_GFX

inline nice_gfx::Panel *nice_gfx::detail::DisplayAccess::panel(NICE_GFX &display) {
  return display._panel;
}

// Swap any type
template <typename T> static inline void
transpose(T& a, T& b) { T t = a; a = b; b = t; }

// Fast alphaBlend
template <typename A, typename F, typename B> static inline uint16_t
fastBlend(A alpha, F fgc, B bgc)
{
  // Split out and blend 5-bit red and blue channels
  uint32_t rxb = bgc & 0xF81F;
  rxb += ((fgc & 0xF81F) - rxb) * (alpha >> 2) >> 6;
  // Split out and blend 6-bit green channel
  uint32_t xgx = bgc & 0x07E0;
  xgx += ((fgc & 0x07E0) - xgx) * alpha >> 8;
  // Recombine channels
  return (rxb & 0xF81F) | (xgx & 0x07E0);
}


#endif
