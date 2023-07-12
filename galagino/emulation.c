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

#define CPU_EMULATION
#include "emulation.h"

#ifdef ENABLE_DKONG
#include "i8048.h"
#include "dkong_rom2.h"
#endif

extern void StepZ80(Z80 *R);
extern void digdug_StepZ80(Z80 *R);
Z80 cpu[3];    // up to three z80 supported

unsigned char *memory;

char game_started = 0;
char current_cpu = 0;
char irq_enable[3] = { 0,0,0 };

#if defined(ENABLE_GALAGA) || defined(ENABLE_DIGDUG) || defined(ENABLE_1942)
char sub_cpu_reset = 1;
#endif

#ifdef ENABLE_GALAGA
// special variables for galaga
unsigned char credit = 0;
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

#ifdef ENABLE_DIGDUG
unsigned char namco_command = 0;
unsigned char namco_mode = 0;
unsigned char namco_nmi_counter = 0;
unsigned char namco_credit = 0x00;
extern unsigned char digdug_video_latch;
#define NAMCO_NMI_DELAY  30  // 10 results in errors
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

#if defined(ENABLE_PACMAN) || defined(ENABLE_GALAGA) || defined(ENABLE_FROGGER) || defined(ENABLE_DIGDUG) || defined(ENABLE_1942)
// mirror of sounds registers
unsigned char soundregs[32] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
#endif

void PatchZ80(Z80 *R) {}

#ifdef ENABLE_DIGDUG
void namco_write_dd(unsigned short Addr, unsigned char Value) {      
  if(Addr & 0x100) {
    namco_command = Value;    
    if(Value == 0xa1) namco_mode = 1;
    if((Value == 0xc1) || (Value == 0xe1)) namco_mode = 0;

    // any command other than 0x10 triggers an NMI to the main CPU after ~50us
    if(Value != 0x10) namco_nmi_counter = NAMCO_NMI_DELAY;
    else              namco_nmi_counter = 0;
  }
}

unsigned char namco_read_dd(unsigned short Addr) {
  if(Addr & 0x100)
    return namco_command;
  else {
    unsigned char keymask = buttons_get();

    static unsigned char keymask_d[] = { 0x00, 0x00, 0x00};	
    keymask_d[2] = keymask_d[1];
    keymask_d[1] = keymask_d[0];
    keymask_d[0] = keymask;

    // rising edge, prolonged for two calls
    unsigned char keymask_p = (keymask ^ keymask_d[2]) & keymask;
    
    if((keymask_d[0] & BUTTON_COIN) && !(keymask_d[1] & BUTTON_COIN) && (namco_credit < 99))
      namco_credit++;

    // this decrease of credit actually triggers the game start
    if((keymask_d[0] & BUTTON_START) && !(keymask_d[1] & BUTTON_START) && (namco_credit > 0))
      namco_credit--;
    
    if(namco_command == 0x71) {
      if(namco_mode) {
      	if((Addr & 15) == 0) {
	        unsigned char retval = 0x00;
	        if(keymask & BUTTON_COIN)   retval |= 0x01;
	        if(keymask & BUTTON_START)  retval |= 0x10;
	        return ~retval;
	      } else if((Addr & 15) == 1) {
	        unsigned char retval = 0x00;
	        if(keymask & BUTTON_UP)        retval |= 0x01;
	        if(keymask & BUTTON_DOWN)      retval |= 0x04;
	        if(keymask & BUTTON_LEFT)      retval |= 0x08;
	        if(keymask & BUTTON_RIGHT)     retval |= 0x02;
	        if(keymask_d[1] & BUTTON_FIRE) retval |= 0x20;
	        if(keymask_p & BUTTON_FIRE)    retval |= 0x10;
	        return ~retval;
	      } else if((Addr & 15) == 2) 
	        return ~0b00000000;
      } else {
	      if((Addr & 15) == 0) {
	        return 16*(namco_credit/10) + namco_credit % 10;
	      } else if((Addr & 15) == 1) {
	        unsigned char retval = 0x00;
	        // first player controls
	        if(keymask & BUTTON_FIRE)       retval |= 0x20;
	        if(keymask_p & BUTTON_FIRE)     retval |= 0x10;
          
	        if(keymask & BUTTON_UP)	        retval |= 0x0f;
	        else if(keymask & BUTTON_RIGHT) retval |= 0x0d;
	        else if(keymask & BUTTON_DOWN)  retval |= 0x0b;
	        else if(keymask & BUTTON_LEFT)  retval |= 0x09;
	        else	                          retval |= 0x07;   // nothing
	  
      	  return ~retval;
	      } else if((Addr & 15) == 2) 
	        return 0xf8;  // f8 -> no second player control
      }
      
    } else if(namco_command == 0xd2) {
      
      if((Addr & 15) == 0)
      	// DSW0
	      return ~DIGDUG_DIP_A;
      else if((Addr & 15) == 1)
	      // DSW1
        return (unsigned char)~DIGDUG_DIP_B;
      
    } else if(namco_command == 0xb1) {
      // {8{~(ADR<=2)}};
      if((Addr & 15) <= 2) return 0x00;      
      return 0xff;
    } else if(namco_command == 0x08) {
      return 0xff;      
    } else if(namco_command == 0xc1)
      return 0xff;      
  }
  return 0xff;
}
#endif

