/*
 * emulation.c
 *
 */

/*
 * TODO: 
 * Don't use reset to return to menu in order to suppress noise when returning from master attract game
 * 
 */

#include <stdio.h>    // for printf
#include <string.h>   // for memcpy

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
#include "dkong_rom1.h"
#include "dkong_rom2.h"
#include "i8048.h"
#endif

#ifdef ENABLE_FROGGER
#include "frogger_rom1.h"
#include "frogger_rom2.h"
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

// the audio cpu is a mb8884 which in turn is 8048/49 compatible
struct i8048_state_S cpu_8048;

// sound effects register between 8048 and z80
unsigned char dkong_sfx_index = 0x00;
#endif

#ifdef ENABLE_FROGGER
unsigned char frogger_snd_irq_state = 0;
unsigned char frogger_snd_command;
unsigned char frogger_snd_ay_port;
unsigned long frogger_snd_icnt;
#endif

#ifndef SINGLE_MACHINE

// if galaga is installed, then use it as default in menu
#ifdef ENABLE_GALAGA
signed char menu_sel = MCH_GALAGA;
#else
// otherwise just use the first game
signed char menu_sel = 1;
#endif

#ifdef MASTER_ATTRACT_MENU_TIMEOUT
// menu timeout for master attract mode which randomly start games
unsigned long master_attract_timeout = 0;
#endif

#endif

#if defined(ENABLE_PACMAN) || defined(ENABLE_GALAGA) || defined(ENABLE_FROGGER)
// mirror of sounds registers
unsigned char soundregs[32] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
#endif

