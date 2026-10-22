#include "FS.h"
#if defined(ESP32)
#include <SPIFFS.h>
#endif
#include <SPI.h>
#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include "Hardware.h"
ExampleDisplay gfx;

#define CALIBRATION_FILE "/calibrationData"

void setup(void) {
  uint16_t calibrationData[5];
  uint8_t calDataOK = 0;

  Serial.begin(115200);
  Serial.println("starting");

  gfx.begin();
  exampleAttachTouch(gfx); // Runtime XPT2046 pins in ExampleBoard.h

  gfx.setRotation(3);
  gfx.fillScreen((0xFFFF));

  nice_gfx::text::cursor(gfx, 20, 0, nice_gfx::fonts::GfxTerm8x16, 1);
  gfx.setTextColor(GFX_BLACK, GFX_WHITE);  gfx.setTextSize(1);
  gfx.println("calibration run");

  // check file system
  if (!SPIFFS.begin()) {
    Serial.println("formatting file system");

    SPIFFS.format();
    SPIFFS.begin();

  }

  // check if calibration file exists
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    File f = SPIFFS.open(CALIBRATION_FILE, "r");
    if (f) {
      if (f.readBytes((char *)calibrationData, 14) == 14)
        calDataOK = 1;
      f.close();
    }
  }
  if (calDataOK) {
    // calibration data valid
    gfx.setTouch(calibrationData);
  } else {
    // data not valid. recalibrate
    gfx.calibrateTouch(calibrationData, GFX_WHITE, GFX_RED, 15);
    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calibrationData, 14);
      f.close();
    }
  }

  gfx.fillScreen((0xFFFF));

}

void loop() {
  uint16_t x, y;
  static uint16_t color;

  if (gfx.getTouch(&x, &y)) {

    nice_gfx::text::cursor(gfx, 5, 5, nice_gfx::fonts::GfxTerm8x16, 1);
    gfx.printf("x: %i     ", x);
    nice_gfx::text::cursor(gfx, 5, 20, nice_gfx::fonts::GfxTerm8x16, 1);
    gfx.printf("y: %i    ", y);

    gfx.drawPixel(x, y, color);
    color += 155;
  }
}



