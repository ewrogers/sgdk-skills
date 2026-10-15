#include "audio.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const u8 music[256], click[256], alarm[512];
static bool loaded, playing, fading, accept = true;
static int plays, pauses, resumes, stops, pcm_stops, effects, fades;
static int loop_value, priority;
static u16 fm, psg;
static const u8 *effect_sample;
static u32 effect_size;
static SoundPCMChannel effect_channel;
static bool pal;
static u16 fade_frames;
static u32 elapsed;
static bool dma_delay;

u16 SYS_isPAL(void) { return pal; }
u32 XGM2_getElapsed(void) { return elapsed; }
void Z80_setForceDelayDMA(bool value) { assert(loaded); dma_delay = value; }

void Z80_loadDriver(u16 driver, bool waitReady)
{ assert(driver == Z80_DRIVER_XGM2 && waitReady); loaded = true; }
void XGM2_setFMVolume(u16 value) { assert(loaded && value <= 100); fm = value; }
void XGM2_setPSGVolume(u16 value) { assert(loaded && value <= 100); psg = value; }
void XGM2_setLoopNumber(s8 value) { assert(loaded); loop_value = value; }
bool XGM2_isPlaying(void) { return playing; }
bool XGM2_isProcessingFade(void) { return fading; }
bool XGM2_playPCMEx(const u8 *sample, u32 size, SoundPCMChannel channel, u8 prio, bool half, bool loop)
{
    assert((channel == SOUND_PCM_CH1 || channel == SOUND_PCM_CH2) && !half && !loop);
    ++effects; effect_sample = sample; effect_size = size; priority = prio; effect_channel = channel;
    return accept;
}
void XGM2_stop(void) { ++stops; }
void XGM2_stopPCM(SoundPCMChannel channel) { assert(channel == SOUND_PCM_CH1 || channel == SOUND_PCM_CH2); ++pcm_stops; }
void XGM2_play(const u8 *song) { assert(song == music); ++plays; }
void XGM2_pause(void) { ++pauses; }
void XGM2_resume(void) { ++resumes; }
void XGM2_fadeOutAndStop(u16 frames) { fade_frames = frames; ++fades; fading = true; }

static void press(u16 button) { audio_step(0); audio_step(button); }
static void begin(void)
{ press(BUTTON_A); playing = true; audio_step(0); assert(audio_status()->state == AUDIO_PLAYING); }

