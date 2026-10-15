#ifndef SPLIT_LAB_VIDEO_H
#define SPLIT_LAB_VIDEO_H
#include <genesis.h>
void split_video_init(void);
void split_video_prepare(u16 top_scroll, u16 bottom_scroll);
void split_video_stop(void);
u16 split_video_late(void);
u16 split_video_timeouts(void);
HINTERRUPT_CALLBACK split_video_interrupt(void);
#endif
