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

#define USE_NAMCO_WAVETABLE

static inline void pacman_prepare_frame(void) {
  // Do all the preparations to render a screen.

  /* preprocess sprites */
  active_sprites = 0;
  for(int idx=0;idx<8 && active_sprites<92;idx++) {
    unsigned char *sprite_base_ptr = memory + 2*(7-idx);
    struct sprite_S spr;     
      
    spr.code = sprite_base_ptr[0x0ff0] >> 2;
    spr.color = sprite_base_ptr[0x0ff1] & 63;
    spr.flags = sprite_base_ptr[0x0ff0] & 3;
    
    // adjust sprite position on screen for upright screen
    spr.x = 255 - 16 - sprite_base_ptr[0x1060];
    spr.y = 16 + 256 - sprite_base_ptr[0x1061];

    if((spr.code < 64) &&
       (spr.y > -16) && (spr.y < 288) &&
       (spr.x > -16) && (spr.x < 224)) {      
      
      // save sprite in list of active sprites
      sprite[active_sprites++] = spr;
    }
  }
}

// draw a single 8x8 tile
static inline void pacman_blit_tile(short row, char col) {
  unsigned short addr = tileaddr[row][col];

  const unsigned short *tile = pacman_5e[memory[addr]];
  const unsigned short *colors = pacman_colormap[memory[0x400 + addr] & 63];

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

static inline void pacman_blit_sprite(short row, unsigned char s) {
  const unsigned long *spr = pacman_sprites[sprite[s].flags & 3][sprite[s].code];
  const unsigned short *colors = pacman_colormap[sprite[s].color & 63];

  // create mask for sprites that clip left or right
  unsigned long mask = 0xffffffff;
  if(sprite[s].x < 0)      mask <<= -2*sprite[s].x;
  if(sprite[s].x > 224-16) mask >>= (2*(sprite[s].x-(224-16)));		

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
      if(col) *ptr = col;
      ptr++;
    }
  }
}

static inline void pacman_render_row(short row) {
  // render 28 tile columns per row
  for(char col=0;col<28;col++)
    pacman_blit_tile(row, col);
  
  // render sprites
  for(unsigned char s=0;s<active_sprites;s++) {
    // check if sprite is visible on this row
    if((sprite[s].y < 8*(row+1)) && ((sprite[s].y+16) > 8*row))
      pacman_blit_sprite(row, s);
  }
}

#endif // IO_EMULATION
