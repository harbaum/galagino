/* pacman.h */

#ifdef CPU_EMULATION
#include "pacman_rom.h"

static inline unsigned char pacman_RdZ80(unsigned short Addr) {
  Addr &= 0x7fff;   // a15 is unused
    
  if(Addr < 16384)
    return pacman_rom[Addr];

  if((Addr & 0xf000) == 0x4000) {    
    // this includes spriteram 1
    return memory[Addr - 0x4000];
  }   

  if((Addr & 0xf000) == 0x5000) {
    // get a mask of currently pressed keys
    unsigned char keymask = buttons_get();
    
    if(Addr == 0x5080)    // dip switch
      return PACMAN_DIP;
    
    if(Addr == 0x5000) {
      unsigned char retval = 0xff;
      if(keymask & BUTTON_UP)    retval &= ~0x01;
      if(keymask & BUTTON_LEFT)  retval &= ~0x02;
      if(keymask & BUTTON_RIGHT) retval &= ~0x04;
      if(keymask & BUTTON_DOWN)  retval &= ~0x08;
      if(keymask & BUTTON_COIN)  retval &= ~0x20;  
      return retval;
    }
    
    if(Addr == 0x5040) {
      unsigned char retval = 0xff; // 0xef for service
      if(keymask & BUTTON_START)  retval &= ~0x20;  
      return retval;
    }
  }
  return 0xff;
}

static inline void pacman_WrZ80(unsigned short Addr, unsigned char Value) {
  Addr &= 0x7fff;   // a15 is unused
    
  if((Addr & 0xf000) == 0x4000) {
    // writing 85 (U, first char of UP) to the top left corner
    // is an indication that the game has booted up      
    if(Addr == 0x4000 + 985 && Value == 85)
      game_started = 1;
    
    memory[Addr - 0x4000] = Value;
    return;
  }
  
  if((Addr & 0xff00) == 0x5000) {
    // 0x5060 to 0x506f writes through to ram (spriteram2)
    if((Addr & 0xfff0) == 0x5060)
      memory[Addr - 0x4000] = Value;
    
    if(Addr == 0x5000) 
      irq_enable[0] = Value & 1;
    
    if((Addr & 0xffe0) == 0x5040) {
      if(soundregs[Addr - 0x5040] != Value & 0x0f)
	soundregs[Addr - 0x5040] = Value & 0x0f;
    }
    
    return;
  }
    
  // initial stack is at 0xf000 (0x6000 with a15 ignored), so catch that
  //if((Addr & 0xfffe) == 0x6ffe)
  //  return;
} 

static inline void pacman_OutZ80(unsigned short Port, unsigned char Value) {
  irq_ptr = Value;
}

//
static inline void pacman_run_frame(void) {
  for(int i=0;i<INST_PER_FRAME;i++) {
    StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
  }
      
  if(irq_enable[0])
    IntZ80(cpu, irq_ptr);     
}
#endif // CPU_EMULATION

#ifdef IO_EMULATION

#ifndef SINGLE_MACHINE
#include "pacman_logo.h"
#endif
#include "pacman_tilemap.h"
#include "pacman_spritemap.h"
#include "pacman_cmap.h"
#include "pacman_wavetable.h"

#endif // IO_EMULATION
