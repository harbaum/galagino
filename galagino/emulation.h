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
enum {
      MCH_MENU = 0,
#ifdef ENABLE_PACMAN
      MCH_PACMAN,
#endif
#ifdef ENABLE_GALAGA
      MCH_GALAGA,
#endif
#ifdef ENABLE_DKONG
      MCH_DKONG,
#endif
#ifdef ENABLE_FROGGER
      MCH_FROGGER,
#endif
      MCH_LAST
};

#define MACHINES  (MCH_LAST-1)
#endif

// wrapper around machine specific code sections allowing to compile only
// code in that's needed for enabled machines

#ifdef ENABLE_PACMAN
  #ifdef SINGLE_MACHINE
    #define MACHINE_IS_PACMAN  1
    #define PACMAN_BEGIN
    #define PACMAN_END
  #else
    #define MACHINE_IS_PACMAN  (machine == MCH_PACMAN)
    // pacman is always the first entry
    #define PACMAN_BEGIN  if(machine == MCH_PACMAN) {
    #define PACMAN_END    }
  #endif
#endif

#ifdef ENABLE_GALAGA
  #ifdef SINGLE_MACHINE
    #define MACHINE_IS_GALAGA  1
    #define GALAGA_BEGIN
    #define GALAGA_END
  #else
    #define MACHINE_IS_GALAGA  (machine == MCH_GALAGA)
    // galaga is not the first if pacman is enabled
    #ifdef ENABLE_PACMAN
      // donkey kong or frogger may come afterwards
      #if defined(ENABLE_DKONG) || defined(ENABLE_FROGGER)      
        #define GALAGA_BEGIN  else if(machine == MCH_GALAGA) {
      #else
        #define GALAGA_BEGIN  else {
      #endif
    #else
      #define GALAGA_BEGIN  if(machine == MCH_GALAGA) {
    #endif
    #define GALAGA_END    }
  #endif
#endif

#ifdef ENABLE_DKONG
  #ifdef SINGLE_MACHINE
    #define MACHINE_IS_DKONG  1
    #define DKONG_BEGIN
    #define DKONG_END
  #else
    #define MACHINE_IS_DKONG  (machine == MCH_DKONG)
    // dkong is not the first if pacman or galaga are enabled
    #if defined(ENABLE_PACMAN) || defined(ENABLE_GALAGA)
      // frogger may come afterwards
      #ifdef ENABLE_FROGGER
        #define DKONG_BEGIN  else if(machine == MCH_DKONG) {
      #else
        #define DKONG_BEGIN  else {
      #endif
    #else
      #define DKONG_BEGIN  if(machine == MCH_DKONG) {
    #endif
    #define DKONG_END    }
  #endif
#endif

#ifdef ENABLE_FROGGER
  #ifdef SINGLE_MACHINE
    #define MACHINE_IS_FROGGER  1
    #define FROGGER_BEGIN
    #define FROGGER_END
  #else
    #define MACHINE_IS_FROGGER  (machine == MCH_FROGGER)
    // frogger is never first and always last
    #define FROGGER_BEGIN  else {
    #define FROGGER_END    }
  #endif
#endif

// scolling menu is needed whenever more than 3 machies are enabled
#if defined(ENABLE_PACMAN) && defined(ENABLE_GALAGA) && defined(ENABLE_DKONG) && defined(ENABLE_FROGGER)
#define MENU_SCROLL
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
