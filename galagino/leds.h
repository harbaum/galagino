#ifndef LEDS_H
#define LEDS_H

#include "config.h"

#ifdef LED_PIN 

#include <FastLED.h>

// seven LEDs of a 144 leds/m stripe are slightly less than 5cm
#define NUM_LEDS  7

#define LED_BLACK    CRGB::Black
#define LED_RED      CRGB::Red
#define LED_GREEN    CRGB::Green
#define LED_BLUE     CRGB::Blue
#define LED_YELLOW   CRGB::Yellow
#define LED_MAGENTA  CRGB::Magenta
#define LED_CYAN     CRGB::Cyan
#define LED_WHITE    CRGB::White

extern void leds_init(void); 
extern void leds_update(void); 
extern void leds_check_galaga_sprite(struct sprite_S *spr);
extern void leds_state_reset();
  
#endif // LED_PIN 

struct sprite_S {
  unsigned char code, color, flags;
  short x, y; 
};

#endif // LEDS_H
