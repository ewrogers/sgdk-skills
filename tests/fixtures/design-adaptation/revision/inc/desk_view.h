#ifndef DESK_VIEW_H
#define DESK_VIEW_H
#include "desk_ui.h"
enum { VIEW_W = 40, VIEW_H = 28, VIEW_CELLS = VIEW_W * VIEW_H,
       VIEW_PLANES = 2, VIEW_MAX_ROWS = VIEW_PLANES * VIEW_H,
       VIEW_MAX_BYTES = VIEW_MAX_ROWS * VIEW_W * 2 };
/* Pattern offsets: solid, rail, busy rail, tram, horizontal/vertical border. */
enum { ART_SOLID, ART_RAIL, ART_BUSY, ART_TRAM, ART_H, ART_V, ART_PLANNED, ART_COUNT };
typedef struct {
    unsigned short next[VIEW_PLANES][VIEW_CELLS];
    unsigned short shown[VIEW_PLANES][VIEW_CELLS];
    unsigned long dirty[VIEW_PLANES];
    unsigned short clipped, font, art;
    unsigned char pending;
} DeskView;
typedef void (*ViewRow)(void *context, unsigned char plane, unsigned char row,
                        const unsigned short *words);
void view_init(DeskView *view);
/* Refuses to overwrite borrowed sources while a transfer is pending. */
unsigned char view_compose(DeskView *view, const DeskUI *ui,
                           unsigned short font, unsigned short art);
unsigned char view_queue(DeskView *view, ViewRow sink, void *context);
/* Call only after VBlank has drained the queued rows. */
void view_complete(DeskView *view);
#endif
