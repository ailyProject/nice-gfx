/*
 An example digital clock using a TFT LCD screen to show the time.
 Demonstrates use of the font printing routines. (Time updates but date does not.)

 It uses the time of compile/upload to set the time
 For a more accurate clock, it would be better to use the RTClib library.
 But this is just a demo...

 Make sure all the display driver and pin connections are correct by
 editing the Display/Presets/ExampleBoard.h file in the NICE_GFX library folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE Display/Presets/ExampleBoard.h FILE IN THE LIBRARY ######
 #########################################################################

 Based on clock sketch by Gilchrist 6/2/2014 1.0

A few colour codes:

code	color
0x0000	Black
0xFFFF	White
0xBDF7	Light Gray
0x7BEF	Dark Gray
0xF800	Red
0xFFE0	Yellow
0xFBE0	Orange
0x79E0	Brown
0x7E0	Green
0x7FF	Cyan
0x1F	Blue
0xF81F	Pink

 */

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include <Display/Presets/ExampleBoard.h>
#include <SPI.h>

#define GFX_GREY 0x5AEB

ExampleDisplay gfx;       // Invoke custom library

uint32_t targetTime = 0;                    // for next 1 second timeout

static uint8_t conv2d(const char* p); // Forward declaration needed for IDE 1.6.x

uint8_t hh = conv2d(__TIME__), mm = conv2d(__TIME__ + 3), ss = conv2d(__TIME__ + 6); // Get H, M, S from compile time

byte omm = 99, oss = 99;
byte xcolon = 0, xsecs = 0;
unsigned int colour = 0;

void setup(void) {
  //Serial.begin(115200);
  gfx.begin();
  gfx.setRotation(1);
  gfx.fillScreen(GFX_BLACK);

  gfx.setTextSize(1);
  gfx.setTextColor(GFX_YELLOW, GFX_BLACK);

  targetTime = millis() + 1000;
}

void loop() {
  if (targetTime < millis()) {
    // Set next update for 1 second later
    targetTime = millis() + 1000;

    // Adjust the time values by adding 1 second
    ss++;              // Advance second
    if (ss == 60) {    // Check for roll-over
      ss = 0;          // Reset seconds to zero
      omm = mm;        // Save last minute time for display update
      mm++;            // Advance minute
      if (mm > 59) {   // Check for roll-over
        mm = 0;
        hh++;          // Advance hour
        if (hh > 23) { // Check for 24hr roll-over (could roll-over on 13)
          hh = 0;      // 0 for 24 hour clock, set to 1 for 12 hour clock
        }
      }
    }


    // Update digital time
    int xpos = 0;
    int ypos = 85; // Top left corner ot clock text, about half way down
    int ysecs = ypos + 24;

    if (omm != mm) { // Redraw hours and minutes time every minute
      omm = mm;
      // Draw hours and minutes
      if (hh < 10) xpos += nice_gfx::text::glyph(gfx, '0', xpos, ypos, nice_gfx::fonts::Spleen12x24, 3); // Add hours leading zero for 24 hr clock
      xpos += nice_gfx::text::number(gfx, hh, xpos, ypos, nice_gfx::fonts::Spleen12x24, 3);             // Draw hours
      xcolon = xpos; // Save colon coord for later to flash on/off later
      xpos += nice_gfx::text::glyph(gfx, ':', xpos, ypos - 8, nice_gfx::fonts::Spleen12x24, 3);
      if (mm < 10) xpos += nice_gfx::text::glyph(gfx, '0', xpos, ypos, nice_gfx::fonts::Spleen12x24, 3); // Add minutes leading zero
      xpos += nice_gfx::text::number(gfx, mm, xpos, ypos, nice_gfx::fonts::Spleen12x24, 3);             // Draw minutes
      xsecs = xpos; // Sae seconds 'x' position for later display updates
    }
    if (oss != ss) { // Redraw seconds time every second
      oss = ss;
      xpos = xsecs;

      if (ss % 2) { // Flash the colons on/off
        gfx.setTextColor(0x39C4, GFX_BLACK);        // Set colour to grey to dim colon
        nice_gfx::text::glyph(gfx, ':', xcolon, ypos - 8, nice_gfx::fonts::Spleen12x24, 3);     // Hour:minute colon
        xpos += nice_gfx::text::glyph(gfx, ':', xsecs, ysecs, nice_gfx::fonts::Spleen12x24, 2); // Seconds colon
        gfx.setTextColor(GFX_YELLOW, GFX_BLACK);    // Set colour back to yellow
      }
      else {
        nice_gfx::text::glyph(gfx, ':', xcolon, ypos - 8, nice_gfx::fonts::Spleen12x24, 3);     // Hour:minute colon
        xpos += nice_gfx::text::glyph(gfx, ':', xsecs, ysecs, nice_gfx::fonts::Spleen12x24, 2); // Seconds colon
      }

      //Draw seconds
      if (ss < 10) xpos += nice_gfx::text::glyph(gfx, '0', xpos, ysecs, nice_gfx::fonts::Spleen12x24, 2); // Add leading zero
      nice_gfx::text::number(gfx, ss, xpos, ysecs, nice_gfx::fonts::Spleen12x24, 2);                     // Draw seconds
    }
  }
}


// Function to extract numbers from compile time string
static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}