int main(int argc, char **argv)
{
    assert(argc == 2);
    const char *test = argv[1];
    audio_init(music, click, sizeof click, alarm, sizeof alarm);
    if (!strcmp(test, "init"))
    {
        assert(loaded && fm == 100 && psg == 100 && loop_value == -1);
        assert(dma_delay);
        assert(audio_status()->state == AUDIO_IDLE && plays == 0);
    }
    else if (!strcmp(test, "start_pending"))
    {
        press(BUTTON_A);
        for (int i = 0; i < 10; ++i) audio_step(BUTTON_A);
        assert(plays == 1 && audio_status()->state == AUDIO_STARTING);
        assert(audio_status()->completed == 0);
        playing = true; audio_step(0); assert(audio_status()->state == AUDIO_PLAYING);
    }
    else if (!strcmp(test, "completion"))
    {
        begin(); playing = false;
        for (int i = 0; i < 10; ++i) audio_step(0);
        assert(audio_status()->completed == 1 && audio_status()->state == AUDIO_IDLE);
        begin(); playing = false; audio_step(0); assert(audio_status()->completed == 2);
    }
    else if (!strcmp(test, "pause_resume"))
    {
        begin(); press(BUTTON_B); assert(pauses == 1 && audio_status()->state == AUDIO_PAUSING);
        audio_step(0); assert(audio_status()->state == AUDIO_PAUSING);
        playing = false; audio_step(0); assert(audio_status()->state == AUDIO_PAUSED);
        press(BUTTON_B); assert(resumes == 1 && audio_status()->state == AUDIO_RESUMING);
        audio_step(0); assert(audio_status()->completed == 0);
        playing = true; audio_step(0); assert(audio_status()->state == AUDIO_PLAYING);
    }
    else if (!strcmp(test, "idle_pause"))
    {
        press(BUTTON_B); press(BUTTON_DOWN);
        assert(pauses == 0 && resumes == 0 && fades == 0);
    }
    else if (!strcmp(test, "stop"))
    {
        begin(); press(BUTTON_START); assert(stops == 1 && pcm_stops == 2);
        playing = false; audio_step(0);
        assert(audio_status()->state == AUDIO_IDLE && audio_status()->completed == 0);
        press(BUTTON_B); assert(resumes == 0);
    }
    else if (!strcmp(test, "loop"))
    {
        press(BUTTON_LEFT); assert(loop_value == 0 && !audio_status()->loop);
        audio_step(BUTTON_LEFT); assert(loop_value == 0);
        press(BUTTON_LEFT); assert(loop_value == -1 && audio_status()->loop);
    }
    else if (!strcmp(test, "volume"))
    {
        press(BUTTON_RIGHT); assert(fm == 50 && psg == 50);
        audio_step(BUTTON_RIGHT); assert(fm == 50);
        press(BUTTON_RIGHT); assert(fm == 100 && psg == 100 && effects == 0);
    }
    else if (!strcmp(test, "effects"))
    {
        press(BUTTON_C); audio_step(BUTTON_C);
        assert(effects == 1 && effect_sample == click && effect_size == sizeof click && priority == 1);
        assert(audio_status()->accepted == 1);
        press(BUTTON_UP); assert(effects == 2 && effect_sample == alarm && effect_size == sizeof alarm && priority == 15);
    }
    else if (!strcmp(test, "rejection"))
    {
        accept = false; press(BUTTON_C);
        for (int i = 0; i < 20; ++i) audio_step(BUTTON_C);
        assert(effects == 1 && audio_status()->accepted == 0 && audio_status()->rejected == 1);
    }
    else if (!strcmp(test, "effect_conflict"))
    {
        press(BUTTON_C | BUTTON_UP); assert(effects == 1 && effect_sample == alarm);
    }
    else if (!strcmp(test, "fade"))
    {
        begin(); press(BUTTON_DOWN); assert(fades == 1 && audio_status()->state == AUDIO_FADING);
        press(BUTTON_A); press(BUTTON_START); press(BUTTON_RIGHT); press(BUTTON_LEFT);
        assert(plays == 1 && stops == 1 && pcm_stops == 2 && fm == 100 && loop_value == -1);
        assert(audio_status()->state == AUDIO_FADING);
        press(BUTTON_C); assert(effects == 1);
        fading = false; playing = false; audio_step(0);
        assert(audio_status()->state == AUDIO_IDLE && audio_status()->completed == 0);
        press(BUTTON_A); assert(plays == 2);
    }
    else if (!strcmp(test, "transport_conflict"))
    {
        begin(); press(BUTTON_START | BUTTON_A | BUTTON_B | BUTTON_DOWN);
        assert(stops == 1 && plays == 1 && pauses == 0 && fades == 0);
    }
    else if (!strcmp(test, "regions"))
    {
        assert(audio_status()->refresh_hz == 60);
        begin(); press(BUTTON_DOWN); assert(fade_frames == 60);
        pal = true; fading = false; playing = false;
        audio_init(music, click, sizeof click, alarm, sizeof alarm);
        assert(audio_status()->refresh_hz == 50);
        begin(); press(BUTTON_DOWN); assert(fade_frames == 50);
    }
    else if (!strcmp(test, "test_switches"))
    {
        press(BUTTON_C | BUTTON_LEFT);
        assert(audio_status()->music_channel && effects == 0 && loop_value == -1);
        audio_step(BUTTON_C | BUTTON_LEFT); assert(audio_status()->music_channel);
        press(BUTTON_C); assert(effect_channel == SOUND_PCM_CH1 && priority == 1);
        press(BUTTON_C | BUTTON_RIGHT);
        assert(audio_status()->graphics_dma && fm == 100 && psg == 100 && effects == 1);
        press(BUTTON_C | BUTTON_LEFT); assert(!audio_status()->music_channel);
        press(BUTTON_UP); assert(effect_channel == SOUND_PCM_CH2 && priority == 15);
        press(BUTTON_C | BUTTON_RIGHT); assert(!audio_status()->graphics_dma);
        press(BUTTON_START | BUTTON_C | BUTTON_LEFT);
        assert(!audio_status()->music_channel && stops == 1 && pcm_stops == 2);
    }
    else if (!strcmp(test, "timing"))
    {
        for (int region = 0; region < 2; region++) {
            u16 hz = region ? 50 : 60;
            u16 phase = 0;
            pal = region != 0; elapsed = 0xFFFFA0; playing = false;
            audio_init(music, click, sizeof click, alarm, sizeof alarm);
            begin();
            for (u16 frame = 0; frame < hz * 4; frame++) {
                phase += 60;
                elapsed = (elapsed + phase / hz) & 0xFFFFFF;
                phase %= hz;
                audio_step(0);
                if (frame + 1 < hz * 4) assert(!audio_status()->timing_ready);
            }
            assert(audio_status()->timing_ready && audio_status()->measured_frames == 240);
            press(BUTTON_B); playing = false; audio_step(0);
            for (int i = 0; i < 300; i++) audio_step(0);
            assert(audio_status()->measured_frames == 240);
            press(BUTTON_A); assert(!audio_status()->timing_ready);
        }
    }
    else { fprintf(stderr, "unknown case: %s\n", test); return 2; }
    puts(test);
    return 0;
}
