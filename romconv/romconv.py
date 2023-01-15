#!/usr/bin/env python3

ROMLOC="../roms/"
CPU1 = [ "gg1_1b.3p", "gg1_2b.3m", "gg1_3.2m", "gg1_4b.2l" ]
CPU2 = [ "gg1_5b.3f" ]
CPU3 = [ "gg1_7b.2c" ]

def parse_rom(id, names):
    print("const unsigned char "+id+"[] = {\n  ", end="")
    
    for name_idx in range(len(names)):
        f = open(ROMLOC + names[name_idx], "rb")
        rom_data = f.read()
        f.close()

        if len(rom_data) != 4096:
            raise ValueError("Missing rom data")

        for i in range(len(rom_data)):
            print("0x{:02X}".format(rom_data[i]), end="")
            if i != len(rom_data)-1 or name_idx != len(names)-1:
                print(",", end="")
                if i&15 == 15:
                    print("\n  ", end="")
            else:
                print("")
        
    print("};")

parse_rom("rom_cpu1", CPU1)
parse_rom("rom_cpu2", CPU2)
parse_rom("rom_cpu3", CPU3)
    
