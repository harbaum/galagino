#!/usr/bin/env python3

ROMLOC="../roms/"
WAVETABLE = "prom-1.1d"
BOOMSOUND = "boom_12k_s8.raw"

def parse_rom(name):
    f = open(name, "rb")
    wave_data = f.read()
    f.close()

    if len(wave_data) != 256:
        raise ValueError("Missing rom data")

    print("const signed char wavetables[][32] = {")
    
    # 8 waveforms
    for w in range(8):
        print("// wave #{:d}".format(w))
        # draw waveform        
        for y in range(8):
            print("//", end="")
            for s in range(32):                
                if wave_data[32*w+s] == 15-2*y:
                    print("---", end="")
                elif wave_data[32*w+s] == 15-(2*y+1):
                    print("___", end="")
                else:
                    print("   ",end="")                    
            print("")            

        print(" {", end="");        
        # with 32 values each
        for s in range(32):
            print("{:2d}".format(wave_data[32*w+s]-7), end="")
            if s!= 31:    print(",", end="")
            elif w != 7:  print("},");
            else:         print("}");
            
            # print(",", end="")
        print("")            
            
    print("};")

def parse_boom(name):
    f = open(name, "rb")
    boom_data = f.read()
    f.close()

    print("// boom sound, 12khz signed 8 bits pcm")
    print("const signed char boom[] = {")
    
    for w in range(len(boom_data)):
        b = boom_data[w] if boom_data[w] < 128 else boom_data[w]-256
        print("{:d},".format(b), end="")
        if (w % 32) == 31: print("")
    print("};")

parse_rom(ROMLOC + WAVETABLE)
print("")
parse_boom(ROMLOC + BOOMSOUND)
    
