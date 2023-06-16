/* galaga.h */

#ifdef CPU_EMULATION

#include "galaga_rom1.h"
#include "galaga_rom2.h"
#include "galaga_rom3.h"

static inline unsigned char galaga_RdZ80(unsigned short Addr) {
  static const unsigned char *rom[] = { galaga_rom_cpu1, galaga_rom_cpu2, galaga_rom_cpu3 };
  
  if(Addr < 16384)
    return rom[current_cpu][Addr];
  
  /* video/sprite ram */
  if((Addr & 0xe000) == 0x8000)
    return memory[Addr-0x8000];

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

  return 0xff;
}

static inline void galaga_WrZ80(unsigned short Addr, unsigned char Value) {
  if(Addr < 16384) return;   // ignore rom writes

  if((Addr & 0xe000) == 0x8000)
    memory[Addr-0x8000] = Value;
  
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
      
      if(sub_cpu_reset) {
	current_cpu = 1; ResetZ80(&cpu[1]);
	current_cpu = 2; ResetZ80(&cpu[2]);
      }	
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
  
#if 0
  // inject a fixed score to be able to check hiscore routines easily
  if(game_started && (Addr == 0x8000 + 1020) && (memory[1017] == 0) && (memory[1018] == 0x24)) {
    // start with score = 23450
      memory[1020] = 2; memory[1019] = 3; memory[1018] = 4; memory[1017] = 5; 
  }     
#endif
}

static inline void galaga_run_frame(void) {
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

#endif // CPU_EMULATION

#ifdef IO_EMULATION

#ifndef SINGLE_MACHINE
#include "galaga_logo.h"
#endif
#include "galaga_spritemap.h"
#include "galaga_tilemap.h"
#include "galaga_cmap_tiles.h"
#include "galaga_cmap_sprites.h"
#include "galaga_wavetable.h"
#include "galaga_sample_boom.h"
#include "galaga_starseed.h"

#endif // IO_EMULATION
