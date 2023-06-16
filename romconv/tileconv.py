#!/usr/bin/env python3

import sys

def show_chr(data):
    for row in data:
        for pix in row:
            print(" .x*"[pix], end="")
        print("")

def dump_chr(data):
    hexs = [ ]
    
    for y in range(8):
        val = 0
        for x in range(8):
            val = (val >> 2) + (data[y][x] << (16-2))
        hexs.append(hex(val))

    return ",".join(hexs)
    
def parse_chr_2(data0, data1):
    # characters are 8x8 pixels
    char = []    
    for y in range(8):
        row = []
        for x in range(8):
            c0 = 1 if data0[7-x] & (0x80>>y) else 0
            c1 = 2 if data1[7-x] & (0x80>>y) else 0
            row.append(c0 + c1)
        char.append(row)
    return char

def parse_chr(data):
    # characters are 8x8 pixels
    char = []    
    for y in range(8):
        row = []
        for x in range(8):
            byte = data[15 - x - 2*(y&4)]
            c0 = 1 if byte & (0x08 >> (y&3)) else 0
            c1 = 2 if byte & (0x80 >> (y&3)) else 0
            row.append(c0+c1)
        char.append(row)
    return char
            
def parse_chr_dd(data):
    # characters are 8x8 pixels
    char = []    
    for y in range(8):
        row = []
        for x in range(8):
            byte = data[7 - x]
            c = 3 if byte & (0x01 << y) else 0
            row.append(c)
        char.append(row)
    return char
            
def parse_charmap(inname, outname):
    # The character map rom contains the same set of 128 characters
    # two times. The second set is upside down for cocktail mode. We
    # ignore that.

    f = open(inname, "rb")
    charmap_data = f.read()
    f.close()

    if len(charmap_data) != 4096 and len(charmap_data) != 2048:
        raise ValueError("Missing charmap data")

    chars = []
    if len(charmap_data) == 4096:
        # galaga 2bpp format
    
        # read and parse all 256 characters
        for chr in range(256):
            chars.append(parse_chr(charmap_data[16*chr:16*(chr+1)]))

    else:
        # digdug format
        for chr in range(128):
            chars.append(parse_chr_dd(charmap_data[8*chr:8*(chr+1)]))
            
    # for c in chars: show_chr(c)

    # write as c source
    f = open(outname, "w")

    name = inname.split("/")[-1].replace(".", "_")
    
    print("const unsigned short "+name+"[][8] = {", file=f )
    chars_str = []
    for c in chars:
        chars_str.append(" { " + dump_chr(c) + " }")
    print(",\n".join(chars_str), file=f)
    print("};", file=f)
    
    f.close()

def parse_charmap_2(innames, outname):

    # swap bits 0 and 1 in an integer
    def bit01_swap(a):
        return (a & 0xfffc) | ((a & 2)>>1) | ((a & 1)<<1) 
    
    f = open(innames[0], "rb")
    charmap_data_0 = f.read()
    f.close()
    
    f = open(innames[1], "rb")
    charmap_data_1 = f.read()
    f.close()

    if len(charmap_data_0) != 2048 or len(charmap_data_1) != 2048:
        raise ValueError("Missing charmap data")

    # in frogger the second gfx rom has D0 and D1 swapped ... wtf ...
    # check first 16 bytes / two characters as they should be bitswapped identical in frogger
    d01_swap = True
    for i in range(16):
        if bit01_swap(charmap_data_0[i]) != charmap_data_1[i]:
            d01_swap = False

    if d01_swap:
        charmap_data_0 = list(charmap_data_0)
        for i in range(len(charmap_data_0)):
            charmap_data_0[i] = bit01_swap(charmap_data_0[i])
        charmap_data_0 = bytes(charmap_data_0)
    
    # read and parse all 256 characters
    chars = []
    for chr in range(256):
        chars.append(parse_chr_2(charmap_data_0[8*chr:8*(chr+1)],
                     charmap_data_1[8*chr:8*(chr+1)]))

#    for c in range(len(chars)):
#        print("---", c)
#        show_chr(chars[c])
#     for c in chars: show_chr(c)

    # write as c source
    f = open(outname, "w")

    name = innames[0].split("/")[-1].replace(".", "_")
    
    print("const unsigned short "+name+"[][8] = {", file=f )
    chars_str = []
    for c in chars:
        chars_str.append(" { " + dump_chr(c) + " }")
    print(",\n".join(chars_str), file=f)
    print("};", file=f)
    
    f.close()

if len(sys.argv) != 3 and len(sys.argv) != 4:
    print("Usage:",sys.argv[0], "<infile> <outfile>")
    print("  Galaga:     ", sys.argv[0], "../roms/gg1_9.4l ../galagino/galaga_tilemap.h")
    print("  Pacman:     ", sys.argv[0], "../roms/pacman.5e ../galagino/pacman_tilemap.h")
    print("  Donkey Kong:", sys.argv[0], "../roms/v_5h_b.bin ../roms/v_3pt.bin ../galagino/dkong_tilemap.h")
    print("  Frogger:    ", sys.argv[0], "../roms/frogger.606 ../roms/frogger.607 ../galagino/frogger_tilemap.h")
    print("  Digdug:     ", sys.argv[0], "../roms/dd1.9 ../galagino/digdug_tilemap.h")
    print("  Digdug pf:  ", sys.argv[0], "../roms/dd1.11 ../galagino/digdug_pftiles.h")
    exit(-1)

if len(sys.argv) == 3:
    parse_charmap(sys.argv[1], sys.argv[2])
else:
    parse_charmap_2(sys.argv[1:3], sys.argv[3])
