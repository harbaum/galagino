#ifndef _CONFIG_H_
#define _CONFIG_H_

// game config
// #define FREEPLAY     // define for free play, no coin button required, but also no attract mode
 
// video config
#define TFT_SPICLK  40000000    // 40 Mhz. Some displays cope with 80 Mhz

#define TFT_CS   5
#define TFT_DC  32
#define TFT_RST 27
#define TFT_ILI9341   // define for ili9341, otherwise st7789
// #define TFT_VFLIP            // define for upside down

// x and y offset of 224x288 pixels inside the 240x320 screen
#define TFT_X_OFFSET  8
#define TFT_Y_OFFSET 16

// audio config
// #define SND_DIFF   // set to output differential audio on GPIO25 _and_ inverted on GPIO26

// pins used for buttons
#define BTN_START_PIN  22
#define BTN_COIN_PIN   21
#define BTN_LEFT_PIN   33
#define BTN_RIGHT_PIN  14
#define BTN_FIRE_PIN   12

#endif // _CONFIG_H_
