#include "Hardware.h"
#include <Drivers/Touch/GT911.h>
nice_gfx::ST7701_480x480_RGB gfx(displayConfig());
nice_gfx::Gt911 touch(boardI2cBus(),480,480,0x5D);
void setup() {
 Serial.begin(115200);
 if(!beginBoardPeripherals()||touch.begin()!=nice_gfx::Status::Ok||!gfx.begin()) {Serial.println("Hardware initialization failed");return;}
 gfx.fillScreen(0x0841);digitalWrite(4,LOW);
}
void loop() {uint16_t x=0,y=0;if(touch.getTouch(&x,&y,480,480,0))Serial.printf("Touch: %u,%u\n",unsigned(x),unsigned(y));delay(10);}
