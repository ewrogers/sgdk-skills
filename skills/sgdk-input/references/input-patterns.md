# Input patterns for a game

## One refresh, one snapshot

The ordinary loop is:

```c
while (TRUE)
{
    SYS_doVBlankProcess(); /* Includes JOY_update(). */
    input_poll();         /* Example helper: copies cached state. */
    update_game();
    prepare_graphics();
}
```

The placement of game/render preparation may differ in an existing project.
Keep a clear boundary between SGDK refreshing input and the game consuming
it. `JOY_readJoypad()` is already a cache read. `input_poll()` is useful for
edges and action mapping; it is not needed to make repeated SDK reads cheap.

Do not call the snapshot function again from individual actors. Otherwise a
second call can replace a pressed edge with a held state and consume mouse
movement before another actor sees it.

## Hold, press and release

For raw button masks, derive edges from consecutive snapshots:

```c
u16 pressed = held & (u16)~previous;
u16 released = previous & (u16)~held;
```

Use held for walking, pressed for starting a jump or choosing a menu item,
and released for ending a charge. `JOY_setEventHandler()` is another option;
its callback receives `(u16 joy, u16 changed, u16 state)`. There, pressed is
`changed & state` and released is `changed & ~state`. SGDK owns one callback
slot. Dispatch from one handler if multiple systems need it.

The Input Lab helper uses polling, with a no-op callback for the pinned
Team Player issue described in [devices](devices.md). Callbacks run inside
`JOY_update()` with interrupts disabled. Keep them short and defer work.

At setup or a reported device-type change, the example treats the current held
state as its baseline. Held buttons stay held, but do not create phantom press
or release actions. This is an application policy, not SGDK's edge behavior.
It cannot fix an undetected device change or stale state from the driver.

## Small named actions

An action can be a plain C function. There is no need for string lookup:

```c
static InputButton confirm(const InputFrame *frame)
{
    if (frame->type == JOY_TYPE_MOUSE)
        return input_button(frame, BUTTON_LMB);
    return input_button(frame, BUTTON_A | BUTTON_X);
}
```

This lets a 3-button player use A, a 6-button player use A or X, and a mouse
player click. `InputFrame`, `InputButton` and `input_button()` belong to the
example, not SGDK. Move the mapping into your game's own code as needed.

Alternate bindings need edges on the **combined action**, not an OR of each
button's pressed bits. If A is held and X is pressed too, Confirm is still
held; it should not fire a second time. Releasing A while X remains held
should not release Confirm. The helper handles both cases.

For remapping during play, rebaseline the affected action or require release
before accepting it. The example's binding masks are fixed.

## Movement axes

For a digital axis, subtract the negative direction from the positive:

```c
s16 move_x = input_axis(input_frame(JOY_1), BUTTON_LEFT, BUTTON_RIGHT);
```

The result is -1, 0 or 1. Opposite directions cancel. That is a deliberate
policy; a fighting game may need a different one. Mouse movement uses `dx`
and `dy`, not this digital axis. The example normalizes mouse movement to
right-positive and down-positive screen directions and consumes the SDK
accumulators once per refresh.

## Simulation, pause and regions

Keep input sampling at display refresh even when gameplay updates more slowly.
For a 30 Hz simulation on a 60 Hz display, retain press/release bits between
simulation steps and sum mouse deltas, then consume them once. Held state is
the most recent sample. If press/release ordering matters, use a bounded event
queue rather than two ORed masks. A tap between two *hardware polls* cannot
be recovered by software buffering.

Continue sampling while paused so menus can respond. Decide whether gameplay
edges accumulated during pause should be discarded on resume. Do not replay
the same pressed edge through several catch-up simulation steps.

PAL and NTSC have different refresh rates. Keep repeat delays and movement
units deliberate; the input API does not make a game region-independent.
Additional mice/adapters also add polling work. Measure the actual busy game
with its supported device configurations before making performance claims.

Sources: SGDK [joy API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/joy.h),
[input implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/joy.c),
[system processing](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c),
Plutiedev [button edges and polling timing](https://plutiedev.com/controllers).
