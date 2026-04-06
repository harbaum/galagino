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


bool nun=false;
bool classic=false;



ExtensionPort check; //Instance to check Controllertype
ClassicController inputC; //Classic Instance
Nunchuk inputN; //Nunchuk instance

void inputSetup() {

  Wire.begin(NINTENDO_SDA, NINTENDO_SCL); //use our pins
  check.begin();
	check.connect();
  while (!check.connect()) {
		Serial.println("Controller not detected!");
		delay(1000);};
    
	ExtensionType conType = check.getControllerType(); //check our controllertype

	switch (conType)    {
		case(ExtensionType::NoController):
			Serial.println("No controller detected");
			break;
		case(ExtensionType::UnknownController):
			Serial.println("Unknown controller connected");
			break;
		case(ExtensionType::Nunchuk):
			Serial.println("Nunchuk connected!");
      inputN.begin(); //instantiate Nunchuk-Inputfunction
	    inputN.connect();
      while (!inputN.connect()) {
		      Serial.println("Controller not detected!");
		      delay(1000);};
      nun=true; // we have a Nunchuk connected! 
		break;

		case(ExtensionType::ClassicController):
			Serial.println("Classic Controller connected!");
      inputC.begin(); //instantiate ClassicController-Inputfunction
	    inputC.connect();
      while (!inputC.connect()) {
		      Serial.println("Controller not detected!");
		      delay(1000);};
      classic=true; // we have a ClassicController connected!
			break;

		case(ExtensionType::GuitarController):
			Serial.println("Guitar controller connected! WTF?"); //Not yet supported
			break;

		case(ExtensionType::DrumController):
			Serial.println("Drum set controller connected! WTF?"); //Not yet supported
			break;

		case(ExtensionType::DJTurntableController):
			Serial.println("DJ turntable connected! WTF?"); //Not yet supported
			break;

		case(ExtensionType::uDrawTablet):
			Serial.println("uDraw Tablet connected! WTF?"); //Not yet supported
			break;

		case(ExtensionType::DrawsomeTablet):
			Serial.println("Drawsome Tablet connected! WTF?"); //Not yet supported
			break;

		default: break;
	}
};





  
  


