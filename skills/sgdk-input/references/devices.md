# Devices and the actual SGDK API

This reference was checked against SGDK 2.11 using the pinned container in
[Input Lab](../assets/input-lab/README.md). If a project uses another revision,
recheck its `inc/joy.h`, `src/joy.c` and `src/sys.c` before carrying these
details across.

## Ports are not player numbers

`PORT_1` and `PORT_2` select the two physical sockets. `JOY_1` through `JOY_8`
select SGDK's cached device slots. The numbers in the names are one-based;
the C constants start at zero. `JOY_NUM` is eight.

| Connection | Socket A | Socket B | Socket C | Socket D |
| --- | --- | --- | --- | --- |
| Team Player on port 1 | `JOY_1` | `JOY_3` | `JOY_4` | `JOY_5` |
| Team Player on port 2 | `JOY_2` | `JOY_6` | `JOY_7` | `JOY_8` |
| EA 4-Way Play, both ports | `JOY_1` | `JOY_3` | `JOY_4` | `JOY_5` |

A direct device uses `JOY_1` or `JOY_2` according to its port. Two Team Players
can provide eight slots. EA 4-Way Play provides four pads, not eight, and leaves
no second physical port available for another device. Keep your game's player
assignments separate from these slot IDs. A join screen can map whichever
controller pressed Start to the next player.

Set a Team Player's hardware switch to **MULTI** for multitap operation.
It accepts pads and mice. EA 4-Way Play accepts pads. Do not assume every
emulator implements both adapters or every mixed-device arrangement.

