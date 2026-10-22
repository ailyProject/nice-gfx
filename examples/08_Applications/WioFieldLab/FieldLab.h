#pragma once
#include "Hardware.h"
#include "ui.h"
#include "TouchAdapter.h"
#include "Config.h"
#include "Buzzer.h"
#include "WifiSurvey.h"
#include <Wire.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#if WIO_ENABLE_SD
#include <Seeed_FS.h>
#include <SD/Seeed_SD.h>
#endif
#if WIO_ENABLE_WIFI
#include <rpcWiFi.h>
#endif
#if WIO_ENABLE_IR
#define DISABLE_CODE_FOR_RECEIVER
#include <IRremote.hpp>
#endif

namespace fieldlab {
using namespace nice_gfx::htmlui;
constexpr char kUiRevision[]="seeed-modern-v4.1-ir-fix";
#ifdef WIO_FIELDLAB_TILE_LINES
constexpr size_t kAppTilePixels=320*WIO_FIELDLAB_TILE_LINES;
#else
constexpr size_t kAppTilePixels=kTilePixels;
#endif
alignas(4) static uint16_t tile[kAppTilePixels];
View ui(gfx,tile,nullptr,kAppTilePixels);
} // namespace fieldlab
#include "WioPerf.h"
namespace fieldlab {
bool ready=false,imuOk=false,hold=false,muted=false,logging=false,cardOk=false;
FieldLabBuzzer buzzer;
bool pendingScan=false,pendingCard=false;
bool scanRunning=false;uint32_t scanStarted=0;
bool radarAnimating=false,motionPreview=false;
const char *radarMotionIds[]={"radio-tail-0","radio-tail-1","radio-tail-2","radio-tail-3","radio-tail-4","radio-beam"};
uint32_t scanAfter=0,lastSensors=0,lastPaint=0,lastLog=0,lastReport=0,rows=0;
int light=0,amplitude=0,peak=0,brightness=70,micMin=4095,micMax=0;
uint16_t micSamples=0;
float ax=0,ay=0,az=0,roll=0,pitch=0,rollZero=0,pitchZero=0;
float bubbleOffsetX=0,bubbleOffsetY=0;uint32_t lastBubbleFrame=0;
float trailNearX=0,trailNearY=0,trailFarX=0,trailFarY=0;
int32_t brightnessPose=70*256;uint32_t lastBrightnessPose=0;
int32_t history[32]={};size_t historyCount=0;
uint8_t imuAddress=0x19,imuIdentity=0;uint32_t lastImuRetry=0;
char fileName[20]="",logState[44]="SD not checked",radioState[44]="RADIO IDLE",networks[3][48]={},irState[64]="NEC test profile / address 0x00";
#if WIO_ENABLE_SD
File logFile;
#endif
const char *routes[]={"/","/level","/sound","/log","/remote","/radio","/settings"};
const char *routeFocus[]={"open-level","level-zero","sound-hold","log-toggle","ir-send-0","radio-scan","brightness-down"};

void text(const char *id,const char *value){ui.setText(ui_config::id(id),value);}
void beep(){if(!muted)buzzer.play(WIO_BUZZER,2400,40);}
void setBrightness(){analogWrite(LCD_BACKLIGHT,brightness*255/100);}
bool writeImu(uint8_t reg,uint8_t value){
  Wire1.beginTransmission(imuAddress);Wire1.write(reg);Wire1.write(value);return Wire1.endTransmission()==0;
}
bool readImu(uint8_t reg,uint8_t *out,uint8_t size){
  Wire1.beginTransmission(imuAddress);Wire1.write(reg);
  if(Wire1.endTransmission()!=0)return false;
  if(Wire1.requestFrom(imuAddress,size)!=size){while(Wire1.available())Wire1.read();return false;}
  for(uint8_t i=0;i<size;++i)out[i]=Wire1.read();return true;
}
bool initializeImu(){
  // Probe both SA0 addresses and validate WHO_AM_I rather than accepting any
  // I2C responder. STOP between accesses matches Seeed's LIS3DHTR driver.
  for(uint8_t address=0x18;address<=0x19;++address){
    imuAddress=address;
    if(readImu(0x0F,&imuIdentity,1)&&imuIdentity==0x33&&writeImu(0x20,0x47)&&writeImu(0x23,0x88))return true;
  }return false;
}
void beginSensors(){
  analogReadResolution(12);analogWriteResolution(8);
  pinMode(WIO_LIGHT,INPUT);pinMode(WIO_MIC,INPUT);pinMode(WIO_BUZZER,OUTPUT);
  Wire1.begin();Wire1.setClock(100000);
  imuOk=initializeImu();
  Serial.print("IMU WHO_AM_I=");Serial.print(imuIdentity,HEX);Serial.print(" ok=");Serial.println(imuOk);
#if WIO_ENABLE_IR
  IrSender.begin(WIO_IR);
#endif
  setBrightness();
}
void sample(uint32_t now){
  // Short ADC bursts, no delay/catch-up loop. This measures a relative envelope,
  // not calibrated SPL or a uniformly sampled spectrum.
  for(uint8_t i=0;i<8;++i){int v=analogRead(WIO_MIC);micMin=min(micMin,v);micMax=max(micMax,v);++micSamples;}
  if(uint32_t(now-lastSensors)<40)return;lastSensors=now;
  light=analogRead(WIO_LIGHT);
  if(micSamples){amplitude=micMax-micMin;peak=max(peak,amplitude);micMin=4095;micMax=0;micSamples=0;}
  if(!hold){
    if(historyCount<32)history[historyCount++]=amplitude;
    else{memmove(history,history+1,31*sizeof(history[0]));history[31]=amplitude;}
  }
  if(imuOk){
    uint8_t b[6];imuOk=readImu(0x28|0x80,b,6);
    if(imuOk){
      ax=(int16_t(uint16_t(b[1])<<8|b[0])>>4)*0.001f;
      ay=(int16_t(uint16_t(b[3])<<8|b[2])>>4)*0.001f;
      az=(int16_t(uint16_t(b[5])<<8|b[4])>>4)*0.001f;
      // Wio's accelerometer Z points into the screen. Screen-up rest is -1g;
      // flip into display coordinates so a near-flat board reads near zero.
      roll=atan2f(-ay,-az)*57.29578f;
      pitch=atan2f(ax,sqrtf(ay*ay+az*az))*57.29578f;
    }
  }
  else if(uint32_t(now-lastImuRetry)>=2000){lastImuRetry=now;imuOk=initializeImu();}
}
void stopLog(const char *status){
#if WIO_ENABLE_SD
  if(logFile){logFile.flush();logFile.close();}
#endif
  logging=false;snprintf(logState,sizeof(logState),"%s",status);
}
void beginCard(){
  stopLog("SD unavailable");cardOk=false;
#if WIO_ENABLE_SD
  SD.end();cardOk=SD.begin(SDCARD_SS_PIN,SDCARD_SPI,4000000);
  snprintf(logState,sizeof(logState),"%s",cardOk?"SD ready / press START":"NO CARD / insert and RETRY");
#else
  snprintf(logState,sizeof(logState),"SD disabled in Config.h");
#endif
}
void startLog(){
#if WIO_ENABLE_SD
  if(!cardOk){snprintf(logState,sizeof(logState),"NO CARD / press RETRY");return;}
  // Each session gets a new file; existing recordings are never overwritten.
  for(unsigned i=0;i<1000;++i){
    snprintf(fileName,sizeof(fileName),"/WIO%03u.CSV",i);
    if(SD.exists(fileName))continue;
    logFile=SD.open(fileName,FILE_WRITE);
    const char header[]="uptime_ms,light_adc,mic_pp_adc,ax_mg,ay_mg,az_mg,imu_ok\r\n";
    if(!logFile||logFile.write((const uint8_t *)header,sizeof(header)-1)!=sizeof(header)-1){stopLog("SD open/header write failed");return;}
    rows=0;lastLog=millis();logging=true;snprintf(logState,sizeof(logState),"RECORDING / 1 Hz");return;
  }
  snprintf(logState,sizeof(logState),"All 1000 filenames in use");
#else
  snprintf(logState,sizeof(logState),"SD disabled in Config.h");
#endif
}
void record(uint32_t now){
#if WIO_ENABLE_SD
  if(!logging||uint32_t(now-lastLog)<1000)return;lastLog=now;
  char line[128];int size=snprintf(line,sizeof(line),"%lu,%d,%d,%ld,%ld,%ld,%d\r\n",(unsigned long)now,light,amplitude,
    imuOk?lroundf(ax*1000):0L,imuOk?lroundf(ay*1000):0L,imuOk?lroundf(az*1000):0L,int(imuOk));
  if(size<=0||size>=int(sizeof(line))||logFile.write((const uint8_t *)line,size)!=size){cardOk=false;stopLog("SD write failed / stopped");return;}
  ++rows;if(rows%5==0)logFile.flush();
#else
  (void)now;
#endif
}
void angleText(const char *id,float degrees){char b[20];int n=lroundf(degrees*10);snprintf(b,sizeof(b),"%s%d.%d",n<0?"-":"",abs(n)/10,abs(n)%10);text(id,b);}
void motionVisible(const char *id,bool visible){
  const Id key=ui_config::id(id);ui.setVisible(key,visible);
  if(visible&&!ui.animationRunning(key))ui.playAnimation(key,millis());
  else if(!visible&&ui.animationRunning(key))ui.stopAnimation(key);
}
void brightnessFeedback(){
  ui.setVisible(ui_config::id("settings-glint"),true);ui.playAnimation(ui_config::id("settings-glint"),millis());
}
void animateBrightness(uint32_t now){
  if(strcmp(ui.path(),"/settings")){lastBrightnessPose=now;return;}
  const uint32_t dt=std::min<uint32_t>(uint32_t(now-lastBrightnessPose),64);lastBrightnessPose=now;
  const int32_t target=brightness*256,difference=target-brightnessPose;
  if(abs(difference)<128)brightnessPose=target;
  else brightnessPose+=difference*int32_t(dt)/140;
  const int value=(brightnessPose+128)/256;
  ui.setValue(ui_config::id("settings-brightness-bar"),value);
  ui.setBounds(ui_config::id("settings-glint"),Rect{22+(276*value/100)-3,107,6,6});
}
void refresh(){
  char b[96];
  if(!strcmp(ui.path(),"/")){
    snprintf(b,sizeof(b),"%04d",light);text("home-light",b);
    if(imuOk)snprintf(b,sizeof(b),"%+ld %+ld %+ld",lroundf(ax*1000),lroundf(ay*1000),lroundf(az*1000));
    else snprintf(b,sizeof(b),"IMU unavailable");text("home-motion",b);
    for(int axis=0;axis<3;++axis){const char *ids[]={"home-x-bar","home-y-bar","home-z-bar"};const float values[]={ax,ay,az};ui.setValue(ui_config::id(ids[axis]),imuOk?constrain(lroundf(values[axis]*1000)+2000,0L,4000L):0);}
  }else if(!strcmp(ui.path(),"/level")){
    if(imuOk){
      angleText("level-roll",roll-rollZero);angleText("level-pitch",pitch-pitchZero);
      // Static tilt only: the LIS3DHTR has no gyroscope or magnetometer.
      // Use actual frame time, not a fixed per-frame factor. The EMA removes
      // sensor chatter; a speed limit keeps large tilts and ZERO from jumping.
      const uint32_t now=millis();
      if(!lastBubbleFrame)lastBubbleFrame=now;
      const float dt=uint32_t(now-lastBubbleFrame)*0.001f;lastBubbleFrame=now;
      const float alpha=1.0f-expf(-dt/0.14f),maxStep=90.0f*dt;
      const float targetX=constrain(roll-rollZero,-35.0f,35.0f);
      const float targetY=constrain(pitch-pitchZero,-35.0f,35.0f);
      bubbleOffsetX+=constrain((targetX-bubbleOffsetX)*alpha,-maxStep,maxStep);
      bubbleOffsetY+=constrain((targetY-bubbleOffsetY)*alpha,-maxStep,maxStep);
      const int x=150+lroundf(bubbleOffsetX),y=95+lroundf(bubbleOffsetY);
      ui.setBounds(ui_config::id("level-bubble"),Rect{x,y,20,20});
      ui.setBounds(ui_config::id("level-dot"),Rect{x+7,y+7,6,6});
      trailNearX+=(bubbleOffsetX-trailNearX)*0.4f;trailNearY+=(bubbleOffsetY-trailNearY)*0.4f;
      trailFarX+=(trailNearX-trailFarX)*0.25f;trailFarY+=(trailNearY-trailFarY)*0.25f;
      ui.setBounds(ui_config::id("level-trail-near"),Rect{155+lroundf(trailNearX),100+lroundf(trailNearY),10,10});
      ui.setBounds(ui_config::id("level-trail-far"),Rect{157+lroundf(trailFarX),102+lroundf(trailFarY),6,6});
    }else lastBubbleFrame=millis();
    text("level-status",imuOk?"Static tilt / +/-2g":"IMU unavailable");
  }else if(!strcmp(ui.path(),"/sound")){
    motionVisible("sound-scan-cursor",!hold);
    if(!hold){
      snprintf(b,sizeof(b),"%04d",amplitude);text("sound-value",b);
      int32_t windowPeak=64,plot[32];for(size_t i=0;i<historyCount;++i)windowPeak=max(windowPeak,history[i]);
      for(size_t i=0;i<historyCount;++i)plot[i]=history[i]*4095/windowPeak;
      ui.setChartSeries(ui_config::id("sound-chart"),0,plot,historyCount);ui.setValue(ui_config::id("sound-meter"),amplitude);
    }
    snprintf(b,sizeof(b),"%s / AUTO / peak %d ADC",hold?"HOLD":"LIVE",peak);text("sound-status",b);
  }else if(!strcmp(ui.path(),"/log")){
    text("log-state",logState);snprintf(b,sizeof(b),"%05lu",(unsigned long)rows);text("log-count",b);ui.setVisible(ui_config::id("log-led"),logging);
    motionVisible("log-write-flow",logging);
    snprintf(b,sizeof(b),"%s / 1 Hz",fileName[0]?fileName:"New file on START");text("log-file",b);
  }else if(!strcmp(ui.path(),"/remote")){
    text("ir-status",irState);
  }else if(!strcmp(ui.path(),"/radio")){
    text("radio-state",radioState);for(int i=0;i<3;++i){char id[12];snprintf(id,sizeof(id),"radio-%d",i);text(id,networks[i]);}
    const bool scanning=pendingScan||scanRunning||motionPreview;
    ui.setVisible(ui_config::id("radio-sweep"),scanning);
    if(scanning&&!radarAnimating){
      const uint32_t epoch=millis();for(const char *id:radarMotionIds)ui.playAnimation(ui_config::id(id),epoch);
    }else if(!scanning){
      for(const char *id:radarMotionIds)if(ui.animationRunning(ui_config::id(id)))ui.stopAnimation(ui_config::id(id));
    }
    radarAnimating=scanning;
    ui.setEnabled(ui_config::id("radio-scan"),!pendingScan&&!scanRunning);
  }else if(!strcmp(ui.path(),"/settings")){
    snprintf(b,sizeof(b),"BACKLIGHT / %d%%",brightness);text("settings-backlight",b);text("settings-status",muted?"Buzzer muted / RAM settings":"Buzzer enabled / RAM settings");
    animateBrightness(millis());
  }
}
void onRoute(void *,Event &){lastBubbleFrame=millis();radarAnimating=false;if(strcmp(ui.path(),"/radio"))motionPreview=false;refresh();}
void zeroLevel(void *,Event &){if(imuOk){rollZero=roll;pitchZero=pitch;ui.setVisible(ui_config::id("level-settle"),true);ui.playAnimation(ui_config::id("level-settle"),millis());beep();}}
void toggleHold(void *,Event &){hold=!hold;}
void resetPeak(void *,Event &){peak=0;}
void toggleLogging(void *,Event &){if(logging)stopLog("STOPPED / safe to remove");else startLog();beep();}
void retryCard(void *,Event &){pendingCard=true;}
void scanWifi(void *,Event &){if(pendingScan||scanRunning)return;pendingScan=true;scanAfter=millis()+300;snprintf(radioState,sizeof(radioState),"SCANNING...");refresh();}
void dimDisplay(void *,Event &){brightness=max(10,brightness-10);setBrightness();brightnessFeedback();}
void brightenDisplay(void *,Event &){brightness=min(100,brightness+10);setBrightness();brightnessFeedback();}
void testBuzzer(void *,Event &){beep();}
void toggleMute(void *,Event &){muted=!muted;if(muted)buzzer.stop(WIO_BUZZER);}
void sendIr(size_t index){
  const uint32_t epoch=millis();for(const char *id:{"ir-orbit-0","ir-orbit-1","ir-orbit-2"})ui.playAnimation(ui_config::id(id),epoch);
#if WIO_ENABLE_IR
  // IRremote generates a measured 38 kHz software carrier; one finite NEC frame.
  const uint32_t started=micros();
  buzzer.stop(WIO_BUZZER);IrSender.sendNEC(kIrAddress,kIrCommands[index],0);
  Serial.print("IR_TX cmd=");Serial.print(kIrCommands[index],HEX);
  Serial.print(" elapsed_us=");Serial.println(uint32_t(micros()-started));
  snprintf(irState,sizeof(irState),"NEC sent: %04X / %02X",kIrAddress,kIrCommands[index]);text("ir-status",irState);
#else
  (void)index;snprintf(irState,sizeof(irState),"IR disabled in Config.h");text("ir-status",irState);
#endif
}
void sendIrOne(void *,Event &){sendIr(0);}void sendIrTwo(void *,Event &){sendIr(1);}void sendIrThree(void *,Event &){sendIr(2);}
void finishScan(int count){
  scanRunning=false;
#if WIO_ENABLE_WIFI
  if(count<0)snprintf(radioState,sizeof(radioState),"SCAN FAILED / press RETRY");
  else snprintf(radioState,sizeof(radioState),"%d networks found",count);
  wifi_survey::TopNetworks top;
  for(int i=0;i<count;++i){const String name=WiFi.SSID(i);top.add(name.c_str(),WiFi.RSSI(i));}
  for(int j=0;j<3;++j)top.format(j,networks[j],sizeof(networks[j]));
  WiFi.scanDelete();
#else
  snprintf(radioState,sizeof(radioState),"Wi-Fi disabled in Config.h");
#endif
  refresh();Serial.print("SCAN ");Serial.print(radioState);Serial.print(" code=");Serial.print(count);Serial.print(" elapsed_ms=");Serial.println(millis()-scanStarted);
}
void runScan(){
  pendingScan=false;
#if WIO_ENABLE_WIFI
  // SDK event callbacks collect the scan results on their existing RPC task.
  // UI, sensor sampling and logging keep running while the radio scans.
  scanStarted=millis();const int count=WiFi.scanNetworks(true,false,false,500);
  Serial.print("SCAN START code=");Serial.print(count);Serial.print(" call_ms=");Serial.println(millis()-scanStarted);
  if(count==WIFI_SCAN_RUNNING){scanRunning=true;refresh();return;}
  finishScan(count);
#else
  finishScan(-2);
#endif
}
void pollScan(uint32_t now){
#if WIO_ENABLE_WIFI
  if(!scanRunning)return;
  const int count=WiFi.scanComplete();
  if(count==WIFI_SCAN_RUNNING&&uint32_t(now-scanStarted)<12000)return;
  finishScan(count);
#else
  (void)now;
#endif
}
void report(){
  if(!Serial)return;
  const auto mem=ui.memoryStats();
  Serial.print("WIO route=");Serial.print(ui.path());Serial.print(" light=");Serial.print(light);
  Serial.print(" mic_pp=");Serial.print(amplitude);Serial.print(" imu=");Serial.print(imuOk);
  Serial.print(" xyz_mg=");Serial.print(lroundf(ax*1000));Serial.print(',');Serial.print(lroundf(ay*1000));Serial.print(',');Serial.print(lroundf(az*1000));
  Serial.print(" ui_bytes=");Serial.print(mem.totalBytes());
#if WIO_ENABLE_WIFI
  // rpcUnified runs setup/loop on a FreeRTOS task. A stack-address minus sbrk
  // estimate would underflow because that stack lives inside the RTOS heap.
  Serial.print(" rtos_free=");Serial.print(xPortGetFreeHeapSize());
  Serial.print(" stack_free=");Serial.print(uxTaskGetStackHighWaterMark(nullptr)*sizeof(StackType_t));
#endif
  Serial.print(" imu_who=");Serial.print(imuIdentity,HEX);
  Serial.print(" imu_addr=");Serial.print(imuAddress,HEX);
  Serial.print(" hold=");Serial.print(hold);Serial.print(" brightness=");Serial.print(brightness);
  Serial.print(" sd=");Serial.print(cardOk);Serial.print(" logging=");Serial.print(logging);
  Serial.print(" scanning=");Serial.print(pendingScan||scanRunning);
  Serial.print(" panel_pixel=");Serial.print(gfx.readPixel(319,210),HEX);
  Serial.print(" rows=");Serial.print(rows);
  Serial.print(" ui_rev=");Serial.print(kUiRevision);Serial.println();
  if(!strcmp(ui.path(),"/log")){Serial.print("SD ");Serial.println(logState);}
  if(!strcmp(ui.path(),"/remote")){
    Serial.print("IR_STATE ");Serial.print(irState);Serial.print(" animations=");
    unsigned active=0;for(const char *id:{"ir-orbit-0","ir-orbit-1","ir-orbit-2"})active+=ui.animationRunning(ui_config::id(id));
    Serial.println(active);
  }
  if(!strcmp(ui.path(),"/radio"))for(int j=0;j<3;++j){
    char id[12];snprintf(id,sizeof(id),"radio-%d",j);
    // Distinguish scanned application data from the native text binding.
    Serial.print("SSID_ROW ");Serial.print(j);Serial.print("\tvalue=");Serial.print(networks[j]);
    Serial.print("\tshown=");const char *shown=ui.text(ui_config::id(id));Serial.println(shown?shown:"");
  }
}
void capture(){
  // Capture actual ILI9341 GRAM through MISO, not a host-side rendering.
  ui.finish();uint8_t line[320*3];Serial.println("WIO_RGB888 320 240");
  for(int y=0;y<240;++y){
    gfx.readRectRGB(0,y,320,1,line);
    size_t offset=0;uint32_t stalledAt=millis();
    while(offset<sizeof(line)){
      if(!Serial||uint32_t(millis()-stalledAt)>2000)return;
      const size_t size=min(size_t(64),sizeof(line)-offset);
      const size_t sent=Serial.write(line+offset,size);
      // This core exposes a USB timeout as unsigned -1; it is not a count.
      if(sent&&sent<=size){offset+=sent;stalledAt=millis();}else delay(1);
      yield();
    }
  }
}
bool writeCaptureBytes(const uint8_t *bytes,size_t size){
  size_t offset=0;uint32_t stalledAt=millis();
  while(offset<size){
    if(!Serial||uint32_t(millis()-stalledAt)>2000)return false;
    const size_t requested=min(size_t(64),size-offset);
    const size_t sent=Serial.write(bytes+offset,requested);
    if(sent&&sent<=requested){offset+=sent;stalledAt=millis();}else delay(1);
    yield();
  }
  return true;
}
void captureRadar(bool full=false){
  // Read the actual panel ROI. RLE keeps a diagnostic snapshot short enough
  // to capture several phases during a real scan, with no framebuffer.
  ui.finish();uint8_t line[320*3],packet[63];size_t used=0;
  const int width=full?320:128,height=full?240:61,left=full?0:96,top=full?0:58;
  uint16_t previous=0;uint8_t count=0;
  if(full)Serial.println("WIO_FRAME_RLE 320 240");
  else {Serial.print("WIO_RADAR_RLE 128 61 scan=");Serial.println(pendingScan||scanRunning?1:0);}
  auto emit=[&]()->bool{
    if(!count)return true;
    packet[used++]=count;packet[used++]=previous>>8;packet[used++]=previous&255;
    if(used==sizeof(packet)){if(!writeCaptureBytes(packet,used))return false;used=0;}
    return true;
  };
  for(int y=0;y<height;++y){
    gfx.readRectRGB(left,top+y,width,1,line);
    for(int x=0;x<width;++x){
      const uint8_t *pixel=line+x*3;
      const uint16_t color=(uint16_t(pixel[0]&0xF8)<<8)|(uint16_t(pixel[1]&0xFC)<<3)|(pixel[2]>>3);
      if(count&&color==previous&&count<255){++count;continue;}
      if(!emit())return;previous=color;count=1;
    }
  }
  if(emit()&&used)writeCaptureBytes(packet,used);
}
void serialInput(){
  if(!Serial.available())return;const char cmd=Serial.read();
  if(cmd>='0'&&cmd<='6'){const size_t route=cmd-'0';ui.replace(routes[route]);ui.focus(ui_config::id(routeFocus[route]));}
  else if(cmd=='n')ui.inputButton(ui_config::id("down"),KeyGesture::Press,millis());
  // Serial a is a logical confirm diagnostic, independent of the physical A
  // key (HOME). Inject C (SELECT) so existing diagnostic scripts stay useful.
  else if(cmd=='a')ui.inputButton(ui_config::id("c"),KeyGesture::Short,millis());
  else if(cmd=='b')ui.back();else if(cmd=='c')ui.replace("/");
  else if(cmd=='p')capture();else if(cmd=='r')captureRadar();else if(cmd=='P')captureRadar(true);else if(cmd=='v')report();
  else if(cmd=='m')startPerf();
  else if(cmd=='s'&&!strcmp(ui.path(),"/radio")){motionPreview=!motionPreview;snprintf(radioState,sizeof(radioState),motionPreview?"MOTION PREVIEW / no radio I/O":"RADIO IDLE");refresh();}
}
void setup(){
  Serial.begin(115200); // Never wait for USB; standalone power must work.
  Serial.print("WIO BUILD ui_rev=");Serial.print(kUiRevision);
  Serial.println(" build=20261022-ir-fix-v4.1");
  // Regression probe: LCD pins must never be accepted on the header SPI bus.
  {nice_gfx::ArduinoSpi wrong(SPI,nice_gfx::SpiPins(LCD_SCK_PIN,LCD_MOSI_PIN,LCD_MISO_PIN,LCD_SS_PIN,LCD_DC));
   if(wrong.begin()!=nice_gfx::Status::Unsupported)return;}
  if(!beginDisplayHardware())return;
  beginSensors();
  if(ui_config::load(ui)!=nice_gfx::Status::Ok)return;
  bool bound=ui.on(0,EventType::RouteChange,onRoute);
#define WIO_BIND(name) bound=ui_config::bind_##name(ui,name,nullptr)&&bound
  WIO_BIND(zeroLevel);WIO_BIND(toggleHold);WIO_BIND(resetPeak);WIO_BIND(toggleLogging);WIO_BIND(retryCard);WIO_BIND(scanWifi);
  WIO_BIND(dimDisplay);WIO_BIND(brightenDisplay);WIO_BIND(testBuzzer);WIO_BIND(toggleMute);WIO_BIND(sendIrOne);WIO_BIND(sendIrTwo);WIO_BIND(sendIrThree);
#undef WIO_BIND
  ready=bound;pendingCard=true;
  // Real firmware always replaces the editor's illustrative chart data.
  for(auto &network:networks)snprintf(network,sizeof(network),"--");
  refresh();
  for(const char *id:{"home-core","home-live","home-link-flow","home-link-flow-2","home-sample-flow"})ui.playAnimation(ui_config::id(id),millis());
}
void loop(){
  if(!ready){
    if(Serial&&uint32_t(millis()-lastReport)>=2000){lastReport=millis();Serial.println("WIO startup failed: check display/mount/bindings");}
    while(Serial.available())Serial.read();delay(10);return;
  }const uint32_t loopAt=micros(),now=millis();
  sample(now);serialInput();record(now);
  if(pendingCard&&!ui.refreshing()){pendingCard=false;beginCard();}
  if(pendingScan&&int32_t(now-scanAfter)>=0&&!ui.refreshing())runScan();
  // Starting a scan can cross a millisecond tick or wait for radio setup.
  // A timestamp captured before scanStarted would underflow into a timeout.
  pollScan(millis());
  if(uint32_t(now-lastPaint)>=50){lastPaint=now;refresh();}
  ready=pollUi(ui,now);
  if(!ready&&perf.active)++perf.errors;
  samplePerf(micros()-loopAt);
  if(uint32_t(now-lastReport)>=2000){lastReport=now;report();}
  // Physical key edges are diagnostic evidence; serial-injected UI keys are
  // deliberately separate and do not claim to test the actual switches.
  static uint8_t lastKeys=0;uint8_t keys=0;const int pins[]={31,34,32,33,35,28,29,30};
  for(int i=0;i<8;++i)if(digitalRead(pins[i])==LOW)keys|=1<<i;
  if(keys!=lastKeys&&Serial&&Serial.availableForWrite()>32){lastKeys=keys;Serial.print("KEYS ");Serial.println(keys,HEX);}
  // Let lower-priority SDK/idle work run too: taskYIELD alone only schedules
  // peers of this high-priority rpcUnified application task.
#ifndef WIO_FIELDLAB_YIELD_MS
#define WIO_FIELDLAB_YIELD_MS 4
#endif
  // Drain successive tiles within a short CPU slice. Sleeping after every
  // tile throttles tiny dirty regions and wastes most of the SPI budget.
  // Inputs are still polled each loop; SDK tasks get a real sleep every slice.
  static uint32_t lastYield=0;
  if(!ui.refreshing()||uint32_t(millis()-lastYield)>=WIO_FIELDLAB_YIELD_MS){lastYield=millis();delay(1);}
}
} // namespace fieldlab
