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

static inline void dkong_prepare_frame(void) {
  active_sprites = 0;
  for(int idx=0;idx<96 && active_sprites<92;idx++) {
    // sprites are stored at 0x7000
    unsigned char *sprite_base_ptr = memory + 0x1000 + 4*idx;
    struct sprite_S spr;     
    
    // adjust sprite position on screen for upright screen
    spr.x = sprite_base_ptr[0] - 23;
    spr.y = sprite_base_ptr[3] + 8;
    
    spr.code = sprite_base_ptr[1] & 0x7f;
    spr.color = sprite_base_ptr[2] & 0x0f;
    spr.flags =  ((sprite_base_ptr[2] & 0x80)?1:0) |
      ((sprite_base_ptr[1] & 0x80)?2:0);

    // save sprite in list of active sprites
    if((spr.y > -16) && (spr.y < 288) &&
       (spr.x > -16) && (spr.x < 224))
      sprite[active_sprites++] = spr;
  }
}

// draw a single 8x8 tile
static inline void dkong_blit_tile(short row, char col) {
  unsigned short addr = tileaddr[row][col];

  if((row < 2) || (row >= 34)) return;    
  // skip blank dkong tiles (0x10) in rendering  
  if(memory[0x1400 + addr] == 0x10) return;   
  const unsigned short *tile = v_5h_b_bin[memory[0x1400 + addr]];
  // donkey kong has some sort of global color table
  const unsigned short *colors = dkong_colormap[colortable_select][row-2 + 32*(col/4)];

  unsigned short *ptr = frame_buffer + 8*col;

  // 8 pixel rows per tile
  for(char r=0;r<8;r++,ptr+=(224-8)) {
    unsigned short pix = *tile++;
    // 8 pixel columns per tile
    for(char c=0;c<8;c++,pix>>=2) {
      if(pix & 3) *ptr = colors[pix&3];
      ptr++;
    }
  }
}

// dkong has its own sprite drawing routine since unlike the other
// games, in dkong black is not always transparent. Black pixels
// are instead used for masking
static inline void dkong_blit_sprite(short row, unsigned char s) {
  const unsigned long *spr = dkong_sprites[sprite[s].flags & 3][sprite[s].code];
  const unsigned short *colors = dkong_colormap_sprite[colortable_select][sprite[s].color];
  
  // create mask for sprites that clip left or right
  unsigned long mask = 0xffffffff;
  if(sprite[s].x < 0)      mask <<= -2*sprite[s].x;
  if(sprite[s].x > 224-16) mask >>= 2*(sprite[s].x-224-16);    

  short y_offset = sprite[s].y - 8*row;

  // check if there are less than 8 lines to be drawn in this row
  unsigned char lines2draw = 8;
  if(y_offset < -8) lines2draw = 16+y_offset;

  // check which sprite line to begin with
  unsigned short startline = 0;
  if(y_offset > 0) {
    startline = y_offset;
    lines2draw = 8 - y_offset;
  }

  // if we are not starting to draw with the first line, then
  // skip into the sprite image
  if(y_offset < 0)
    spr -= y_offset;  

  // calculate pixel lines to paint  
  unsigned short *ptr = frame_buffer + sprite[s].x + 224*startline;
  
  // 16 pixel rows per sprite
  for(char r=0;r<lines2draw;r++,ptr+=(224-16)) {
    unsigned long pix = *spr++ & mask;
    // 16 pixel columns per tile
    for(char c=0;c<16;c++,pix>>=2) {
      unsigned short col = colors[pix&3];
      if(pix & 3) *ptr = col;
      ptr++;
    }
  }
}

static inline void dkong_render_row(short row) {
  // render 28 tile columns per row
  for(char col=0;col<28;col++)
    dkong_blit_tile(row, col);
  
  // render sprites
  for(unsigned char s=0;s<active_sprites;s++) {
    // check if sprite is visible on this row
    if((sprite[s].y < 8*(row+1)) && ((sprite[s].y+16) > 8*row))
      dkong_blit_sprite(row, s);
  }
}

#endif // IO_EMULATION
