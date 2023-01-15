#include <stdio.h>

#include "Z80.h"
#include "rom.h"
#include "emulation.h"
#include "config.h"

extern void StepZ80(Z80 *R);
Z80 cpu[3];

unsigned char *memory;

#ifdef FREEPLAY
unsigned char credit = 100;
#else
unsigned char credit = 0;
#endif

char game_started = 0;

char current_cpu = 0;
char sub_cpu_reset = 1;
char irq_enable[3] = { 0,0,0 };
char credit_mode = 0;

int namco_cnt = 0;
int namco_busy = 0;
unsigned char cs_ctrl = 0;  // xxx----- != 000 -> nmi counter
int nmi_cnt = 0;
int coincredMode = 0;

unsigned char starcontrol = 0;

// mirror of sounds registers to monitor for changes
unsigned char soundregs[32] = {
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void OutZ80(unsigned short Port, unsigned char Value) {}
unsigned char InZ80(unsigned short Port) { return 0; }
void PatchZ80(Z80 *R) {}

// Memory write -- write the Value to memory location Addr
void WrZ80(unsigned short Addr, unsigned char Value) {
  if(Addr < 16384) return;   // ignore rom writes

  // namco 06xx
  if((Addr & 0xf800) == 0x7000) {
    if(Addr & 0x100) {  // 7100
      // see task_man.s L450
      namco_cnt = 0;
      cs_ctrl = Value;
      namco_busy = 5000;   // this delay is important for proper startup 
      
      if(Value == 0xa8) snd_trigger_explosion();   // trigger explosion sound
      
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

unsigned char RdZ80(unsigned short Addr) {
  if(Addr < 16384) {
    if(current_cpu == 1)       return rom_cpu2[Addr];
    else if(current_cpu == 2)  return rom_cpu3[Addr];
    return rom_cpu1[Addr];
  }

  // latch
  if((Addr & 0xfff8) == 0x6800) {
    // http://www.arcaderestoration.com/gamedips/3291/All/Galaga.aspx
#ifdef FREEPLAY
    unsigned char dip_a = (0b11110110 & (0x80 >> (Addr&7))) ? 0:1;
#else
    unsigned char dip_a = (0b00010110 & (0x80 >> (Addr&7))) ? 0:1;
#endif
    unsigned char dip_b = (0b00010000 & (0x80 >> (Addr&7))) ? 0:2;
    
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
	        if(keymask & KEYCODE_LEFT)  mapb1[1] &= ~0x08;
	        if(keymask & KEYCODE_UP)    mapb1[1] &= ~0x04;
	        if(keymask & KEYCODE_RIGHT) mapb1[1] &= ~0x02;
	        if(keymask & KEYCODE_DOWN)  mapb1[1] &= ~0x01;

	        // report fire only when it was pressed
	        if((keymask & KEYCODE_FIRE) && !(prev_mask & KEYCODE_FIRE)) {
	          mapb1[1] &= ~0x10;
	          fire_timer = 1;         // 0 is too short for score enter, 5 is too long
      	    // should probably be done via a global counter
	        } else if(fire_timer) {
	          mapb1[1] &= ~0x10;
	          fire_timer--;
      	  }

    	    // 51xx leaves credit mode when user presses start? Nope ...
	        if((keymask & KEYCODE_1) && !(prev_mask & KEYCODE_1) && credit)
	          credit -= 1;

	        if((keymask & KEYCODE_3) && !(prev_mask & KEYCODE_3) && (credit < 99))
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
  unsigned long t = micros();

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

  t = micros() - t;

  // printf("us per frame: %d\n", t);

  // delay the time required to emulate a 60Hz frame rate
  if(game_started)
    vTaskDelay((1000000/60 - t)/1000);
}
