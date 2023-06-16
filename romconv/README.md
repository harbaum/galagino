# romconv - ROM/bin file conversion

Arcade emulation involves dealing with the original ROM files of the
machines to be emulated. Sometimes the conversion is limited to the
transcription from binary to an equivalent C source file. But in many
cases the conversion includes further data processing. E.g. all color
tables are converted into the 16 bit color format used by the ILI9341
or ST7789 displays. Sprite and tile data is converted into a format
easier to process on the ESP32.

The conversion could be done on the ESP32 target at run time. But in
Galagino it's done beforehand. This offloads these tasks from the
ESP32.

It's possible to implement only one or two of the three arcade
machines. In that case the related ROM conversion can be omitted and
the machine in question has to be disabled in the file
[config.h](../galagino/config.h).

The necessary ROM files need be placed in the [roms
directory](../roms) before these scripts can be run. Once these are
converted the [audio samples](../samples) also need to be converted.

The logo conversion for the game selection menu might require the
seperate installation of the ```imageio python module``` which can
e.g. be done by the following command. This is usually not needed as
the logos are included pre-converted. This is only needed if you intend
the change the logos.

```pip3 install imageio```

## Do-it-all script

A [shell script](conv.sh) is included that does all the conversion.
If you prefer to do everything manually, then use the instructions
below. Otherwise running the script is all you need to do.

## Generic ROM conversion

The tilemap as well as the patched Z80 emulation are need by all three
arcade machines and thus have to be converted regardless which
machines are to be emulated.

```
./tileaddr.py ../galagino/tileaddr.h
./z80patch.py
```

## Pac-Man ROM conversion

Pac-Man comes with code ROMs as well as ROMs containing graphic data,
color tables and audio waveforms. These can be converted using the
following commands:

```
./audioconv.py pacman_wavetable ../roms/82s126.1m ../roms/82s126.3m ../galagino/pacman_wavetable.h
./cmapconv.py pacman_colormap ../roms/82s123.7f 0 ../roms/82s126.4a ../galagino/pacman_cmap.h
./logoconv.py ../logos/pacman.png ../galagino/pacman_logo.h
./romconv.py pacman_rom ../roms/pacman.6e ../roms/pacman.6f ../roms/pacman.6h ../roms/pacman.6j ../galagino/pacman_rom.h
./spriteconv.py pacman_sprites pacman ../roms/pacman.5f ../galagino/pacman_spritemap.h
./tileconv.py ../roms/pacman.5e ../galagino/pacman_tilemap.h
```

## Galaga ROM conversion

Galaga is technically very similar to Pac-Man and needs very similar
files. In this case three code ROMs need to be converted as the Galaga
arcade was driven by three Z80 CPUs.

The Galaga ROM for CPU #1 will be patched when given the ```-p```
option. This will disable the RAM and ROM tests of the Galaga arcade
machine and will speed up the loading of that machine in Galagino:

```
./romconv.py -p galaga_rom_cpu1 ../roms/gg1_1b.3p ../roms/gg1_2b.3m ../roms/gg1_3.2m ../roms/gg1_4b.2l ../galagino/galaga_rom1.h
```

For the full retro-experience simply omit the option and the resulting
Galagino setup will include all the self tests of the original
machine:

```
./romconv.py galaga_rom_cpu1 ../roms/gg1_1b.3p ../roms/gg1_2b.3m ../roms/gg1_3.2m ../roms/gg1_4b.2l ../galagino/galaga_rom1.h
```

The remaining files are just converted without patching.

```
./audioconv.py galaga_wavetable ../roms/prom-1.1d ../galagino/galaga_wavetable.h
./cmapconv.py galaga_colormap_sprites ../roms/prom-5.5n 0 ../roms/prom-3.1c ../galagino/galaga_cmap_sprites.h
./cmapconv.py galaga_colormap_tiles ../roms/prom-5.5n 16 ../roms/prom-4.2n ../galagino/galaga_cmap_tiles.h
./logoconv.py ../logos/galaga.png ../galagino/galaga_logo.h
./romconv.py galaga_rom_cpu2 ../roms/gg1_5b.3f ../galagino/galaga_rom2.h
./romconv.py galaga_rom_cpu3 ../roms/gg1_7b.2c ../galagino/galaga_rom3.h
./spriteconv.py galaga_sprites galaga ../roms/gg1_11.4d ../roms/gg1_10.4f ../galagino/galaga_spritemap.h
./starsets.py ../galagino/galaga_starseed.h
./tileconv.py ../roms/gg1_9.4l ../galagino/galaga_tilemap.h
```

