# ROMs

This directory contains ROM files and other data required to build
galagino. The *Galaga Namco Rev. B* romset contains the orignal rom
files from the galaga arcade. These files are also needed for
emulators like MAME and can easily be found online. If Pac-Man and/or
Donkey Kong, Frogger, Digdug an 1942 are to be included as well, then
their ROM files are also needed.

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

Files needed from the [Galaga (Namco Rev. B ROM)](https://www.bing.com/search?q=galaga+namco+b+rom) romset:

* ```gg1_1b.3p```, ```gg1_2b.3m```, ```gg1_3.2m``` and ```gg1_4b.2l``` - CPU1 rom
* ```gg1_5b.3f``` - CPU2 rom
* ```gg1_7b.2c``` - CPU3 rom
* ```gg1_9.4l``` - tile graphics
* ```gg1_10.4f```, ```gg1_11.4d``` - sprite graphics
* ```prom-1.1d``` - audio wavetables
* ```prom-3.1c``` - sprite colormap
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

Files needed from the [Frogger (Konami)](https://www.bing.com/search?q=frogger+konami+arcade+rom) romset:

* ```frogger.26```, ```frogger.27```, ```frsm3.7``` - CPU1 rom
* ```frogger.608```, ```frogger.609```, ```frogger.610``` - CPU2 rom
* ```frogger.606```, ```frogger.607``` - tile and sprite graphics
* ```pr-91.6l ``` - color palette

## Digdug

Files needed from the [Digdug (Namco)](https://www.bing.com/search?q=digdug+namco+arcade+rom) romset:

* ```dd1a.1```, ```dd1a.2```, ```dd1a.3```, ```dd1a.4``` - CPU1 rom
* ```dd1a.5```, ```dd1a.6``` - CPU2 rom
* ```dd1a.7``` - CPU3 rom
* ```dd1.10b``` - playfield tilemaps
* ```136007.110```, ```136007.109 ``` - audio wavetables
* ```dd1.9``` - foreground tiles
* ```dd1.11``` - playfield tiles
* ```dd1.15```, ```dd1.14```, ```dd1.13```, ```dd1.12``` - sprite graphics
* ```136007.112``` - playfield tile colormap
* ```136007.111``` - sprite colormap
* ```136007.113``` - color palette

## 1942

Files needed from the [1942 (Capcom 1984)](https://www.bing.com/search?q=1942+arcade+rom) romset:

* ```srb-03.m3```, ```srb-04.m4``` - CPU1 rom
* ```srb-05.m5``` - CPU1 banked rom 0
* ```srb-06.m6``` - CPU1 banked rom 1
* ```srb-07.m7``` - CPU1 banked rom 2
* ```sr-01.c11``` - CPU2 rom
* ```sr-14.l1```, ```sr-15.l2```, ```sr-16.n1```, ```sr-17.n2``` - sprite graphics
* ```sr-02.f2``` - character graphics
* ```sr-08.a1```, ```sr-09.a2```, ```sr-10.a3```, ```sr-11.a4```, ```sr-12.a5```, ```sr-13.a6``` - tile graphics
* ```sb-5.e8```, ```sb-6.e9```, ```sb-7.e10``` - color palette
* ```sb-0.f1``` - character colormap
* ```sb-4.d6```, ```sb-3.d2```, ```sb-2.d1``` - tile colormap
* ```sb-8.k3``` - sprite colormap
