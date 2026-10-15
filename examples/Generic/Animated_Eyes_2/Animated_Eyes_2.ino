// An adaption of the "UncannyEyes" sketch (see eye_functions tab)
// for the NICE_GFX library. As written the sketch is for driving
// two TFT displays.

// The size of the displayed eye is determined by the screen size and
// resolution. The eye image is 128 pixels wide. In humans the palpebral
// fissure (open eye) width is about 30mm so a low resolution, large
// pixel size display works best to show a scale eye image. Note that
// display manufacturers usually quote the diagonal measurement, so a
// 128 x 128 1.7" display or 128 x 160 2" display is about right.

// The number of displays and chip selects used are defined in the
// config.h tab. The display count can be set to 1. If using one
// TFT and the chip select for that display is already defined in
// the NICE_GFX library then change the chip select pins to -1 in the
// "config.h" tab.

// The wiring for 2 TFT displays to an ESP32 is described in the
// "wiring" tab of this sketch.

// Configuration settings for the eye, eye style, display count,
// chip selects and x offsets can be defined in the sketch "config.h" tab.

// Performance (frames per second = fps) can be improved by using
// DMA (for SPI displays only) on ESP32 and STM32 processors. Use
// as high a SPI clock rate as is supported by the display. 27MHz
// minimum, some displays can be operated at higher clock rates in
// the range 40-80MHz.

// Single defaultEye performance for different processors
//                                  No DMA   With DMA
// ESP8266 (160MHz CPU) 40MHz SPI   36 fps
// ESP32 27MHz SPI                  53 fps     85 fps
// ESP32 40MHz SPI                  67 fps    102 fps
// ESP32 80MHz SPI                  82 fps    116 fps // Note: Few displays work reliably at 80MHz
// STM32F401 55MHz SPI              44 fps     90 fps
// STM32F446 55MHz SPI              83 fps    155 fps
// STM32F767 55MHz SPI             136 fps    197 fps

// DMA can be used with RP2040, STM32 and ESP32 processors when the interface
// is SPI, uncomment the next line:
//#define USE_DMA

// Load TFT driver library
#include <SPI.h>
#include <NICE_GFX.h>
// Dedicated two-screen ST7735 wiring from the original Setup47 example.
// CS=-1 leaves selection to the two per-eye GPIOs in config.h.
DisplayOptions eyePanelOptions() {
  DisplayOptions options;
  options.width = options.height = 128;
  options.tab = ST7735Tab::Green3;
  options.colorOrder = DisplayColorOrder::BGR;
  return options;
}
ST7735_128x160_SPI gfx(-1, 2, 4, 18, 23, 19, 27000000, eyePanelOptions());

// A pixel buffer is used during eye rendering
#define BUFFER_SIZE 1024 // 128 to 1024 seems optimum

#ifdef USE_DMA
  #define BUFFERS 2      // 2 toggle buffers with DMA
#else
  #define BUFFERS 1      // 1 buffer for no DMA
#endif

uint16_t pbuffer[BUFFERS][BUFFER_SIZE]; // Pixel rendering buffer
bool     dmaBuf   = 0;                  // DMA buffer selection

// This struct is populated in config.h
typedef struct {        // Struct is defined before including config.h --
  int8_t  select;       // pin numbers for each eye's screen select line
  int8_t  wink;         // and wink button (or -1 if none) specified there,
  uint8_t rotation;     // also display rotation and the x offset
  int16_t xposition;    // position of eye on the screen
} eyeInfo_t;

#include "config.h"     // ****** CONFIGURATION IS DONE IN HERE ******

extern void setupAnimation(void); // Functions in the user*.cpp files
extern void user_loop(void);

#define SCREEN_X_START 0
#define SCREEN_X_END   SCREEN_WIDTH   // Badly named, actually the "eye" width!
#define SCREEN_Y_START 0
#define SCREEN_Y_END   SCREEN_HEIGHT  // Actually "eye" height

// A simple state machine is used to control eye blinks/winks:
#define NOBLINK 0       // Not currently engaged in a blink
#define ENBLINK 1       // Eyelid is currently closing
#define DEBLINK 2       // Eyelid is currently opening
typedef struct {
  uint8_t  state;       // NOBLINK/ENBLINK/DEBLINK
  uint32_t duration;    // Duration of blink state (micros)
  uint32_t startTime;   // Time (micros) of last state change
} eyeBlink;

struct {                // One-per-eye structure
  int16_t   gfx_cs;     // Chip select pin for each display
  eyeBlink  blink;      // Current blink/wink state
  int16_t   xposition;  // x position of eye image
} eye[NUM_EYES];

uint32_t startTime;  // For FPS indicator

// INITIALIZATION -- runs once at startup ----------------------------------
void setup(void) {
  Serial.begin(115200);
  //while (!Serial);
  Serial.println("Starting");

if (displayBacklight >= 0) {
  // Enable backlight pin, initially off
  Serial.println("Backlight turned off");
  pinMode(displayBacklight, OUTPUT);
  digitalWrite(displayBacklight, LOW);
}

  // User call for additional features
  setupAnimation();

  // Initialise the eye(s), this will set all chip selects low for the gfx.begin()
  initEyes();

  // Initialise TFT
  Serial.println("Initialising displays");
  gfx.begin();

#ifdef USE_DMA
  gfx.initDMA();
#endif

  // Raise chip select(s) so that displays can be individually configured
  if (eye[0].gfx_cs >= 0) digitalWrite(eye[0].gfx_cs, HIGH);
  if (NUM_EYES > 1) if (eye[1].gfx_cs >= 0) digitalWrite(eye[1].gfx_cs, HIGH);

  for (uint8_t e = 0; e < NUM_EYES; e++) {
    if (eye[e].gfx_cs >= 0) digitalWrite(eye[e].gfx_cs, LOW);
    gfx.setRotation(eyeInfo[e].rotation);
    gfx.fillScreen(GFX_BLACK);
    if (eye[e].gfx_cs >= 0) digitalWrite(eye[e].gfx_cs, HIGH);
  }

if (displayBacklight >= 0) {
  Serial.println("Backlight now on!");
  analogWrite(displayBacklight, BACKLIGHT_MAX);
}

  startTime = millis(); // For frame-rate calculation
}

// MAIN LOOP -- runs continuously after setup() ----------------------------
void loop() {
  updateEye();
}
