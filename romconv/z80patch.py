#!/usr/bin/env python3
import zipfile

ZIPLOC="../roms/"
ZIP="Z80-081707.zip"
DEST="../galagino/"

FILES2COPY=[ "CodesCB.h", "Codes.h", "CodesXX.h", "CodesED.h", "CodesXCB.h", "Tables.h" ]

Z80H_EXTRA=b"""
#include "config.h"
#include "emulation.h"

extern char current_cpu;
extern const unsigned char pacman_rom[];
extern const unsigned char galaga_rom_cpu1[];
extern const unsigned char galaga_rom_cpu2[];
extern const unsigned char galaga_rom_cpu3[];
extern const unsigned char dkong_rom_cpu1[];
extern const unsigned char frogger_rom_cpu1[];
extern const unsigned char frogger_rom_cpu2[];

static inline byte OpZ80_INL(register word Addr) {
#ifdef ENABLE_PACMAN
  #ifndef SINGLE_MACHINE
    if(machine == MCH_PACMAN)
  #endif
      return pacman_rom[Addr];
  #ifndef SINGLE_MACHINE
    else 
  #endif 
#endif
  
#ifdef ENABLE_GALAGA
  #if defined(ENABLE_DKONG) || defined(ENABLE_FROGGER)
    if(machine == MCH_GALAGA)
  #endif 
    { 
      if(current_cpu == 1)       return galaga_rom_cpu2[Addr];
      else if(current_cpu == 2)  return galaga_rom_cpu3[Addr];
      return galaga_rom_cpu1[Addr];
    }
  #if definded(ENABLE_DKONG) || defined(ENABLE_FROGGER)
    else /* if(machine == MCH_DKONG) */
  #endif
#endif

#ifdef ENABLE_DKONG
  #ifdef ENABLE_FROGGER
    if(machine == MCH_DKONG)
  #endif 
  {
    return dkong_rom_cpu1[Addr];
  }
#endif

#ifdef ENABLE_FROGGER
   if(current_cpu == 0) return frogger_rom_cpu1[Addr];
   else                 return frogger_rom_cpu2[Addr];
#endif
}
"""

Z80C_EXTRA=b"""
void StepZ80(Z80 *R)
{
  register byte I;
  register pair J;

  I=OpZ80_INL(R->PC.W++);

  switch(I)
  {
#include "Codes.h"
    case PFX_CB: CodesCB(R);break;
    case PFX_ED: CodesED(R);break;
    case PFX_FD: CodesFD(R);break;
    case PFX_DD: CodesDD(R);break;
  }

  if(R->IFF&IFF_EI)
    R->IFF=(R->IFF&~IFF_EI)|IFF_1; /* Done with AfterEI state */
}
"""

def unpack_z80(name):
    with zipfile.ZipFile(name, 'r') as zip:
        # most files are just unpacked
        for i in FILES2COPY:
            print("Copying", i)
            code = zip.read("Z80/"+i)
            with open(DEST+i, "wb") as of:
                of.write(code)

        # Z80.h has a small patch
        print("Patching Z80.h")
        code = zip.read("Z80/Z80.h")
        with open(DEST+"Z80.h", "wb") as of:
            # read file line by line and uncomment this line:
            STR = b"/* #define LSB_FIRST */        /* Compile for low-endian CPU */"

            for i in code.split(b"\r\n"):
                if i == STR:
                    of.write(b"#define LSB_FIRST              /* Compile for low-endian CPU */\r\n")
                else:
                    of.write(i + b"\r\n")
            of.write(Z80H_EXTRA)
    
        # Z80.c gets an additional function
        print("Patching Z80.c")
        code = zip.read("Z80/Z80.c")
        with open(DEST+"Z80.c", "wb") as of:
            of.write(code)
            of.write(Z80C_EXTRA)

unpack_z80(ZIPLOC + ZIP)

