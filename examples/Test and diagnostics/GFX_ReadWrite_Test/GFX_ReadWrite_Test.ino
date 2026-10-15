// Walking 1 write and read pixel test

#include <NICE_GFX.h>
#include <Display/Presets/ExampleBoard.h>
#include <SPI.h>

#define TDELAY 500

ExampleDisplay gfx; 

void setup() {
  Serial.begin(115200);

  gfx.begin();
  gfx.fillScreen(0xF81F);
}

void loop() {
  static uint32_t wr = 1;
  static uint32_t rd = 0xFFFFFFFF;

  delay(TDELAY);

  gfx.drawPixel(30,30,wr);
  Serial.print(" Pixel value written = ");Serial.println(wr,HEX);
  
  rd = gfx.readPixel(30,30);
                Serial.print(" Pixel value read    = ");Serial.println(rd,HEX);

  if (rd!=wr) {
    Serial.println(" ERROR                 ^^^^");
    //while(1) yield();
  }
  else Serial.println(" PASS ");
  // Walking 1 test
  wr = wr<<1;
  if (wr >= 0x10000) wr = 1;
}
