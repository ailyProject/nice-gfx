// Viewport Demo

// See viewport_commands tab for details of functions available

// This example uses the viewport commands to create a "virtual TFT" within the
// normal TFT display area. This allows a sketch written for a smaller screen to
// be run in a viewport window. By default, the graphics 0,0 datum is set to the
// top left corner of the viewport, but optionally the datum can be kept at the
// corner of the TFT.

// Viewports have a number of potential uses:
// -  create a "virtual" TFT screen smaller than the actual TFT screen
//  - render GUI items (menus etc.) in a viewport, erase GUI item by redrawing whole screen,
//    this will be fast because only the viewport will be refreshed (e.g. clearing menu)
//  - limit screen refresh to a particular area, e.g. changing numbers, icons or graph plotting
//  - showing a small portion of a larger image or sprite, this allows panning and scrolling

// A viewport can have the coordinate datum (position 0,0) either at the top left corner of
// the viewport or at the normal top left corner of the TFT.
// Putting the coordinate datum at the viewport corner means that functions that draw graphics
// in a fixed position can be relocated anywhere on the screen. (see plotBox() below). This
// makes it easier to reposition groups of graphical objects (for example GUI buttons) that have
// fixed relative positions.

#include <SPI.h>
#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include <Display/Presets/ExampleBoard.h>

ExampleDisplay gfx;

void setup() {
  Serial.begin(115200);

  gfx.begin();
  gfx.setRotation(1);
  gfx.fillScreen(GFX_BLACK);
}

void loop()
{
  // Normal Screen
  drawX();

  delay(2000);

  // Viewport screen
  gfx.setViewport(10, 10, 140, 100);
  gfx.frameViewport(GFX_NAVY, -2);
  gfx.fillScreen(GFX_BLACK);
  drawX();
  gfx.resetViewport();

  delay(2000);

  //Normal screen
  gfx.fillScreen(GFX_BLACK);
  drawX();

  delay(2000);

  gfx.fillScreen(GFX_BLACK);

  // Viewport as a clipping window (false parameter means coordinate datum stays at TFT top left)
  gfx.setViewport(10, 10, gfx.width()/2 - 10, gfx.height() - 20, false);
  //gfx.frameViewport(GFX_NAVY,  2); // Add 2 pixel border inside viewport
  //gfx.frameViewport(GFX_NAVY, -2); // Add 2 pixel border outside viewport
  drawX();

  delay(2000);

  while(1)
  {
    gfx.resetViewport(); // Reset viewport so width() and height() return TFT size

    uint16_t w = 40;
    uint16_t h = 40;
    uint16_t x = random(gfx.width()  - w);
    uint16_t y = random(gfx.height() - h);

    gfx.setViewport(x, y, w, h);

    plotBox();

    delay(0);
  }
}

void drawX(void)
{
  gfx.fillScreen(gfx.color565(25,25,25)); // Grey

  // Draw circle
  gfx.drawCircle(gfx.width()/2, gfx.height()/2, gfx.width()/4, GFX_RED);

  // Draw diagonal lines
  gfx.drawLine(0 ,                0, gfx.width()-1, gfx.height()-1, GFX_GREEN);
  gfx.drawLine(0 , gfx.height()-1, gfx.width()-1,                0, GFX_BLUE);

  gfx.setTextDatum(MC_DATUM);
  gfx.setTextColor(GFX_WHITE, gfx.color565(25,25,25));
  nice_gfx::text::draw(gfx, "Hello World!", gfx.width()/2, gfx.height()/2, nice_gfx::fonts::Spleen12x24, 1); // Spleen12x24
}

void plotBox(void)
{
  // These are always plotted at a fixed position but they can 
  // be plotted into a viewport anywhere on the screen because
  // a viewport can move the screen datum
  gfx.fillScreen(GFX_BLACK); // When a viewport is set, this just fills the viewport
  gfx.drawRect(0,0, 40,40, GFX_BLUE);
  gfx.setTextDatum(MC_DATUM);
  gfx.setTextColor(GFX_WHITE);
  nice_gfx::text::number(gfx, random(100), 20, 23, nice_gfx::fonts::Spleen12x24, 1); // Number in Spleen12x24
}