Sources: SGDK [slot translation and readers](https://github.com/Stephane-D/SGDK/blob/v2.11/src/joy.c),
Plutiedev [Team Player](https://plutiedev.com/sega-multitap) and
[EA 4-Way Play](https://plutiedev.com/ea-multitap).

## Detection and support selection

| API | Meaning |
| --- | --- |
| `u8 JOY_getPortType(u16 port)` | Port type saved during initialization/reset. It does not probe the port now. |
| `u8 JOY_getJoypadType(u16 joy)` | Most recently cached logical-device type. Useful for choosing button prompts, with the limitations below. |
| `void JOY_setSupport(u16 port, u16 support)` | Choose the reader for that physical port. It does not perform fresh topology detection. |
| `void JOY_reset(void)` | Redetect ports, clear state/axes and wait through three VBlank-processing calls. Use deliberately during setup. |
| `void JOY_init(void)` | Clears the callback and resets the subsystem. SGDK already calls it before `main`. |

For detected pads, `JOY_SUPPORT_6BTN` reads both 3-button and 6-button pads.
`JOY_SUPPORT_3BTN` intentionally limits polling to the simpler protocol.
Use `JOY_TYPE_PAD3`/`JOY_TYPE_PAD6` for the reported device, not the selected
support mode.

SGDK's default initialization enables pads. If neither port is a pad, it
enables one detected Team Player or mouse. Therefore explicitly enable each
detected mouse with `JOY_SUPPORT_MOUSE` and each Team Player with
`JOY_SUPPORT_TEAMPLAYER`, even when a pad exists on the other port.

EA detection is special: port 1 uses `JOY_SUPPORT_EA4WAYPLAY`; port 2 must
remain `JOY_SUPPORT_OFF` because the adapter uses it for selection. The
example's `configure_ports()` handles that before the ordinary port loop.

## Buttons and cached state

`u16 JOY_readJoypad(u16 joy)` returns a held-button bitmask. Use
`BUTTON_UP`, `BUTTON_DOWN`, `BUTTON_LEFT`, `BUTTON_RIGHT`, `BUTTON_A`,
`BUTTON_B`, `BUTTON_C` and `BUTTON_START`. Six-button pads add `BUTTON_X`,
`BUTTON_Y`, `BUTTON_Z` and `BUTTON_MODE`.

Mouse aliases are **`BUTTON_LMB = BUTTON_A`**, **`BUTTON_MMB = BUTTON_B`**
and **`BUTTON_RMB = BUTTON_C`**. Mega Mouse also has `BUTTON_START`.
Use the real symbol `BUTTON_RMB`; one comment in the header misspells it.
The Japanese/European two-button Sega Mouse lacks middle and Start, so keep
those optional if supporting that variant.

`JOY_ALL` ORs every slot's cached buttons. It loses player identity and does
not give meaningful combined mouse coordinates. Use individual slots for
gameplay and joining players.

## Mouse coordinates

`s16 JOY_readJoypadX(u16 joy)` and `JOY_readJoypadY(u16 joy)` return accumulated
movement counts, not a fresh delta or D-pad axis. Reading does not consume
them. `JOY_writeJoypadX/Y(u16 joy, u16 pos)` resets or replaces the accumulator;
both functions return `s16` (the implementation returns zero).

Read movement once after SGDK's update, then either take a wrap-aware difference
or reset the accumulators to zero. The example uses the latter and shares the
result with all consumers. It discards the first sample after setup/type change
to avoid a cursor jump. Clamp the game cursor, not an unbounded SDK accumulator.

In **SGDK 2.11 source**, a direct mouse subtracts protocol Y, producing
screen-down coordinates. The Team Player reader adds protocol Y, producing
up-positive coordinates. This differs from the header's general Y description.
Input Lab flips Team Player Y so its `dy` is consistently down-positive.
Retest this adaptation when upgrading SGDK.

The driver also synthesizes direction bits from motion beyond a small threshold.
Use actual axis counts for aiming or a cursor; those bits lose small movement.
Packet overflow handling is driver behavior, not a promise of lossless motion.

Sources: SGDK [declarations](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/joy.h)
and [mouse implementations](https://github.com/Stephane-D/SGDK/blob/v2.11/src/joy.c),
Plutiedev [mouse protocol and variants](https://plutiedev.com/mouse).

## SGDK 2.11 caveats to keep visible

- **Team Player callback:** its reader calls the event callback on changed
  buttons without checking for null. Install a valid callback before enabling
  it or explicitly resetting. Input Lab installs a no-op callback even though
  it uses polling. Preserve an existing handler when integrating into a game.
- **Before `main`:** SGDK initializes the subsystem with a null callback.
  If it auto-enables a Team Player and detects held buttons during startup,
  that callback path can be reached before application code can install a
  handler. Release controls when booting the example. A robust shipping fix
  requires a reviewed SDK fix; the example does not repair SDK startup.
- **Disconnects:** a cached type is not a reliable `is_connected` signal.
  Direct pad readers can still report a pad with nothing attached; the mouse
  reader can still report mouse after a failed packet. Adapter failures and
  missing EA pads can leave old state. Do not infer fresh presence or promise
  safe hotplug from these getters.
- **Rediscovery:** do it in a setup/menu operation, with a valid callback, then
  reapply support and rebaseline application state. It is not an every-frame
  operation. If all working controls are removed, the example's rescan shortcut
  cannot be used; reset the ROM after configuring devices in the emulator.

These are observations of the pinned implementation, not general hardware
restrictions. Source: [SGDK 2.11 joy.c](https://github.com/Stephane-D/SGDK/blob/v2.11/src/joy.c).

## Timing, bus ownership and ROM metadata

The standard `SYS_doVBlankProcess()` path invokes `JOY_update()` once near its
end. Polling the physical protocol again can disrupt 6-button reads. Keep
low-level I/O and Z80 bus handling with SGDK; do not copy raw register access
into a normal gameplay loop. Its I/O code observes `HALT_Z80_ON_IO`.

When maintaining a game's ROM header, advertise only devices the game uses:
`J` for pads, `6` for six-button features, `M` for mouse, `4` for multitap.
These tags describe support; they do not configure it.

Sources: SGDK [VBlank processing](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c),
[joy.c](https://github.com/Stephane-D/SGDK/blob/v2.11/src/joy.c),
Plutiedev [controller timing](https://plutiedev.com/controllers) and
[multitap ROM metadata](https://plutiedev.com/sega-multitap#header).
