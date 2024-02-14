#ifndef _NINTENDO_H_
#define _NINTENDO_H_

// ----------------------------
// Standard Libraries
// ----------------------------

#include <Wire.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <NintendoExtensionCtrl.h>
// This library is for interfacing with the Nunchuck

// Can be installed from the library manager
// https://github.com/dmadison/NintendoExtensionCtrl

#ifdef CLASSIC_CTRL
ClassicController input;
#endif
#ifdef NUNCHUK
Nunchuk input;
#endif

void inputSetup() {

  Wire.begin(NINTENDO_SDA, NINTENDO_SCL);
  #ifdef NUNCHUK
  if (!input.connect()) {
    Serial.println("Nunchuk on bus #1 not detected!");
    delay(1000);
  }
  #endif
  #ifdef CLASSIC_CTRL
  if (!input.connect()) {
    Serial.println("Classic Controller on bus #1 not detected!");
    delay(1000);
  }
  #endif
  

}

unsigned char getNintendoInput() {

  boolean success = input.update();  // Get new data from the controller

  if (!success) {  // Ruh roh
    Serial.println("Nintendo Controller device disconnected!");
    return 0;
  }
  else 
  
    {
    #ifdef CLASSIC_CTRL 

    //int joyY = input.leftJoyY(); //uncomment if Joystick should be used
    //int joyX = input.leftJoyX(); //uncomment if Joystick should be used
    

    return (input.dpadLeft() ? BUTTON_LEFT : 0) | //Move Left, comment out if Joystick should be used
           (input.dpadRight() ? BUTTON_RIGHT : 0) | //Move Right, comment out if Joystick should be used
           (input.dpadUp() ? BUTTON_UP : 0) | //Move Up,comment out if Joystick should be used
           (input.dpadDown() ? BUTTON_DOWN : 0) | //Move Down,comment out if Joystick should be used
           (input.buttonMinus() ? BUTTON_COIN : 0) | //Coin
           (input.buttonPlus() ? BUTTON_START : 0) | //start
           (input.buttonHome() ? BUTTON_HOME : 0) |//home

          //  (joyX < 127 - JOYSTICK_MOVE_THRESHOLD) ? BUTTON_LEFT : 0) | //Move Left //uncomment if Joystick should be used
          //  ((joyX > 127 + JOYSTICK_MOVE_THRESHOLD) ? BUTTON_RIGHT : 0) | //Move Right //uncomment if Joystick should be used
          //  ((joyY > 127 + JOYSTICK_MOVE_THRESHOLD) ? BUTTON_UP : 0) | //Move Up //uncomment if Joystick should be used
          //  ((joyY < 127 - JOYSTICK_MOVE_THRESHOLD) ? BUTTON_DOWN : 0) | //Move Down //uncomment if Joystick should be used


           (input.buttonA() ? BUTTON_FIRE : 0) ;
  
  #endif
  #ifdef NUNCHUK 

    // Read a joystick axis (0-255, X and Y)
    // Roughly 127 will be the axis centered
    int joyY = input.joyY();
    int joyX = input.joyX();

    return ((joyX < 127 - JOYSTICK_MOVE_THRESHOLD) ? BUTTON_LEFT : 0) | //Move Left
           ((joyX > 127 + JOYSTICK_MOVE_THRESHOLD) ? BUTTON_RIGHT : 0) | //Move Right
           ((joyY > 127 + JOYSTICK_MOVE_THRESHOLD) ? BUTTON_UP : 0) | //Move Up
           ((joyY < 127 - JOYSTICK_MOVE_THRESHOLD) ? BUTTON_DOWN : 0) | //Move Down
           (input.buttonZ() ? BUTTON_FIRE : 0)|
           (input.buttonC() ? BUTTON_COIN : 0) ;
  
  #endif
    }

  
}

#endif //_NINTENDO_H_
