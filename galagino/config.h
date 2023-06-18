#ifndef _CONFIG_H_
#define _CONFIG_H_

// disable e.g. if roms are missing
#define ENABLE_PACMAN
#define ENABLE_GALAGA
#define ENABLE_DKONG
#define ENABLE_FROGGER
#define ENABLE_DIGDUG

#if !defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG)
#error "At least one machine has to be enabled!"
#endif

// check if only one machine is enabled
#if (( defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG)) || \
     (!defined(ENABLE_PACMAN) &&  defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG)) || \
     (!defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) &&  defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG)) || \
     (!defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) &&  defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG)) || \
     (!defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) &&  defined(ENABLE_DIGDUG)))
  #define SINGLE_MACHINE
#endif

// game config

#define MASTER_ATTRACT_MENU_TIMEOUT  20000   // start games randomly while sitting idle in menu for 20 seconds, undefine to disable

#include "dip_switches.h"

// video config
#define TFT_SPICLK  40000000    // 40 Mhz. Some displays cope with 80 Mhz

#define TFT_CS   5
#define TFT_DC  32
#define TFT_RST 27
#define TFT_BL  13    // don't set if backlight is hard wired
#define TFT_ILI9341   // define for ili9341, otherwise st7789
// #define TFT_VFLIP     // define for upside down

// x and y offset of 224x288 pixels inside the 240x320 screen
#define TFT_X_OFFSET  8
#define TFT_Y_OFFSET 16

#define LED_PIN        16   // pin used for optional WS2812 stripe
#define LED_BRIGHTNESS 50   // range 0..255

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
