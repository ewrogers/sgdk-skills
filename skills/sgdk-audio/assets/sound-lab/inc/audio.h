#ifndef SOUND_LAB_AUDIO_H
#define SOUND_LAB_AUDIO_H
#include <genesis.h>

typedef enum { AUDIO_IDLE, AUDIO_STARTING, AUDIO_PLAYING, AUDIO_PAUSING,
               AUDIO_PAUSED, AUDIO_RESUMING, AUDIO_FADING } AudioState;
typedef struct
{
    AudioState state;
    bool loop;
    u16 volume;
    u16 completed, accepted, rejected;
} AudioStatus;

void audio_init(const u8 *music, const u8 *click, u32 click_size,
                const u8 *alarm, u32 alarm_size);
void audio_step(u16 held);
const AudioStatus *audio_status(void);
#endif
