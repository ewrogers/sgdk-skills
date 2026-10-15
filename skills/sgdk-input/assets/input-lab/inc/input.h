#ifndef INPUT_LAB_INPUT_H
#define INPUT_LAB_INPUT_H

#include <genesis.h>

/* Example-owned snapshot; these are not additional SGDK APIs. */
typedef struct
{
    u8 type;
    u16 held;
    u16 previous;
    u16 pressed;
    u16 released;
    s32 dx;
    s32 dy;
} InputFrame;

typedef struct
{
    bool held;
    bool pressed;
    bool released;
} InputButton;

void input_init(void);
void input_rescan(void);
void input_poll(void);
const InputFrame *input_frame(u16 joy);

InputButton input_button(const InputFrame *frame, u16 bindings);
s16 input_axis(const InputFrame *frame, u16 negative, u16 positive);

#endif
