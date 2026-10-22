// Icon images are stored in tabs ^ e.g. Alert.h etc.above this line
// more than one icon can be in a header file

// Arrays containing FLASH images can be created with UTFT library tool:
// (libraries\UTFT\Tools\ImageConverter565.exe)
// Convert to .c format then copy into a new tab

/*
 This sketch demonstrates loading images from arrays stored in program (FLASH) memory.

 Works with NICE_GFX library here:
 This sketch does not use/need any fonts at all...

 Code derived from ILI9341_due library example

 Make sure all the display driver and pin connections are correct by
 editing the Hardware.h file in this sketch folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE Hardware.h FILE IN THIS SKETCH ######
 #########################################################################
*/

#include <NICE_GFX.h>       // Hardware-specific library
#include "Hardware.h"

ExampleDisplay gfx;  // Invoke custom library

// Include the header files that contain the icons
#include "Alert.h"
#include "Close.h"
#include "Info.h"

long count = 0; // Loop count

void setup()
{
  Serial.begin(115200);
  gfx.begin();
  gfx.setRotation(1);	// landscape

  gfx.fillScreen(GFX_BLACK);

  // Swap the colour byte order when rendering
  gfx.setSwapBytes(true);

  // Draw the icons
  gfx.pushImage(100, 100, infoWidth, infoHeight, info);
  gfx.pushImage(140, 100, alertWidth, alertHeight, alert);
  gfx.pushImage(180, 100, closeWidth, closeHeight, closeX);

  // Pause here to admire the icons!
  delay(2000);

}

void loop()
{
  // Loop filling and clearing screen
  gfx.pushImage(random(gfx.width() -  infoWidth), random(gfx.height() -  infoHeight),  infoWidth,  infoHeight, info);
  gfx.pushImage(random(gfx.width() - alertWidth), random(gfx.height() - alertHeight), alertWidth, alertHeight, alert);
  gfx.pushImage(random(gfx.width() - closeWidth), random(gfx.height() - closeHeight), alertWidth, closeHeight, closeX);

  // Clear screen after 100 x 3 = 300 icons drawn
  if (1000 == count++) {
    count = 1;
    gfx.setRotation(2 * random(2)); // Rotate randomly to clear display left>right or right>left to reduce monotony!
    gfx.fillScreen(GFX_BLACK);
    gfx.setRotation(1);
  }
}
