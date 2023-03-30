#ifndef _EMULATION_H_
#define _EMULATION_H_

#ifdef ENABLE_DKONG
#define DKONG_AUDIO_QUEUE_LEN   16
#define DKONG_AUDIO_QUEUE_MASK (DKONG_AUDIO_QUEUE_LEN-1)

extern unsigned char dkong_audio_transfer_buffer[DKONG_AUDIO_QUEUE_LEN][64];
extern unsigned char dkong_audio_rptr, dkong_audio_wptr;
#endif

// a total of 7 button is needed for
// most games
#define BUTTON_LEFT  0x01
#define BUTTON_RIGHT 0x02
#define BUTTON_UP    0x04
#define BUTTON_DOWN  0x08
#define BUTTON_FIRE  0x10
#define BUTTON_START 0x20
#define BUTTON_COIN  0x40

#ifndef SINGLE_MACHINE
  #define MCH_MENU         0
  #ifdef ENABLE_PACMAN
    #define MCH_PACMAN     1
    #ifdef ENABLE_GALAGA
      #define MCH_GALAGA   2
      #ifdef ENABLE_DKONG
        #define MCH_DKONG  3
        #define MACHINES   3
      #else
        #define MACHINES   2
      #endif
    #else
      #define MCH_DKONG    2
      #define MACHINES     2
    #endif
  #else
    #define MCH_GALAGA     1
    #define MCH_DKONG      2   
    #define MACHINES       2
  #endif
#endif

#if defined(__cplusplus)
extern "C"
{
#endif
void prepare_emulation(void);
void emulate_frame(void);
extern unsigned char *memory;
extern char game_started;
#ifdef ENABLE_GALAGA
extern unsigned char starcontrol;
#endif
#if defined(ENABLE_GALAGA) || defined(ENABLE_PACMAN)
extern unsigned char soundregs[32];
#endif
#ifndef SINGLE_MACHINE
extern signed char machine;
extern signed char menu_sel;
#endif
#ifdef ENABLE_DKONG
extern unsigned char colortable_select;
extern void audio_dkong_bitrate(char);
#endif

#ifndef SINGLE_MACHINE
extern void emulation_reset(void);

#ifdef MASTER_ATTRACT_MENU_TIMEOUT
extern unsigned long master_attract_timeout;
#endif
#endif

// external functions called by emulation
#ifdef ENABLE_GALAGA
extern void galaga_trigger_sound_explosion(void);
#endif
#ifdef ENABLE_DKONG
extern void dkong_trigger_sound(char);   // 0..15 = sfx, 16.. = mus
#endif
extern unsigned char buttons_get(void);
#if defined(__cplusplus)
}
#endif

#endif // _EMULATION_H_
