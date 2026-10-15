// Read UTF-8 text from an SD card; glyphs are supplied by a Flash MonoFont.
#include <SPI.h>
#include <SD.h>
#include <NICE_GFX.h>
#include <Display/Presets/ExampleBoard.h>
ExampleDisplay gfx;
constexpr int SD_CS = 5; // Change this pin to your card's chip select.
bool cardReady = false;
void setup() {
  Serial.begin(115200);
  gfx.begin();
  gfx.setRotation(1);
  gfx.setFont(nice_gfx::fonts::GfxSansCN16);
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  gfx.setTextWrap(true, true);
  cardReady = SD.begin(SD_CS);
}
void loop() {
  gfx.fillScreen(GFX_BLACK);
  gfx.setCursor(4, 4);
  File text = cardReady ? SD.open("/font-demo.txt") : File();
  if (text) {
    while (text.available()) gfx.write(uint8_t(text.read()));
    text.close();
  } else {
    gfx.println("Copy font-demo.txt to SD");
    gfx.println("UTF-8: 中文显示");
    gfx.println("Glyphs remain in Flash");
  }
  delay(3000);
}
