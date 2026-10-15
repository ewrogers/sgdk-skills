#include "input.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static u8 ports[2];
static u8 types[JOY_NUM];
static u16 held[JOY_NUM];
static s16 axes_x[JOY_NUM], axes_y[JOY_NUM];
static u16 support[2];
static unsigned reads, writes, resets;
static JoyEventCallback *handler;

void JOY_setEventHandler(JoyEventCallback *callback) { handler = callback; }
void JOY_setSupport(u16 port, u16 mode) { assert(port < 2); support[port] = mode; }
u8 JOY_getPortType(u16 port) { assert(port < 2); return ports[port]; }
u8 JOY_getJoypadType(u16 joy) { assert(joy < JOY_NUM); ++reads; return types[joy]; }
u16 JOY_readJoypad(u16 joy) { assert(joy < JOY_NUM); ++reads; return held[joy]; }
s16 JOY_readJoypadX(u16 joy) { ++reads; return axes_x[joy]; }
s16 JOY_readJoypadY(u16 joy) { ++reads; return axes_y[joy]; }
s16 JOY_writeJoypadX(u16 joy, u16 pos) { ++writes; axes_x[joy] = (s16)pos; return 0; }
s16 JOY_writeJoypadY(u16 joy, u16 pos) { ++writes; axes_y[joy] = (s16)pos; return 0; }
void JOY_reset(void)
{
    ++resets;
    assert(handler != NULL);
    /* Model buttons held when reset finishes its internal refreshes. */
    handler(JOY_1, BUTTON_A, held[JOY_1]);
}

static void setup(void)
{
    ports[0] = ports[1] = PORT_TYPE_PAD;
    memset(types, JOY_TYPE_UNKNOWN, sizeof(types));
    memset(held, 0, sizeof(held));
    memset(axes_x, 0, sizeof(axes_x));
    memset(axes_y, 0, sizeof(axes_y));
    types[JOY_1] = JOY_TYPE_PAD3;
    types[JOY_2] = JOY_TYPE_PAD6;
    reads = writes = resets = 0;
    handler = NULL;
}

static void edges(void)
{
    input_init(); input_poll();
    held[JOY_1] = BUTTON_A | BUTTON_RIGHT;
    input_poll();
    const InputFrame *f = input_frame(JOY_1);
    assert(f->held == (BUTTON_A | BUTTON_RIGHT));
    assert(f->pressed == f->held && f->released == 0);
    input_poll();
    assert(f->pressed == 0 && f->released == 0);
    held[JOY_1] = BUTTON_RIGHT; input_poll();
    assert(f->pressed == 0 && f->released == BUTTON_A);
    held[JOY_1] = 0; input_poll();
    assert(f->released == BUTTON_RIGHT && f->held == 0);
}

static void snapshot(void)
{
    input_init(); input_poll(); held[0] = BUTTON_START; input_poll();
    const unsigned before = reads;
    held[0] = 0; /* Hardware/mock changes cannot alter the captured frame. */
    for (unsigned i = 0; i < 20; ++i)
        assert(input_frame(0)->pressed == BUTTON_START);
    assert(reads == before);
}

static void bindings(void)
{
    input_init(); input_poll();
    const InputFrame *f = input_frame(0);
    const u16 mask = BUTTON_A | BUTTON_X;
    held[0] = BUTTON_A; input_poll(); assert(input_button(f, mask).pressed);
    held[0] |= BUTTON_X; input_poll();
    assert(input_button(f, mask).held && !input_button(f, mask).pressed);
    held[0] = BUTTON_X; input_poll();
    assert(input_button(f, mask).held && !input_button(f, mask).released);
    held[0] = 0; input_poll(); assert(input_button(f, mask).released);
    assert(!input_button(f, 0).held && !input_button(f, 0).released);
}

