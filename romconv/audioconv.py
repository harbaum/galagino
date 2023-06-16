#!/usr/bin/env python3
import sys

def parse_wavetable(name, infiles, outfile):
    of = open(outfile, "w")
    print("const signed char "+name+"[][32] = {", file=of)
    
    for infile in infiles:
        f = open(infile, "rb")
        wave_data = f.read()
        f.close()

        if len(wave_data) != 256:
            raise ValueError("Missing rom data")

        # 8 waveforms per rom
        for w in range(8):
            print("// "+infile.split("/")[-1]+" wave #{:d}".format(w), file=of)
            # draw waveform        
            for y in range(8):
                print("//", end="", file=of)
                for s in range(32):                
                    if wave_data[32*w+s] == 15-2*y:
                        print("---", end="", file=of)
                    elif wave_data[32*w+s] == 15-(2*y+1):
                        print("___", end="", file=of)
                    else:
                        print("   ",end="", file=of)                    
                print("", file=of)            

            print(" {", end="", file=of);        
            # with 32 values each
            for s in range(32):
                print("{:2d}".format(wave_data[32*w+s]-7), end="", file=of)
                if s!= 31:    print(",", end="", file=of)
                elif w != 7 or infile != infiles[-1]:  print("},", file=of);
                else:         print("}", file=of);
            
            print("", file=of)            
            
    print("};", file=of)

    of.close()
        
if len(sys.argv) < 4:
    print("Usage:",sys.argv[0], "name <infiles> <outfile>")
    print("  Galaga wavetable: ", sys.argv[0], "galaga_wavetable ../roms/prom-1.1d ../galagino/galaga_wavetable.h")
    print("  Pacman wavetable: ", sys.argv[0], "pacman_wavetable ../roms/82s126.1m ../roms/82s126.3m ../galagino/pacman_wavetable.h")
    print("  Digdug wavetable: ", sys.argv[0], "digdug_wavetable ../roms/136007.110 ../roms/136007.109 ../galagino/digdug_wavetable.h")

    exit(-1)

parse_wavetable(sys.argv[1], sys.argv[2:-1], sys.argv[-1])


    
