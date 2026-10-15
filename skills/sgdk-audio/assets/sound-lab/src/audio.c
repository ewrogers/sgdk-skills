#include "audio.h"

static const u8 *song, *soft_sample, *loud_sample;
static u32 soft_size, loud_size;
static u16 previous;
static AudioStatus status;

void audio_init(const u8 *music, const u8 *click, u32 click_size,
                const u8 *alarm, u32 alarm_size)
{
    song = music;
    soft_sample = click;
    soft_size = click_size;
    loud_sample = alarm;
    loud_size = alarm_size;
    previous = 0;
    status = (AudioStatus){AUDIO_IDLE, TRUE, 100, 0, 0, 0};
    /* Load before setters: several XGM2 setters do nothing with another driver. */
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);
    XGM2_setFMVolume(100);
    XGM2_setPSGVolume(100);
    XGM2_setLoopNumber(-1);
}

static void poll_music(void)
{
    bool playing = XGM2_isPlaying();
    switch (status.state)
    {
        case AUDIO_STARTING:
        case AUDIO_RESUMING:
            if (playing) status.state = AUDIO_PLAYING;
            break;
        case AUDIO_PLAYING:
            if (!playing) { status.state = AUDIO_IDLE; ++status.completed; }
            break;
        case AUDIO_PAUSING:
            if (!playing) status.state = AUDIO_PAUSED;
            break;
        case AUDIO_FADING:
            if (!XGM2_isProcessingFade()) status.state = AUDIO_IDLE;
            break;
        default: break;
    }
}

static void play_effect(const u8 *sample, u32 size, u8 priority)
{
    /* Deliberately share CH2 so the priority contest is easy to hear. */
    if (XGM2_playPCMEx(sample, size, SOUND_PCM_CH2, priority, FALSE, FALSE))
        ++status.accepted;
    else
        ++status.rejected;
}

void audio_step(u16 held)
{
    u16 pressed = held & (u16)~previous;
    previous = held;
    poll_music();

    if (pressed & BUTTON_UP) play_effect(loud_sample, loud_size, 10);
    else if (pressed & BUTTON_C) play_effect(soft_sample, soft_size, 1);

    if (pressed & BUTTON_START)
    {
        XGM2_stop();
        XGM2_stopPCM(SOUND_PCM_CH2);
        /* An existing fade still owns its deferred stop. Don't start a new
           track until it finishes, even though the sound has stopped now. */
        if (status.state != AUDIO_FADING) status.state = AUDIO_IDLE;
        return;
    }
    /* A fade owns the music controls until its deferred stop has finished. */
    if (status.state == AUDIO_FADING) return;

    if (pressed & BUTTON_LEFT)
    {
        status.loop = !status.loop;
        XGM2_setLoopNumber(status.loop ? -1 : 0);
    }
    if (pressed & BUTTON_RIGHT)
    {
        status.volume = status.volume == 100 ? 50 : 100;
        XGM2_setFMVolume(status.volume);
        XGM2_setPSGVolume(status.volume);
    }
    if (pressed & BUTTON_A)
    {
        XGM2_play(song);
        status.state = AUDIO_STARTING;
    }
    else if (pressed & BUTTON_B)
    {
        if (status.state == AUDIO_PLAYING)
        {
            XGM2_pause();
            status.state = AUDIO_PAUSING;
        }
        else if (status.state == AUDIO_PAUSED)
        {
            XGM2_resume();
            status.state = AUDIO_RESUMING;
        }
    }
    else if ((pressed & BUTTON_DOWN) && status.state == AUDIO_PLAYING)
    {
        XGM2_fadeOutAndStop(60);
        status.state = AUDIO_FADING;
    }
}

const AudioStatus *audio_status(void) { return &status; }
