
#define KEYCODE_LEFT  0x01
#define KEYCODE_RIGHT 0x02
#define KEYCODE_UP    0x04
#define KEYCODE_DOWN  0x08
#define KEYCODE_FIRE  0x10
#define KEYCODE_1     0x20
#define KEYCODE_2     0x40
#define KEYCODE_3     0x80

#if defined(__cplusplus)
extern "C"
{
#endif
void prepare_emulation(void);
void emulate_frame(void);
extern unsigned char *memory;
extern char game_started;
extern unsigned char starcontrol;
extern unsigned char soundregs[32];

// external functions called by emulation
extern void snd_trigger_explosion(void);
extern unsigned char buttons_get(void);
#if defined(__cplusplus)
}
#endif
