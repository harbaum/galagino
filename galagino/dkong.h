/* dkong.h */

#ifdef CPU_EMULATION

#include "dkong_rom1.h"

static inline unsigned char dkong_RdZ80(unsigned short Addr) {
  if(Addr < 16384)
    return dkong_rom_cpu1[Addr];

  // 0x6000 - 0x77ff
  if(((Addr & 0xf000) == 0x6000) || ((Addr & 0xf800) == 0x7000)) 
    return memory[Addr - 0x6000];
  
  if((Addr & 0xfff0) == 0x7c00) {
    // get a mask of currently pressed keys
    unsigned char keymask = buttons_get();
    
    unsigned char retval = 0x00;
    if(keymask & BUTTON_RIGHT) retval |= 0x01;
    if(keymask & BUTTON_LEFT)  retval |= 0x02;
    if(keymask & BUTTON_UP)    retval |= 0x04;
    if(keymask & BUTTON_DOWN)  retval |= 0x08;
    if(keymask & BUTTON_FIRE)  retval |= 0x10;
    return retval;
  }
  
  if((Addr & 0xfff0) == 0x7c80) {  // IN1
    return 0x00;
  }
  
  if((Addr & 0xfff0) == 0x7d00) {
    // get a mask of currently pressed keys
    unsigned char keymask = buttons_get();
    
    unsigned char retval = 0x00;
    if(keymask & BUTTON_COIN)   retval |= 0x80;  
    if(keymask & BUTTON_START)  retval |= 0x04; 
    return retval;
  }
  
  if((Addr & 0xfff0) == 0x7d80)
    return DKONG_DIP;

  return 0xff;
}

static inline void dkong_WrZ80(unsigned short Addr, unsigned char Value) {
  if(((Addr & 0xf000) == 0x6000) || ((Addr & 0xf800) == 0x7000)) {
    memory[Addr - 0x6000] = Value;
    return;
  }
  
  // ignore DMA register access
  if((Addr & 0xfe00) == 0x7800)
    return;
  
  if((Addr & 0xfe00) == 0x7c00) {  // 7cxx and 7dxx
    // music effect
    if(Addr == 0x7c00) dkong_sfx_index = Value;
    
    // 7d0x
    if((Addr & 0xfff0) == 0x7d00) {
      
      // trigger samples 0 (walk), 1 (jump) and 2 (stomp)
      if((Addr & 0x0f) <= 2  && Value)
	dkong_trigger_sound(Addr & 0x0f);
      
      if((Addr & 0x0f) == 3) {
	if(Value & 1) cpu_8048.p2_state &= ~0x20;
	else          cpu_8048.p2_state |=  0x20;
      }
      
      if((Addr & 0x0f) == 4)
	cpu_8048.T1 = !(Value & 1);
      
      if((Addr & 0x0f) == 5)
	cpu_8048.T0 = !(Value & 1);          
    }
    
    if(Addr == 0x7d80)
      cpu_8048.notINT = !(Value & 1);
    
    if(Addr == 0x7d84)
      irq_enable[0] = Value & 1;
    
    if((Addr == 0x7d85) && (Value & 1)) {
      // trigger DRQ to start DMA
      // Dkong uses the DMA only to copy sprite data from 0x6900 to 0x7000
      memcpy(memory+0x1000, memory+0x900, 384);
    }
    
    if(Addr == 0x7d86) {
      colortable_select &= ~1;
      colortable_select |= (Value & 1);
    }
    
    if(Addr == 0x7d87) {
      colortable_select &= ~2;
      colortable_select |= ((Value<<1) & 2);
    }
    return;
  }
}

static inline void dkong_run_frame(void) {
  game_started = 1; // TODO: make this from some graphic thing
    
  // dkong      
  for(int i=0;i<INST_PER_FRAME;i++) {
    StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);

    // run audio cpu only when audio transfer buffers are not full. The
    // audio CPU seems to need more CPU time than the main Z80 itself.
    if(((dkong_audio_wptr+1)&DKONG_AUDIO_QUEUE_MASK) != dkong_audio_rptr) {
      i8048_step(&cpu_8048); i8048_step(&cpu_8048);
      i8048_step(&cpu_8048); i8048_step(&cpu_8048);
      i8048_step(&cpu_8048); i8048_step(&cpu_8048);
    }
  }
      
  if(irq_enable[0])
    IntZ80(cpu, INT_NMI);
}
#endif // CPU_EMULATION

#ifdef IO_EMULATION

#ifndef SINGLE_MACHINE
#include "dkong_logo.h"
#endif
#include "dkong_tilemap.h"
#include "dkong_spritemap.h"
#include "dkong_cmap.h"

#include "dkong_sample_walk0.h"
#include "dkong_sample_walk1.h"
#include "dkong_sample_walk2.h"
#include "dkong_sample_jump.h"
#include "dkong_sample_stomp.h"

#endif // IO_EMULATION