Additionally the sample for the Galaga explosion sound needs to be
converted as described in the [samples](../samples) directory.

## Donkey Kong ROM conversion

Donkey Kong differs slightly from Pac-Man and Galaga but it also
conists code ROMs, graphics and colormaps that need to be converted.

```
./cmapconv.py dkong_colormap ../roms/c-2k.bpr ../roms/c-2j.bpr 0 ../roms/v-5e.bpr ../galagino/dkong_cmap.h
./logoconv.py ../logos/dkong.png ../galagino/dkong_logo.h
./romconv.py dkong_rom_cpu1 ../roms/c_5et_g.bin ../roms/c_5ct_g.bin ../roms/c_5bt_g.bin ../roms/c_5at_g.bin ../galagino/dkong_rom1.h
./romconv.py dkong_rom_cpu2 ../roms/s_3i_b.bin ../roms/s_3j_b.bin ../galagino/dkong_rom2.h
./spriteconv.py dkong_sprites dkong ../roms/l_4m_b.bin  ../roms/l_4n_b.bin  ../roms/l_4r_b.bin  ../roms/l_4s_b.bin ../galagino/dkong_spritemap.h  
./tileconv.py ../roms/v_5h_b.bin ../roms/v_3pt.bin ../galagino/dkong_tilemap.h
```

Sound was implemented in two different ways in Donkey Kong. Some
sounds were generated by the 8048 CPU which is emulated in
Galagino. But some sounds were generated using discrete logic. These
are generated from samples and need to be converted as described in
the [samples](../samples) directory.

## Frogger ROM conversion

Frogger uses the same ROM for tiles and sprites. Sprite and tile
coversion thus operate on the same files using the following commands:

```
./cmapconv.py frogger_colormap ../roms/pr-91.6l ../galagino/frogger_cmap.h
./logoconv.py ../logos/frogger.png ../galagino/frogger_logo.h
./romconv.py frogger_rom_cpu1 ../roms/frogger.26 ../roms/frogger.27 ../roms/frsm3.7 ../galagino/frogger_rom1.h
./romconv.py frogger_rom_cpu2 ../roms/frogger.608 ../roms/frogger.609 ../roms/frogger.610 ../galagino/frogger_rom2.h
./spriteconv.py frogger_sprites frogger ../roms/frogger.606 ../roms/frogger.607 ../galagino/frogger_spritemap.h
./tileconv.py ../roms/frogger.606 ../roms/frogger.607 ../galagino/frogger_tilemap.h
```

## Digdug ROM conversion

The Digdug hardware is very similar to Galaga. The video hardware is slightly more complex and
there are more video related ROMs to be converted.

```
./cmapconv.py digdug_colormap_tiles ../roms/136007.113 0 ../roms/136007.112 ../galagino/digdug_cmap_tiles.h
./cmapconv.py digdug_colormap_sprites ../roms/136007.113 16 ../roms/136007.111 ../galagino/digdug_cmap_sprites.h
./cmapconv.py digdug_colormaps ../roms/136007.113 ../galagino/digdug_cmap.h
./logoconv.py ../logos/digdug.png ../galagino/digdug_logo.h
./audioconv.py digdug_wavetable ../roms/136007.110 ../roms/136007.109 ../galagino/digdug_wavetable.h
./romconv.py digdug_rom_cpu1 ../roms/dd1a.1 ../roms/dd1a.2 ../roms/dd1a.3 ../roms/dd1a.4 ../galagino/digdug_rom1.h
./romconv.py digdug_rom_cpu2 ../roms/dd1a.5 ../roms/dd1a.6 ../galagino/digdug_rom2.h
./romconv.py digdug_rom_cpu3 ../roms/dd1.7 ../galagino/digdug_rom3.h
./romconv.py digdug_playfield ../roms/dd1.10b ../galagino/digdug_playfield.h
./spriteconv.py digdug_sprites digdug ../roms/dd1.15 ../roms/dd1.14 ../roms/dd1.13 ../roms/dd1.12 ../galagino/digdug_spritemap.h
./tileconv.py ../roms/dd1.9 ../galagino/digdug_tilemap.h
./tileconv.py ../roms/dd1.11 ../galagino/digdug_pftiles.h
```
