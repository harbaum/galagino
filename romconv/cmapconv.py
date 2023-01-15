#!/usr/bin/env python3

ROMLOC="../roms/"

SPRITE_CMAP = "prom-3.1c"
TILE_CMAP = "prom-4.2n"
PALETTE = "prom-5.5n"

def parse_palette(name):
    # the palette contains 32 8 bit rgb values. The first 16 are
    # used by sprites and the second 16 by tiles
    f = open(name, "rb")
    palette_data = f.read()
    f.close()

    if len(palette_data) != 32:
        raise ValueError("Missing palette data")

    # map to 16 bit rgb in swapped format for ili9341 display
    palette = []
    for c in palette_data:
        r = 31*((c>>0) & 0x7)//7
        g = 63*((c>>3) & 0x7)//7
        b = 31*((c>>6) & 0x3)//3
        rgb = (r << 11) + (g << 5) + b
        rgbs = ((rgb & 0xff00) >> 8) + ((rgb & 0xff) << 8)
        palette.append(rgbs)

    return palette
        
def parse_colormap(name, palette):
    # the colormaps contain 64*4 4-bit entries that point
    # to palette entries.
    f = open(name, "rb")
    colormap_data = f.read()
    f.close()

    if len(colormap_data) != 256:
        raise ValueError("Missing colormap data")
    
    # write as c source
    print("const unsigned short "+name.split("/")[-1].split(".")[0].replace("-","_")+"[][4] = {" )
    colors = []
    for idx in range(64):
        c = colormap_data[4*idx:4*(idx+1)]
        # check if values are sane
        if ( c[0] < 0 or c[0] > 15 or c[0] < 0 or c[1] > 15 or
             c[2] < 0 or c[2] > 15 or c[3] < 0 or c[3] > 15):
            raise ValueError("Color index out of range")
        colors.append("{" + ",".join([ hex(palette[a]) for a in c ]) +"}")

    print(",\n".join(colors))
    print("};")

palette = parse_palette(ROMLOC + PALETTE)
parse_colormap(ROMLOC + SPRITE_CMAP, palette[0:16])
parse_colormap(ROMLOC + TILE_CMAP, palette[16:32])
    
