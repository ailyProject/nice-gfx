/*
 An example analogue meter using a ST7735 TFT LCD screen

 This example uses the hardware SPI only
 Needs GfxTerm8x16 (also Spleen12x24 if using large scale label)

 Updated by Bodmer for variable meter size
*/

// Define meter size
#define M_SIZE 0.667

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include <Display/Presets/ExampleBoard.h>
#include <SPI.h>

ExampleDisplay gfx;       // Invoke custom library

#define GFX_GREY 0x5AEB
#define GFX_ORANGE      0xFD20      /* 255, 165,   0 */

float ltx = 0;    // Saved x coord of bottom of needle
uint16_t osx = M_SIZE*120, osy = M_SIZE*120; // Saved x & y coords
uint32_t updateTime = 0;       // time for next update

int old_analog =  -999; // Value last displayed

int value[6] = {0, 0, 0, 0, 0, 0};
int old_value[6] = { -1, -1, -1, -1, -1, -1};
int d = 0;

void setup(void) {
  Serial.begin(57600); // For debug
  gfx.begin();
  gfx.setRotation(3);

  gfx.fillScreen(GFX_BLACK);

  analogMeter(); // Draw analogue meter

  updateTime = millis(); // Next update time
}


void loop() {
  if (updateTime <= millis()) {
    updateTime = millis() + 35; // Update meter every 35 milliseconds
 
    // Create a Sine wave for testing
    d += 4; if (d >= 360) d = 0;
    value[0] = 50 + 50 * sin((d + 0) * 0.0174532925);
    //value[0] = random(0,100);
    //unsigned long tt = millis();
    plotNeedle(value[0], 0); // It takes between 2 and 14ms to replot the needle with zero delay
    //Serial.println(millis()-tt);
  }
}


// #########################################################################
//  Draw the analogue meter on the screen
// #########################################################################
void analogMeter()
{

  // Meter outline
  gfx.fillRect(0, 0, M_SIZE*239, M_SIZE*131, GFX_GREY);
  gfx.fillRect(1, M_SIZE*3, M_SIZE*234, M_SIZE*125, GFX_WHITE);

  gfx.setTextColor(GFX_BLACK);  // Text colour

  // Draw ticks every 5 degrees from -50 to +50 degrees (100 deg. FSD swing)
  for (int i = -50; i < 51; i += 5) {
    // Long scale tick length
    int tl = 15;

    // Coordinates of tick to draw
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (M_SIZE*100 + tl) + M_SIZE*120;
    uint16_t y0 = sy * (M_SIZE*100 + tl) + M_SIZE*150;
    uint16_t x1 = sx * M_SIZE*100 + M_SIZE*120;
    uint16_t y1 = sy * M_SIZE*100 + M_SIZE*150;

    // Coordinates of next tick for zone fill
    float sx2 = cos((i + 5 - 90) * 0.0174532925);
    float sy2 = sin((i + 5 - 90) * 0.0174532925);
    int x2 = sx2 * (M_SIZE*100 + tl) + M_SIZE*120;
    int y2 = sy2 * (M_SIZE*100 + tl) + M_SIZE*150;
    int x3 = sx2 * M_SIZE*100 + M_SIZE*120;
    int y3 = sy2 * M_SIZE*100 + M_SIZE*150;

    // Yellow zone limits
    //if (i >= -50 && i < 0) {
    //  gfx.fillTriangle(x0, y0, x1, y1, x2, y2, GFX_YELLOW);
    //  gfx.fillTriangle(x1, y1, x2, y2, x3, y3, GFX_YELLOW);
    //}

    // Green zone limits
    if (i >= 0 && i < 25) {
      gfx.fillTriangle(x0, y0, x1, y1, x2, y2, GFX_GREEN);
      gfx.fillTriangle(x1, y1, x2, y2, x3, y3, GFX_GREEN);
    }

    // Orange zone limits
    if (i >= 25 && i < 50) {
      gfx.fillTriangle(x0, y0, x1, y1, x2, y2, GFX_ORANGE);
      gfx.fillTriangle(x1, y1, x2, y2, x3, y3, GFX_ORANGE);
    }

    // Short scale tick length
    if (i % 25 != 0) tl = 8;

    // Recalculate coords incase tick lenght changed
    x0 = sx * (M_SIZE*100 + tl) + M_SIZE*120;
    y0 = sy * (M_SIZE*100 + tl) + M_SIZE*150;
    x1 = sx * M_SIZE*100 + M_SIZE*120;
    y1 = sy * M_SIZE*100 + M_SIZE*150;

    // Draw tick
    gfx.drawLine(x0, y0, x1, y1, GFX_BLACK);

    // Check if labels should be drawn, with position tweaks
    if (i % 25 == 0) {
      // Calculate label positions
      x0 = sx * (M_SIZE*100 + tl + 10) + M_SIZE*120;
      y0 = sy * (M_SIZE*100 + tl + 10) + M_SIZE*150;
      switch (i / 25) {
        case -2: nice_gfx::text::center(gfx, "0", x0+4, y0-4, nice_gfx::fonts::TomThumb4x6, 1); break;
        case -1: nice_gfx::text::center(gfx, "25", x0+2, y0, nice_gfx::fonts::TomThumb4x6, 1); break;
        case 0: nice_gfx::text::center(gfx, "50", x0, y0, nice_gfx::fonts::TomThumb4x6, 1); break;
        case 1: nice_gfx::text::center(gfx, "75", x0, y0, nice_gfx::fonts::TomThumb4x6, 1); break;
        case 2: nice_gfx::text::center(gfx, "100", x0-2, y0-4, nice_gfx::fonts::TomThumb4x6, 1); break;
      }
    }

    // Now draw the arc of the scale
    sx = cos((i + 5 - 90) * 0.0174532925);
    sy = sin((i + 5 - 90) * 0.0174532925);
    x0 = sx * M_SIZE*100 + M_SIZE*120;
    y0 = sy * M_SIZE*100 + M_SIZE*150;
    // Draw scale arc, don't draw the last part
    if (i < 50) gfx.drawLine(x0, y0, x1, y1, GFX_BLACK);
  }

  nice_gfx::text::draw(gfx, "%RH", M_SIZE*(3 + 230 - 40), M_SIZE*(119 - 20), nice_gfx::fonts::GfxTerm8x16, 1); // Units at bottom right
  nice_gfx::text::center(gfx, "%RH", M_SIZE*120, M_SIZE*75, nice_gfx::fonts::Spleen12x24, 1); // Comment out to avoid Spleen12x24
  gfx.drawRect(1, M_SIZE*3, M_SIZE*236, M_SIZE*126, GFX_BLACK); // Draw bezel line

  plotNeedle(0, 0); // Put meter needle at 0
}

