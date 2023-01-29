/*
 * emulation.c
 *
 */

#include <stdio.h>

#include "Z80.h"
#include "config.h"

#include "emulation.h"

#ifdef ENABLE_GALAGA
#include "galaga_rom1.h"
#include "galaga_rom2.h"
#include "galaga_rom3.h"
#endif

#ifdef ENABLE_PACMAN
#include "pacman_rom.h"
#endif

#ifdef ENABLE_DKONG
#include "dkong_rom.h"
#endif

extern void StepZ80(Z80 *R);
Z80 cpu[3];    // up to three z80 supported

unsigned char *memory;

#ifdef ENABLE_GALAGA
#ifdef FREEPLAY
unsigned char credit = 100;
#else
unsigned char credit = 0;
#endif
#endif

char game_started = 0;

char current_cpu = 0;
char irq_enable[3] = { 0,0,0 };

#ifdef ENABLE_GALAGA
// special variables for galaga
char sub_cpu_reset = 1;
char credit_mode = 0;
int namco_cnt = 0;
int namco_busy = 0;
unsigned char cs_ctrl = 0;
int nmi_cnt = 0;
int coincredMode = 0;   
unsigned char starcontrol = 0;
#endif

#ifdef ENABLE_PACMAN
// special variables for pacman
unsigned char irq_ptr = 0;
#endif

#ifdef ENABLE_DKONG
// special variables for dkong
unsigned char colortable_select = 0;
#endif

#ifndef SINGLE_MACHINE
signed char menu_sel = (int)((MACHINES+1)/2);       // default in game selection
#endif

#if defined(ENABLE_PACMAN) || defined(ENABLE_GALAGA)
// mirror of sounds registers
unsigned char soundregs[32] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
#endif

void OutZ80(unsigned short Port, unsigned char Value) {
#ifdef ENABLE_PACMAN
#ifndef SINGLE_MACHINE
  if(machine == MCH_PACMAN)
#endif
    irq_ptr = Value;
#endif
}
  
unsigned char InZ80(unsigned short Port) { return 0; }
void PatchZ80(Z80 *R) {}

