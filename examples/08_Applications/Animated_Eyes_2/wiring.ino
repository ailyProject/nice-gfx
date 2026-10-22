/*
This is the example wiring used for the sketch testing.

The sketch constructs ArduinoSpi with CS=-1 and supplies each display's
chip-select GPIO in the eyeInfo objects in config.h. The runtime ST7735 panel
uses the original Setup47 wiring, size, tab variant and color order below.

Function    ESP32 pin       TFT 1       TFT 2
 MOSI         23     ->     SDA   ->    SDA     // The TFT pin may be named DIN
 MISO         19                                // Not connected
 SCLK         18     ->     CLK   ->    CLK     // The TFT pin may be named SCK
 GFX_DC        2     ->     DC    ->    DC      // The TFT pin may be named AO
 GFX_RST       4     ->     RST   ->    RST
 CS 1         22     ->     CS                  // Connected to TFT 1 only 
 CS 2         21                  ->    CS      // Connected to TFT 2 only 
 +5V/VIN             ->     VCC   ->    VCC
 0V                  ->     GND   ->    GND
 +5V/VIN             ->     LED   ->    LED     // Some displays do not have a backlight BL/LED pin     

The displays used for testing were 128x128 ST7735 displays. Adjust
eyePanelOptions() in the sketch for a different panel variant.


 */
