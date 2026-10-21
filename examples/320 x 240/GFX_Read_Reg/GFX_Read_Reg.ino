/*
  Test the library readcommand8 member function

  This sketch reports via the Serial Monitor window

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE Hardware.h FILE IN THIS SKETCH ######
  ######       SELECT ILI9341 AND CONNECT MISO TO READ REGISTERS      ######
  #########################################################################

  Created by Bodmer 14/1/17
*/

//====================================================================================
//                                  Libraries
//====================================================================================

#include <NICE_GFX.h>
#include <Graphics/TextDrawing.h> // Font selection and text drawing helpers.
#include "Hardware.h"
#include <SPI.h>

// ILI9341 read commands; the sketch does not depend on private driver headers.
constexpr uint8_t ILI9341_RDDID=0x04, ILI9341_RDDST=0x09;
constexpr uint8_t ILI9341_RDMODE=0x0A, ILI9341_RDMADCTL=0x0B, ILI9341_RDPIXFMT=0x0C;
constexpr uint8_t ILI9341_RDSELFDIAG=0x0F, ILI9341_RAMRD=0x2E, ILI9341_RDID4=0xD3;
constexpr uint8_t ILI9341_RDID1=0xDA, ILI9341_RDID2=0xDB, ILI9341_RDID3=0xDC, ILI9341_RDIDX=0xDD;

ExampleDisplay gfx;  // Invoke library

//====================================================================================
//                                    Setup
//====================================================================================

void setup(void) {
  Serial.begin(115200);

  gfx.begin();
  gfx.setRotation(2);
}

//====================================================================================
//                                    Loop
//====================================================================================

void loop() {

  gfx.fillScreen(GFX_BLUE);
  nice_gfx::text::cursor(gfx, 0, 0, nice_gfx::fonts::GfxTerm8x16, 1);
  // Set the font colour to be white with a black background
  gfx.setTextColor(GFX_WHITE, GFX_BLACK);
  // We can now plot text on screen using the "print" class
  gfx.println("Hello World!");

  delay(2000);

  // OK, now it has been shown that the display is working reset it to defaults
  // This will make the screen go "white" but we can still read registers

  const int resetPin = gfx.displayInfo().resetPin;
  if (resetPin >= 0) {
    digitalWrite(resetPin, LOW);
    delay(10);
    digitalWrite(resetPin, HIGH);
    delay(10);
  } else gfx.writecommand(0x01);

  printSubset(); // Print a useful subset of the readable registers

  readTest(); // Test 8, 16 and 32-bit reads and index on the ID register

  //printRange32(0x00, 0xFF); // Print a range of registers (32 bits, index = 0)

  delay(2000);
  while (1) yield();
}

//====================================================================================
//                                  Supporting functions
//====================================================================================

void readTest(void)
{
  Serial.println(); Serial.println("Test 8, 16 and 32-bit reads and the index...");
  // Test 8, 16 and 32-bit reads and index
  // Note at index 0 the register values are typically undefined (Bxxxxxxxx)
  Serial.println(gfx.readcommand8(ILI9341_RDID4, 2), HEX);
  Serial.println(gfx.readcommand16(ILI9341_RDID4, 2), HEX);
  Serial.println(gfx.readcommand32(ILI9341_RDID4, 0), HEX);
}

//====================================================================================

void printRange32(uint8_t readStart, uint8_t readEnd)
{
  Serial.print("Registers from "); Serial.print(readStart, HEX);
  Serial.print(" to "); Serial.println(readEnd, HEX);

  for (uint8_t cmd_reg = readStart; cmd_reg < readEnd; cmd_reg++) {
    readRegister(cmd_reg, 4, 0);
  }
}

//====================================================================================

void printSubset(void)
{
  Serial.println();  Serial.println();
  readRegister(ILI9341_RDDID, 3, 1);
  readRegister(ILI9341_RDDST, 4, 1);
  readRegister(ILI9341_RDMODE, 1, 1);
  readRegister(ILI9341_RDMADCTL, 1, 1);
  readRegister(ILI9341_RDPIXFMT, 1, 1);
  readRegister(ILI9341_RDSELFDIAG, 1, 1);
  readRegister(ILI9341_RAMRD, 3, 1);

  readRegister(ILI9341_RDID1, 1, 1);
  readRegister(ILI9341_RDID2, 1, 1);
  readRegister(ILI9341_RDID3, 1, 1);
  readRegister(ILI9341_RDIDX, 1, 1); // ?
  readRegister(ILI9341_RDID4, 3, 1);  // ID
}

//====================================================================================

uint32_t readRegister(uint8_t reg, int16_t bytes, uint8_t index)
{
  uint32_t  data = 0;

  while (bytes > 0) {
    bytes--;
    data = (data << 8) | gfx.readcommand8(reg, index);
    index++;
  }

  Serial.print("Register 0x");
  if (reg < 0x10) Serial.print("0");
  Serial.print(reg , HEX);

  Serial.print(": 0x");

  // Add leading zeros as needed
  uint32_t mask = 0x1 << 28;
  while (data < mask && mask > 0x1) {
    Serial.print("0");
    mask = mask >> 4;
  }

  Serial.println(data, HEX);

  return data;
}

//====================================================================================