// one inst at 3Mhz ~ 500k inst/sec = 500000/60 inst per frame
#define INST_PER_FRAME 300000/60/4

#ifdef ENABLE_PACMAN
#include "pacman.h"
#endif

#ifdef ENABLE_GALAGA
#include "galaga.h"
#endif

#ifdef ENABLE_DKONG
#include "dkong.h"
#endif

#ifdef ENABLE_FROGGER
#include "frogger.h"
#endif

#ifdef ENABLE_DIGDUG
#include "digdug.h"
#endif

#ifdef ENABLE_1942
#include "1942.h"
#endif


void OutZ80(unsigned short Port, unsigned char Value) {
#ifdef ENABLE_PACMAN
  if(MACHINE_IS_PACMAN) {
    pacman_OutZ80(Port, Value);
    return;
  }
#endif

#ifdef ENABLE_FROGGER
  if(MACHINE_IS_FROGGER) {
    frogger_OutZ80(Port, Value);
    return;
  }
#endif
}
  
unsigned char InZ80(unsigned short Port) {
#ifdef ENABLE_FROGGER
  if(MACHINE_IS_FROGGER)
    return frogger_InZ80(Port);
#endif

 return 0; 
}

// Memory write -- write the Value to memory location Addr
void WrZ80(unsigned short Addr, unsigned char Value) {
#if 0
  // jump table
  const void (*wrz80[])(unsigned short, unsigned char) = {
     0,
#ifdef ENABLE_PACMAN
     pacman_WrZ80,
#endif
#ifdef ENABLE_GALAGA
     galaga_WrZ80,
#endif
#ifdef ENABLE_DKONG
     dkong_WrZ80,
#endif
#ifdef ENABLE_FROGGER
     frogger_WrZ80,
#endif
#ifdef ENABLE_DIGDUG
     digdug_WrZ80,
#endif
#ifdef ENABLE_1942
     _1942_WrZ80,
#endif   						    
    };

  (*(wrz80[machine]))(Addr, Value);
  
#else
  // digdug is very timing critical and gets a special
  // treatment
#ifdef ENABLE_DIGDUG
  if(MACHINE_IS_DIGDUG) {
    digdug_WrZ80(Addr, Value);
    return;
  } 
#endif

#ifndef SINGLE_MACHINE
  switch(machine) {
#endif

#ifdef ENABLE_PACMAN
#ifndef SINGLE_MACHINE
    case MCH_PACMAN:
#endif
      pacman_WrZ80(Addr, Value);
      return;
#endif
#ifndef SINGLE_MACHINE
      break;
#endif
      
#ifdef ENABLE_GALAGA
#ifndef SINGLE_MACHINE
    case MCH_GALAGA:
#endif
      galaga_WrZ80(Addr, Value);
      return;
#endif
#ifndef SINGLE_MACHINE
      break;
#endif
      
#ifdef ENABLE_DKONG
#ifndef SINGLE_MACHINE
    case MCH_DKONG:
#endif
      dkong_WrZ80(Addr, Value);
      return;
#endif
#ifndef SINGLE_MACHINE
      break;
#endif
      
#ifdef ENABLE_FROGGER
#ifndef SINGLE_MACHINE
    case MCH_FROGGER:
#endif
      frogger_WrZ80(Addr, Value);
      return;
#endif
#ifndef SINGLE_MACHINE
      break;
#endif

#ifdef ENABLE_DIGDUG
#ifndef SINGLE_MACHINE
    case MCH_DIGDUG:
#endif
      digdug_WrZ80(Addr, Value);
      return;
#endif
#ifndef SINGLE_MACHINE
      break;
#endif
      
#ifdef ENABLE_1942
#ifndef SINGLE_MACHINE
    case MCH_1942:
#endif
      _1942_WrZ80(Addr, Value);
      return;
#endif
#ifndef SINGLE_MACHINE
      break;
  }
#endif
#endif
}

