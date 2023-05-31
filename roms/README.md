# ROMs

This directory contains ROM files and other data required to build
galagino. The *Galaga Namco Rev. B* romset contains the orignal rom
files from the galaga arcade. These files are also needed for
emulators like MAME and can easily be found online. If Pac-Man
and/or Donkey Kong are to be included as well, then their ROM files
are also needed.

Once all files have been placed here, the conversion scripts
in the [romconv directory](../romconv) can be used to convert
the ROMs into source files to be compiled for the ESP32.

## Generic files

* [Z80-081707.zip](https://fms.komkon.org/EMUL8/Z80-081707.zip) - Z80 CPU emulator

## PacMan

Files needed from the [Pac-Man (Midway)](https://www.bing.com/search?q=pacman+midway+arcade+rom) romset:

* ```pacman.6e```, ```pacman.6f```, ```pacman.6h```, ```pacman.6j``` - CPU rom
* ```pacman.5e``` - tile graphics
* ```pacman.5f``` - sprite graphics
* ```82s126.1m```, ```82s126.3m``` - audio wavetables
* ```82s123.7f``` - color palette
* ```82s126.4a``` - colormap

## Galaga

Files needed from the [Galaga Namco Rev. B ROM](https://www.bing.com/search?q=galaga+namco+b+rom) romset:

* ```gg1_1b.3p```, ```gg1_2b.3m```, ```gg1_3.2m``` and ```gg1_4b.2l``` - CPU1 rom
* ```gg1_5b.3f``` - CPU2 rom
* ```gg1_7b.2c``` - CPU3 rom
* ```gg1_9.4l``` - tile graphics
* ```gg1_10.4f```, ```gg1_11.4d``` - sprite graphics
* ```prom-1.1d``` - audio wavetables
* ```prom-3.1c``` - sprites colormap
* ```prom-4.2n``` - tile colormap
* ```prom-5.5n``` - color palette

## Donkey Kong

Files needed from the [Donkey Kong (US set 1)](https://www.bing.com/search?q=donkey+kong+arcade+rom) romset:

* ```c_5et_g.bin```, ```c_5ct_g.bin```, ```c_5bt_g.bin```, ```c_5at_g.bin``` - CPU1 rom
* ```s_3i_b.bin```, ```s_3j_b.bin``` - CPU2 rom
* ```v_5h_b.bin```, ```v_3pt.bin``` - tile graphics
* ```l_4m_b.bin```, ```l_4n_b.bin```, ```l_4r_b.bin```, ```l_4s_b.bin``` - sprite graphics
* ```c-2j.bpr```, ```v-5e.bpr``` - colormap
* ```c-2k.bpr``` - color palette

## Frogger

Files needed from the [Frogger Konami](https://www.bing.com/search?q=frogger+konami+arcade+rom) romset:

* ```frogger.26```, ```frogger.27```, ```frsm3.7``` - CPU1 rom
* ```frogger.608```, ```frogger.609```, ```frogger.610``` - CPU2 rom
* ```frogger.606```, ```frogger.607``` - tile and sprite graphics
* ```pr-91.6l ``` - color palette
