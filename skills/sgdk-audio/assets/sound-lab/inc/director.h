#ifndef SOUND_LAB_DIRECTOR_H
#define SOUND_LAB_DIRECTOR_H
#ifdef AD_HOST
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;
#else
#include <genesis.h>
#endif
#define AD_NONE 255
#define AD_QUEUE 4
enum {AD_IDLE,AD_STARTING,AD_PLAYING,AD_FADING};
enum {AD_NO_COMMAND,AD_PLAY,AD_FADE,AD_STOP};
enum {AD_COIN,AD_JUMP,AD_IMPACT,AD_ALERT,AD_FX_COUNT};
typedef struct {u16 command,track,frames;bool loop;} AdCommand;
typedef struct {u8 id,priority;u16 remaining;} AdVoice;
typedef struct {
    u8 state,current,desired;bool jingle,pending_jingle,paused,stop_requested,timed_out;
    u16 wait,phase,cooldown[AD_FX_COUNT];
    AdVoice voice[2];u8 queue[AD_QUEUE],queued;
    u32 scheduled,suppressed,dropped,driver_rejected;
} AdDirector;
void ad_init(AdDirector *d);
bool ad_scene(AdDirector *d,u16 track);
bool ad_jingle(AdDirector *d);
void ad_stop(AdDirector *d);
void ad_pause(AdDirector *d,bool paused);
AdCommand ad_step(AdDirector *d,bool playing,bool fading,u16 hz);
AdCommand ad_service(AdDirector *d,bool playing,bool fading,u16 hz,u16 elapsed_vblanks);
bool ad_post(AdDirector *d,u16 effect);
void ad_clock(AdDirector *d,u16 hz);
void ad_clock_frames(AdDirector *d,u16 hz,u16 elapsed_vblanks);
void ad_flush(AdDirector *d,u8 effects[2]);
void ad_rejected(AdDirector *d,u16 voice);
bool ad_valid(const AdDirector *d);
u32 ad_digest(const AdDirector *d);
#endif
