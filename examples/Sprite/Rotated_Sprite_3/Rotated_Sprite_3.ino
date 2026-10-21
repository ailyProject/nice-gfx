/*====================================================================================

  This example draws a jpeg image in a Sprite then plot a rotated copy of the Sprite
  to the TFT.

  The JPEG is in this sketch's data folder (press Ctrl+K to see the folder).
  Upload that folder to LittleFS using the filesystem uploader appropriate for
  your Arduino core and IDE. Select LittleFS as the filesystem format and close
  the Serial Monitor before uploading.

  Created by Bodmer 6/1/19; adapted for NICE_GFX:
  The helpers in this sketch list files and decode JPEG tiles into the Sprite.

  To render the Jpeg image the JPEGDecoder library is needed, this can be obtained
  with the IDE library manager, or downloaded from here:
  https://github.com/Bodmer/JPEGDecoder

  ==================================================================================*/

//====================================================================================
//                                  Libraries
//====================================================================================
// The filesystem is provided by the ESP Arduino core.
#include <FS.h>
#include <LittleFS.h>
#include <NICE_GFX.h>                 // Hardware-specific library
#include "Hardware.h"
#include <JPEGDecoder.h>
ExampleDisplay gfx;            // Invoke custom library
GFX_Sprite spr = GFX_Sprite(&gfx);  // Create Sprite object "spr" with pointer to "gfx" object

void listImageFiles() {
#if defined(ESP8266)
  fs::Dir files = LittleFS.openDir("/");
  while (files.next()) {
    Serial.print(files.fileName());
    Serial.print("  ");
    Serial.println(files.fileSize());
  }
#else
  fs::File directory = LittleFS.open("/");
  for (fs::File file = directory.openNextFile(); file; file = directory.openNextFile()) {
    Serial.print(file.name());
    Serial.print("  ");
    Serial.println(file.size());
    file.close();
  }
#endif
}

bool drawSpriteJpeg(const char *path) {
  fs::File file = LittleFS.open(path, "r");
  if (!file || JpegDec.decodeFsFile(file) <= 0) {
    Serial.println("JPEG open/decode failed");
    JpegDec.abort();
    return false;
  }
  Serial.print("JPEG width: ");
  Serial.println(JpegDec.width);
  Serial.print("JPEG height: ");
  Serial.println(JpegDec.height);

  const bool oldSwap = spr.getSwapBytes();
  spr.setSwapBytes(true); // Decoder returns native RGB565 words.
  while (JpegDec.read()) {
    const int x = JpegDec.MCUx * JpegDec.MCUWidth;
    const int y = JpegDec.MCUy * JpegDec.MCUHeight;
    if (y >= spr.height()) {
      JpegDec.abort();
      break;
    }
    int width = JpegDec.width - x;
    int height = JpegDec.height - y;
    if (width > JpegDec.MCUWidth) width = JpegDec.MCUWidth;
    if (height > JpegDec.MCUHeight) height = JpegDec.MCUHeight;
    // Preserve decoder row stride at partial image edges; Sprite clips the
    // destination if the JPEG is larger than its 80 x 64 canvas.
    for (int row = 0; row < height; ++row)
      spr.pushImage(x, y + row, width, 1,
                    JpegDec.pImage + row * JpegDec.MCUWidth);
  }
  spr.setSwapBytes(oldSwap);
  return true;
}


//====================================================================================
//                                    Setup
//====================================================================================
void setup()
{
  Serial.begin(250000); // Used for messages

  gfx.begin();


  gfx.setRotation(0);  // 0 & 2 Portrait. 1 & 3 landscape
  gfx.fillScreen(GFX_BLACK);

  // Create a sprite to hold the jpeg (or part of it)
  if (!spr.createSprite(80, 64)) {
    Serial.println("Sprite allocation failed");
    while (1) yield();
  }

  // Initialise LittleFS
  if (!LittleFS.begin()) {
    Serial.println("LittleFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nInitialisation done.\r\n");

  listImageFiles();

  // Filenames start at the filesystem root. Print metadata and draw at 0,0.
  drawSpriteJpeg("/Eye_80x64.jpg");
}

//====================================================================================
//                                    Loop
//====================================================================================
void loop()
{

  gfx.fillScreen(random(0xFFFF));


  // Set the TFT pivot point to the centre of the screen
  gfx.setPivot(gfx.width() / 2, gfx.height() / 2);

  // Set Sprite pivot point to centre of Sprite
  spr.setPivot(spr.width() / 2, spr.height() / 2);

  // Push Sprite to the TFT at 0,0 (not rotated)
  spr.pushSprite(0, 0);

  delay(1000);

  // Push copies of Sprite rotated through increasing angles 0-360 degrees
  // with 45 degree increments
  for (int16_t angle = 0; angle <= 360; angle += 45) {
    spr.pushRotated(angle);
    delay(500);
  }

  delay(2000);

  // Move Sprite pivot to a point above the image at 40,-60
  // (Note: Top left corner is Sprite coordinate 0,0)
  // The TFT pivot point has already been set to middle of screen.
  /*                    .Pivot point at 40,-60
                        ^
                        |
                       -60
                  < 40 >|
                  ______V______
                 |             |
                 |   Sprite    |
                 |_____________|
  */
  spr.setPivot(40, -60);

  // Push Sprite to screen rotated about the new pivot points
  // negative angle rotates Sprite anticlockwise
  for (int16_t angle = 330; angle >= 0; angle -= 30) {
    spr.pushRotated(angle);
    yield(); // Stop watchdog triggering
  }

  delay(5000);
}
//====================================================================================
