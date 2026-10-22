// Automated SAMD51/ILI9341 readback, graphics and allocation regression.
#include <NICE_GFX.h>
#include <Graphics/Sprite.h>
#include <malloc.h>
#include <initializer_list>

#if !defined(SEEED_WIO_TERMINAL)
#error "Select Seeeduino Wio Terminal."
#endif

using nice_gfx::Status;
using nice_gfx::PixelView;
using nice_gfx::PixelOrder;
WioTerminalDisplay gfx;
uint32_t checks = 0, failures = 0;
uint16_t rowBuffer[320];

void rawSpiTiming() {
  // CS remains inactive: measure core transfer setup without altering GRAM.
  pinMode(LCD_SS_PIN,OUTPUT);digitalWrite(LCD_SS_PIN,HIGH);
  LCD_SPI.begin();LCD_SPI.beginTransaction(SPISettings(40000000,MSBFIRST,SPI_MODE0));
  static const uint8_t tx[1536]={};
  for(unsigned n:{1,2,4,8,16,32,64,128,384,1536}) {
    LCD_SPI.transfer(tx,nullptr,n);
    uint32_t start=micros();
    for(int repeat=0;repeat<100;++repeat)for(unsigned i=0;i<n;++i)LCD_SPI.transfer(tx[i]);
    uint32_t polling=micros()-start;start=micros();
    for(int repeat=0;repeat<100;++repeat)LCD_SPI.transfer(tx,nullptr,n);
    uint32_t dma=micros()-start;
    Serial.print("PERF_RAW bytes=");Serial.print(n);Serial.print(" polling_100_us=");Serial.print(polling);
    Serial.print(" dma_100_us=");Serial.println(dma);
  }
  LCD_SPI.endTransaction();LCD_SPI.end();
}

void check(bool ok, const char *label) {
  ++checks;
  if (ok) return;
  if (++failures <= 12) {
    Serial.print("VALIDATE_FAIL "); Serial.println(label);
  }
}

void pixel(int x, int y, uint16_t expected, const char *label) {
  const uint16_t actual = gfx.readPixel(x,y);
  check(gfx.lastError() == Status::Ok && actual == expected,label);
  if (actual != expected && failures <= 12) {
    Serial.print("VALIDATE_PIXEL actual="); Serial.print(actual,HEX);
    Serial.print(" expected="); Serial.println(expected,HEX);
  }
}

template<class Canvas> void pattern(Canvas &c) {
  c.fillRect(0,0,c.width(),c.height(),GFX_NAVY);
  c.fillRect(-4,-4,18,18,GFX_RED);
  c.fillRect(c.width()-10,c.height()-10,18,18,GFX_GREEN);
  c.drawLine(-20,30,c.width()+20,100,GFX_YELLOW);
  c.drawRect(10,20,100,70,GFX_CYAN);
  c.fillCircle(75,120,24,GFX_BLUE);
  c.fillTriangle(120,20,190,110,110,130,GFX_MAGENTA);
  c.fillRoundRect(130,140,80,48,8,GFX_ORANGE);
  c.fillRectVGradient(15,145,80,35,GFX_RED,GFX_BLUE);
  c.drawSmoothCircle(230,140,18,GFX_WHITE,GFX_NAVY);
  c.setFont(nice_gfx::fonts::Fixed6x13);
  c.setTextColor(GFX_WHITE,GFX_NAVY);
  c.drawString("Wio 0123456789",12,200);
}

void rotations() {
  for (uint8_t r=0;r<8;++r) {
    gfx.setRotation(r);
    check(gfx.lastError()==Status::Ok,"rotation accepted");
    check(gfx.width()==(r&1?320:240) && gfx.height()==(r&1?240:320),"rotation dimensions");
    GFX_Sprite ref(&gfx);
    ref.setColorDepth(16);
    if (!ref.createSprite(gfx.width(),gfx.height())) { check(false,"reference allocation"); return; }
    pattern(gfx); pattern(ref);
    // Compare complete scan lines, including asymmetric corners and text.
    for (int y=0;y<gfx.height();y+=7) {
      gfx.readRect(0,y,gfx.width(),1,rowBuffer);
      check(gfx.lastError()==Status::Ok,"scanline read");
      for (int x=0;x<gfx.width();++x) {
        uint16_t wire=rowBuffer[x];
        check(uint16_t(wire>>8|wire<<8)==ref.readPixel(x,y),"direct versus sprite raster");
      }
    }
    pixel(0,0,GFX_RED,"top left");
    pixel(gfx.width()-1,gfx.height()-1,GFX_GREEN,"bottom right");
    Serial.print("VALIDATE_ROTATION "); Serial.println(r);
  }
  gfx.setRotation(3);
}

