#!/usr/bin/env python3

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
            
def parse_charmap(name):
    # The character map rom contains the same set of 128 characters
    # two times. The second set is upside down for cocktail mode. We
    # ignore that.

    f = open(name, "rb")
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
    print("const unsigned short "+name.split("/")[-1].split(".")[0]+"[][8] = {" )
    chars_str = []
    for c in chars:
        chars_str.append(" { " + dump_chr(c) + " }")
    print(",\n".join(chars_str))
    print("};")
        
parse_charmap(ROMLOC + CHARMAP)
    