static void axes(void)
{
    input_init(); input_poll(); const InputFrame *f = input_frame(0);
    held[0] = BUTTON_LEFT; input_poll(); assert(input_axis(f, BUTTON_LEFT, BUTTON_RIGHT) == -1);
    held[0] = BUTTON_RIGHT; input_poll(); assert(input_axis(f, BUTTON_LEFT, BUTTON_RIGHT) == 1);
    held[0] |= BUTTON_LEFT; input_poll(); assert(input_axis(f, BUTTON_LEFT, BUTTON_RIGHT) == 0);
}

static void all_players(void)
{
    for (unsigned i = 0; i < JOY_NUM; ++i) types[i] = JOY_TYPE_PAD6;
    input_init(); input_poll();
    for (unsigned i = 0; i < JOY_NUM; ++i) held[i] = (u16)(1u << i);
    input_poll();
    for (unsigned i = 0; i < JOY_NUM; ++i) assert(input_frame(i)->pressed == (1u << i));
    held[3] = 0; input_poll();
    assert(input_frame(3)->released == 8);
    assert(input_frame(4)->held == 16 && input_frame(4)->released == 0);
}

static void initial_hold(void)
{
    held[0] = BUTTON_A; input_init(); input_poll();
    assert(input_frame(0)->held == BUTTON_A && input_frame(0)->pressed == 0);
    held[0] = 0; input_poll(); assert(input_frame(0)->released == BUTTON_A);
}

static void six_button_changes(void)
{
    input_init(); input_poll();
    held[JOY_2] = BUTTON_X | BUTTON_Y | BUTTON_Z | BUTTON_MODE;
    input_poll();
    assert(input_frame(JOY_2)->pressed == held[JOY_2]);
    held[JOY_2] = BUTTON_A | BUTTON_X;
    input_poll();
    assert(input_frame(JOY_2)->pressed == BUTTON_A);
    assert(input_frame(JOY_2)->released == (BUTTON_Y | BUTTON_Z | BUTTON_MODE));
    assert(input_frame(JOY_1)->held == 0);
}

static void rescan(void)
{
    input_init(); input_poll(); held[0] = BUTTON_A; input_poll();
    input_rescan(); input_poll();
    assert(resets == 1 && input_frame(0)->held == BUTTON_A);
    assert(!input_frame(0)->pressed && !input_frame(0)->released);
    held[0] = 0; input_poll(); held[0] = BUTTON_A; input_poll();
    assert(input_frame(0)->pressed == BUTTON_A);
}

static void configuration(void)
{
    input_init();
    assert(support[0] == JOY_SUPPORT_6BTN && support[1] == JOY_SUPPORT_6BTN);
    assert(handler != NULL && resets == 0);
    ports[1] = PORT_TYPE_MOUSE; input_init();
    assert(support[0] == JOY_SUPPORT_6BTN && support[1] == JOY_SUPPORT_MOUSE);
    ports[0] = PORT_TYPE_MOUSE; ports[1] = PORT_TYPE_PAD; input_init();
    assert(support[0] == JOY_SUPPORT_MOUSE && support[1] == JOY_SUPPORT_6BTN);
    ports[0] = PORT_TYPE_UNKNOWN; input_init(); assert(support[0] == JOY_SUPPORT_OFF);
}

static void ea_adapter(void)
{
    ports[0] = ports[1] = PORT_TYPE_EA4WAYPLAY; input_init();
    assert(support[0] == JOY_SUPPORT_EA4WAYPLAY && support[1] == JOY_SUPPORT_OFF);
    assert(handler != NULL);
}

