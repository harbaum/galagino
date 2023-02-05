#ifndef _CONFIG_H_
#define _CONFIG_H_

// disable e.g. if roms are missing
#define ENABLE_PACMAN
#define ENABLE_GALAGA
#define ENABLE_DKONG

#if !defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG)
#error "At least one machine has to be enabled!"
#endif

// check if only one machine is enabled
#if (( defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG)) || \
     (!defined(ENABLE_PACMAN) &&  defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG)) || \
     (!defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) &&  defined(ENABLE_DKONG)))
  #define SINGLE_MACHINE
#endif

// game config
// #define FREEPLAY     // define for free play, no coin button required, but also no attract mode

// include dip switches after defining FREEPLAY as e.g. galaga has a
// freeplay dip 
#include "dip_switches.h"

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

// Pins used for buttons
#define BTN_START_PIN  22
#define BTN_COIN_PIN   21   // if this is not defined, then start will act as coin & start
#define BTN_LEFT_PIN   33
#define BTN_RIGHT_PIN  14
#define BTN_DOWN_PIN   15
#define BTN_UP_PIN      4
#define BTN_FIRE_PIN   12

#endif // _CONFIG_H_
