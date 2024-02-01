#ifndef _NUNCHUCK_H_
#define _NUNCHUCK_H_

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

Nunchuk nchuk;

void nunchuckSetup() {

  Wire.begin(NUNCHUCK_SDA, NUNCHUCK_SCL);
  if (!nchuk.connect()) {
    Serial.println("Nunchuk on bus #1 not detected!");
    delay(1000);
  }

}

unsigned char getNunchuckInput() {

  boolean success = nchuk.update();  // Get new data from the controller

  if (!success) {  // Ruh roh
    Serial.println("Nunchuck disconnected!");
    return 0;
  }
  else {

    // Read a joystick axis (0-255, X and Y)
    // Roughly 127 will be the axis centered
    int joyY = nchuk.joyY();
    int joyX = nchuk.joyX();

    return ((joyX < 127 - NUNCHUCK_MOVE_THRESHOLD) ? BUTTON_LEFT : 0) | //Move Left
           ((joyX > 127 + NUNCHUCK_MOVE_THRESHOLD) ? BUTTON_RIGHT : 0) | //Move Right
           ((joyY > 127 + NUNCHUCK_MOVE_THRESHOLD) ? BUTTON_UP : 0) | //Move Up
           ((joyY < 127 - NUNCHUCK_MOVE_THRESHOLD) ? BUTTON_DOWN : 0) | //Move Down
           (nchuk.buttonZ() ? BUTTON_FIRE : 0) |
           (nchuk.buttonC() ? BUTTON_EXTRA : 0) ;
  }
}

#endif //_NUNCHUCK_H_
