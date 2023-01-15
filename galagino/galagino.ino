/*
 * galagino.ino - Galaga arcade for ESP32 and Arduino IDE
 *
 * 
 *
 */

#include "driver/i2s.h"
#include "video.h"
#include "emulation.h"

// include converted rom data
#include "tilemap.h"
#include "spritemap.h"
#include "colormaps.h"
#include "tileaddr.h"
#include "wavetables.h"

#include "config.h"

Video tft = Video();

// buffer space for one row of 28 characters
unsigned short *frame_buffer;

TaskHandle_t videotask;

// the hardware supports 64 sprites
unsigned char active_sprites = 0;
struct sprite_S {
  unsigned char code, color, flags;
  short x, y; 
} *sprite;

struct star { unsigned char y, x; unsigned short col; };
#include "starseed.h"
unsigned char stars_scroll_y = 0;

// the ship explosion sound is stored as a digi sample.
// All other souunds are generated on the fly via the
// original wave tables
unsigned short snd_boom_cnt = 0;
const signed char *snd_boom_ptr = NULL;

void prepare_frame(void) {
  // Do all the preparations to render a screen.
  
  /* preprocess sprites */
  active_sprites = 0;
  for(int idx=0;idx<64 && active_sprites<92;idx++) {
    unsigned char *sprite_base_ptr = memory + 2*(63-idx);
    // check if sprite is visible
    if ((sprite_base_ptr[0x1b80 + 1] & 2) == 0) {
      struct sprite_S spr;     
      
      spr.code = sprite_base_ptr[0x0b80];
      spr.color = sprite_base_ptr[0x0b80 + 1];
      spr.flags = sprite_base_ptr[0x1b80];
      // adjust sprite position on screen for upright screen
      spr.x = sprite_base_ptr[0x1380] - 16;
      spr.y = sprite_base_ptr[0x1380 + 1] +
	      0x100*(sprite_base_ptr[0x1b80 + 1] & 1) - 40;

      if((spr.code < 128) &&
    	   (spr.y > -16) && (spr.y < 288) &&
	       (spr.x > -16) && (spr.x < 224)) {      

    	  // save sprite in list of active sprites
	      sprite[active_sprites] = spr;
      	// for horizontally doubled sprites, this one becomes the code + 2 part
	      if(spr.flags & 0x08) sprite[active_sprites].code += 2;	
	      active_sprites++;
      }

      // handle horizontally doubled sprites
      if((spr.flags & 0x08) &&
    	   (spr.y > -16) && (spr.y < 288) &&
    	   ((spr.x+16) >= -16) && ((spr.x+16) < 224)) {
	      // place a copy right to the current one
	      sprite[active_sprites] = spr;
	      sprite[active_sprites].x += 16;
	      active_sprites++;
      }

      // handle vertically doubled sprites
      // (these don't seem to happen in galaga)
      if((spr.flags & 0x04) &&
	       ((spr.y+16) > -16) && ((spr.y+16) < 288) && 
	      (spr.x > -16) && (spr.x < 224)) {      
	      // place a copy below the current one
	      sprite[active_sprites] = spr;
	      sprite[active_sprites].code += 3;
	      sprite[active_sprites].y += 16;
	      active_sprites++;
      }
	
      // handle in both directions doubled sprites
      if(((spr.flags & 0x0c) == 0x0c) &&
	       ((spr.y+16) > -16) && ((spr.y+16) < 288) &&
	       ((spr.x+16) > -16) && ((spr.x+16) < 224)) {
	      // place a copy right and below the current one
	      sprite[active_sprites] = spr;
	      sprite[active_sprites].code += 1;
	      sprite[active_sprites].x += 16;
	      sprite[active_sprites].y += 16;
	      active_sprites++;
      }
    }
  }
}

void render_stars_set(short row, const struct star *set) {    
  for(char star_cntr = 0;star_cntr < 63 ;star_cntr++) {
    const struct star *s = set+star_cntr;

    unsigned short x = (244 - s->x) & 0xff;
    unsigned short y = ((s->y + stars_scroll_y) & 0xff) + 16 - row * 8;

    if(y < 8 && x < 224)
      frame_buffer[224*y + x] = s->col;
  }     
}

