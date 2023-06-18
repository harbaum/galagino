/* frogger.h */

#ifdef CPU_EMULATION

#include "frogger_rom1.h"
#include "frogger_rom2.h"

static inline unsigned char frogger_RdZ80(unsigned short Addr) {
  // frogger main cpu
  if(current_cpu == 0) {
    
    if(Addr < 16384)
      return frogger_rom_cpu1[Addr];

    // 0x8000 - 0x87ff - main RAM
    if((Addr & 0xf800) == 0x8000) return memory[Addr - 0x8000];
    
    // frogger reads from $4000 and if that returns $55 it jumps t0 $4001
    if(Addr == 0x4000) return 0x00;
    
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
  return 0xff;
}

static inline void frogger_WrZ80(unsigned short Addr, unsigned char Value) {
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

static inline void frogger_OutZ80(unsigned short Port, unsigned char Value) {
  if(current_cpu == 1) {
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
}

static inline unsigned char frogger_InZ80(unsigned short Port) {
  static const unsigned char frogger_timer[20] = {
	  0x00, 0x10, 0x00, 0x10, 0x08, 0x18, 0x08, 0x18, 0x40, 0x50,
	  0x80, 0x90, 0x80, 0x90, 0x88, 0x98, 0x88, 0x98, 0xc0, 0xd0
  };

  if(current_cpu == 1) {
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
  return 0;
}

static inline void frogger_run_frame(void) {
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

#endif // CPU_EMULATION

#ifdef IO_EMULATION

#ifndef SINGLE_MACHINE
#include "frogger_logo.h"
#endif
#include "frogger_tilemap.h"
#include "frogger_spritemap.h"
#include "frogger_cmap.h"

static inline void frogger_prepare_frame(void) {
  active_sprites = 0;
  // frogger supports a total of 8 sprites of 8x8 size
  for(int idx=7;idx>=0 && active_sprites < 92;idx--) {
    // sprites are stored at 0x0c40
    unsigned char *sprite_base_ptr = memory + 0xc40 + 4*idx;
    struct sprite_S spr;     

    if(sprite_base_ptr[3]) {
      spr.x = sprite_base_ptr[0];
      spr.x = (((spr.x << 4) & 0xf0) | ((spr.x >> 4) & 0x0f)) - 16;
      spr.y = sprite_base_ptr[3] + 16;
      spr.color = sprite_base_ptr[2] & 7;
      spr.color = ((spr.color >> 1) & 0x03) | ((spr.color << 2) & 0x04);
      spr.code = sprite_base_ptr[1] & 0x3f;
      spr.flags =  ((sprite_base_ptr[1] & 0x80)?1:0) | ((sprite_base_ptr[1] & 0x80)?2:0);

      if((spr.y > -16) && (spr.y < 288) && (spr.x > -16) && (spr.x < 224))
      	sprite[active_sprites++] = spr;
    }    
  }
}

// draw a single 8x8 tile
static inline void frogger_blit_tile(short row, char col) {
  unsigned short addr = tileaddr[row][col];

  if((row < 2) || (row >= 34))
    return;

  const unsigned short *tile = frogger_606[memory[0x0800 + addr]];

  // frogger has a very reduced color handling
  int c = memory[0xc00 + 2 * (addr & 31) + 1] & 7;
  const unsigned short *colors = frogger_colormap[((c >> 1) & 0x03) | ((c << 2) & 0x04)];

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

// frogger can scroll single lines
void frogger_blit_tile_scroll(short row, signed char col, short scroll) {
  if((row < 2) || (row >= 34))
    return;

  unsigned short addr;
  unsigned short mask = 0xffff;
  int sub = scroll & 0x07;
  if(col >= 0) {
    addr = tileaddr[row][col];

    // one tile (8 pixels) further is an address offset of 32
    addr = (addr + ((scroll & ~7) << 2)) & 1023;

    if((sub != 0) && (col == 27))
      mask = 0xffff >> (2*sub);    
  } else {
    // negative column is a special case for the leftmost
    // tile when it's only partly visible
    addr = tileaddr[row][0];
    addr = (addr + 32 + ((scroll & ~7) << 2)) & 1023;

    mask = 0xffff << (2*(8-sub));
  }
    
  const unsigned char chr = memory[0x0800 + addr];
  const unsigned short *tile = frogger_606[chr];

  // frogger has a very reduced color handling
  int c = memory[0xc00 + 2 * (addr & 31) + 1] & 7;
  const unsigned short *colors =
    frogger_colormap[((c >> 1) & 0x03) | ((c << 2) & 0x04)];

  unsigned short *ptr = frame_buffer + 8*col + sub;

  // 8 pixel rows per tile
  for(char r=0;r<8;r++,ptr+=(224-8)) {
    unsigned short pix = *tile++ & mask;
    // 8 pixel columns per tile
    for(char c=0;c<8;c++,pix>>=2) {      
      if(pix & 3) *ptr = colors[pix&3];
      ptr++;      
    }
  }
}

static inline void frogger_blit_sprite(short row, unsigned char s) {
  const unsigned long *spr = frogger_sprites[sprite[s].flags & 3][sprite[s].code];
  const unsigned short *colors = frogger_colormap[sprite[s].color];
  
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
  if(y_offset < 0) spr -= y_offset;  

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

static inline void frogger_render_row(short row) {
  // don't render lines 0, 1, 34 and 35
  if(row <= 1 || row >= 34) return;

  // get scroll info for this row
  unsigned char scroll = memory[0xc00 + 2 * (row - 2)];
  scroll = ((scroll << 4) & 0xf0) | ((scroll >> 4) & 0x0f);
  
  // in frogger scroll will only affect rows
  // water:  8/ 9, 10/11, 12/13, 14/15, 16/17
  // road:  20/21, 22/23, 24/25, 26/27, 28/29
  
  // render 28 tile columns per row. Handle frogger specific
  // scroll capabilities
  if(scroll == 0) // no scroll in this line?
    for(char col=0;col<28;col++)
      frogger_blit_tile(row, col);
  else {
    // if scroll offset is multiple of 8, then
    // 28 tiles are sufficient, otherwise the first
    // fragment needs to be drawn
    if(scroll & 7) 
      frogger_blit_tile_scroll(row, -1, scroll);
    
    for(char col=0;col<28;col++)
      frogger_blit_tile_scroll(row, col, scroll);
  }
  // render sprites
  for(unsigned char s=0;s<active_sprites;s++) {
    // check if sprite is visible on this row
    if((sprite[s].y < 8*(row+1)) && ((sprite[s].y+16) > 8*row))
      frogger_blit_sprite(row, s);
  }
}

#endif // IO_EMULATION
