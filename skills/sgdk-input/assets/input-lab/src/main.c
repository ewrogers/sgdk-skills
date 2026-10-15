#include <genesis.h>
#include "input.h"

static const char *device_name(u8 type)
{
    switch (type)
    {
        case JOY_TYPE_PAD3: return "PAD 3";
        case JOY_TYPE_PAD6: return "PAD 6";
        case JOY_TYPE_MOUSE: return "MOUSE";
        default: return "OTHER";
    }
}

static void hex4(u16 value, char *text)
{
    static const char digits[] = "0123456789ABCDEF";
    u16 i;
    for (i = 0; i < 4; ++i)
        text[i] = digits[(value >> ((3 - i) * 4)) & 15];
    text[4] = 0;
}

int main(bool hard_reset)
{
    u16 last_down[JOY_NUM] = {0};
    u16 last_up[JOY_NUM] = {0};
    s32 x[JOY_NUM] = {0};
    s32 y[JOY_NUM] = {0};
    u16 redraw = 0;
    (void)hard_reset;

    VDP_setScreenWidth320();
    input_init();
    VDP_drawText("SGDK INPUT LAB", 12, 1);
    VDP_drawText("JOY TYPE   HELD DOWN UP   CONFIRM", 1, 3);
    VDP_drawText("DOWN/UP keep the latest edge mask", 1, 22);
    VDP_drawText("Mouse: X right, Y down; click confirm", 1, 23);
    VDP_drawText("Start+A+B+C: rescan; release to boot", 1, 25);

    while (TRUE)
    {
        u16 joy;
        bool rescan = FALSE;
        SYS_doVBlankProcess();
        input_poll();
        for (joy = 0; joy < JOY_NUM; ++joy)
        {
            const InputFrame *frame = input_frame(joy);
            const u16 combo = BUTTON_START | BUTTON_A | BUTTON_B | BUTTON_C;
            if (frame->pressed) last_down[joy] = frame->pressed;
            if (frame->released) last_up[joy] = frame->released;
            x[joy] += frame->dx;
            y[joy] += frame->dy;
            /* Keep the diagnostic coordinates bounded for indefinite use. */
            if (x[joy] > 9999) x[joy] = 9999;
            if (x[joy] < -9999) x[joy] = -9999;
            if (y[joy] > 9999) y[joy] = 9999;
            if (y[joy] < -9999) y[joy] = -9999;
            if ((frame->held & combo) == combo && frame->pressed != 0)
                rescan = TRUE;
        }
        if (rescan)
        {
            input_rescan();
            for (joy = 0; joy < JOY_NUM; ++joy)
                last_down[joy] = last_up[joy] = 0;
        }

        /* Text refresh is throttled; input sampling is still every frame. */
        if (++redraw < 6) continue;
        redraw = 0;
        for (joy = 0; joy < JOY_NUM; ++joy)
        {
            const InputFrame *frame = input_frame(joy);
            const u16 row = 5 + joy * 2;
            char text[12];
            const InputButton confirm = input_button(frame,
                frame->type == JOY_TYPE_MOUSE ? BUTTON_LMB : BUTTON_A | BUTTON_X);
            VDP_clearText(1, row, 38);
            uintToStr(joy + 1, text, 1);
            VDP_drawText(text, 1, row);
            VDP_drawText(device_name(frame->type), 5, row);
            hex4(frame->held, text); VDP_drawText(text, 12, row);
            hex4(last_down[joy], text); VDP_drawText(text, 17, row);
            hex4(last_up[joy], text); VDP_drawText(text, 22, row);
            VDP_drawText(confirm.held ? "YES" : "-", 28, row);
            VDP_clearText(1, row + 1, 38);
            if (frame->type == JOY_TYPE_MOUSE)
            {
                intToStr(x[joy], text, 1); VDP_drawText(text, 12, row + 1);
                intToStr(y[joy], text, 1); VDP_drawText(text, 22, row + 1);
            }
        }
    }
}
