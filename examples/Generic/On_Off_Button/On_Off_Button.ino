// Example of drawing a graphical "switch" and using
// the touch screen to change it's state.

// This sketch does not use the libraries button drawing
// and handling functions.

// Based on Adafruit_GFX library onoffbutton example.

// Touch handling for XPT2046 based screens is handled by
// the NICE_GFX library.

// Calibration data is stored in SPIFFS so we need to include it
#include "FS.h"
#if defined(ESP32)
#include <SPIFFS.h>
#endif

#include <SPI.h>

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include "Hardware.h"

ExampleDisplay gfx;       // Invoke custom library

// This is the file name used to store the touch coordinate
// calibration data. Change the name to start a new calibration.
#define CALIBRATION_FILE "/TouchCalData3"

// Set REPEAT_CAL to true instead of false to run calibration
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL false

bool SwitchOn = false;

// Comment out to stop drawing black spots
#define BLACK_SPOT

// Switch position and size
#define FRAME_X 100
#define FRAME_Y 64
#define FRAME_W 120
#define FRAME_H 50

// Red zone size
#define REDBUTTON_X FRAME_X
#define REDBUTTON_Y FRAME_Y
#define REDBUTTON_W (FRAME_W/2)
#define REDBUTTON_H FRAME_H

// Green zone size
#define GREENBUTTON_X (REDBUTTON_X + REDBUTTON_W)
#define GREENBUTTON_Y FRAME_Y
#define GREENBUTTON_W (FRAME_W/2)
#define GREENBUTTON_H FRAME_H

//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
void setup(void)
{
  Serial.begin(9600);
  gfx.begin();
  exampleAttachTouch(gfx); // Runtime XPT2046 pins in ExampleBoard.h

  // Set the rotation before we calibrate
  gfx.setRotation(1);

  // call screen calibration
  touch_calibrate();

  // clear screen
  gfx.fillScreen(GFX_BLUE);

  // Draw button (this example does not use library Button class)
  redBtn();
}
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
void loop()
{
  uint16_t x, y;

  // See if there's any touch data for us
  if (gfx.getTouch(&x, &y))
  {
    // Draw a block spot to show where touch was calculated to be
    #ifdef BLACK_SPOT
      gfx.fillCircle(x, y, 2, GFX_BLACK);
    #endif
    
    if (SwitchOn)
    {
      if ((x > REDBUTTON_X) && (x < (REDBUTTON_X + REDBUTTON_W))) {
        if ((y > REDBUTTON_Y) && (y <= (REDBUTTON_Y + REDBUTTON_H))) {
          Serial.println("Red btn hit");
          redBtn();
        }
      }
    }
    else //Record is off (SwitchOn == false)
    {
      if ((x > GREENBUTTON_X) && (x < (GREENBUTTON_X + GREENBUTTON_W))) {
        if ((y > GREENBUTTON_Y) && (y <= (GREENBUTTON_Y + GREENBUTTON_H))) {
          Serial.println("Green btn hit");
          greenBtn();
        }
      }
    }

    Serial.println(SwitchOn);

  }
}
//------------------------------------------------------------------------------------------

void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!SPIFFS.begin()) {
    Serial.println("Formatting file system");
    SPIFFS.format();
    SPIFFS.begin();

  }

  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      SPIFFS.remove(CALIBRATION_FILE);
    }
    else
    {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    gfx.setTouch(calData);
  } else {
    // data not valid so recalibrate
    gfx.fillScreen(GFX_BLACK);
    gfx.setCursor(20, 0);
    nice_gfx::text::select(gfx, nice_gfx::fonts::GfxTerm8x16, 1);
    gfx.setTextSize(1);
    gfx.setTextColor(GFX_WHITE, GFX_BLACK);

    gfx.println("Touch corners as indicated");

    nice_gfx::text::select(gfx, nice_gfx::fonts::TomThumb4x6, 1);
    gfx.println();

    if (REPEAT_CAL) {
      gfx.setTextColor(GFX_RED, GFX_BLACK);
      gfx.println("Set REPEAT_CAL to false to stop this running again!");
    }

    gfx.calibrateTouch(calData, GFX_MAGENTA, GFX_BLACK, 15);

    gfx.setTextColor(GFX_GREEN, GFX_BLACK);
    gfx.println("Calibration complete!");

    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}

void drawFrame()
{
  gfx.drawRect(FRAME_X, FRAME_Y, FRAME_W, FRAME_H, GFX_BLACK);
}

// Draw a red button
void redBtn()
{
  gfx.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, GFX_RED);
  gfx.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, GFX_DARKGREY);
  drawFrame();
  gfx.setTextColor(GFX_WHITE);
  gfx.setTextSize(2);
  gfx.setTextDatum(MC_DATUM);
  gfx.drawString("ON", GREENBUTTON_X + (GREENBUTTON_W / 2), GREENBUTTON_Y + (GREENBUTTON_H / 2));
  SwitchOn = false;
}

// Draw a green button
void greenBtn()
{
  gfx.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, GFX_GREEN);
  gfx.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, GFX_DARKGREY);
  drawFrame();
  gfx.setTextColor(GFX_WHITE);
  gfx.setTextSize(2);
  gfx.setTextDatum(MC_DATUM);
  gfx.drawString("OFF", REDBUTTON_X + (REDBUTTON_W / 2) + 1, REDBUTTON_Y + (REDBUTTON_H / 2));
  SwitchOn = true;
}

