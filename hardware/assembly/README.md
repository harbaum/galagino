# Hardware Assembly

If the hardware is to be placed inside a tiny arcade like
the one depicted below, then it needs to be assembled more
compact than the breadboard allows.

![Cardboard arcade](galagino_cardboard_arcade.jpg)

[Video](https://youtube.com/shorts/vverV4NphlQ)

## Wiring 

First are the controls (buttons). These need to be wired first as they
are located right beside the ESP32 board and it's wires would interfere
with the display wiring afterwards.

![controls](wire_control.png)

[PDF](wire_control.pdf)

This connects the [controlboard](../controlboard/) and the coin button
to the ESP32 board. You can solder the wires directly to the ESP32 board
or use female pin headers if you want to be able to easily replace
the ESP32.

![controls](wire_control.jpg)

Next is the display.

![controls](wire_display.png)

[PDF](wire_display.pdf)

It's connections overlay the control wires so the display can later
be mounted directly above the controls. 

![displays](wire_display.jpg)

The display is mounted 180° rotated compared to the breadboard setup.
This can be adjusted for in the [config.h](../../galagino/config.h) file.

The setup should be working at this point:

![test](display_test.jpg)

Finally the speaker and the optional LEDs stripe is attached. Each of them
requires only three wires:

![controls](wire_audio_led.png)

[PDF](wire_audio_led.pdf)

The final assembly should then look like this:

![assembled](assembled.jpg)

The amplifier board cannot be seen as it's taped to the back of the speaker
compartment using double sided tape.