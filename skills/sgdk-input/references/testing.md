# Test the controls a player will use

Record the SGDK image/revision, emulator version or console, region, physical
port setup and results. Mark untried configurations as unverified. Do not call
a mocked SDK test a hardware test.

## First check: one pad

Build [Input Lab](../assets/input-lab/README.md), then test a 3-button pad in each
port. Every button should change `HELD`; the latest press and release masks
should remain visible in `DOWN` and `UP`. Holding a button should not create
new press edges. The on-screen Confirm indicator follows A.

Repeat with a 6-button pad, including X/Y/Z/Mode. Hold A, press X, release A,
then release X. In a game action test this should produce one Confirm press
and one release, not a second press while the alternate binding is added.
The lab shows raw masks and held Confirm; host tests verify those action edges.

Check opposite D-pad directions where your emulator allows them. The example
axis policy is neutral when both directions are held. Essential actions should
still work on a 3-button pad.

## Mouse

Test direct connection on each port, both alone and beside a pad. Enable mouse
capture and relative-axis mappings in the emulator. Move right, left, up and
down: lab X grows rightward and Y grows downward. Stationary input should not
keep moving. Test Left, Right, Middle and Start independently on Mega Mouse.

Repeat through a Team Player if the emulator or hardware supports it. Check
the Y direction explicitly because the pinned SDK's direct and adapter paths
differ. Test quick movement and cursor bounds in the actual game too.
Two-button Sega Mouse needs a control scheme that does not require Middle or
Start.

## Multitaps

| Configuration | Expected slots |
| --- | --- |
| Team Player on port 1, second port unused or direct device | A/B/C/D are JOY 1/3/4/5; direct port 2 is JOY 2 |
| Team Player on port 2 | A/B/C/D are JOY 2/6/7/8 |
| Two Team Players | All eight slots, with each adapter's mapping above |
| EA 4-Way Play | Four pads at JOY 1/3/4/5; both physical ports occupied |

Use MULTI mode for Team Player. Test mixed 3/6-button pads, several buttons
held on different controllers, empty sockets, and a mouse on Team Player
where supported. One player's release must not clear another player's held
state. Do not substitute a contiguous 1/2/3/4 mapping for SGDK's slot table.

## Setup changes and timing

Release controls before booting the pinned SDK with Team Player. Exercise the
explicit rescan shortcut while a controller still works, then release/repress
buttons. A button held across rescanning should not trigger a phantom action.

Test unplugging separately, but do not assume the example can detect it:
[cached-state limitations](devices.md#sgdk-211-caveats-to-keep-visible) explain
why types or buttons can stay stale. Reset after changing an unsupported
topology. Record stale-state behavior rather than masking it with a guessed
presence flag.

Finally, test the real game while paused, under load and in each supported
region. Check short taps at slower simulation rates and ensure mouse motion
and pressed edges are consumed once. Measure polling cost with the actual
adapter/device setup if frame time is tight. The lab's text display is for
diagnosis, not a performance baseline.
