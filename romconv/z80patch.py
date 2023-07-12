#!/usr/bin/env python3
import zipfile

ZIPLOC="../roms/"
ZIP="Z80-081707.zip"
DEST="../galagino/"

FILES2COPY=[ "CodesCB.h", "Codes.h", "CodesXX.h", "CodesED.h", "CodesXCB.h", "Tables.h" ]

Z80H_EXTRA=b"""
#include "config.h"
#include "emulation.h"
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

#ifdef ENABLE_DIGDUG
// digdug is very close to the performance limit. Implementing 
// it's own main instruction with direct rom access helps keeping
// the performance up
extern char current_cpu;
extern const unsigned char digdug_rom_cpu1[];
extern const unsigned char digdug_rom_cpu2[];
extern const unsigned char digdug_rom_cpu3[];

void digdug_StepZ80(Z80 *R)
{
  static const unsigned char *rom_table[3] =
    { digdug_rom_cpu1, digdug_rom_cpu2, digdug_rom_cpu3 };

  register byte I;
  register pair J;

  // I=OpZ80_INL(R->PC.W++);
  I=rom_table[current_cpu][R->PC.W++];

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
#endif
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