// #########################################################################
// Update needle position
// This function is blocking while needle moves, time depends on ms_delay
// 10ms minimises needle flicker if text is drawn within needle sweep area
// Smaller values OK if text not in sweep area, zero for instant movement but
// does not look realistic... (note: 100 increments for full scale deflection)
// #########################################################################
void plotNeedle(int value, byte ms_delay)
{
  gfx.setTextColor(GFX_BLACK, GFX_WHITE);
  char buf[8]; dtostrf(value, 4, 0, buf);
  nice_gfx::text::right(gfx, buf, 33, M_SIZE*(119 - 20), nice_gfx::fonts::GfxTerm8x16, 1);

  if (value < -10) value = -10; // Limit value to emulate needle end stops
  if (value > 110) value = 110;

  // Move the needle until new value reached
  while (!(value == old_analog)) {
    if (old_analog < value) old_analog++;
    else old_analog--;

    if (ms_delay == 0) old_analog = value; // Update immediately if delay is 0

    float sdeg = map(old_analog, -10, 110, -150, -30); // Map value to angle
    // Calculate tip of needle coords
    float sx = cos(sdeg * 0.0174532925);
    float sy = sin(sdeg * 0.0174532925);

    // Calculate x delta of needle start (does not start at pivot point)
    float tx = tan((sdeg + 90) * 0.0174532925);

    // Erase old needle image
    gfx.drawLine(M_SIZE*(120 + 24 * ltx) - 1, M_SIZE*(150 - 24), osx - 1, osy, GFX_WHITE);
    gfx.drawLine(M_SIZE*(120 + 24 * ltx), M_SIZE*(150 - 24), osx, osy, GFX_WHITE);
    gfx.drawLine(M_SIZE*(120 + 24 * ltx) + 1, M_SIZE*(150 - 24), osx + 1, osy, GFX_WHITE);

    // Re-plot text under needle
    gfx.setTextColor(GFX_BLACK, GFX_WHITE);
    nice_gfx::text::center(gfx, "%RH", M_SIZE*120, M_SIZE*75, nice_gfx::fonts::Spleen12x24, 1); // // Comment out to avoid Spleen12x24

    // Store new needle end coords for next erase
    ltx = tx;
    osx = M_SIZE*(sx * 98 + 120);
    osy = M_SIZE*(sy * 98 + 150);

    // Draw the needle in the new postion, magenta makes needle a bit bolder
    // draws 3 lines to thicken needle
    gfx.drawLine(M_SIZE*(120 + 24 * ltx) - 1, M_SIZE*(150 - 24), osx - 1, osy, GFX_RED);
    gfx.drawLine(M_SIZE*(120 + 24 * ltx), M_SIZE*(150 - 24), osx, osy, GFX_MAGENTA);
    gfx.drawLine(M_SIZE*(120 + 24 * ltx) + 1, M_SIZE*(150 - 24), osx + 1, osy, GFX_RED);

    // Slow needle down slightly as it approaches new postion
    if (abs(old_analog - value) < 10) ms_delay += ms_delay / 5;

    // Wait before next update
    delay(ms_delay);
  }
}

