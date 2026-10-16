#include "lab.h"
#include "dialogue_data.h"

void lab_init(DialogueLab *lab)
{
    *lab=(DialogueLab){0};
}

void lab_frame(DialogueLab *lab, u16 held, u16 hz)
{
    const u16 pressed=held & ~lab->held;
    lab->held=held; lab->events=0;
    if (hz != 50 && hz != 60) return;
    lab->fraction+=1000;
    const u16 elapsed=lab->fraction/hz;
    lab->fraction%=hz;
    if (lab->active && (pressed & LAB_BACK)) { lab->active=false; lab->paused=false; return; }
    if (!lab->active)
    {
        lab->clock_ms+=elapsed;
        if (pressed & LAB_UP) lab->story=(lab->story+LAB_STORY_COUNT-1)%LAB_STORY_COUNT;
        else if (pressed & LAB_DOWN) lab->story=(lab->story+1)%LAB_STORY_COUNT;
        if (pressed & LAB_A)
        {
            lab->active=true; lab->paused=false;
            dlg_start(&lab->dialogue,&lab_program,lab_starts[lab->story],0);
            lab->events=DL_ENTER_EVENT;
        }
        return;
    }
    if (pressed & LAB_PAUSE) { lab->paused=!lab->paused; return; }
    if (lab->paused) return;
    lab->clock_ms+=elapsed;
    if (pressed & LAB_REPLAY)
    {
        dlg_start(&lab->dialogue,&lab_program,lab_starts[lab->story],0);
        lab->events=DL_ENTER_EVENT; return;
    }
    if (lab->dialogue.state == DL_DONE && (pressed & LAB_A)) { lab->active=false; return; }
    dlg_step(&lab->dialogue,elapsed,pressed & (DL_UP|DL_DOWN|DL_ACCEPT));
    lab->events=lab->dialogue.events;
}
