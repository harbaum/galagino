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
#ifdef ENABLE_DIGDUG
      MCH_DIGDUG,
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
      // donkey kong, frogger or digdug may come afterwards
      #if defined(ENABLE_DKONG) || defined(ENABLE_FROGGER) || defined(ENABLE_DIGDUG)     
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
      // frogger or digdug may come afterwards
      #if defined(ENABLE_FROGGER) || defined(ENABLE_DIGDUG)
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
    // frogger is not the first if pacman, galaga or dkong are enabled
    #if defined(ENABLE_PACMAN) || defined(ENABLE_GALAGA) || defined(ENABLE_DKONG)
      // digdug may come afterwards
      #if defined(ENABLE_DIGDUG)
        #define FROGGER_BEGIN  else if(machine == MCH_FROGGER) {
      #else
        #define FROGGER_BEGIN  else {
      #endif
    #else
      #define FROGGER_BEGIN  if(machine == MCH_FROGGER) {
    #endif
    #define FROGGER_END    } 
  #endif
#endif

#ifdef ENABLE_DIGDUG
  #ifdef SINGLE_MACHINE
    #define MACHINE_IS_DIGDUG  1
    #define DIGDUG_BEGIN
    #define DIGDUG_END
  #else
    #define MACHINE_IS_DIGDUG  (machine == MCH_DIGDUG)
    // digdug is never first and always last
    #define DIGDUG_BEGIN  else {
    #define DIGDUG_END    }
  #endif
#endif

// scolling menu is needed whenever more than 3 machies are enabled
#if (defined(ENABLE_PACMAN) && defined(ENABLE_GALAGA) && defined(ENABLE_DKONG) && defined(ENABLE_FROGGER)) || \
    (defined(ENABLE_PACMAN) && defined(ENABLE_GALAGA) && defined(ENABLE_DKONG) && defined(ENABLE_DIGDUG))  || \
    (defined(ENABLE_PACMAN) && defined(ENABLE_GALAGA) && defined(ENABLE_FROGGER) && defined(ENABLE_DIGDUG))  || \
    (defined(ENABLE_PACMAN) && defined(ENABLE_DKONG) && defined(ENABLE_FROGGER) && defined(ENABLE_DIGDUG))  || \
    (defined(ENABLE_GALAGA) && defined(ENABLE_DKONG) && defined(ENABLE_FROGGER) && defined(ENABLE_DIGDUG))
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
#if defined(ENABLE_GALAGA) || defined(ENABLE_PACMAN) || defined(ENABLE_DIGDUG)
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

/* ------------------ the following is used inside Z80.c ----------------- */
extern char current_cpu;
#ifdef ENABLE_PACMAN
extern const unsigned char pacman_rom[];
#endif
#ifdef ENABLE_GALAGA
extern const unsigned char galaga_rom_cpu1[];
extern const unsigned char galaga_rom_cpu2[];
extern const unsigned char galaga_rom_cpu3[];
#endif
#ifdef ENABLE_DKONG
extern const unsigned char dkong_rom_cpu1[];
#endif
#ifdef ENABLE_FROGGER
extern const unsigned char frogger_rom_cpu1[];
extern const unsigned char frogger_rom_cpu2[];
#endif
#ifdef ENABLE_DIGDUG
extern const unsigned char digdug_rom_cpu1[];
extern const unsigned char digdug_rom_cpu2[];
extern const unsigned char digdug_rom_cpu3[];
#endif

#define NONE  ((const unsigned char *)0l)

static inline byte OpZ80_INL(register word Addr) {
#ifndef SINGLE_MACHINE
  static const unsigned char *rom_table[][3] = {
    { NONE, NONE, NONE },
#define ROM_ENDL ,
#else
  static const unsigned char *rom_table[3] =
#define ROM_ENDL ;
#endif
#ifdef ENABLE_PACMAN 
    { pacman_rom, NONE, NONE } ROM_ENDL
#endif
#ifdef ENABLE_GALAGA
    { galaga_rom_cpu1, galaga_rom_cpu2, galaga_rom_cpu3 } ROM_ENDL
#endif
#ifdef ENABLE_DKONG
    { dkong_rom_cpu1, NONE, NONE } ROM_ENDL
#endif
#ifdef ENABLE_FROGGER
    { frogger_rom_cpu1, frogger_rom_cpu2, NONE } ROM_ENDL
#endif
#ifdef ENABLE_DIGDUG
    { digdug_rom_cpu1, digdug_rom_cpu2, digdug_rom_cpu3 } ROM_ENDL
#endif  
#ifndef SINGLE_MACHINE
  };
  return rom_table[machine][current_cpu][Addr];
#else 
  return rom_table[current_cpu][Addr];
#endif
}

#endif // _EMULATION_H_
