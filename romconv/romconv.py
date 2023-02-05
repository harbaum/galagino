#!/usr/bin/env python3
import sys

PATCHES = {
    "galaga_rom_cpu1":
    [
        # jump over tile ram test
        ( 0x3382, 0x06, 0xc3 ),     # 06, jp
        ( 0x3383, 0x0a, 0x35 ),     # xx35
	( 0x3384, 0xd9, 0x34 ),     # 34xx
	# only one ramtest round, instead of 30
	( 0x348a, 0x1e, 0x01 ),
        # skip rom test
	( 0x352b, 0xe5, 0xc9 )      # ret
    ]    
}

def parse_rom(id, infiles, outfile, apply_patches = False):
    offset = 0
    of = open(outfile, "w")

    print("const unsigned char "+id+"[] = {\n  ", end="", file=of)
    
    for name_idx in range(len(infiles)):
        f = open(infiles[name_idx], "rb")
        rom_data = f.read()
        f.close()

        if infiles[0].split(".")[-1] != "s8":
            if len(rom_data) != 4096:
                raise ValueError("Missing rom data")

        # apply patches
        if apply_patches:
            rom_data = list(rom_data)
            for i in PATCHES:
                if i == id:
                    for p in PATCHES[i]:
                        if p[0] - offset < len(rom_data):
                            if rom_data[p[0] - offset] == p[1]:
                                print("Patching", hex(p[0]), ":", p[1], "->", p[2])
                                rom_data[p[0] - offset] = p[2]
                            else:
                                raise ValueError("Unexpected patchdata")
        
        offset += len(rom_data)
        for i in range(len(rom_data)):
            print("0x{:02X}".format(rom_data[i]), end="", file=of)
            if i != len(rom_data)-1 or name_idx != len(infiles)-1:
                print(",", end="", file=of)
                if i&15 == 15:
                    print("\n  ", end="", file=of)
            else:
                print("", file=of)
        
    print("};", file=of)
    of.close()

if len(sys.argv) < 3:
    print("Usage:",sys.argv[0], "name <infiles> <outfile>")
    print("  -p  - apply patches (e.g. fast boot)")
    print("  for Galaga CPU1:  ", sys.argv[0], "-p galaga_rom_cpu1 ../roms/gg1_1b.3p ../roms/gg1_2b.3m ../roms/gg1_3.2m ../roms/gg1_4b.2l ../galagino/galaga_rom1.h")
    print("  for Galaga CPU2:  ", sys.argv[0], "galaga_rom_cpu2 ../roms/gg1_5b.3f ../galagino/galaga_rom2.h")
    print("  for Galaga CPU3:  ", sys.argv[0], "galaga_rom_cpu3 ../roms/gg1_7b.2c ../galagino/galaga_rom3.h")
    print("  for Pacman:       ", sys.argv[0], "pacman_rom ../roms/pacman.6e ../roms/pacman.6f ../roms/pacman.6h ../roms/pacman.6j ../galagino/pacman_rom.h")
    print("  for Donkey Kong:  ", sys.argv[0], "dkong_rom ../roms/c_5et_g.bin ../roms/c_5ct_g.bin ../roms/c_5bt_g.bin ../roms/c_5at_g.bin ../galagino/dkong_rom.h")
    exit(-1)

if sys.argv[1] == "-p":       
    parse_rom(sys.argv[2], sys.argv[3:-1], sys.argv[-1], True)
else:
    parse_rom(sys.argv[1], sys.argv[2:-1], sys.argv[-1])
