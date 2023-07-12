/* 1942.h */

#ifdef CPU_EMULATION
#include "1942_rom1.h"
#include "1942_rom2.h"
#include "1942_rom1_b0.h"
#include "1942_rom1_b1.h"
#include "1942_rom1_b2.h"

unsigned char _1942_bank = 0;
unsigned char _1942_palette = 0;
unsigned short _1942_scroll = 0;
unsigned char _1942_sound_latch = 0;
unsigned char _1942_ay_addr[2];

static inline unsigned char _1942_RdZ80(unsigned short Addr) {
  if(current_cpu == 0) {
    // main CPU    
    if(Addr < 32768) return _1942_rom_cpu1[Addr];

    // CPU1 banked ROM
    if((Addr & 0xc000) == 0x8000) {
      if(_1942_bank == 0)                         return _1942_rom_cpu1_b0[Addr - 0x8000];
      else if((_1942_bank == 1)&&(Addr < 0xb000)) return _1942_rom_cpu1_b1[Addr - 0x8000];
      else if(_1942_bank == 2)	                  return _1942_rom_cpu1_b2[Addr - 0x8000];
    }
    
    // RAM mapping
    // 0000-0fff    4k main CPU work RAM
    // 1000-17ff    2k fgvram
    // 1800-1fff    2k audio CPU work RAM
    // 2000-23ff    1k bgvram
    // 2400-247f   128 spriteram
    // if(Addr == 0xe0a5) return 2;  // invincibility
    
    // 4k main RAM
    if((Addr & 0xf000) == 0xe000) 
      return memory[Addr - 0xe000];
    
    // 128 byte spriteram
    if((Addr & 0xff80) == 0xcc00)
      return memory[Addr - 0xcc00 + 0x2400];
    
    // 2k fgvram
    if((Addr & 0xf800) == 0xd000)
      return memory[Addr - 0xd000 + 0x1000];
    
    // 1k bgvram
    if((Addr & 0xfc00) == 0xd800)
      return memory[Addr - 0xd800 + 0x2000];
    
    // ------------ dip/ctrl io -----------
    
    // COIN1,COIN2,X,SERVICE1,X,START2,START1
    if(Addr == 0xc000) {
      unsigned char keymask = buttons_get();
      unsigned char retval = 0xff;
      static unsigned char last_coin = 0;
      if(keymask & BUTTON_COIN && !last_coin)  retval &= ~0x80;
      if(keymask & BUTTON_START) retval &= ~0x01;
      last_coin = keymask & BUTTON_COIN;
      return retval;
    }
    
    // P1: X,X,B2,B1,U,D,L,R
    if(Addr == 0xc001) {
      unsigned char keymask = buttons_get();
      unsigned char retval = 0xff;
      if(keymask & BUTTON_RIGHT) retval &= ~0x01;
      if(keymask & BUTTON_LEFT)  retval &= ~0x02;
      if(keymask & BUTTON_DOWN)  retval &= ~0x04;
      if(keymask & BUTTON_UP)    retval &= ~0x08;
      if(keymask & BUTTON_FIRE)  retval &= ~0x10;
      if(keymask & BUTTON_START) retval &= ~0x20;
      return retval;
    }
    
    // P2: X,X,B2,B1,U,D,L,R
    if(Addr == 0xc002) return 0xff;
    
    // DIP A
    if(Addr == 0xc003) return ~_1942_DIP_A;
    
    // DIP B
    if(Addr == 0xc004) return ~_1942_DIP_B;
    
  } else {
    // second/audio CPU
    if(Addr < 16384) return _1942_rom_cpu2[Addr];
    
    // 2k audio ram
    if((Addr & 0xf800) == 0x4000)
      return memory[Addr - 0x4000 + 0x1800];
    
    // read sound command from latch from main CPU
    if(Addr == 0x6000)
      return _1942_sound_latch;
  }
  
  return 0xff;
}

