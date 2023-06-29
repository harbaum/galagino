# Nunchuck

A Nunchuck controller from a Nintendo Wii can be used as input device for Galagino builds. 
They are inexpensive and since they use i2c for communication, they only require 2 GPIO pins to connect them up.

## Hardware Required

- Nunchuck controller - Official Nintendo ones are generally better (maybe try second-hand options), but third-party ones also work fine.
- A Nunchuck adapter - Here is one from [Adafaruit](https://www.adafruit.com/product/4836), but there are many different options available for these, even the cheap ones from Aliexpress work perfectly.

## Wiring

- SDA and SCL of the Nunchuck can be connected to any two GPIO of the ESP32. 
- Power pins of the Nunchuck should be connected to 3.3v and Gnd

Note: I have found pull-ups resistors are not required on SDA and SCL

## Configuration

The nunchuck can be enabled in 'conf.h' as follows

```
#define NUNCHUCK_INPUT

#define NUNCHUCK_SDA 22 // The GPIO pin connected to SDA of the nunchuck
#define NUNCHUCK_SCL 27 // The GPIO pin connected to SCL of the nunchuck

#define NUNCHUCK_MOVE_THRESHOLD 30 // This is the dead-zone for where minor movements on the stick will not be considered valid movements
                                   // Increase this value if you want to increase how far you need to move the nunchuck for a valid movement
```

Note: At time of writing the Start and optional Coin button are not enabled on the nunchuck and will still need to be wired to regular GPIO buttons