void images() {
  const uint16_t colors[]={GFX_BLACK,GFX_WHITE,GFX_RED,GFX_GREEN,GFX_BLUE,0x1234,0xABCD};
  for (uint16_t c:colors) { gfx.fillRect(2,2,47,29,c); pixel(3,3,c,"solid color"); }
  uint16_t native[9*5];
  uint8_t wire[sizeof(native)];
  for (unsigned i=0;i<45;++i) {native[i]=uint16_t(i*1439+17);wire[2*i]=native[i]>>8;wire[2*i+1]=native[i];}
  for (PixelOrder order:{PixelOrder::Native,PixelOrder::Wire}) {
    const void *p=order==PixelOrder::Native?static_cast<const void*>(native):static_cast<const void*>(wire);
    check(gfx.flush(17,19,PixelView(p,7,5,9,order),false)==Status::Ok,"strided flush");
    for(int y=0;y<5;++y)for(int x=0;x<7;++x)pixel(17+x,19+y,native[y*9+x],"strided pixels");
  }
  static const uint16_t flashPixels[]={0xF800,0x07E0,0x001F,0x1234,0xABCD,0xFFFF};
  gfx.setSwapBytes(true);
  gfx.pushImage(10,10,3,2,flashPixels);
  for(int i=0;i<6;++i)pixel(10+i%3,10+i/3,flashPixels[i],"immutable flash image");
  gfx.fillScreen(GFX_BLACK);
  gfx.setViewport(20,30,10,12,true);
  gfx.fillRect(-10,-10,40,40,GFX_YELLOW);
  gfx.resetViewport();
  pixel(19,30,GFX_BLACK,"viewport left guard");pixel(20,30,GFX_YELLOW,"viewport origin");
  pixel(29,41,GFX_YELLOW,"viewport end");pixel(30,41,GFX_BLACK,"viewport right guard");
  check(gfx.flush(0,0,PixelView(nullptr,1),false)==Status::InvalidArgument,"invalid image rejected");
  check(gfx.flush(0,0,PixelView(native,2,2,1),false)==Status::InvalidArgument,"invalid stride rejected");
  check(gfx.flush(0,0,PixelView(native,1),false)==Status::Ok,"recovery after invalid image");
  pixel(0,0,native[0],"recovery pixel");
  // Unaligned addresses and spans around polling/staging boundaries.
  uint8_t unaligned[257*7*2+1];
  for(int i=0;i<257*7;++i){uint16_t c=uint16_t(i*283+43);memcpy(unaligned+1+2*i,&c,2);}
  check(gfx.flush(0,0,PixelView(unaligned+1,257,7),false)==Status::Ok,"unaligned multichunk native");
  for(int y=0;y<7;++y)for(int x=0;x<257;++x)pixel(x,y,uint16_t((y*257+x)*283+43),"native staging boundary");
  for(int i=0;i<257*7;++i){uint8_t t=unaligned[1+2*i];unaligned[1+2*i]=unaligned[2+2*i];unaligned[2+2*i]=t;}
  check(gfx.flush(0,0,PixelView(unaligned+1,257,7,0,PixelOrder::Wire),false)==Status::Ok,"unaligned wire");
  for(int n:{1,2,4,7,8,9,16,17,191,192,193,255,256,257}) {
    check(gfx.flush(0,10,PixelView(unaligned+1,n,1,0,PixelOrder::Wire),false)==Status::Ok,"wire span size");
    for(int x=0;x<n;++x)pixel(x,10,uint16_t(x*283+43),"wire span boundary");
  }
}

