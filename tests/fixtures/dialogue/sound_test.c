#include <assert.h>
#include <stdio.h>
#include "lab.h"
#include "sound.h"
#include "genesis.h"
#include "resources.h"
const u8 harbor_music[4]={0},cue_done[4]={1},cue_choice[4]={2},cue_turn[4]={3},cue_tick[4]={4};
static unsigned plays,pauses,resumes,stops;
static const u8 *last;
void XGM2_load(const u8 *song) { assert(song == harbor_music); }
void XGM2_setLoopNumber(signed char loops) { assert(loops == -1); }
void XGM2_playTrack(u16 track) { assert(track == 0); }
void XGM2_setFMVolume(u16 volume) { assert(volume <= 100); }
void XGM2_pause(void) { pauses++; }
void XGM2_resume(void) { resumes++; }
void XGM2_stopPCM(u16 channel) { assert(channel == SOUND_PCM_CH2); stops++; }
bool XGM2_playPCM(const u8 *sample,u32 length,u16 channel)
{ assert(length == 4 && channel == SOUND_PCM_CH2); last=sample; plays++; return true; }
static void frame(DialogueLab *lab,u16 held)
{ lab_frame(lab,held,60); sound_frame(lab); }
int main(void)
{
    DialogueLab lab;
    lab_init(&lab); sound_start(); frame(&lab,LAB_A);
    assert(plays == 1 && last == cue_turn);
    for (unsigned steps=0; lab.dialogue.state != DL_DONE && steps<100; steps++)
    { frame(&lab,0); frame(&lab,LAB_A); }
    assert(lab.dialogue.state == DL_DONE && last == cue_done);
    const unsigned ended=plays;
    /* Regression: a terminal event used to repeat its cue every idle frame. */
    for (unsigned i=0;i<600;i++) frame(&lab,0);
    assert(plays == ended && lab.events == 0);
    frame(&lab,LAB_PAUSE); assert(pauses == 1 && stops == 1);
    for (unsigned i=0;i<60;i++) frame(&lab,0);
    assert(pauses == 1 && plays == ended);
    frame(&lab,LAB_PAUSE); assert(resumes == 1 && plays == ended);
    frame(&lab,0); frame(&lab,LAB_A); assert(!lab.active && plays == ended);
    /* A simultaneous reveal cannot supersede a higher-priority cue. */
    lab.active=true; lab.events=DL_DONE_EVENT|DL_CHOICE_EVENT|DL_REVEAL_EVENT;
    sound_frame(&lab); assert(plays == ended+1 && last == cue_done);
    lab.events=DL_CHOICE_EVENT|DL_REVEAL_EVENT;
    sound_frame(&lab); assert(plays == ended+2 && last == cue_choice);
    puts("sound lifecycle"); return 0;
}