// draw a single 8x8 tile
void blit_tile(short row, char col) {
  unsigned short addr = tileaddr[row][col];

  // skip blank tiles (0x24) in rendering  
  if(memory[addr] == 0x24) return;
    
  const unsigned short *tile = gg1_9[memory[addr]];
  const unsigned short *colors = prom_4[memory[0x400 + addr] & 63];
    
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

// render a single 16x16 sprite. This is called multiple times for
// double sized sprites. This renders onto a single 224 x 8 tile row
// thus will be called multiple times even for single sized sprites
void blit_sprite(short row, unsigned char s) {
  const unsigned long *spr = gg1_10[sprite[s].flags & 3][sprite[s].code];
  const unsigned short *colors = prom_3[sprite[s].color & 63];

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
      if(pix & 3)
	      *ptr = colors[pix&3];
      ptr++;
    }
  }
}

// render one of 36 tile rows (8 x 224 pixel lines)
void render_line(short row) {
  // clear buffer for black background  
  memset(frame_buffer, 0, 2*224*8);

  if(starcontrol & 0x20) {
    /* two sets of stars controlled by these bits */
    render_stars_set(row, star_set[(starcontrol & 0x08)?1:0]);
    render_stars_set(row, star_set[(starcontrol & 0x10)?3:2]);
  }

  // render sprites
  for(unsigned char s=0;s<active_sprites;s++) {
    // check if sprite is visible on this row
    if((sprite[s].y < 8*(row+1)) && ((sprite[s].y+16) > 8*row))
      blit_sprite(row, s);
  }

  // render 28 tile columns per row
  for(char col=0;col<28;col++)
    blit_tile(row, col);
}

void snd_trigger_explosion(void) {
  snd_boom_cnt = 2*sizeof(boom);
  snd_boom_ptr = boom;
}
  
static unsigned long snd_cnt[3] = { 0,0,0 };
static unsigned long snd_freq[3];
static const signed char *snd_wave[3];
static unsigned char snd_volume[3];
#ifdef SND_DIFF
static unsigned short snd_buffer[128]; // buffer space for two channels
#else
static unsigned short snd_buffer[64];  // buffer space for a single channel
#endif

void snd_render_buffer(void) {
  // render first buffer contents
  for(int i=0;i<64;i++) {
    short v = 0;
    // add up to three wave signals
    if(snd_volume[0]) v += snd_volume[0] * snd_wave[0][(snd_cnt[0]>>14) & 0x1f];
    if(snd_volume[1]) v += snd_volume[1] * snd_wave[1][(snd_cnt[1]>>14) & 0x1f];
    if(snd_volume[2]) v += snd_volume[2] * snd_wave[2][(snd_cnt[2]>>14) & 0x1f];
      
    if(snd_boom_cnt) {
      v += *snd_boom_ptr;
      if(snd_boom_cnt & 1) snd_boom_ptr++;
      snd_boom_cnt--;
    }
    
    // v is now max 3*15*(15-7)+127 = 487 and min 3 * 15 * (0-7) - 128 = -443
    // expand to +/- 15 bit
    v = v*64;

#ifdef SND_DIFF
    // generate differential output
    snd_buffer[2*i]   = 0x8000 + v;    // positive signal on GPIO26
    snd_buffer[2*i+1] = 0x8000 - v;    // negatve signal on GPIO25 
#else
    snd_buffer[i]     = 0x8000 + v;
#endif
      
    snd_cnt[0] += snd_freq[0];
    snd_cnt[1] += snd_freq[1];
    snd_cnt[2] += snd_freq[2];
  }
}

void snd_prepare(void) {
  // parse all three wsg channels
  for(char ch=0;ch<3;ch++) {  
    // channel volume
    snd_volume[ch] = soundregs[ch * 5 + 0x15];
    
    if(snd_volume[ch]) {
      // frequency
      snd_freq[ch] = (ch == 0) ? soundregs[0x10] : 0;
      snd_freq[ch] += soundregs[ch * 5 + 0x11] << 4;
      snd_freq[ch] += soundregs[ch * 5 + 0x12] << 8;
      snd_freq[ch] += soundregs[ch * 5 + 0x13] << 12;
      snd_freq[ch] += soundregs[ch * 5 + 0x14] << 16;

      // wavetable entry
      snd_wave[ch] = wavetables[soundregs[ch * 5 + 0x05] & 0x07];
    }
  }

  // render first buffer contents
  // snd_render_buffer();
}

