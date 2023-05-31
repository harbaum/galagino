#!/bin/bash
# convert everything script

echo "Audio"
./audioconv.py galaga_wavetable ../roms/prom-1.1d ../galagino/galaga_wavetable.h
./audioconv.py pacman_wavetable ../roms/82s126.1m ../roms/82s126.3m ../galagino/pacman_wavetable.h

echo "Colormaps"
./cmapconv.py galaga_colormap_sprites ../roms/prom-5.5n 0 ../roms/prom-3.1c ../galagino/galaga_cmap_sprites.h
./cmapconv.py galaga_colormap_tiles ../roms/prom-5.5n 16 ../roms/prom-4.2n ../galagino/galaga_cmap_tiles.h
./cmapconv.py pacman_colormap ../roms/82s123.7f 0 ../roms/82s126.4a ../galagino/pacman_cmap.h
./cmapconv.py dkong_colormap ../roms/c-2k.bpr ../roms/c-2j.bpr 0 ../roms/v-5e.bpr ../galagino/dkong_cmap.h
./cmapconv.py frogger_colormap ../roms/pr-91.6l ../galagino/frogger_cmap.h

# converted logos are included
#echo "Logos"
#./logoconv.py ../logos/pacman.png ../galagino/pacman_logo.h
#./logoconv.py ../logos/galaga.png ../galagino/galaga_logo.h
#./logoconv.py ../logos/dkong.png ../galagino/dkong_logo.h
#./logoconv.py ../logos/frogger.png ../galagino/frogger_logo.h

echo "CPU code"
./romconv.py -p galaga_rom_cpu1 ../roms/gg1_1b.3p ../roms/gg1_2b.3m ../roms/gg1_3.2m ../roms/gg1_4b.2l ../galagino/galaga_rom1.h
./romconv.py galaga_rom_cpu2 ../roms/gg1_5b.3f ../galagino/galaga_rom2.h
./romconv.py galaga_rom_cpu3 ../roms/gg1_7b.2c ../galagino/galaga_rom3.h
./romconv.py pacman_rom ../roms/pacman.6e ../roms/pacman.6f ../roms/pacman.6h ../roms/pacman.6j ../galagino/pacman_rom.h
./romconv.py dkong_rom_cpu1 ../roms/c_5et_g.bin ../roms/c_5ct_g.bin ../roms/c_5bt_g.bin ../roms/c_5at_g.bin ../galagino/dkong_rom1.h
./romconv.py dkong_rom_cpu2 ../roms/s_3i_b.bin ../roms/s_3j_b.bin ../galagino/dkong_rom2.h
./romconv.py frogger_rom_cpu1 ../roms/frogger.26 ../roms/frogger.27 ../roms/frsm3.7 ../galagino/frogger_rom1.h
./romconv.py frogger_rom_cpu2 ../roms/frogger.608 ../roms/frogger.609 ../roms/frogger.610 ../galagino/frogger_rom2.h

echo "Sprites"
./spriteconv.py galaga_sprites galaga ../roms/gg1_11.4d ../roms/gg1_10.4f ../galagino/galaga_spritemap.h
./spriteconv.py pacman_sprites pacman ../roms/pacman.5f ../galagino/pacman_spritemap.h
./spriteconv.py dkong_sprites dkong ../roms/l_4m_b.bin  ../roms/l_4n_b.bin  ../roms/l_4r_b.bin  ../roms/l_4s_b.bin ../galagino/dkong_spritemap.h
./spriteconv.py frogger_sprites frogger ../roms/frogger.606 ../roms/frogger.607 ../galagino/frogger_spritemap.h

# converted starset is included
#echo "Starset"
#./starsets.py ../galagino/galaga_starseed.h

# converted tile address map is included
#echo "Tile address map"
#./tileaddr.py ../galagino/tileaddr.h

echo "Tiles"
./tileconv.py ../roms/gg1_9.4l ../galagino/galaga_tilemap.h
./tileconv.py ../roms/pacman.5e ../galagino/pacman_tilemap.h
./tileconv.py ../roms/v_5h_b.bin ../roms/v_3pt.bin ../galagino/dkong_tilemap.h
./tileconv.py ../roms/frogger.606 ../roms/frogger.607 ../galagino/frogger_tilemap.h

echo "Z80"
./z80patch.py

