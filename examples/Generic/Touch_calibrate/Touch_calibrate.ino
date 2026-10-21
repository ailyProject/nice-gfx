/*
  Sketch to generate the setup() calibration values, these are reported
  to the Serial Monitor.

  The sketch has been tested on the ESP8266 and screen with XPT2046 driver.
*/

#include <SPI.h>
#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include "Hardware.h"

ExampleDisplay gfx; // Invoke custom library

//------------------------------------------------------------------------------------------

void setup() {
  // Use serial port
  Serial.begin(115200);

  // Initialise the TFT screen
  gfx.begin();
  exampleAttachTouch(gfx); // Runtime XPT2046 pins in ExampleBoard.h

  // Set the rotation to the orientation you wish to use in your project before calibration
  // (the touch coordinates returned then correspond to that rotation only)
  gfx.setRotation(1);

  // Calibrate the touch screen and retrieve the scaling factors
  touch_calibrate();

/*
  // Replace above line with the code sent to Serial Monitor
  // once calibration is complete, e.g.:
  uint16_t calData[5] = { 286, 3534, 283, 3600, 6 };
  gfx.setTouch(calData);
*/

  // Clear the screen
  gfx.fillScreen(GFX_BLACK);
  nice_gfx::text::center(gfx, "Touch screen to test!", gfx.width()/2, gfx.height()/2, nice_gfx::fonts::GfxTerm8x16, 1);
}

//------------------------------------------------------------------------------------------

void loop(void) {
  uint16_t x = 0, y = 0; // To store the touch coordinates

  // Pressed will be set true is there is a valid touch on the screen
  bool pressed = gfx.getTouch(&x, &y);

  // Draw a white spot at the detected coordinates
  if (pressed) {
    gfx.fillCircle(x, y, 2, GFX_WHITE);
    //Serial.print("x,y = ");
    //Serial.print(x);
    //Serial.print(",");
    //Serial.println(y);
  }
}

//------------------------------------------------------------------------------------------

// Code to run a screen calibration, not needed when calibration values set in setup()
void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // Calibrate
  gfx.fillScreen(GFX_BLACK);
  gfx.setCursor(20, 0);
  nice_gfx::text::select(gfx, nice_gfx::fonts::GfxTerm8x16, 1);
  gfx.setTextSize(1);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);

  gfx.println("Touch corners as indicated");

  nice_gfx::text::select(gfx, nice_gfx::fonts::TomThumb4x6, 1);
  gfx.println();

  gfx.calibrateTouch(calData, GFX_MAGENTA, GFX_BLACK, 15);

  Serial.println(); Serial.println();
  Serial.println("// Use this calibration code in setup():");
  Serial.print("  uint16_t calData[5] = ");
  Serial.print("{ ");

  for (uint8_t i = 0; i < 5; i++)
  {
    Serial.print(calData[i]);
    if (i < 4) Serial.print(", ");
  }

  Serial.println(" };");
  Serial.print("  gfx.setTouch(calData);");
  Serial.println(); Serial.println();

  gfx.fillScreen(GFX_BLACK);
  
  gfx.setTextColor(GFX_GREEN, GFX_BLACK);
  gfx.println("Calibration complete!");
  gfx.println("Calibration code sent to Serial port.");

  delay(4000);
}

