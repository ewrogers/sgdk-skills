#ifndef DL_LAB_H
#define DL_LAB_H
#include "dialogue.h"
enum { LAB_UP=DL_UP, LAB_DOWN=DL_DOWN, LAB_A=DL_ACCEPT, LAB_BACK=8, LAB_REPLAY=16, LAB_PAUSE=32 };
typedef struct {
    Dialogue dialogue;
    u32 clock_ms;
    u16 held, fraction, events;
    u8 story;
    bool active, paused;
} DialogueLab;
void lab_init(DialogueLab *lab);
/* One cached controller snapshot per display frame. Only 50/60 Hz accepted. */
void lab_frame(DialogueLab *lab, u16 held, u16 hz);
#endif