static void mouse(void)
{
    ports[0] = PORT_TYPE_MOUSE; types[0] = JOY_TYPE_MOUSE;
    axes_x[0] = 90; axes_y[0] = 60; input_init(); input_poll();
    assert(input_frame(0)->dx == 0 && input_frame(0)->dy == 0);
    assert(axes_x[0] == 0 && axes_y[0] == 0);
    axes_x[0] = 7; axes_y[0] = -4; held[0] = BUTTON_LMB; input_poll();
    assert(input_frame(0)->dx == 7 && input_frame(0)->dy == -4);
    assert(input_frame(0)->pressed == BUTTON_LMB);
    const unsigned before = writes;
    assert(input_frame(0)->dx == 7); assert(writes == before);
    input_poll(); assert(input_frame(0)->dx == 0 && input_frame(0)->dy == 0);
}

static void team_player(void)
{
    static const unsigned first[] = {0, 2, 3, 4};
    static const unsigned second[] = {1, 5, 6, 7};
    for (unsigned port = 0; port < 2; ++port)
    {
        ports[port] = PORT_TYPE_TEAMPLAYER;
        ports[1 - port] = PORT_TYPE_MOUSE;
        for (unsigned i = 0; i < JOY_NUM; ++i) types[i] = JOY_TYPE_MOUSE;
        input_init(); input_poll();
        assert(support[port] == JOY_SUPPORT_TEAMPLAYER);
        for (unsigned i = 0; i < JOY_NUM; ++i) axes_y[i] = 4;
        input_poll();
        const unsigned *slots = port ? second : first;
        for (unsigned i = 0; i < 4; ++i) assert(input_frame(slots[i])->dy == -4);
        assert(input_frame(1 - port)->dy == 4);
    }
    ports[0] = ports[1] = PORT_TYPE_TEAMPLAYER; input_init(); input_poll();
    for (unsigned i = 0; i < JOY_NUM; ++i) axes_y[i] = -3;
    input_poll();
    for (unsigned i = 0; i < JOY_NUM; ++i) assert(input_frame(i)->dy == 3);
}

static void type_change(void)
{
    input_init(); input_poll(); held[0] = BUTTON_A; input_poll();
    types[0] = JOY_TYPE_MOUSE; axes_x[0] = 150; axes_y[0] = 100;
    input_poll();
    assert(input_frame(0)->dx == 0 && input_frame(0)->dy == 0);
    assert(!input_frame(0)->pressed && !input_frame(0)->released);
    types[0] = JOY_TYPE_UNKNOWN; input_poll();
    assert(input_frame(0)->held == 0 && input_frame(0)->dx == 0);
    assert(!input_frame(0)->pressed && !input_frame(0)->released);
}

static void mouse_buttons(void)
{
    types[0] = JOY_TYPE_MOUSE; input_init(); input_poll();
    const u16 buttons[] = {BUTTON_LMB, BUTTON_MMB, BUTTON_RMB, BUTTON_START};
    for (unsigned i = 0; i < 4; ++i)
    {
        held[0] = buttons[i]; input_poll(); assert(input_frame(0)->pressed == buttons[i]);
        held[0] = 0; input_poll(); assert(input_frame(0)->released == buttons[i]);
    }
}

static void invalid_slot(void)
{
    input_init();
    const unsigned before = reads;
    assert(input_frame(JOY_NUM)->type == JOY_TYPE_UNKNOWN);
    assert(input_frame(65535)->held == 0 && reads == before);
}

int main(int argc, char **argv)
{
    static const struct { const char *name; void (*run)(void); } cases[] = {
        {"edges", edges}, {"snapshot", snapshot}, {"bindings", bindings},
        {"axes", axes}, {"all_players", all_players}, {"initial_hold", initial_hold},
        {"six_button_changes", six_button_changes},
        {"rescan", rescan}, {"configuration", configuration}, {"ea_adapter", ea_adapter},
        {"mouse", mouse}, {"team_player", team_player}, {"type_change", type_change},
        {"mouse_buttons", mouse_buttons}, {"invalid_slot", invalid_slot}
    };
    assert(argc == 2);
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
        if (strcmp(argv[1], cases[i].name) == 0)
        {
            setup(); cases[i].run(); puts(cases[i].name); return 0;
        }
    return 2;
}
