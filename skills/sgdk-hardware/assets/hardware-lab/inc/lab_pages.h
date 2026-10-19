#ifndef HARDWARE_LAB_PAGES_H
#define HARDWARE_LAB_PAGES_H
#include <genesis.h>
#include "usb_input.h"
typedef struct {
    bool music,pcm,overflow,capture,valid;
    u16 mode,rejected,write_status,held;
    u32 missed,sequence,vints;
    UiPort input[2];
    u16 pointer_x[2],pointer_y[2];
} HlPageView;
/* Presentation only; save writes and input decisions remain in main. */
void hl_pages_open(u16 page,const HlPageView *view);
void hl_pages_update(u16 page,const HlPageView *view);
#endif