void OutZ80(unsigned short Port, unsigned char Value) {
#ifdef ENABLE_PACMAN
PACMAN_BEGIN
    irq_ptr = Value;
PACMAN_END
#endif

#ifdef ENABLE_FROGGER
 if(MACHINE_IS_FROGGER && (current_cpu == 1)) {
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
#endif
}
  
unsigned char InZ80(unsigned short Port) {
#ifdef ENABLE_FROGGER  
  static const unsigned char frogger_timer[20] = {
    0x00, 0x10, 0x00, 0x10, 0x08, 0x18, 0x08, 0x18, 0x40, 0x50,
    0x80, 0x90, 0x80, 0x90, 0x88, 0x98, 0x88, 0x98, 0xc0, 0xd0
  };

 if(MACHINE_IS_FROGGER && (current_cpu == 1)) {
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
#endif

 return 0; 
}

void PatchZ80(Z80 *R) {}

// Memory write -- write the Value to memory location Addr
void WrZ80(unsigned short Addr, unsigned char Value) {
#ifdef ENABLE_PACMAN
PACMAN_BEGIN
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
      // writing 85 (U, first char of UP) to the top left corner
      // is an indication that the game has booted up      
      if(Addr == 0x4000 + 985 && Value == 85)
        game_started = 1;
      
      memory[Addr - 0x4000] = Value;
      return;
    }

    // initial stack is at 0xf000 (0x6000 with a15 ignored), so catch that
    if((Addr & 0xfffe) == 0x6ffe)
      return;
  } 
PACMAN_END
#endif

#ifdef ENABLE_GALAGA
GALAGA_BEGIN
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
        // printf("OUT OF RESET %d\n", !Value);
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
GALAGA_END
#endif  

#ifdef ENABLE_DKONG
DKONG_BEGIN
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
DKONG_END
#endif  

#ifdef ENABLE_FROGGER
FROGGER_BEGIN
  {
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
FROGGER_END
#endif
}

unsigned char RdZ80(unsigned short Addr) {
#ifdef ENABLE_PACMAN
PACMAN_BEGIN
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
PACMAN_END
#endif
  
#ifdef ENABLE_GALAGA
GALAGA_BEGIN
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
GALAGA_END
#endif

#ifdef ENABLE_DKONG
DKONG_BEGIN
  {
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
  }
DKONG_END
#endif

#ifdef ENABLE_FROGGER
FROGGER_BEGIN
  {
    // frogger main cpu
    if(current_cpu == 0) {
    
      if(Addr < 16384) {
        // printf("#%d@%04x ROM read %04x\n", current_cpu, cpu[current_cpu].PC.W, Addr);
        return frogger_rom_cpu1[Addr];
      }

      // frogger reads from $4000 and if that returns $55 it jumps t0 $4001
      if(Addr == 0x4000) return 0x00;
      
      // 0x8000 - 0x87ff - main RAM
      if((Addr & 0xf800) == 0x8000) return memory[Addr - 0x8000];
      
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
  }
FROGGER_END
#endif
  
  return -1;
}

#ifndef SINGLE_MACHINE
extern void hw_reset(void);

// return to main menu
void emulation_reset(void) {
#if 1
  hw_reset();
#else
  printf("EMULATION RESET!!!\n");

  // return to menu
  machine = MCH_MENU;

  // reset all CPUs
  for(current_cpu=0;current_cpu<3;current_cpu++)
    ResetZ80(&cpu[current_cpu]);

#ifdef ENABLE_DKONG
  i8048_reset(&cpu_8048);
#endif      
#endif
}
#endif

#ifdef ENABLE_DKONG
static unsigned char dkong_audio_assembly_buffer[64];
unsigned char dkong_audio_transfer_buffer[DKONG_AUDIO_QUEUE_LEN][64];
unsigned char dkong_audio_rptr = 0, dkong_audio_wptr = 0;

void i8048_port_write(struct i8048_state_S *state, unsigned char port, unsigned char pos) {
  if(port == 0)
    return;
  
  else if(port == 1) {
    static int bptr = 0;
    
    dkong_audio_assembly_buffer[bptr++] = pos;
    
    // buffer now full?
    if(bptr == 64) {
      bptr = 0;

      // It must never happen that we get here with no free transfer buffers
      // available. This would mean that the buffers were full and the
      // 8048 emulation was still running. It should be stoppped as long as the
      // buffers are full.
      if(((dkong_audio_wptr + 1)&DKONG_AUDIO_QUEUE_MASK) == dkong_audio_rptr) {
        printf("DK audio overflow\n");
      } else {
        // copy data into transfer buffer
        memcpy(dkong_audio_transfer_buffer[dkong_audio_wptr], dkong_audio_assembly_buffer, 64);
        dkong_audio_wptr = (dkong_audio_wptr + 1)&DKONG_AUDIO_QUEUE_MASK;
      }
    }
  } else if(port == 2)
    state->p2_state = pos;
}

unsigned char i8048_port_read(struct i8048_state_S *state, unsigned char port) {
  if(port == 2) return state->p2_state;
  return 0;
}

void i8048_xdm_write(struct i8048_state_S *state, unsigned char addr, unsigned char data) { }

unsigned char i8048_xdm_read(struct i8048_state_S *state, unsigned char addr) {
  // printf("@%04x EXT RD %02x\n", state->PC-1, addr);

  // inverted Z80 MUS register
  if(state->p2_state & 0x40) {
    // printf("REQ MUS = %d\n", dkong_sfx_index);
    return dkong_sfx_index ^ 0x0f;
  }
  
  return dkong_rom_cpu2[2048 + addr + 256 * (state->p2_state & 7)];
}

// this is inlined in emulation.h
//unsigned char i8048_rom_read(struct i8048_state_S *state, unsigned short addr) {
//  return dkong_rom_cpu2[addr];
//}
#endif

void prepare_emulation(void) {
  memory = malloc(8192);
  memset(memory, 0, 8192);

#if defined(MASTER_ATTRACT_MENU_TIMEOUT) && !defined(SINGLE_MACHINE)
  master_attract_timeout = millis();
#endif

  for(current_cpu=0;current_cpu<3;current_cpu++)
    ResetZ80(&cpu[current_cpu]);

#ifdef ENABLE_DKONG
  i8048_reset(&cpu_8048);
#endif      
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

    // any key will cancel the master attract mode and
    // the machine stays in the menu forever once the
    // user has touched it
    if(keymask)
      master_attract_timeout = 0;
	    
    if((keymask & BUTTON_UP) &&
	    !(last_mask & BUTTON_UP)) menu_sel--;
    if((keymask & BUTTON_DOWN) &&
	    !(last_mask & BUTTON_DOWN)) menu_sel++;

    if(((keymask & BUTTON_FIRE) &&
  	  !(last_mask & BUTTON_FIRE)) ||
	    ((keymask & BUTTON_START) &&
	    !(last_mask & BUTTON_START))) {
	    machine = menu_sel;

#if defined(ENABLE_DKONG)
      // dkong uses a different sample rate,
      // so reconfigure audio if kong or frog is requested
      audio_dkong_bitrate(machine == MCH_DKONG);
#endif
    }

#ifndef MENU_SCROLL
    if(menu_sel < 1)         menu_sel = 1;
    if(menu_sel > MACHINES)  menu_sel = MACHINES;
#else
    if(menu_sel < 1)         menu_sel = MACHINES;
    if(menu_sel > MACHINES)  menu_sel = 1;
#endif
      
    last_mask = keymask;

#ifdef MASTER_ATTRACT_MENU_TIMEOUT
    // check for master attract timeout
    if(master_attract_timeout && 
      (millis() - master_attract_timeout > MASTER_ATTRACT_MENU_TIMEOUT)) {
      master_attract_timeout = millis();  // new timeout for running game

      // randomly select a machine
      machine = 1 + ((unsigned long)esp_random()) % MACHINES;
      printf("MASTER ATTRACT to machine %d!!!\n", machine);
      
#ifdef ENABLE_DKONG
      // dkong uses a different sample rate, so reconfigure audio if
      // kong is requested
      audio_dkong_bitrate(machine == MCH_DKONG);
#endif
    }
#endif
  } 
  else
#endif  

#ifdef ENABLE_PACMAN
PACMAN_BEGIN
  {
    for(int i=0;i<INST_PER_FRAME;i++) {
      StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
    }
      
    if(irq_enable[0])
      IntZ80(cpu, irq_ptr);     
  }
PACMAN_END
#endif  

#ifdef ENABLE_GALAGA
GALAGA_BEGIN
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
	        current_cpu = 0;
          IntZ80(&cpu[0], INT_NMI);
          nmi_cnt = 0;          
        }
      } 

      // run cpu2 nmi at ~line 64 and line 192
      if(!sub_cpu_reset && !irq_enable[2] &&
         ((i == INST_PER_FRAME/4) || (i == 3*INST_PER_FRAME/4))) {
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
GALAGA_END 
#endif  

#ifdef ENABLE_DKONG
DKONG_BEGIN
  {
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
DKONG_END
#endif

#ifdef ENABLE_FROGGER
FROGGER_BEGIN
  {
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
FROGGER_END
#endif
  
  sof = micros() - sof;

  // It may happen that the emulation runs too slow. It will then miss the
  // vblank notification and in turn will miss a frame and significantly
  // slow down. This risk is only given with Galaga as the emulation of
  // all three CPUs takes nearly 13ms. The 60hz vblank rate is in turn 
  // 16.6 ms.

  // Wait for signal from video task to emulate a 60Hz frame rate. Don't do
  // this unless the game has actually started to speed up the boot process
  // a little bit.
  if(game_started
  #ifndef SINGLE_MACHINE
    || (machine == MCH_MENU)
  #endif
    )   
    ulTaskNotifyTake(1, 0xffffffffUL);
  else
    // give a millisecond delay to make the watchdog happy
    vTaskDelay(1);

  // printf("us/f: %d\n", sof);
  
#ifdef MASTER_ATTRACT_GAME_TIMEOUT
  if((machine != MCH_MENU) && master_attract_timeout && (millis() - master_attract_timeout > MASTER_ATTRACT_GAME_TIMEOUT)) {
    printf("MASTER ATTRACT game timeout, return to menu\n");
    master_attract_timeout = millis();
    emulation_reset();
  }
#endif  
}
