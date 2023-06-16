/* frogger.h */

#ifdef CPU_EMULATION

#include "frogger_rom1.h"
#include "frogger_rom2.h"

static inline unsigned char frogger_RdZ80(unsigned short Addr) {
  // frogger main cpu
  if(current_cpu == 0) {
    
    if(Addr < 16384)
      return frogger_rom_cpu1[Addr];

    // 0x8000 - 0x87ff - main RAM
    if((Addr & 0xf800) == 0x8000) return memory[Addr - 0x8000];
    
    // frogger reads from $4000 and if that returns $55 it jumps t0 $4001
    if(Addr == 0x4000) return 0x00;
    
    // watchdog
    if(Addr == 0x8800) return 0x00;
    
    // 0xc000 - 0xffff - 8255
    if((Addr & 0xc000) == 0xc000) {
      unsigned char keymask = buttons_get();
      unsigned char retval = 0xff;
      
      // e000 = IN0: Coin1,Coin0,L1,R1,Fire1A,CREDIT,Fire1B,U2      
      if(Addr == 0xe000) {
	if(keymask & BUTTON_LEFT)  retval &= ~0x20;
	if(keymask & BUTTON_RIGHT) retval &= ~0x10;
	if(keymask & BUTTON_FIRE)  retval &= ~0x08;
	if(keymask & BUTTON_COIN)  retval &= ~0x04;
      }
      
      // e002 = IN1: START1,START2,L2,R2,Fire2A,Fire2B,LIVES1,LIVES0
      if(Addr == 0xe002) {
	retval = FROGGER_DIP1 | 0xfc;
	if(keymask & BUTTON_START) retval &= ~0x80;
      }
      
      // e004 = IN2: -,D1,-,U1,UPRIGHT,COINS_P_PLAY,D2
      if(Addr == 0xe004) {
	retval = FROGGER_DIP2 | 0xf9;
	if(keymask & BUTTON_UP)    retval &= ~0x10;
	if(keymask & BUTTON_DOWN)  retval &= ~0x40;
      }
      
      // e006 = ???
      
      return retval;
    }
  } else {
    // frogger audio cpu
    if(Addr <  6144) return frogger_rom_cpu2[Addr];
    
    // main ram
    if((Addr & 0xfc00) == 0x4000) return memory[Addr - 0x4000 + 0x1000];
  }
  return 0xff;
}

static inline void frogger_WrZ80(unsigned short Addr, unsigned char Value) {
  // frogger main cpu 
  if(current_cpu == 0) {
    
    // main ram
    if((Addr & 0xf800) == 0x8000) {
      memory[Addr - 0x8000] = Value;
      return;
    }
    
    // tile ram
    if((Addr & 0xfc00) == 0xa800) {
      if(!game_started && Addr == 0xa800 + 800 && Value == 1)
	game_started = 1;
      
      memory[Addr - 0xa800 + 0x800] = Value;  // map to 0x0800
      return;
    }
    
    // sprite ram
    if((Addr & 0xff00) == 0xb000) {
      memory[Addr - 0xb000 + 0xc00] = Value;   // map to 0x0c00
      return;
    }
    
    // unknown
    if(Addr == 0x8805) return;
    
    if(Addr == 0xb808) {
      irq_enable[0] = Value & 1;
      return;
    }
    
    if(Addr == 0xb80c) return;      // flip screen Y
    if(Addr == 0xb810) return;      // flip screen X
    if(Addr == 0xb818) return;      // coin counter 1
    if(Addr == 0xb81c) return;      // coin counter 2
    
    // AY
    if((Addr & 0xc000) == 0xc000) {
      if(Addr == 0xd000) {
	// PA goes to AY port A and can be read by SND CPU through the AY
	frogger_snd_command = Value;
	return;
      }
      
      if(Addr == 0xd002) {
	// rising edge on bit 3 sets audio irq
	// bit 4 is "am pm"
	// other bits go to connector
	
	// bit 0 = state written by CPU1
	if(!(frogger_snd_irq_state & 1) && (Value & 8))
	  frogger_snd_irq_state |= 1;
	
	// CPU1 writes 0
	if(!(Value & 8) && (frogger_snd_irq_state & 1)) {
	  frogger_snd_irq_state |= 2;
	  frogger_snd_irq_state &= 0xfe;
	}
	
	return;
      }
      
      if(Addr == 0xd006) return;  // ignore write to first 8255 control register	
      if(Addr == 0xe006) return;  // ignore write to second 8255 control register
    }
  } else {
    // frogger audio cpu
    if((Addr & 0xfc00) == 0x4000) {
      memory[Addr - 0x4000 + 0x1000] = Value;
      return;
    }
    
    if((Addr & 0xf000) == 0x6000) return;
  }
}

static inline void frogger_OutZ80(unsigned short Port, unsigned char Value) {
  if(current_cpu == 1) {
    // 40 is the ay data port
    if((Port & 0xff) == 0x40) {
      soundregs[frogger_snd_ay_port] = Value;      
      return;
    }
    
    // 80 is the ay control port selecting the register
    if((Port & 0xff) == 0x80) {
      frogger_snd_ay_port = Value & 15;
      return;
    }
  }
}

static inline unsigned char frogger_InZ80(unsigned short Port) {
  static const unsigned char frogger_timer[20] = {
	  0x00, 0x10, 0x00, 0x10, 0x08, 0x18, 0x08, 0x18, 0x40, 0x50,
	  0x80, 0x90, 0x80, 0x90, 0x88, 0x98, 0x88, 0x98, 0xc0, 0xd0
  };

  if(current_cpu == 1) {
    // read AY data port
    if((Port & 0xff) == 0x40) {
      if(frogger_snd_ay_port < 14)
        return soundregs[frogger_snd_ay_port];
      
      // port A, command register
      if(frogger_snd_ay_port == 14)
        return frogger_snd_command;
      
      // port B, timer
      if(frogger_snd_ay_port == 15) {
        // AY and Z80 run at 1.75 MHz, the counter runs at the same
        // speed. The cycle time thus is 570ns
	
        // using the instruction counter for the rate isn't perfect as it assumes all
        // instructions run the same time ... which they don't
        return frogger_timer[(frogger_snd_icnt/20)%20];
      }
      
      return 0;
    }
  }
  return 0;
}

static inline void frogger_run_frame(void) {
  for(int i=0;i<INST_PER_FRAME;i++) {
    // audio CPU speed is crucial here as it determines the
    // audio playback rate
    current_cpu=0; StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]);
    current_cpu=1; StepZ80(&cpu[1]); frogger_snd_icnt++; StepZ80(&cpu[1]); frogger_snd_icnt++;

    // "latch" interrupt until CPU has them enabled
    if(frogger_snd_irq_state & 2 && cpu[1].IFF & IFF_1) {
      IntZ80(&cpu[1], INT_RST38);
      frogger_snd_irq_state &= ~2;  // clear flag
    } 
  }
      
  if(irq_enable[0]) {
    current_cpu = 0;
    IntZ80(&cpu[0], INT_NMI);
  }
}

#endif // CPU_EMULATION

#ifdef IO_EMULATION

#ifndef SINGLE_MACHINE
#include "frogger_logo.h"
#endif
#include "frogger_tilemap.h"
#include "frogger_spritemap.h"
#include "frogger_cmap.h"

#endif // IO_EMULATION
