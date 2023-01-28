#!/usr/bin/env python3
import sys

def parse_palette(name, name2=None):
    # the palette contains 32 8 bit rgb values. The first 16 are
    # used by sprites and the second 16 by tiles
    f = open(name, "rb")
    palette_data = f.read()
    f.close()

    if name2:
        f = open(name2, "rb")
        palette_data_2 = f.read()
        f.close()
    else:
        palette_data_2 = None
        # pacman and galaga have only 32 palette entries
        if len(palette_data) != 32:
            raise ValueError("Missing palette data")

    # map to 16 bit rgb in swapped format for ili9341 display
    palette = []
    for idx in range(len(palette_data)):
        c = palette_data[idx]
        if palette_data_2:
            c = (palette_data_2[idx] << 4) + c 

            # donkey kong rrrgggbb mapping
            r = 31 - 31*((c>>5) & 0x7)//7
            g = 63 - 63*((c>>2) & 0x7)//7
            b = 31 - 31*((c>>0) & 0x3)//3
        else:
            # galaga and pacman bbgggrrr mapping
            
            # This doesn't 100% match the weighting each bit has
            # on the real machine with 1000, 470 and 220 ohms resistors.
            #
            # Here the weighting is 4/7, 2/7 and 1/7 while on the
            # real thing it's 4.15/7, 1.94/7 and 0.91/7. So the
            # weighting of the lsb is slightly higher on the real
            # device.
            
            b = 31*((c>>6) & 0x3)//3
            g = 63*((c>>3) & 0x7)//7
            r = 31*((c>>0) & 0x7)//7

        rgb = (r << 11) + (g << 5) + b
        rgbs = ((rgb & 0xff00) >> 8) + ((rgb & 0xff) << 8)
        palette.append(rgbs)

    return palette
        
def parse_colormap(id, inname, palette, outname):
    # the colormaps contain 64*4 4-bit entries that point
    # to palette entries.
    f = open(inname, "rb")
    colormap_data = f.read()
    f.close()

    if len(colormap_data) != 256:
        raise ValueError("Missing colormap data")
    
    # write as c source
    f = open(outname, "w")
    print("const unsigned short "+id+"[][4] = {", file=f )
    colors = []
    for idx in range(64):
        c = colormap_data[4*idx:4*(idx+1)]
        # check if values are sane
        if ( c[0] < 0 or c[0] > 15 or c[0] < 0 or c[1] > 15 or
             c[2] < 0 or c[2] > 15 or c[3] < 0 or c[3] > 15):
            raise ValueError("Color index out of range")
        colors.append("{" + ",".join([ hex(palette[a]) for a in c ]) +"}")

    print(",\n".join(colors), file=f)
    print("};", file=f)
    f.close()
    
def parse_colormap_dkong(id, inname, palette, outname):
    f = open(inname, "rb")
    colormap_data = f.read()
    f.close()

    if len(colormap_data) != 256:
        raise ValueError("Missing colormap data")
    
    # write as c source
    f = open(outname, "w")
    print("const unsigned short "+id+"[][256][4] = {", file=f )
    # four different screen setups
    screens = [ ]
    for s in range(4):    
        colors = []
        for idx in colormap_data:
            idx += 16*s
            c = [4*idx+0, 4*idx+1, 4*idx+2, 4*idx+3]
            colors.append("{" + ",".join([ hex(palette[a]) for a in c ]) +"}")

        screens.append("{" + ",\n".join(colors)+"}")
    print(",\n".join(screens), file=f)
        
    print("};", file=f)

    print("", file=f)
    print("", file=f)

    # create the four sprite colormap tables
    print("const unsigned short "+id+"_sprite[][16][4] = {", file=f )
    screens = [ ]
    for s in range(4):
        colors = []
        for i in range(16):
            idx = i + 16*s
            c = [4*idx+0, 4*idx+1, 4*idx+2, 4*idx+3]
            colors.append("{" + ",".join([ hex(palette[a]) for a in c ]) +"}")
        screens.append("{" + ",\n".join(colors)+"}")
    print(",\n".join(screens), file=f)
    print("};", file=f)
    
    f.close()
    
if len(sys.argv) != 6 and len(sys.argv) != 7:
    print("Usage:",sys.argv[0], "name <palettefiles> offset <tablefile> <outfile>")
    print("  for Galaga sprites:", sys.argv[0], "galaga_colormap_sprites ../roms/prom-5.5n 0 ../roms/prom-3.1c ../galagino/galaga_cmap_sprites.h")
    print("  for Galaga tiles:  ", sys.argv[0], "galaga_colormap_tiles ../roms/prom-5.5n 16 ../roms/prom-4.2n ../galagino/galaga_cmap_tiles.h")
    print("  for Pacman:        ", sys.argv[0], "pacman_colormap ../roms/82s123.7f 0 ../roms/82s126.4a ../galagino/pacman_cmap.h")
    print("  for Donkey Kong:   ", sys.argv[0], "dkong_colormap ../roms/c-2k.bpr ../roms/c-2j.bpr 0 ../roms/v-5e.bpr ../galagino/dkong_cmap.h")
    exit(-1)

if len(sys.argv) == 6:
    palette = parse_palette(sys.argv[2])
    offset = int(sys.argv[3])
    parse_colormap(sys.argv[1], sys.argv[4], palette[offset:offset+16], sys.argv[5])
else:
    # dkong has the palette in two 4 bit roms
    palette = parse_palette(sys.argv[2], sys.argv[3])
    parse_colormap_dkong(sys.argv[1], sys.argv[5], palette, sys.argv[6])
    
