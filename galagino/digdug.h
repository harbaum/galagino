/* digdug.h */

#ifdef CPU_EMULATION

#include "digdug_rom1.h"
#include "digdug_rom2.h"
#include "digdug_rom3.h"

static inline unsigned char digdug_RdZ80(unsigned short Addr) {
  static const unsigned char *rom[] = { digdug_rom_cpu1, digdug_rom_cpu2, digdug_rom_cpu3 };
  
  if(Addr < 16384)
    return rom[current_cpu][Addr];

  /* video/sprite ram */
  if((Addr & 0xe000) == 0x8000)
    return memory[Addr - 0x8000];
  
  // 0x7100 namco_06xx_device, ctrl_r, ctrl_w
  if((Addr & 0xfe00) == 0x7000)
    return namco_read_dd(Addr & 0x1ff);
  
  //  if((Addr & 0xfff0) == 0xa000)
  //    return 0xff;
  
  if((Addr & 0xffc0) == 0xb800)
    return 0x00;

  return 0xff;
}
  
static inline void digdug_WrZ80(unsigned short Addr, unsigned char Value) {
  if((Addr & 0xe000) == 0x8000) {
    // 8000 - 83ff - tile ram
    // 8400 - 87ff - work
    // 8800 - 8bff - sprite ram
    // 9000 - 93ff - sprite ram
    // 9800 - 9bff - sprite ram
    
    // this is actually 2k + 1k + 1k gap + 1k + 1k gap + 1k + 1k gap
    
    // this comaprison really hurts performance wise. Try to
    // find some other trigger, like IO
    
    // writing 46 (U), first char of UP, to the top left corner
    // is an indication that the game has booted up      
    if(Addr == 0x8000 + 985 && (Value & 0x7f) == 46)
      game_started = 1;
    
    memory[Addr - 0x8000] = Value;
    return;
  }
  
  if((Addr & 0xffe0) == 0x6800) {
    soundregs[Addr - 0x6800] = Value & 0x0f;      
    return;
  }
  
  if((Addr & 0xfff8) == 0x6820) {
    if((Addr & 0x0c) == 0x00) {
      if((Addr & 3) < 3) {
	irq_enable[Addr & 3] = Value & 1;
      } else {
	sub_cpu_reset = !Value;
	
	if(sub_cpu_reset) {
	  // this also resets the 51xx
	  namco_command = 0x00;
	  namco_mode = 0;
	  namco_nmi_counter = 0;
	  
	  current_cpu = 1; ResetZ80(&cpu[1]);
	  current_cpu = 2; ResetZ80(&cpu[2]);
	}
      }
    }
    return;
  }
  
  // if(Addr == 0x6830) return; // watchdog etc
  
  if((Addr & 0xfe00) == 0x7000) {
    namco_write_dd(Addr & 0x1ff, Value);
    return;
  }
  
  // control port
  if((Addr & 0xfff8) == 0xa000) {
    if(Value & 1) digdug_video_latch |=  (1<<(Addr & 7));
    else          digdug_video_latch &= ~(1<<(Addr & 7));
    return;
  }
  
  //if((Addr & 0xffc0) == 0xb800) return;
  //if(Addr == 0xb840) return;
  
  // digdug cpu #1 writes to 0xffff, 0000 and 0002 @ #1@0866/868/86c
  // if((current_cpu == 1) && ((Addr == 0xffff) || (Addr == 0) || (Addr == 2))) return;
}

static inline void digdug_run_frame(void) {
  for(char c=0;c<4;c++) {    
    for(int i=0;i<INST_PER_FRAME/4;i++) {
      current_cpu = 0;
      StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
      if(!sub_cpu_reset) {
        // running both sub-cpus at full speed as well makes the setup instable :-(
        current_cpu = 1;
        StepZ80(cpu+1); StepZ80(cpu+1); StepZ80(cpu+1); StepZ80(cpu+1);       
        current_cpu = 2;
        StepZ80(cpu+2); StepZ80(cpu+2); StepZ80(cpu+2); StepZ80(cpu+2);       
      }

      // nmi counter for cpu0
      if(namco_nmi_counter) {
        namco_nmi_counter--;
        if(!namco_nmi_counter) {
          current_cpu = 0;
          IntZ80(&cpu[0], INT_NMI);
          namco_nmi_counter = NAMCO_NMI_DELAY;
        }
      }
    }

    // run cpu2 nmi at ~line 64 and line 192
    if(!sub_cpu_reset && !irq_enable[2] && ((c == 1) || (c == 3))) {
      current_cpu = 2;
      IntZ80(&cpu[2], INT_NMI);
    }
  }
  
  if(irq_enable[0]) {
    current_cpu = 0;
    IntZ80(&cpu[0], INT_RST38);
  }

  if(!sub_cpu_reset && irq_enable[1]) {
    current_cpu = 1;
    IntZ80(&cpu[1], INT_RST38);
  }
}
#endif // CPU_EMULATION

#ifdef IO_EMULATION

#ifndef SINGLE_MACHINE
#include "digdug_logo.h"
#endif
#include "digdug_spritemap.h"
#include "digdug_tilemap.h"
#include "digdug_pftiles.h"
#include "digdug_cmap_tiles.h"
#include "digdug_cmap_sprites.h"
#include "digdug_cmap.h"
#include "digdug_playfield.h"
#include "digdug_wavetable.h"

#endif // IO_EMULATION
