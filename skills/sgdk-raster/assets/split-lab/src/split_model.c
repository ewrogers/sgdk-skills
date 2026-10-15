#include "split_model.h"

static s16 clamp(s16 value, s16 limit)
{
    return value < 0 ? 0 : value > limit ? limit : value;
}

static void cameras(SplitModel *s)
{
    for (u16 i = 0; i < 2; ++i) {
        s->actor[i].camera_x = clamp(s->actor[i].x + 8 - 160, SPLIT_WORLD - SPLIT_WIDTH);
        s->actor[i].camera_y = clamp(s->actor[i].y + 8 - SPLIT_VIEW_H / 2, SPLIT_WORLD - SPLIT_VIEW_H);
    }
}

void split_model_init(SplitModel *s)
{
    *s = (SplitModel){.actor = {{96, 96, 0, 0}, {384, 256, 0, 0}}, .phase = 384, .cpu = TRUE};
    cameras(s);
}

static void move(SplitActor *a, u16 held)
{
    a->x = clamp(a->x + 2 * (((held & SPLIT_RIGHT) != 0) - ((held & SPLIT_LEFT) != 0)), SPLIT_WORLD - SPLIT_BODY);
    a->y = clamp(a->y + 2 * (((held & SPLIT_DOWN) != 0) - ((held & SPLIT_UP) != 0)), SPLIT_WORLD - SPLIT_BODY);
}

bool split_model_step(SplitModel *s, u16 p1, u16 p2, u16 hz)
{
    if (!s || (hz != 50 && hz != 60)) return FALSE;
    u16 pressed = p1 & (u16)~s->previous;
    s->previous = p1;
    if (pressed & SPLIT_CPU) s->cpu = !s->cpu;
    if (pressed & SPLIT_PAUSE) s->paused = !s->paused;
    if (pressed & SPLIT_RESET) {
        bool cpu = s->cpu, paused = s->paused;
        split_model_init(s);
        s->cpu = cpu; s->paused = paused; s->previous = p1;
    }
    if (s->paused) return TRUE;
    s->clock += 60;
    while (s->clock >= hz) {
        s->clock -= hz;
        move(&s->actor[0], p1);
        if (!s->cpu) move(&s->actor[1], p2);
        else {
            u16 t = s->phase;
            if (t < 256) { s->actor[1].x = 128 + t; s->actor[1].y = 128; }
            else if (t < 384) { s->actor[1].x = 384; s->actor[1].y = 128 + t - 256; }
            else if (t < 640) { s->actor[1].x = 384 - (t - 384); s->actor[1].y = 256; }
            else { s->actor[1].x = 128; s->actor[1].y = 256 - (t - 640); }
            s->phase = (t + 1) % 768;
        }
        cameras(s);
    }
    return TRUE;
}

bool split_scroll(const SplitModel *s, s16 *lines, u16 count)
{
    if (!s || !lines || count != SPLIT_HEIGHT) return FALSE;
    for (u16 y = 0; y < count; ++y)
        lines[y] = -s->actor[y < SPLIT_MIDDLE ? 0 : 1].camera_x;
    return TRUE;
}

u16 split_bottom_scroll(const SplitModel *s)
{
    /* VDP adds screen Y to VSRAM: subtract the lower viewport's screen origin.
       The resident plane wraps every 512 pixels; no signed C overflow needed. */
    return (u16)(s->actor[1].camera_y - SPLIT_BOTTOM_Y) & 511;
}

u16 split_top_scroll(const SplitModel *s)
{
    return (u16)(s->actor[0].camera_y - SPLIT_TOP_Y) & 511;
}

bool split_actor_position(const SplitModel *s, u16 view, u16 actor, s16 *x, s16 *y)
{
    if (!s || view > 1 || actor > 1 || !x || !y) return FALSE;
    *x = s->actor[actor].x - s->actor[view].camera_x;
    *y = s->actor[actor].y - s->actor[view].camera_y + (view ? SPLIT_BOTTOM_Y : SPLIT_TOP_Y);
    /* No hardware viewport scissor: omit a whole sprite if it crosses a HUD,
       edge or the eight-pixel divider. A real game can provide cropped frames. */
    s16 first = view ? 116 : 16;
    s16 last = view ? 208 : 108;
    return *x >= 0 && *x + SPLIT_BODY <= SPLIT_WIDTH && *y >= first && *y + SPLIT_BODY <= last;
}
