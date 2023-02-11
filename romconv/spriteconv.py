#!/usr/bin/env python3
import sys

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
    
def parse_sprite_2(data):
    # dkong has parts of each sprite distributed over all four roms
    sprite = []    
    for y in range(16):
        row = [ ]
        for x in range(16):
            c0 = 1 if data[y//8][15-x] & (0x80 >> (y&7)) else 0
            c1 = 2 if data[y//8+2][15-x] & (0x80 >> (y&7)) else 0
            row.append(c0+c1)
        sprite.append(row)
    return sprite
            
def parse_sprite(data, pacman_fmt):
    # the pacman sprite format differs from the galaga
    # one. The top 4 pixels are in fact the bottom four
    # ones for pacman

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

    if pacman_fmt:
        sprite = sprite[4:] + sprite[:4]
    return sprite

def dump_c_source(sprites, flip_x, flip_y, f):
    # write as c source
    print(" {" ,file=f)
    sprites_str = []
    for s in sprites:
        sprites_str.append("  { " + dump_sprite(s, flip_x, flip_y) + " }")
    print(",\n".join(sprites_str), file=f)
    if flip_x and flip_y: print(" }", file=f)
    else:                 print(" },", file=f)

def parse_spritemap(id, pacman_fmt, infiles, outfile):
    sprites = []

    # pacman and galaga
    if len(infiles) <= 2:    
        for name in infiles:    
            f = open(name, "rb")
            spritemap_data = f.read()
            f.close()

            if len(spritemap_data) != 4096:
                raise ValueError("Missing spritemap data")

            # read and parse all 64 sprites
            for sprite in range(64):
                sprites.append(parse_sprite(spritemap_data[64*sprite:64*(sprite+1)], pacman_fmt))
    else: # dkong
        spritemap_data = []
        for file in infiles:        
            f = open(file, "rb")
            spritemap_data.append(f.read())
            f.close()
            
            if len(spritemap_data[-1]) != 2048:
                raise ValueError("Missing spritemap data")
            
        for sprite in range(128):
            data = []
            for i in range(4):
                data.append(spritemap_data[i][16*sprite:16*(sprite+1)])
            
            sprites.append(parse_sprite_2(data))

    # for s in sprites: show_sprite(s)
    
    f=open(outfile, "w")
    print("// autoconverted sprite data", file=f)
    print("", file=f)
    
    print("const unsigned long "+id+"[][128][16] = {", file=f)
    
    dump_c_source(sprites, False, False, f)
    
    # we have plenty of flash space, so we simply pre-compute x/y flipped
    # versions of all sprites
    dump_c_source(sprites, False,  True, f)
    dump_c_source(sprites,  True, False, f)
    dump_c_source(sprites,  True,  True, f)
    print("};", file=f)

if len(sys.argv) < 5:
    print("Usage:",sys.argv[0], "id format <infiles> <outfile>")
    print("  for Galaga:     ", sys.argv[0], "galaga_sprites galaga ../roms/gg1_11.4d ../roms/gg1_10.4f ../galagino/galaga_spritemap.h")
    print("  for Pacman:     ", sys.argv[0], "pacman_sprites pacman ../roms/pacman.5f ../galagino/pacman_spritemap.h")
    print("  for Donkey Kong:", sys.argv[0], "dkong_sprites dkong ../roms/l_4m_b.bin  ../roms/l_4n_b.bin  ../roms/l_4r_b.bin  ../roms/l_4s_b.bin../galagino/dkong_spritemap.h")
    exit(-1)
    
parse_spritemap(sys.argv[1], sys.argv[2] == "pacman", sys.argv[3:-1], sys.argv[-1])