void sprites() {
  for (int depth:{1,4,8,16}) {
    GFX_Sprite sprite(&gfx);
    sprite.setColorDepth(depth);
    check(sprite.createSprite(65,39)!=nullptr,"sprite depth allocation");
    if (!sprite.created()) continue;
    if(depth==1)sprite.setBitmapColor(GFX_WHITE,GFX_BLACK);
    if(depth==4){sprite.createPalette(static_cast<uint16_t*>(nullptr));sprite.setPaletteColor(1,GFX_RED);}
    const uint16_t color=depth<=4?1:GFX_RED;
    sprite.fillSprite(0);sprite.fillRect(2,3,20,16,color);
    sprite.drawLine(0,0,64,38,color);
    sprite.pushSprite(33,44);
    for(int y=0;y<39;y+=3)for(int x=0;x<65;x+=3)
      pixel(33+x,44+y,sprite.readPixel(x,y),"sprite depth readback");
    sprite.setScrollRect(0,0,65,39,0);sprite.scroll(3,2);
    sprite.pushSprite(0,0);
    for(int y=0;y<39;y+=5)for(int x=0;x<65;x+=5)
      pixel(x,y,sprite.readPixel(x,y),"sprite scroll readback");
  }
  // Full framebuffer exceeds the SAMD DMA descriptor's 65535-byte limit.
  GFX_Sprite large(&gfx); large.setColorDepth(16);
  check(large.createSprite(320,240)!=nullptr,"full frame allocation");
  if (!large.created()) return;
  for(int y=0;y<240;++y)large.drawFastHLine(0,y,320,uint16_t(0x1234+y*173));
  large.pushSprite(0,0); check(gfx.wait()==Status::Ok,"large wire transfer");
  for(int y=0;y<240;++y)for(int x:{0,103,104,204,205,319})
    pixel(x,y,large.readPixel(x,y),"DMA chunk boundary pixels");
  gfx.fillRect(12,12,20,20,GFX_BLUE);
  large.fillSprite(GFX_BLACK);large.fillRect(1,1,5,5,GFX_RED);
  large.pushSprite(12,12,GFX_BLACK);
  pixel(12,12,GFX_BLUE,"transparent background");pixel(13,13,GFX_RED,"transparent foreground");
}

void soak() {
  // Warm the allocator before recording retained allocation bytes.
  {GFX_Sprite s(&gfx);s.createSprite(96,64);}
  const int before=mallinfo().uordblks;
  for(int i=0;i<2000;++i) {
    GFX_Sprite s(&gfx);s.setColorDepth((i%4==0)?1:(i%4==1)?4:(i%4==2)?8:16);
    check(s.createSprite(32+i%65,24+i%41)!=nullptr,"soak allocation");
    s.fillSprite(i&15);
    if(i%100==0){s.pushSprite(0,0);check(gfx.wait()==Status::Ok,"soak transfer");}
  }
  const int after=mallinfo().uordblks;
  Serial.print("SOAK_HEAP before=");Serial.print(before);Serial.print(" after=");Serial.println(after);
  check(after==before,"sprite allocation leak");
}

void runValidation() {
  checks=failures=0;
  Serial.println("VALIDATE_BEGIN WioTerminal RGB565 40MHz");
  check(gfx.beginChecked()==Status::Ok,"display initialization");
  check(gfx.beginChecked()==Status::Ok,"idempotent initialization");
  if(gfx.lastError()!=Status::Ok)return;
  rotations(); images(); sprites(); soak();
  gfx.setRotation(3);gfx.fillScreen(failures?GFX_RED:GFX_DARKGREEN);
  gfx.setFont(nice_gfx::fonts::Fixed6x13);gfx.setTextColor(GFX_WHITE);
  gfx.drawString(failures?"VALIDATION FAILED":"VALIDATION PASSED",30,80);
  Serial.print("VALIDATE_DONE checks=");Serial.print(checks);
  Serial.print(" failures=");Serial.println(failures);
}

void setup(){Serial.begin(115200);while(!Serial && millis()<30000)delay(10);rawSpiTiming();runValidation();}
void loop(){if(Serial.available() && Serial.read()=='r')runValidation();delay(10);}
