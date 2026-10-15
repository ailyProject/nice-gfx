/*
 An example analogue clock using a TFT LCD screen to show the time
 use of some of the drawing commands with the ST7735 library.

 For a more accurate clock, it would be better to use the RTClib library.
 But this is just a demo. 

 Uses compile time to set the time so a reset will start with the compile time again
 
 Gilchrist 6/2/2014 1.0
 Updated by Bodmer
 */

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include <Display/Presets/ExampleBoard.h>
#include <SPI.h>

ExampleDisplay gfx;  // Invoke library, pins in Display/Presets/ExampleBoard.h

#define GFX_GREY 0xBDF7

float sx = 0, sy = 1, mx = 1, my = 0, hx = -1, hy = 0;    // Saved H, M, S x & y multipliers
float sdeg=0, mdeg=0, hdeg=0;
uint16_t osx=64, osy=64, omx=64, omy=64, ohx=64, ohy=64;  // Saved H, M, S x & y coords
uint16_t x0=0, x1=0, yy0=0, yy1=0;
uint32_t targetTime = 0;                    // for next 1 second timeout

static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}

uint8_t hh=conv2d(__TIME__), mm=conv2d(__TIME__+3), ss=conv2d(__TIME__+6);  // Get H, M, S from compile time

bool initial = 1;

void setup(void) {
  gfx.begin();
  gfx.setRotation(0);
  gfx.fillScreen(GFX_GREY);
  gfx.setTextColor(GFX_GREEN, GFX_GREY);  // Adding a black background colour erases previous text automatically
  
  // Draw clock face
  gfx.fillCircle(64, 64, 61, GFX_BLUE);
  gfx.fillCircle(64, 64, 57, GFX_BLACK);

  // Draw 12 lines
  for(int i = 0; i<360; i+= 30) {
    sx = cos((i-90)*0.0174532925);
    sy = sin((i-90)*0.0174532925);
    x0 = sx*57+64;
    yy0 = sy*57+64;
    x1 = sx*50+64;
    yy1 = sy*50+64;

    gfx.drawLine(x0, yy0, x1, yy1, GFX_BLUE);
  }

  // Draw 60 dots
  for(int i = 0; i<360; i+= 6) {
    sx = cos((i-90)*0.0174532925);
    sy = sin((i-90)*0.0174532925);
    x0 = sx*53+64;
    yy0 = sy*53+64;
    
    gfx.drawPixel(x0, yy0, GFX_BLUE);
    if(i==0 || i==180) gfx.fillCircle(x0, yy0, 1, GFX_CYAN);
    if(i==0 || i==180) gfx.fillCircle(x0+1, yy0, 1, GFX_CYAN);
    if(i==90 || i==270) gfx.fillCircle(x0, yy0, 1, GFX_CYAN);
    if(i==90 || i==270) gfx.fillCircle(x0+1, yy0, 1, GFX_CYAN);
  }

  gfx.fillCircle(65, 65, 3, GFX_RED);

  // Draw text at position 64,125 using fonts 4
  nice_gfx::text::center(gfx, "Time flies", 64, 130, nice_gfx::fonts::Spleen12x24, 1);

  targetTime = millis() + 1000; 
}

void loop() {
  if (targetTime < millis()) {
    targetTime = millis()+1000;
    ss++;              // Advance second
    if (ss==60) {
      ss=0;
      mm++;            // Advance minute
      if(mm>59) {
        mm=0;
        hh++;          // Advance hour
        if (hh>23) {
          hh=0;
        }
      }
    }

    // Pre-compute hand degrees, x & y coords for a fast screen update
    sdeg = ss*6;                  // 0-59 -> 0-354
    mdeg = mm*6+sdeg*0.01666667;  // 0-59 -> 0-360 - includes seconds
    hdeg = hh*30+mdeg*0.0833333;  // 0-11 -> 0-360 - includes minutes and seconds
    hx = cos((hdeg-90)*0.0174532925);    
    hy = sin((hdeg-90)*0.0174532925);
    mx = cos((mdeg-90)*0.0174532925);    
    my = sin((mdeg-90)*0.0174532925);
    sx = cos((sdeg-90)*0.0174532925);    
    sy = sin((sdeg-90)*0.0174532925);

    if (ss==0 || initial) {
      initial = 0;
      // Erase hour and minute hand positions every minute
      gfx.drawLine(ohx, ohy, 65, 65, GFX_BLACK);
      ohx = hx*33+65;    
      ohy = hy*33+65;
      gfx.drawLine(omx, omy, 65, 65, GFX_BLACK);
      omx = mx*44+65;    
      omy = my*44+65;
    }

      // Redraw new hand positions, hour and minute hands not erased here to avoid flicker
      gfx.drawLine(osx, osy, 65, 65, GFX_BLACK);
      gfx.drawLine(ohx, ohy, 65, 65, GFX_WHITE);
      gfx.drawLine(omx, omy, 65, 65, GFX_WHITE);
      osx = sx*47+65;    
      osy = sy*47+65;
      gfx.drawLine(osx, osy, 65, 65, GFX_RED);

    gfx.fillCircle(65, 65, 3, GFX_RED);
  }
}


