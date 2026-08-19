# Bluetooth Controllers (Bluepad32)

A Nintendo Switch Pro Controller (or other compatible Bluetooth gamepads) can be used as a wireless input device for Galagino builds. This provides a modern, comfortable alternative to wiring physical arcade buttons.

This feature is powered by the excellent [Bluepad32](https://github.com/ricardoquesada/bluepad32) project, created and maintained by Ricardo Quesada. 

## Hardware Required

- An ESP32 development board (e.g., standard Dev Module, or a Cheap Yellow Display).
- A compatible Bluetooth controller, such as the Nintendo Switch Pro Controller, PS4/PS5 DualSense, or Xbox Wireless Controller.

## Software & IDE Configuration

Because Bluepad32 requires a custom Bluetooth stack to run efficiently in the background without interrupting the emulator, it is installed as an **Arduino Board Package** rather than a standard library.

1. Open the Arduino IDE and go to **File > Preferences**.
2. In the "Additional Boards Manager URLs" field, add the following URL:
   `https://raw.githubusercontent.com/ricardoquesada/esp32-arduino-lib-builder/master/bluepad32_files/package_esp32_bluepad32_index.json`
3. Go to **Tools > Board > Boards Manager**, search for `esp32_bluepad32`, and install the package. 
   *(Note: Tested with the 4.1.0 board version).*
4. When you are ready to compile and upload, go to **Tools > Board** and select **ESP32 Dev Module (Bluepad32)** (or whichever Bluepad32 board matches your specific hardware variant).

## Configuration

The Bluetooth controller support can be enabled in the `config.h` file. Locate the following line and ensure it is uncommented:
   
    #define BLUE32_CONTROLLER_INPUT

>Note: The input logic dynamically merges all inputs. This means if you still have physical arcade buttons wired to the GPIO pins, they will continue to work perfectly alongside the wireless Bluetooth controller.

## Controls & Mapping
For the Nintendo Switch Pro Controller, the default Galagino mapping is as follows:

- Movement: D-Pad OR Left Analog Stick (features a built-in deadzone)
- Fire: A / B / X / Y (Any face button triggers a shot)
- Start: Plus (+) Button
- Coin: Minus (-) Button

## Pairing Instructions
Power on your ESP32 with the newly flashed Galagino code. By default, Bluepad32 automatically enters pairing mode on boot.

Put your controller into Bluetooth pairing mode. (For the Switch Pro Controller, hold the small circular Sync button on the top of the controller until the bottom LEDs begin flashing back and forth).

The ESP32 will automatically find the controller and connect.

