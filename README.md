# Galagino
## galaga arcade emulator for ESP32

![Galagino screencast](images/galagino.gif)

## Youtube videos

* [First test](https://www.youtube.com/shorts/LZRI6izM8XM)
* [Sound and Starfield working](https://www.youtube.com/shorts/8uNSv0aRtgY)
* [Finally playable](https://www.youtube.com/shorts/wqnJzOAAths)

## Software

```
$ cd romconv
$ ./audioconv.py > ../galagino/wavetables.h
$ ./cmapconv.py > ../galagino/colormaps.h
$ ./romconv.py > ../galagino/rom.h
$ ./spriteconv.py > ../galagino/spritemap.h
$ ./tileconv.py > ../galagino/tilemap.h
$ ./tileaddr.py > ../galagino/tileaddr.h
$ ./starsets.py > ../galagino/starseed.h
$ ./z80patch.py 
Copying CodesCB.h
Copying Codes.h
Copying CodesXX.h
Copying CodesED.h
Copying CodesXCB.h
Copying Tables.h
Patching Z80.h
Patching Z80.c
$
```

## Hardware

The hardware is built around one of those cheap ESP32 development
boards like the ESP32 Devkit V4 depicted in the images below. The
components needed are:

* ESP32 development board (e.g. Devkit V4)
* A 320x240 SPI TFT screen (no touch needed)
  * Either a ILI9341 based screen as depicted, or
  * a ST7789 based screen with 320x240 pixels
* An audio amplifier
  * e.g. a PAM8302 and a 3W speaker (as seen in the photos), or
  * a Keyestudio SC8002B, or similar
* five push buttons
* breadboard and wires

The entire setup should be connected as depiced below. The
Devkit is too wide for the breadboard leaving no space above it to
connect wires. Thus the wires going to the top pin row of the
Devkit are placed underneath the DevKit with the connections done
as shown in the image below.

![Breadboard scheme](images/galagino_bb.png)

Download as [Fritzing](images/galagino_bb.fzz) or [PDF](images/galagino_bb.pdf)


![Breadboard photo](images/galagino_breadboard.jpeg)