static inline void _1942_WrZ80(unsigned short Addr, unsigned char Value) {
  // RAM mapping
  // 0000-0fff    4k main CPU work RAM
  // 1000-17ff    2k fgvram
  // 1800-1fff    2k audio CPU work RAM
  // 2000-23ff    1k bgvram
  // 2400-247f   128 spriteram
  
  if(current_cpu == 0) {
    // 4k main RAM
    if((Addr & 0xf000) == 0xe000) {
      memory[Addr - 0xe000] = Value;
      return;
    }
    
    // 128 byte spriteram
    if((Addr & 0xff80) == 0xcc00) {
      memory[Addr - 0xcc00 + 0x2400] = Value;
      return;
    }
    
    // 2k fgvram
    if((Addr & 0xf800) == 0xd000) {
      // printf("#%d@%04x fgvram write %04x = %02x\n", current_cpu, cpu[current_cpu].PC.W, Addr, Value);
      memory[Addr - 0xd000 + 0x1000] = Value;
      return;
    }
    
    // 1k bgvram
    if((Addr & 0xfc00) == 0xd800) {
      memory[Addr - 0xd800 + 0x2000] = Value;
      return;
    }
    
    if((Addr & 0xfff0) == 0xc800) {
      
      if(Addr == 0xc800) {
	_1942_sound_latch = Value;
	return;
      }
      
      if(Addr == 0xc802) {
	_1942_scroll = Value | (_1942_scroll & 0xff00);
	return;
      }
      
      if(Addr == 0xc803) {
	_1942_scroll = (Value << 8)|(_1942_scroll & 0xff);
	return;
      }
      
      if(Addr == 0xc804) {
	// bit 7: flip screen
	// bit 4: cpu b reset
	// bit 0: coin counter
	sub_cpu_reset = Value & 0x10;
	
	if(sub_cpu_reset) {
	  current_cpu = 1;
	  ResetZ80(&cpu[1]);
	}
	return;
      }
      
      if(Addr == 0xc805) {
	_1942_palette = Value;
	return;
      }
      
      if(Addr == 0xc806) {
	_1942_bank = Value;
	return;
      }
    }
    
  } else {
    // 2k audio ram
    if((Addr & 0xf800) == 0x4000) {
      memory[Addr - 0x4000 + 0x1800] = Value;
      return;
    }
    
    // AY's
    if((Addr & 0x8000) == 0x8000) {
      int ay = (Addr & 0x4000) != 0;
      
      if(Addr & 1) soundregs[16*ay + _1942_ay_addr[ay]] = Value;      
      else   	   _1942_ay_addr[ay] = Value & 15;
      
      return;
    }
  }
} 

//
static inline void _1942_run_frame(void) {
  game_started = 1;
      
  for(char f=0;f<4;f++) {
    for(int i=0;i<INST_PER_FRAME/3;i++) {
      current_cpu = 0; 
      StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]);
      if(!sub_cpu_reset) {
        current_cpu = 1;
        StepZ80(&cpu[1]); StepZ80(&cpu[1]); StepZ80(&cpu[1]);
      }
    }

    // generate interrupts, main CPU two times per frame
    if(f & 1) {
      current_cpu = 0;
      IntZ80(&cpu[0], 0xc7 | ((f&2)?(1<<3):(2<<3)));
    }

    // sound CPU four times per frame
    if(!sub_cpu_reset) {
      // the audio CPU updates the AY registers exactly one time
      // during each interrupt.
      current_cpu = 1;
      IntZ80(&cpu[1], INT_RST38);
    }
  }
}
#endif // CPU_EMULATION

#ifdef IO_EMULATION

#ifndef SINGLE_MACHINE
#include "1942_logo.h"
#endif
#include "1942_character_cmap.h"
#include "1942_spritemap.h"
#include "1942_tilemap.h"
#include "1942_charmap.h"
#include "1942_sprite_cmap.h"
#include "1942_tile_cmap.h"

extern unsigned char _1942_palette;
extern unsigned short _1942_scroll;