unsigned char RdZ80(unsigned short Addr) {
#if 0
  // jump table
  const unsigned char (*rdz80[])(unsigned short) = {
     0,
#ifdef ENABLE_PACMAN
     pacman_RdZ80,
#endif
#ifdef ENABLE_GALAGA
     galaga_RdZ80,
#endif
#ifdef ENABLE_DKONG
     dkong_RdZ80,
#endif
#ifdef ENABLE_FROGGER
     frogger_RdZ80,
#endif
#ifdef ENABLE_DIGDUG
     digdug_RdZ80,
#endif
#ifdef ENABLE_1942
     _1942_RdZ80,
#endif                   
    };
  return (*(rdz80[machine]))(Addr);
  
#else

  // fast treatment for Digdug as it's quite
  // performance sensitive
#ifdef ENABLE_DIGDUG
  if(MACHINE_IS_DIGDUG) return digdug_RdZ80(Addr);
#endif

#ifndef SINGLE_MACHINE
  switch(machine) {
#endif   

#ifdef ENABLE_PACMAN
#ifndef SINGLE_MACHINE
    case MCH_PACMAN:
#endif
      return pacman_RdZ80(Addr);
#endif
#ifndef SINGLE_MACHINE
      break;
#endif
      
#ifdef ENABLE_GALAGA
#ifndef SINGLE_MACHINE
    case MCH_GALAGA:
#endif    
      return galaga_RdZ80(Addr);
#endif
#ifndef SINGLE_MACHINE
      break;
#endif
      
#ifdef ENABLE_DKONG
#ifndef SINGLE_MACHINE
    case MCH_DKONG:
#endif
      return dkong_RdZ80(Addr);
#endif
#ifndef SINGLE_MACHINE
      break;
#endif
      
#ifdef ENABLE_FROGGER
#ifndef SINGLE_MACHINE
    case MCH_FROGGER:
#endif
      return frogger_RdZ80(Addr);
#endif
#ifndef SINGLE_MACHINE
      break;
#endif
      
#ifdef ENABLE_DIGDUG
#ifndef SINGLE_MACHINE
    case MCH_DIGDUG:
#endif
      return digdug_RdZ80(Addr);
#endif
#ifndef SINGLE_MACHINE
      break;
#endif
      
#ifdef ENABLE_1942
#ifndef SINGLE_MACHINE
    case MCH_1942:
#endif
      return _1942_RdZ80(Addr);
#endif
#ifndef SINGLE_MACHINE
      break;
  }
  return 0xff;
#endif
#endif
}

#ifndef SINGLE_MACHINE
extern void hw_reset(void);

// return to main menu
void emulation_reset(void) {
#if 1
  hw_reset();
#else
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
	// overflow
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
  // inverted Z80 MUS register
  if(state->p2_state & 0x40)
    return dkong_sfx_index ^ 0x0f;
  
  return dkong_rom_cpu2[2048 + addr + 256 * (state->p2_state & 7)];
}

// this is inlined in emulation.h
//unsigned char i8048_rom_read(struct i8048_state_S *state, unsigned short addr) {
//  return dkong_rom_cpu2[addr];
//}
#endif


#ifdef ENABLE_1942
#define RAMSIZE   (8192 + 1024 + 128)
#else
#define RAMSIZE   (8192)
#endif

void prepare_emulation(void) {
  memory = malloc(RAMSIZE);
  memset(memory, 0, RAMSIZE);

#if defined(MASTER_ATTRACT_MENU_TIMEOUT) && !defined(SINGLE_MACHINE)
  master_attract_timeout = millis();
#endif

  // reset all three z80's although we might not use them all
  for(current_cpu=0;current_cpu<3;current_cpu++)
    ResetZ80(&cpu[current_cpu]);

#ifdef ENABLE_DKONG
  i8048_reset(&cpu_8048);
#endif       
}

void emulate_frame(void) {
  unsigned long sof = micros();
  current_cpu = 0;

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
      // so reconfigure audio if dkong is requested
      audio_dkong_bitrate(machine == MCH_DKONG);
#endif
    }

    if(MACHINES <= 3) {
      if(menu_sel < 1)         menu_sel = 1;
      if(menu_sel > MACHINES)  menu_sel = MACHINES;
    } else {
      if(menu_sel < 1)         menu_sel = MACHINES;
      if(menu_sel > MACHINES)  menu_sel = 1;
    }
      
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
  pacman_run_frame();
PACMAN_END
#endif  

#ifdef ENABLE_GALAGA
GALAGA_BEGIN
  galaga_run_frame();
GALAGA_END 
#endif  

#ifdef ENABLE_DKONG
DKONG_BEGIN
  dkong_run_frame();
DKONG_END
#endif

#ifdef ENABLE_FROGGER
FROGGER_BEGIN
  frogger_run_frame();
FROGGER_END
#endif
  
#ifdef ENABLE_DIGDUG
DIGDUG_BEGIN
  digdug_run_frame();
DIGDUG_END 
#endif  

#ifdef ENABLE_1942
_1942_BEGIN
  _1942_run_frame();
_1942_END 
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

#ifdef MASTER_ATTRACT_GAME_TIMEOUT
  if((machine != MCH_MENU) && master_attract_timeout && (millis() - master_attract_timeout > MASTER_ATTRACT_GAME_TIMEOUT)) {
    printf("MASTER ATTRACT game timeout, return to menu\n");
    master_attract_timeout = millis();
    emulation_reset();
  }
#endif  
}
