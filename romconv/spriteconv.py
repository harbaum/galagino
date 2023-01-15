#!/usr/bin/env python3

ROMLOC="../roms/"
SPRITEMAPS=[ "gg1_11.4d", "gg1_10.4f" ]

def show_sprite(data):
    for row in data:
        for pix in row:
            print(" .x*"[pix], end="")
        print("")

def dump_sprite(data, flip_x, flip_y):
    hexs = [ ]
    
    for y in range(16) if not flip_y else reversed(range(16)):
        val = 0
        for x in range(16):
            if not flip_x:
                val = (val >> 2) + (data[y][x] << (32-2))
            else:
                val = (val << 2) + data[y][x]
        hexs.append(hex(val))

    return ",".join(hexs)
    
def parse_sprite(data):
    # sprites are 16x16 pixels
    sprite = []    
    for y in range(16):
        row = []
        for x in range(16):
            idx = ((y&8)<<1) + (((x&8)^8)<<2) + (7-(x&7)) + 2*(y&4)
            c0 = 1 if data[idx] & (0x08 >> (y&3)) else 0
            c1 = 2 if data[idx] & (0x80 >> (y&3)) else 0
            row.append(c0+c1)
        sprite.append(row)
    return sprite

def dump_c_source(sprites, flip_x, flip_y):
    # write as c source
    print(" {" )
    sprites_str = []
    for s in sprites:
        sprites_str.append("  { " + dump_sprite(s, flip_x, flip_y) + " }")
    print(",\n".join(sprites_str))
    if flip_x and flip_y: print(" }")
    else:                 print(" },")

def parse_spritemap(names):
    sprites = []
    
    for name in names:    
        f = open(ROMLOC + name, "rb")
        spritemap_data = f.read()
        f.close()

        if len(spritemap_data) != 4096:
            raise ValueError("Missing charmap data")

        # read and parse all 64 sprites
        for sprite in range(64):
            sprites.append(parse_sprite(spritemap_data[64*sprite:64*(sprite+1)]))

    # for s in sprites: show_sprite(s)

    print("// autoconverted sprite data")
    print("")
    
    print("const unsigned long "+name.split("/")[-1].split(".")[0]+"[][128][16] = {" )
    
    dump_c_source(sprites, False, False)
    
    # we have plenty of flash space, so we simply pre-compute x/y flipped
    # versions of all sprites
    dump_c_source(sprites, False,  True)
    dump_c_source(sprites,  True, False)
    dump_c_source(sprites,  True,  True)
    print("};")
    
parse_spritemap(SPRITEMAPS)

