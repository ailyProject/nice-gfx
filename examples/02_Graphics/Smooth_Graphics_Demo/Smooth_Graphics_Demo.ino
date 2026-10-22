// Sketch to demonstrate smooth (anti-aliased) graphics functions:
// Smooth graphics result in less pixel resolution jaggedness.

#include <NICE_GFX.h>
#include "Hardware.h"

ExampleDisplay gfx;  // Invoke library, pins in Hardware.h

GFX_Sprite spr = GFX_Sprite(&gfx);

// =========================================================================
// Setup
// =========================================================================
void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");

  // Initialise the screen
  gfx.begin();

  // Ideally set orientation for good viewing angle range because
  // the anti-aliasing effectiveness varies with screen viewing angle
  gfx.setRotation(0);

  gfx.fillScreen(GFX_BLACK);

  // Small sprite for spot demo
  spr.createSprite(23, 23);
}

// =========================================================================
// Loop
// =========================================================================
void loop() {

  // drawSpot is for small anti-aliased circles, coordinates and radius are
  // floating point to allow sub-pixel positioning (large circles will
  // be slow to draw). Use fillSmoothCircle() for large circles.
  // In this case black is the background colour for the anti-aliasing
  float x = 10.5;
  float y = 10.5;
  float r = 8.6;
  gfx.drawSpot(x, y, r, GFX_WHITE, GFX_BLACK);

  // Fill sprite with a colour
  spr.fillSprite(GFX_RED);
  // Draw spot in sprite, the background colour is omitted so function
  // reads background colour for aliasing. (To use this method with direct write
  // to TFT (gfx.drawSpot...) requires the capability to read data from the TFT!)
  spr.drawSpot(x, y, r, GFX_WHITE);
  spr.pushSprite(21, 0);


  // Draw a segmented ring meter type display
  // Centre of screen
  int cx = gfx.width()  / 2;
  int cy = gfx.height() / 2;

  // Inner and outer radius of ring
  float r1 = min(cx, cy) - 40.0;
  float r2 = min(cx, cy) - 10.0;

  // Inner and outer line width
  int w1 = r1 / 25;
  int w2 = r2 / 20;

  // The following will be updated by the getCoord function
  float px1 = 0.0;
  float py1 = 0.0;
  float px2 = 0.0;
  float py2 = 0.0;

  // Wedge line function, an anti-aliased wide line between 2 points, with different
  // line widths at the two ends. Background colour is black.
  for (int angle = -130; angle <= 130; angle += 10) {
    getCoord(cx, cy, &px1, &py1, &px2, &py2, r1, r2, angle);
    uint16_t colour = rainbow(map(angle, -130, 130, 0, 127));
    if (angle > 45) colour = GFX_DARKGREY;
    gfx.drawWedgeLine(px1, py1, px2, py2, w1, w2, colour, GFX_BLACK);
  }

  // Smooth dark red filled circle
  gfx.fillSmoothCircle(cx, cy, r1 - 8, GFX_MAROON, GFX_BLACK);

  // Draw a white dial pointer using wedge line function
  getCoord(cx, cy, &px1, &py1, &px2, &py2, 0, r1 - 10, 45);
  // Magenta wedge line pointer on red background
  // Line tapers from radius 5 to zero
  gfx.drawWedgeLine(cx, cy, px2, py2, 5, 0, GFX_WHITE, GFX_MAROON);

  delay(5000);

  // Test wideLine function
  gfx.fillScreen(GFX_BLACK);

  // Line width
  int wd = 5;

  // Screen limits
  int w = gfx.width() - wd;
  int h = gfx.height() - wd;

  // Line end coords
  int x1 = w - 1;
  int x2 = w - 1;
  int y1 = h - 1;
  int y2 = wd;

  for (x2 = wd; x2 < w; x2 += wd * 3) gfx.drawWideLine(x1, y1, x2, y2, wd, GFX_WHITE, GFX_BLACK);

  x2    = wd;
  for (y2 = wd; y2 < h; y2 += wd * 4) gfx.drawWideLine(x1, y1, x2, y2, wd, GFX_WHITE, GFX_BLACK);

  delay(5000);

  // Demo filled smooth rounded rectangle
  gfx.fillScreen(GFX_BLACK);

  x1 = 30;
  y1 = 30;
  w = gfx.width() - 2 * x1;
  h = gfx.height() - 2 * y1;
  int rad = 30;

  gfx.fillSmoothRoundRect(x1, y1, w, h, rad, GFX_CYAN, GFX_BLACK);

  // Wait forever
  while (1) delay(100);
}


// =========================================================================
// Get coordinates of two ends of a line from r1 to r2, pivot at x,y, angle a
// =========================================================================
// Coordinates are returned to caller via the xp and yp pointers
#define DEG2RAD 0.0174532925
void getCoord(int16_t x, int16_t y, float *xp1, float *yp1, float *xp2, float *yp2, int16_t r1, int16_t r2, float a)
{
  float sx = cos( (a - 90) * DEG2RAD);
  float sy = sin( (a - 90) * DEG2RAD);
  *xp1 =  sx * r1 + x;
  *yp1 =  sy * r1 + y;
  *xp2 =  sx * r2 + x;
  *yp2 =  sy * r2 + y;
}

// =========================================================================
// Return a 16-bit rainbow colour
// =========================================================================
unsigned int rainbow(byte value)
{
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to 127 = red

  byte red = 0; // Red is the top 5 bits of a 16-bit colour value
  byte green = 0;// Green is the middle 6 bits
  byte blue = 0; // Blue is the bottom 5 bits

  byte quadrant = value / 32;

  if (quadrant == 0) {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1) {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2) {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3) {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}