void snd_transmit() {
  // (try to) transmit as much audio data as possible. Since we
  // write data in exact the size of the DMA buffers we can be sure
  // that either all or nothing is actually being written
  
  size_t bytesOut = 0;
  do {
    // copy data in i2s dma buffer if possible
    i2s_write(I2S_NUM_0, snd_buffer, sizeof(snd_buffer), &bytesOut, 0);

    // render the next audio chunk if data has actually been sent
    if(bytesOut) {
      snd_prepare();
      snd_render_buffer();
    }

    // printf("WR %ld\n", bytesOut);
  } while(bytesOut);
}

void update_screen(void) {
  uint32_t t = micros();
  
  prepare_frame();
  
  // 8*224 pixels = 8*224*16 = 28672 bits
  // 2790 char rows per sec at 40Mhz = max 38 fps
  for(int c=0;c<36;c+=4) {
    render_line(c+0); tft.write(frame_buffer, 224*8);
    render_line(c+1); tft.write(frame_buffer, 224*8);
    render_line(c+2); tft.write(frame_buffer, 224*8);
    render_line(c+3); tft.write(frame_buffer, 224*8);
    
    // audio is refilled 18 times per screen update. The screen is updated
    // every second frame. So audio is refilled every 9 times per frame.
    // Audio registers are udated by CPU3 two times per frame.
    snd_transmit();
  } 

  /* the screen is only updated every second frame, scroll speed is thus doubled */
  static const signed char speeds[8] = { -1, -2, -3, 0, 3, 2, 1, 0 };
  stars_scroll_y += 2*speeds[starcontrol & 7];
  
  // one screen at 60 Hz is 16.6ms
  // -> one screen at 30 Hz is 33ms   
  t = (micros()-t)/1000;  // calculate time in milliseconds
  if(t<33) vTaskDelay(33-t);
  else     vTaskDelay(1);    // at least 1 ms delay to prevent watchdog timeout
  
  // printf("uspf %d\n", t);
}

void video(void *p) {
  tft.begin();

  // init audio
  // 24 kHz @ 16 bit = 48000 bytes/sec = 800 bytes per 60hz game frame =
  // 1600 bytes per 30hz screen update = ~177 bytes every four tile rows  
  static const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
    .sample_rate = 24000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
#ifdef SND_DIFF
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
#else
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
#endif
    .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = 64,   // 64 samples
    .use_apll = false
  };

  snd_prepare();
  snd_render_buffer();
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, NULL);  // use dac on gpio25/26 */
  i2s_start(I2S_NUM_0);

  while(1) update_screen();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Galagino"); 

  // this should not be needed as the CPU runs by default on 240Mht nowadays
  setCpuFrequencyMhz(240000000);

  Serial.print("Free heap: "); Serial.println(ESP.getFreeHeap());
  Serial.print("Main core: "); Serial.println(xPortGetCoreID());
  Serial.print("Main priority: "); Serial.println(uxTaskPriorityGet(NULL));  

  // allocate memory for a single tile/character row
  frame_buffer = (unsigned short*)malloc(224*8*2);
  sprite = (struct sprite_S*)malloc(96 * sizeof(struct sprite_S));
  Serial.print("Free heap: "); Serial.println(ESP.getFreeHeap());

  // make button pins inputs
  pinMode(BTN_START_PIN, INPUT_PULLUP);
  pinMode(BTN_COIN_PIN, INPUT_PULLUP);
  pinMode(BTN_LEFT_PIN, INPUT_PULLUP);
  pinMode(BTN_RIGHT_PIN, INPUT_PULLUP);
  pinMode(BTN_FIRE_PIN, INPUT_PULLUP);

  prepare_emulation();
  // while(!game_started) emulate_frame();

  // let the video processing run on the second core, so the main core
  // can completely focus on emulation
  xTaskCreatePinnedToCore(
      video, /* Function to implement the task */
      "Video task", /* Name of the task */
      4096,  /* Stack size in words */
      NULL,  /* Task input parameter */
      2,  /* Priority of the task */
      &videotask,  /* Task handle. */
      0); /* Core where the task should run */
}

unsigned char buttons_get(void) {
  return 
    (digitalRead(BTN_START_PIN)?0:KEYCODE_1) |
    (digitalRead(BTN_COIN_PIN)?0:KEYCODE_3) |
    (digitalRead(BTN_LEFT_PIN)?0:KEYCODE_LEFT) |
    (digitalRead(BTN_RIGHT_PIN)?0:KEYCODE_RIGHT) |
    (digitalRead(BTN_FIRE_PIN)?0:KEYCODE_FIRE);
}

void loop(void) {
  // the main task runs the emulation of the three main CPUs
  emulate_frame();
}
