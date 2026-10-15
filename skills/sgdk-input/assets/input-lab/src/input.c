#include "input.h"

static InputFrame frames[JOY_NUM];
static bool baseline[JOY_NUM];
static bool through_team_player[JOY_NUM];

/* SGDK 2.11's Team Player reader calls this without checking for NULL. */
static void on_joy_change(u16 joy, u16 changed, u16 state)
{
    (void)joy;
    (void)changed;
    (void)state;
}

static void configure_ports(void)
{
    static const u16 slots[2][4] = {
        {JOY_1, JOY_3, JOY_4, JOY_5},
        {JOY_2, JOY_6, JOY_7, JOY_8}
    };
    u16 joy;
    u16 port;

    for (joy = 0; joy < JOY_NUM; ++joy)
    {
        frames[joy] = (InputFrame){0};
        frames[joy].type = JOY_TYPE_UNKNOWN;
        baseline[joy] = TRUE;
        through_team_player[joy] = FALSE;
    }

    /* The EA adapter occupies both physical ports and supplies four pads. */
    if (JOY_getPortType(PORT_1) == PORT_TYPE_EA4WAYPLAY)
    {
        JOY_setSupport(PORT_1, JOY_SUPPORT_EA4WAYPLAY);
        JOY_setSupport(PORT_2, JOY_SUPPORT_OFF);
        return;
    }

    for (port = PORT_1; port <= PORT_2; ++port)
    {
        switch (JOY_getPortType(port))
        {
            case PORT_TYPE_PAD:
                JOY_setSupport(port, JOY_SUPPORT_6BTN);
                break;
            case PORT_TYPE_MOUSE:
                JOY_setSupport(port, JOY_SUPPORT_MOUSE);
                break;
            case PORT_TYPE_TEAMPLAYER:
            {
                u16 socket;
                JOY_setSupport(port, JOY_SUPPORT_TEAMPLAYER);
                for (socket = 0; socket < 4; ++socket)
                    through_team_player[slots[port][socket]] = TRUE;
                break;
            }
            default:
                JOY_setSupport(port, JOY_SUPPORT_OFF);
                break;
        }
    }
}

void input_init(void)
{
    /* SGDK already detected ports before main. Own its one callback slot. */
    JOY_setEventHandler(on_joy_change);
    configure_ports();
}

void input_rescan(void)
{
    /* Explicit setup/menu operation: JOY_reset waits several refreshes. */
    JOY_setEventHandler(on_joy_change);
    JOY_reset();
    configure_ports();
}

void input_poll(void)
{
    u16 joy;
    for (joy = 0; joy < JOY_NUM; ++joy)
    {
        InputFrame *frame = &frames[joy];
        const u8 type = JOY_getJoypadType(joy);
        const bool supported = type == JOY_TYPE_PAD3 ||
                               type == JOY_TYPE_PAD6 || type == JOY_TYPE_MOUSE;
        const u16 state = supported ? JOY_readJoypad(joy) : 0;
        const bool rebaseline = baseline[joy] || type != frame->type;

        frame->previous = rebaseline ? state : frame->held;
        frame->type = type;
        frame->held = state;
        frame->pressed = state & (u16)~frame->previous;
        frame->released = frame->previous & (u16)~state;
        frame->dx = 0;
        frame->dy = 0;

        if (type == JOY_TYPE_MOUSE)
        {
            const s32 x = JOY_readJoypadX(joy);
            const s32 y = JOY_readJoypadY(joy);
            /* Consume accumulated counts once; other code reads our snapshot. */
            JOY_writeJoypadX(joy, 0);
            JOY_writeJoypadY(joy, 0);
            if (!rebaseline)
            {
                frame->dx = x;
                /* Direct mouse is already screen-down in SGDK 2.11. */
                frame->dy = through_team_player[joy] ? -y : y;
            }
        }
        baseline[joy] = FALSE;
    }
}

const InputFrame *input_frame(u16 joy)
{
    static const InputFrame unknown = { .type = JOY_TYPE_UNKNOWN };
    return joy < JOY_NUM ? &frames[joy] : &unknown;
}

InputButton input_button(const InputFrame *frame, u16 bindings)
{
    const bool held = (frame->held & bindings) != 0;
    const bool previous = (frame->previous & bindings) != 0;
    const InputButton result = { held, held && !previous, !held && previous };
    return result;
}

s16 input_axis(const InputFrame *frame, u16 negative, u16 positive)
{
    return ((frame->held & positive) != 0) - ((frame->held & negative) != 0);
}
