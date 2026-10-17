# Menus and controls

## Give one item focus

Store a screen and a selected item, then map Up/Down to that index. Decide
whether the cursor wraps and whether disabled items can receive focus.
Use both a cursor shape and color so the selection remains clear on a small
screen. Confirm and back should have consistent meanings across screens.

Menu Lab uses ordinary C enums and structs. Its screen states are an example,
not SGDK scene objects or a required game framework. It shows title, options,
credits, a timed HUD demo, pause and results. Options remain in RAM when
returning to the title; the lab does not save them to SRAM.

For unfamiliar rules, explain the player's goal and one concrete first action
before presenting a list of buttons. Group related choices and keep the next
action visible. If a guide becomes a wall of text, short pages or an in-context
example may be easier to follow. Check with a first-time player: correct input
handling and a successful build do not establish that the game is understandable.

## Press once, repeat deliberately

`JOY_readJoypad(JOY_1)` returns SGDK's cached state. Normal
`SYS_doVBlankProcess()` includes the joypad update. Read the snapshot once,
then derive an edge:

```c
u16 held = JOY_readJoypad(JOY_1);
u16 pressed = held & (u16)~previous;
previous = held;
```

Use edges for A/Start confirmation and B cancellation. Keep the previous
snapshot across screen changes: holding A while opening a menu should not
activate its first choice on the next frame. Decide how simultaneous inputs
are resolved; Menu Lab cancels opposite directions and gives B priority over
A when clearing a score.

For cursor movement, fire once immediately, wait, then repeat while held.
The lab uses 300 ms before the first repeat and 100 ms thereafter. Releasing
or changing direction resets that schedule. These are tunable UI choices,
not hardware limits. Avoid applying that slow menu-repeat behavior to a
game's movement controls; the lab's `+` is only a cell-navigation example.

## Handle time and transitions

NTSC and PAL display frames have different durations. Menu Lab distributes
1000 milliseconds across 60 or 50 frames using an integer remainder, so its
12-second demo and repeat delays have similar real-time durations in either
region. This is a nominal timing policy, not recovery from missed frames.
Input is sampled once per display frame, not once per repeated simulation step.

Pause stops the round countdown, background scroll and score actions, while
the pause menu continues receiving input. Resume keeps the round state;
restart resets it. The lab blanks the display for infrequent full-screen
rebuilds, then draws small changed fields during normal frames. It is not a
fade or animated-transition system.

In a real game, assign ownership of music, sprites, background maps and DMA
buffers during a screen change. Stop old producers before releasing their
storage. Do not reset all VDP state for a menu overlay unless that is the
intended transition. A pause screen may need to keep the game's graphics.

## Check the awkward cases

Hold confirm across entry, hold a direction across wrapping, press opposite
directions, replace a long option with a short one, and leave/re-enter options.
Pause just before timeout and check that resuming does not expire the round
early. If a game uses hotplug or multiple players, decide who owns menu focus
and how disconnecting that controller affects it.

The lab uses P1's three-button controls. It does not prove six-button
extras, mouse menus, multitap focus or hotplug recovery. Add those only when
the game needs them and test with the actual devices/emulator configuration.

## Sources

- [SGDK 2.11 joypad API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/joy.h)
  and [cached reads](https://github.com/Stephane-D/SGDK/blob/v2.11/src/joy.c).
- [VBlank processing](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c)
  and [system/region API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h).