unsigned char getNintendoInput() {
  boolean success = false;
  if (classic)  success = inputC.update();  // Get new data from the Classic-Controller
  else if (nun) success = inputN.update();  // Get new data from the Nunchuk
  

  if (!success) {  // Ruh roh
    Serial.println("Nintendo Controller device disconnected!");
    return 0;
  }
  else 
  
    
    if (classic) //input definitions from Classic Controller
    { 
#ifdef CLASSIC_DPAD
  #ifndef CLASSIC_LJOY
    //int joyY = inputC.leftJoyY(); //uncomment if Joystick should be used
    //int joyX = inputC.leftJoyX(); //uncomment if Joystick should be used
    

    return (inputC.dpadLeft() ? BUTTON_LEFT : 0) | //Move Left, comment out if Joystick should be used
           (inputC.dpadRight() ? BUTTON_RIGHT : 0) | //Move Right, comment out if Joystick should be used
           (inputC.dpadUp() ? BUTTON_UP : 0) | //Move Up,comment out if Joystick should be used
           (inputC.dpadDown() ? BUTTON_DOWN : 0) | //Move Down,comment out if Joystick should be used
           (inputC.buttonMinus() ? BUTTON_COIN : 0) | //Coin
           (inputC.buttonPlus() ? BUTTON_START : 0) | //start
           (inputC.buttonHome() ? BUTTON_HOME : 0) |//home

          //  ((joyX < 127 - JOYSTICK_MOVE_THRESHOLD) ? BUTTON_LEFT : 0) | //Move Left //uncomment if Joystick should be used
          //  ((joyX > 127 + JOYSTICK_MOVE_THRESHOLD) ? BUTTON_RIGHT : 0) | //Move Right //uncomment if Joystick should be used
          //  ((joyY > 127 + JOYSTICK_MOVE_THRESHOLD) ? BUTTON_UP : 0) | //Move Up //uncomment if Joystick should be used
          //  ((joyY < 127 - JOYSTICK_MOVE_THRESHOLD) ? BUTTON_DOWN : 0) | //Move Down //uncomment if Joystick should be used


           (inputC.buttonA() ? BUTTON_FIRE : 0) ;
  #endif
  #endif

#ifdef CLASSIC_LJOY
 #ifndef CLASSIC_DPAD
    int joyY = inputC.leftJoyY(); //uncomment if Joystick should be used
    int joyX = inputC.leftJoyX(); //uncomment if Joystick should be used
    

    return //(inputC.dpadLeft() ? BUTTON_LEFT : 0) | //Move Left, comment out if Joystick should be used
           //(inputC.dpadRight() ? BUTTON_RIGHT : 0) | //Move Right, comment out if Joystick should be used
           //(inputC.dpadUp() ? BUTTON_UP : 0) | //Move Up,comment out if Joystick should be used
           //(inputC.dpadDown() ? BUTTON_DOWN : 0) | //Move Down,comment out if Joystick should be used
           (inputC.buttonMinus() ? BUTTON_COIN : 0) | //Coin
           (inputC.buttonPlus() ? BUTTON_START : 0) | //start
           (inputC.buttonHome() ? BUTTON_HOME : 0) |//home

            ((joyX < 127 - JOYSTICK_MOVE_THRESHOLD) ? BUTTON_LEFT : 0) | //Move Left //uncomment if Joystick should be used
            ((joyX > 127 + JOYSTICK_MOVE_THRESHOLD) ? BUTTON_RIGHT : 0) | //Move Right //uncomment if Joystick should be used
            ((joyY > 127 + JOYSTICK_MOVE_THRESHOLD) ? BUTTON_UP : 0) | //Move Up //uncomment if Joystick should be used
            ((joyY < 127 - JOYSTICK_MOVE_THRESHOLD) ? BUTTON_DOWN : 0) | //Move Down //uncomment if Joystick should be used


           (inputC.buttonA() ? BUTTON_FIRE : 0) ;
  
   #endif
  #endif

  #ifdef CLASSIC_LJOY
   #ifdef CLASSIC_DPAD
    int joyY = inputC.leftJoyY(); //uncomment if Joystick should be used
    int joyX = inputC.leftJoyX(); //uncomment if Joystick should be used
    

    return (inputC.dpadLeft() ? BUTTON_LEFT : 0) | //Move Left, comment out if Joystick should be used
           (inputC.dpadRight() ? BUTTON_RIGHT : 0) | //Move Right, comment out if Joystick should be used
           (inputC.dpadUp() ? BUTTON_UP : 0) | //Move Up,comment out if Joystick should be used
           (inputC.dpadDown() ? BUTTON_DOWN : 0) | //Move Down,comment out if Joystick should be used
           (inputC.buttonMinus() ? BUTTON_COIN : 0) | //Coin
           (inputC.buttonPlus() ? BUTTON_START : 0) | //start
           (inputC.buttonHome() ? BUTTON_HOME : 0) |//home

            ((joyX < 127 - JOYSTICK_MOVE_THRESHOLD) ? BUTTON_LEFT : 0) | //Move Left //uncomment if Joystick should be used
            ((joyX > 127 + JOYSTICK_MOVE_THRESHOLD) ? BUTTON_RIGHT : 0) | //Move Right //uncomment if Joystick should be used
            ((joyY > 127 + JOYSTICK_MOVE_THRESHOLD) ? BUTTON_UP : 0) | //Move Up //uncomment if Joystick should be used
            ((joyY < 127 - JOYSTICK_MOVE_THRESHOLD) ? BUTTON_DOWN : 0) | //Move Down //uncomment if Joystick should be used


           (inputC.buttonA() ? BUTTON_FIRE : 0) ;
  
  #endif
 #endif

 #ifndef CLASSIC_LJOY
  #ifndef CLASSIC_DPAD 
  #error "Define at least one method of input for a Classic Controller in config.h!"
  #endif
 #endif
    }
  

  if (nun)  
    {
    // Read a joystick axis (0-255, X and Y)
    // Roughly 127 will be the axis centered
    int joyY = inputN.joyY();
    int joyX = inputN.joyX();

    return ((joyX < 127 - JOYSTICK_MOVE_THRESHOLD) ? BUTTON_LEFT : 0) | //Move Left
           ((joyX > 127 + JOYSTICK_MOVE_THRESHOLD) ? BUTTON_RIGHT : 0) | //Move Right
           ((joyY > 127 + JOYSTICK_MOVE_THRESHOLD) ? BUTTON_UP : 0) | //Move Up
           ((joyY < 127 - JOYSTICK_MOVE_THRESHOLD) ? BUTTON_DOWN : 0) | //Move Down
           (inputN.buttonZ() ? BUTTON_FIRE : 0)|
           (inputN.buttonC() ? BUTTON_COIN
            : 0) ;
  
    };
    

  
}

#endif //_NINTENDO_H_
