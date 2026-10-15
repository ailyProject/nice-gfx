// This sketch if for an ESP32, it draws Jpeg images pulled from an SD Card
// onto the TFT.

// As well as the NICE_GFX library you will need the JPEG Decoder library.
// A copy can be downloaded here, it is based on the library by Makoto Kurauchi.
// https://github.com/Bodmer/JPEGDecoder

// Images on SD Card must be put in the root folder (top level) to be found
// Use the SD library examples to verify your SD Card interface works!

// The example images used to test this sketch can be found in the library
// JPEGDecoder/extras folder
//----------------------------------------------------------------------------------------------------

#include <SPI.h>

#include <FS.h>
#include <SD.h>

#include <NICE_GFX.h>
// This example shares Arduino SPI with the SD card (CS=5).
// Its display keeps the original CS=15 wiring instead of the common demo board.
ILI9341_240x320_ARDUINO_SPI gfx(SPI, 15, 2, 4, 18, 23, 19);

// JPEG decoder library
#include <JPEGDecoder.h>

//####################################################################################################
// Setup
//####################################################################################################
void setup() {
  Serial.begin(115200);

  // Set all chip selects high to avoid bus contention during initialisation of each peripheral
  digitalWrite(22, HIGH); // Touch controller chip select (if used)
  digitalWrite(15, HIGH); // TFT screen chip select
  digitalWrite( 5, HIGH); // SD card chips select, must use GPIO 5 (ESP32 SS)

  gfx.begin();



  if (!SD.begin(5, SPI)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  Serial.println("initialisation done.");
}

//####################################################################################################
// Main loop
//####################################################################################################
void loop() {

  gfx.setRotation(2);  // portrait
  gfx.fillScreen(random(0xFFFF));

  // The image is 300 x 300 pixels so we do some sums to position image in the middle of the screen!
  // Doing this by reading the image width and height from the jpeg info is left as an exercise!
  int x = (gfx.width()  - 300) / 2 - 1;
  int y = (gfx.height() - 300) / 2 - 1;

  drawSdJpeg("/EagleEye.jpg", x, y);     // This draws a jpeg pulled off the SD Card
  delay(2000);

  gfx.setRotation(2);  // portrait
  gfx.fillScreen(random(0xFFFF));
  drawSdJpeg("/Baboon40.jpg", 0, 0);     // This draws a jpeg pulled off the SD Card
  delay(2000);

  gfx.setRotation(2);  // portrait
  gfx.fillScreen(random(0xFFFF));
  drawSdJpeg("/lena20k.jpg", 0, 0);     // This draws a jpeg pulled off the SD Card
  delay(2000);

  gfx.setRotation(1);  // landscape
  gfx.fillScreen(random(0xFFFF));
  drawSdJpeg("/Mouse480.jpg", 0, 0);     // This draws a jpeg pulled off the SD Card

  delay(2000);

  while(1); // Wait here
}

//####################################################################################################
// Draw a JPEG on the TFT pulled from SD Card
//####################################################################################################
// xpos, ypos is top left corner of plotted image
void drawSdJpeg(const char *filename, int xpos, int ypos) {

  // Open the named file (the Jpeg decoder library will close it)
  File jpegFile = SD.open( filename, FILE_READ);  // or, file handle reference for SD library
 
  if ( !jpegFile ) {
    Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
    return;
  }

  Serial.println("===========================");
  Serial.print("Drawing file: "); Serial.println(filename);
  Serial.println("===========================");

  // Use one of the following methods to initialise the decoder:
  bool decoded = JpegDec.decodeSdFile(jpegFile);  // Pass the SD file handle to the decoder,
  //bool decoded = JpegDec.decodeSdFile(filename);  // or pass the filename (String or character array)

  if (decoded) {
    // print information about the image to the serial port
    jpegInfo();
    // render the image onto the screen at given coordinates
    jpegRender(xpos, ypos);
  }
  else {
    Serial.println("Jpeg file format not supported!");
  }
}

//####################################################################################################
// Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
//####################################################################################################
// This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
// fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void jpegRender(int xpos, int ypos) {

  //jpegInfo(); // Print information from the JPEG file (could comment this line out)

  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  bool swapBytes = gfx.getSwapBytes();
  gfx.setSwapBytes(true);
  
  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
  uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // Fetch data from the file, decode and display
  while (JpegDec.read()) {    // While there is more data in the file
    pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)

    // Calculate coordinates of top left corner of current MCU
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // calculate how many pixels must be drawn
    uint32_t mcu_pixels = win_w * win_h;

    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= gfx.width() && ( mcu_y + win_h ) <= gfx.height())
      gfx.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    else if ( (mcu_y + win_h) >= gfx.height())
      JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }

  gfx.setSwapBytes(swapBytes);

  showTime(millis() - drawTime); // These lines are for sketch testing only
}

//####################################################################################################
// Print image information to the serial port (optional)
//####################################################################################################
// JpegDec.decodeFile(...) or JpegDec.decodeArray(...) must be called before this info is available!
void jpegInfo() {

  // Print information extracted from the JPEG file
  Serial.println("JPEG image info");
  Serial.println("===============");
  Serial.print("Width      :");
  Serial.println(JpegDec.width);
  Serial.print("Height     :");
  Serial.println(JpegDec.height);
  Serial.print("Components :");
  Serial.println(JpegDec.comps);
  Serial.print("MCU / row  :");
  Serial.println(JpegDec.MCUSPerRow);
  Serial.print("MCU / col  :");
  Serial.println(JpegDec.MCUSPerCol);
  Serial.print("Scan type  :");
  Serial.println(JpegDec.scanType);
  Serial.print("MCU width  :");
  Serial.println(JpegDec.MCUWidth);
  Serial.print("MCU height :");
  Serial.println(JpegDec.MCUHeight);
  Serial.println("===============");
  Serial.println("");
}

//####################################################################################################
// Show the execution time (optional)
//####################################################################################################
// WARNING: for UNO/AVR legacy reasons printing text to the screen with the Mega might not work for
// sketch sizes greater than ~70KBytes because 16-bit address pointers are used in some libraries.

// The Due will work fine with the HX8357_Due library.

void showTime(uint32_t msTime) {
  // Select the descriptor with setFont() before printing.
  //gfx.setTextSize(2);
  //gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  //gfx.print(F(" JPEG drawn in "));
  //gfx.print(msTime);
  //gfx.println(F(" ms "));
  Serial.print(F(" JPEG drawn in "));
  Serial.print(msTime);
  Serial.println(F(" ms "));
}