static inline void _1942_prepare_frame(void) {
  // Do all the preparations to render a screen.
  
  /* preprocess sprites */
  active_sprites = 0;
  for(int idx=0;idx<32 && active_sprites<124;idx++) {
    struct sprite_S spr;         
    unsigned char *sprite_base_ptr = memory + 0x2400 + 4*(31-idx);
   
    if(sprite_base_ptr[3] && sprite_base_ptr[2]) {
      // unlike all other machine, 1942 has 512 sprites
      spr.code = (sprite_base_ptr[0] & 0x7f) +
        4*(sprite_base_ptr[1] & 0x20);

      spr.color = sprite_base_ptr[1] & 0x0f;
      spr.x = sprite_base_ptr[2] - 16;
      spr.y = 256 - (sprite_base_ptr[3] - 0x10 * (sprite_base_ptr[1] & 0x10));

      // store 9th code bit in flags
      spr.flags = (sprite_base_ptr[0] & 0x80)>>7;

      // 16 .. 23 only on left screen
      if((idx >= 8) && (idx < 16))
        spr.flags |= 0x80;   // flag for "left screen half only"
  
      if((idx >= 0) && (idx < 8)) 
        spr.flags |= 0x40;   // flag for "right screen half only"
  
      // attr & 0xc0: double and quad height
      if((spr.y > -16) && (spr.y < 288) &&
         (spr.x > -16) && (spr.x < 224)) {      
      
        // save sprite in list of active sprites
        sprite[active_sprites++] = spr;

        // handle double and quad sprites
        if(sprite_base_ptr[1] & 0xc0) {
          spr.x += 16;
          spr.code += 1;
          if(spr.x < 224) {
            sprite[active_sprites++] = spr;
            if(sprite_base_ptr[1] & 0x80) {
              spr.x += 16;
              spr.code += 1;
              if(spr.x < 224) {
                sprite[active_sprites++] = spr;
                spr.x += 16;
                spr.code += 1;
                if(spr.x < 224)
                  sprite[active_sprites++] = spr;
              }
            }
          }
        }
      }
    }
  }
}

void blit_bgtile_row_1942(short row) {
  row += 32-2;  // adjust for top two unused rows and scrolling to zero

  // calculate first pixel line to be displayed from row and
  // scroll register
  int line = (-8*row + _1942_scroll) & 511;
  char yoffset = (16-line) & 15;
  
  // if yoffset > 8, then the data is not sufficient for a full
  // row. The remaining bits have to be taken from the next tile row
  int lines2draw = (yoffset > 8)?(16-yoffset):8;
  
  // determine memory address of first tile to draw
  unsigned short addr = 0x2000 + 32*(((line-1)&511)/16) + 1;

  // tiles are 16x16, thus render 14 tiles
  for(char col=0;col<14;col++) {   
    unsigned short *ptr = frame_buffer + 16*col;
    
    unsigned char attr = memory[addr + col + 16];
    const unsigned short *colors = _1942_colormap_tiles[_1942_palette][attr&31];
    unsigned short chr = memory[addr + col] + ((attr&0x80)<<1);
    const unsigned long *tile = sr_08_a1[(attr>>5)&3][chr] + 2*yoffset;

    // draw up to 8 pixel rows
    char r;
    for(r=0;r<lines2draw;r++,ptr+=(224-16)) {
      unsigned long pix = *tile++;
      for(char c=0;c<8;c++,pix>>=4) *ptr++ = colors[pix&7];
      pix = *tile++;
      for(char c=0;c<8;c++,pix>>=4) *ptr++ = colors[pix&7];
    }

    // more lines from next tile to draw?
    if(r != 8) {
      int next_line = (line-16)&511;
      unsigned short next_addr = 0x2000 + 32*(((line-17)&511)/16) + 1;
      attr = memory[next_addr + col + 16];
      colors = _1942_colormap_tiles[_1942_palette][attr&31];
      chr = memory[next_addr + col] + ((attr&0x80)<<1);    
      tile = sr_08_a1[(attr>>5)&3][chr];
      
      for(;r<8;r++,ptr+=(224-16)) {
	      unsigned long pix = *tile++;
	      for(char c=0;c<8;c++,pix>>=4) *ptr++ = colors[pix&7];
	      pix = *tile++;
      	for(char c=0;c<8;c++,pix>>=4) *ptr++ = colors[pix&7];
      }
    }
  }
}
  