// Memory write -- write the Value to memory location Addr
void WrZ80(unsigned short Addr, unsigned char Value) {
#ifdef ENABLE_PACMAN
#ifndef SINGLE_MACHINE
  if(machine == MCH_PACMAN) 
#endif  
  {
    Addr &= 0x7fff;   // a15 is unused
    
    if((Addr & 0xff00) == 0x5000) {
      if(Addr == 0x5000) 
        irq_enable[0] = Value & 1;

      // 0x5060 to 0x506f writes through to ram (spriteram2)
      if((Addr & 0xfff0) == 0x5060)
        memory[Addr - 0x4000] = Value;
  
      if((Addr & 0xffe0) == 0x5040) {
        if(soundregs[Addr - 0x5040] != Value & 0x0f)
          soundregs[Addr - 0x5040] = Value & 0x0f;
      }
    
      return;
    }
    
    if((Addr & 0xf000) == 0x4000) {
      if(Addr == 0x4000 + 989 && Value == 0x3e)
        game_started = 1;
      
      memory[Addr - 0x4000] = Value;
      return;
    }

    // initial stack is at 0xf000 (0x6000 with a15 ignored), so catch that
    if((Addr & 0xfffe) == 0x6ffe)
      return;
  } 
#ifndef SINGLE_MACHINE
  else
#endif
#endif

#ifdef ENABLE_GALAGA
#ifdef ENABLE_DKONG
  if(machine == MCH_GALAGA) 
#endif
  {  
    if(Addr < 16384) return;   // ignore rom writes

    // namco 06xx
    if((Addr & 0xf800) == 0x7000) {
      if(Addr & 0x100) {  // 7100
        // see task_man.s L450
        namco_cnt = 0;
        cs_ctrl = Value;
        namco_busy = 5000;   // this delay is important for proper startup 
      
        if(Value == 0xa8) galaga_trigger_sound_explosion();
      
      } else {            // 7000
        if(cs_ctrl & 1) {
          if(coincredMode) {
            coincredMode--;
            return;
          }
          switch(Value) {
          case 1:
            // main cpu sets coin and cred mode for both players (four bytes)
            coincredMode = 4;
            break;
          
          case 2:
            credit_mode = 1;
            break;
          
          case 3: // disable joystick remapping
          case 4: // enable joystick remapping
            break;
          
          case 5:
            credit_mode = 0;
            break;
          }
          namco_cnt++;
        }
      }    
    }
    if((Addr & 0xffe0) == 0x6800) {
      int offset = Addr - 0x6800;
      Value &= 0x0f;
    
      if(soundregs[offset] != Value)
        soundregs[offset] = Value;
      
      return;
    }

    if((Addr & 0xfffc) == 0x6820) {
      if((Addr & 3) < 3)
        irq_enable[Addr & 3] = Value;
      else {
        sub_cpu_reset = !Value;
        credit_mode = 0;   // this also resets the 51xx
      }
      return;
    }
 
    if(Value == 0 && Addr == 0x8210) // gg1-4.s L1725
      game_started = 1;

    if((Addr & 0xfff0) == 0xa000) {
      if(Value & 1)  starcontrol |=  (1<<(Addr & 7));   // set bit
      else           starcontrol &= ~(1<<(Addr & 7));   // clear bit
      return;
    }

    if((Addr & 0xe000) == 0x8000)
      memory[Addr-0x8000] = Value;

#if 0
    // inject a fixed score to be able to check hiscore routines easily
    if(game_started && (Addr == 0x8000 + 1020) && (memory[1017] == 0) && (memory[1018] == 0x24)) {
      // start with score = 23450
      memory[1020] = 2; memory[1019] = 3; memory[1018] = 4; memory[1017] = 5; 
    }     
#endif
  }
#ifdef ENABLE_DKONG
  else /* if(machine == MCH_DKONG) */ 
#endif
#endif  

#ifdef ENABLE_DKONG
  {
    if(((Addr & 0xf000) == 0x6000) || ((Addr & 0xf800) == 0x7000)) {
      memory[Addr - 0x6000] = Value;
      return;
    }

    // ignore DMA register access
    if((Addr & 0xfe00) == 0x7800)
      return;
      
    if((Addr & 0xfe00) == 0x7c00) {  // 7cxx and 7dxx
      // music effect
      if(Addr == 0x7c00 && Value) 
        dkong_trigger_sound(16 + Value);

      // special sound effect
      if((Addr & 0xfff0) == 0x7d00 && Value)
        dkong_trigger_sound(Addr & 0x0f);

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
#endif
}

unsigned char RdZ80(unsigned short Addr) {
#ifdef ENABLE_PACMAN
#ifndef SINGLE_MACHINE
  if(machine == MCH_PACMAN)
#endif 
  {
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
            
      // printf("@%04x IO read %04x\n", cpu[0].PC.W, Addr);

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
      return 0xff;
    }
  }
#ifndef SINGLE_MACHINE
  else 
#endif 
#endif
  
#ifdef ENABLE_GALAGA
#ifdef ENABLE_DKONG  
  if(machine == MCH_GALAGA)
#endif 
  { 
    if(Addr < 16384) {
      if(current_cpu == 1)       return galaga_rom_cpu2[Addr];
      else if(current_cpu == 2)  return galaga_rom_cpu3[Addr];
      return galaga_rom_cpu1[Addr];
    }

    // latch
    if((Addr & 0xfff8) == 0x6800) {
      unsigned char dip_a = (GALAGA_DIPA & (0x80 >> (Addr&7))) ? 0:1;
      unsigned char dip_b = (GALAGA_DIPB & (0x80 >> (Addr&7))) ? 0:2;    
      return dip_a + dip_b;
    }

    // 0x7100 namco_06xx_device, ctrl_r, ctrl_w
    if((Addr&0xfe00) == 0x7000) {
      if(Addr & 0x100) {
        return namco_busy?0x00:0x10;   // cmd ack (game_ctrl.s L1024)
      } else  {
        unsigned char retval = 0x00;

        if(cs_ctrl & 1) {   // bit[0] -> 51xx selected
          if(!credit_mode) {
            // galaga doesn't seem to use the button mappings in byte 1 and 2 in 
            // non-credit mode. So we don't implement that
            unsigned char map71[] = { 0b11111111, 0xff, 0xff };
            if(namco_cnt > 2) return 0xff;

            retval = map71[namco_cnt];
          } else {
            static unsigned char prev_mask = 0;
      	    static unsigned char fire_timer = 0;
          
            // byte 0 is credit in BCD, byte 1 and 2: ...FLURD 
            unsigned char mapb1[] = { 16*(credit/10) + credit % 10, 0b11111111, 0b11111111 };

            // get a mask of currently pressed keys
            unsigned char keymask = buttons_get();

    	      // report directions directly
	          if(keymask & BUTTON_LEFT)  mapb1[1] &= ~0x08;
	          if(keymask & BUTTON_UP)    mapb1[1] &= ~0x04;
	          if(keymask & BUTTON_RIGHT) mapb1[1] &= ~0x02;
	          if(keymask & BUTTON_DOWN)  mapb1[1] &= ~0x01;

	          // report fire only when it was pressed
	          if((keymask & BUTTON_FIRE) && !(prev_mask & BUTTON_FIRE)) {
	            mapb1[1] &= ~0x10;
	            fire_timer = 1;         // 0 is too short for score enter, 5 is too long
      	      // should probably be done via a global counter
	          } else if(fire_timer) {
	            mapb1[1] &= ~0x10;
	            fire_timer--;
      	    }

    	      // 51xx leaves credit mode when user presses start? Nope ...
	          if((keymask & BUTTON_START) && !(prev_mask & BUTTON_START) && credit)
	            credit -= 1;

	          if((keymask & BUTTON_COIN) && !(prev_mask & BUTTON_COIN) && (credit < 99))
	            credit += 1;

            if(namco_cnt > 2) return 0xff;
          
            retval = mapb1[namco_cnt];
            prev_mask = keymask;
          }
          namco_cnt++;
        }
        return retval;
      }
    }

    /* video/sprite ram */
    if((Addr & 0xe000) == 0x8000)
      return memory[Addr-0x8000];
  }
#ifdef ENABLE_DKONG
  else /* if(machine == MCH_DKONG) */
#endif
#endif

#ifdef ENABLE_DKONG
  {
    if(Addr < 16384)
      return dkong_rom[Addr];

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
  }
#endif
  
  return -1;
}

void prepare_emulation(void) {
  memory = malloc(8192);

  for(current_cpu=0;current_cpu<3;current_cpu++)
    ResetZ80(&cpu[current_cpu]);
}

// one inst at 3Mhz ~ 500k inst/sec = 500000/60 inst per frame
#define INST_PER_FRAME 300000/60/4

void emulate_frame(void) {
  unsigned long sof = micros();
#ifndef SINGLE_MACHINE
  if(machine == MCH_MENU) {
    static unsigned char last_mask = 0;
    // get a mask of currently pressed keys
    unsigned char keymask = buttons_get();     

    if((keymask & BUTTON_UP) &&
	    !(last_mask & BUTTON_UP)) menu_sel--;
    if((keymask & BUTTON_DOWN) &&
	    !(last_mask & BUTTON_DOWN)) menu_sel++;

    if(((keymask & BUTTON_FIRE) &&
  	  !(last_mask & BUTTON_FIRE)) ||
	    ((keymask & BUTTON_START) &&
	    !(last_mask & BUTTON_START))) {
	    machine = menu_sel;

#ifdef ENABLE_DKONG
      // dkong uses a different sample rate, so reconfigure audio if
      // kong is requested
      if(machine == MCH_DKONG)
        audio_dkong_bitrate();
#endif
    }

    if(menu_sel < 1)         menu_sel = 1;
    if(menu_sel > MACHINES)  menu_sel = MACHINES;
      
    last_mask = keymask;
  } 
  else
#endif  

#ifdef ENABLE_PACMAN
#ifndef SINGLE_MACHINE
  if(machine == MCH_PACMAN) 
#endif  
  {
    for(int i=0;i<INST_PER_FRAME;i++) {
      StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
    }
      
    if(irq_enable[0])
      IntZ80(cpu, irq_ptr);     
  }
#ifndef SINGLE_MACHINE
  else
#endif
#endif  

#ifdef ENABLE_GALAGA
#ifdef ENABLE_DKONG
  if(machine == MCH_GALAGA) 
#endif
  {
    for(int i=0;i<INST_PER_FRAME;i++) {
      current_cpu = 0;
      StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
      if(!sub_cpu_reset) {
        current_cpu = 1;
        StepZ80(cpu+1); StepZ80(cpu+1); StepZ80(cpu+1); StepZ80(cpu+1);       
        current_cpu = 2;
        StepZ80(cpu+2); StepZ80(cpu+2); StepZ80(cpu+2); StepZ80(cpu+2);       
      }

      if(namco_busy) namco_busy--;

      // nmi counter for cpu0
      if((cs_ctrl & 0xe0) != 0) {
        // ctr_ctrl[7:5] * 64 * 330ns (hl)->(de)
        if(nmi_cnt < (cs_ctrl >> 5) * 64) {
          nmi_cnt++;
        } else {
          IntZ80(&cpu[0], INT_NMI);
          nmi_cnt = 0;          
        }
      } 

      // run cpu2 nmi at ~line 64 and line 192
      if(!sub_cpu_reset && !irq_enable[2] &&
         ((i == INST_PER_FRAME/4) || (i == 3*INST_PER_FRAME/4)))
          IntZ80(&cpu[2], INT_NMI);
    }
    
    if(irq_enable[0])                   IntZ80(&cpu[0], INT_RST38);
    if(!sub_cpu_reset && irq_enable[1]) IntZ80(&cpu[1], INT_RST38);
  }
#ifdef ENABLE_DKONG
  else /* if(machine == MCH_DKONG) */ 
#endif
#endif  

#ifdef ENABLE_DKONG
  {
    game_started = 1; // TODO: make this from some graphic thing
    
    // dkong      
    for(int i=0;i<INST_PER_FRAME;i++) {
      StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
    }
      
    if(irq_enable[0])
      IntZ80(cpu, INT_NMI);
  }
#endif

  sof = micros() - sof;

  // it may happen that the emulation runs too slow. It will then miss the
  // vblank notification and in turn will miss a frame and significantly
  // slow down. This risk is only given with Galaga as the emulation of
  // all three CPUs takes nearly 13ms. The 60hz vblank rate is in turn 
  // 16.6 ms.

  // wait for signal from video task to emulate a 60Hz frame rate
  if(game_started
  #ifndef SINGLE_MACHINE
    || (machine == MCH_MENU)
  #endif
    )
    ulTaskNotifyTake(1, 0xffffffffUL);
  else
    vTaskDelay(1);

//  printf("us per frame: %d\n", sof);
}
