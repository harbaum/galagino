#!/usr/bin/env python3

import sys

ROMLOC="../roms/"
CHARMAP="gg1_9.4l"

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
            
def parse_charmap(inname, outname):
    # The character map rom contains the same set of 128 characters
    # two times. The second set is upside down for cocktail mode. We
    # ignore that.

    f = open(inname, "rb")
    charmap_data = f.read()
    f.close()

    if len(charmap_data) != 4096:
        raise ValueError("Missing charmap data")

    # read and parse all 256 characters
    chars = []
    for chr in range(256):
        chars.append(parse_chr(charmap_data[16*chr:16*(chr+1)]))

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
    f = open(innames[0], "rb")
    charmap_data_0 = f.read()
    f.close()
    
    f = open(innames[1], "rb")
    charmap_data_1 = f.read()
    f.close()

    if len(charmap_data_0) != 2048 or len(charmap_data_1) != 2048:
        raise ValueError("Missing charmap data")

    # read and parse all 256 characters
    chars = []
    for chr in range(256):
        chars.append(parse_chr_2(charmap_data_0[8*chr:8*(chr+1)],
                     charmap_data_1[8*chr:8*(chr+1)]))

    # for c in chars: show_chr(c)

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
    print("  for Galaga:     ", sys.argv[0], "../roms/gg1_9.4l ../galagino/galaga_tilemap.h")
    print("  for Pacman:     ", sys.argv[0], "../roms/pacman.5e ../galagino/pacman_tilemap.h")
    print("  for Donkey Kong:", sys.argv[0], "../roms/v_5h_b.bin ../roms/v_3pt.bin ../galagino/dkong_tilemap.h")
    exit(-1)

if len(sys.argv) == 3:
    parse_charmap(sys.argv[1], sys.argv[2])
else:
    parse_charmap_2(sys.argv[1:3], sys.argv[3])
