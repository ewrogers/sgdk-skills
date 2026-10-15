#ifndef TRANSITION_FAKE_SDK_H
#define TRANSITION_FAKE_SDK_H
#include <genesis.h>
enum { EV_DRAIN=1,EV_OFF,EV_ON,EV_STOP,EV_PCM,EV_RESET,EV_SPR,EV_CLEAR,EV_COLOR,EV_OUT,EV_IN };
extern char fake_cells[28][41];
extern u16 fake_ink[28][40],fake_events[4096],fake_event_count;
extern u16 fake_frames,fake_sram_mode,fake_sram_closes,fake_count;
extern bool fake_fade,fake_palette_pending,fake_sprite_pending;
extern int fake_fail_sprite_after;
extern Sprite fake_sprites[32];
void fake_reset(void);
void fake_finish_fade(void);
#endif