void blit_fgchar_1942(short row, char col) {
  // screen is rotated
  unsigned short addr = tileaddr[35-row][27-col];    
  unsigned short chr = memory[0x1000 + addr];
  if(chr == 0x30) return; // empty space
  
  unsigned char attr = memory[0x1400 + addr];
  if(attr & 0x80) chr += 256;

  const unsigned short *tile = sr_02_f2[chr];

  // colorprom sb-0.f1 contains 64 color groups
  const unsigned short *colors = _1942_colormap_chars[attr&63];

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

static inline void lsl64(unsigned long *mask, int pix) {
  if(pix < 8)
    mask[0] <<= 4*pix;
  else {
    mask[0] = 0;
    mask[1] <<= 4*(pix-8);
  }
}
  
static inline void lsr64(unsigned long *mask, int pix) {
  if(pix < 8)
    mask[1] >>= 4*pix;
  else {
    mask[1] = 0;
    mask[0] >>= 4*(pix-8);
  }
}

void blit_sprite_1942(short row, unsigned char s) {
  const unsigned long *spr = _1942_sprites[sprite[s].code + 256*(sprite[s].flags&1)];
  const unsigned short *colors = _1942_colormap_sprites[4 * (sprite[s].color & 15)];

  // create mask for sprites that clip left or right
  // avoid 64 arithmetic
  unsigned long mask[2] = { 0xffffffff, 0xffffffff };
  if(sprite[s].x < 0)      lsl64(mask, -sprite[s].x);
  if(sprite[s].x > 224-16) lsr64(mask,  sprite[s].x-(224-16));

  // set mask for "left screen half only sprites"
  if(sprite[s].flags & 0x80) {
    if(sprite[s].x >= 113)    return;
    if(sprite[s].x > 113-16)  lsr64(mask, sprite[s].x-(113-16));
  }

  // set mask for "right screen half only sprites"
  if(sprite[s].flags & 0x40) {
    if(sprite[s].x <= 113-16) return;
    if(sprite[s].x < 113)     lsl64(mask, 113-sprite[s].x);
  }

  mask[0] = ~mask[0]; mask[1] = ~mask[1];
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
  if(y_offset < 0) spr -= 2*y_offset;  

  // calculate pixel lines to paint  
  unsigned short *ptr = frame_buffer + sprite[s].x + 224*startline;
  
  // 16 pixel rows per sprite
  for(char r=0;r<lines2draw;r++,ptr+=(224-16)) {
    // 16 pixel columns per tile

    unsigned long pix = *spr++ | mask[0];
    for(char c=0;c<8;c++,pix>>=4) {
      if((pix & 15) != 15) *ptr = colors[pix&15];
      ptr++;
    }
    
    pix = *spr++ | mask[1];
    for(char c=0;c<8;c++,pix>>=4) {
      if((pix & 15) != 15) *ptr = colors[pix&15];
      ptr++;
    }
  }
}

static inline void _1942_render_row(short row) {
  if((row < 2) || (row >= 34)) return;
  
  // render one row of background tiles
  blit_bgtile_row_1942(row);
  
  // render sprites
  for(unsigned char s=0;s<active_sprites;s++) {
    // check if sprite is visible on this row
    if((sprite[s].y < 8*(row+1)) && ((sprite[s].y+16) > 8*row))
      blit_sprite_1942(row, s);
  }
  
  // render 28 tile character columns per row
  for(char col=0;col<28;col++)
    blit_fgchar_1942(row, col);
}

#endif // IO_EMULATION
