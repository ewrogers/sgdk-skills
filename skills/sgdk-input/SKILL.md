---
name: sgdk-input
description: Implement or debug SGDK input for 3/6-button pads, Mega Mouse, Sega Team Player and EA 4-Way Play. Use for frame sampling, button edges, action bindings, player slots and controller-test ROMs.
---

# SGDK input

Make controls predictable for the player and easy for the game to consume.
Use the project's selected SGDK. Verify unfamiliar APIs against its headers
and implementation; do not invent a generic controller API or import an
engine's restrictions.

## Choose the devices and controls

Read existing game decisions first. Keep essential pad controls available on
a 3-button controller unless the game explicitly requires six buttons. Ask
about player count or required peripherals only when it changes the task.

Distinguish physical `PORT_1`/`PORT_2` from logical `JOY_1` through `JOY_8`.
A Team Player supports four devices on one port; two can serve eight players.
EA 4-Way Play uses both ports for four pads. Never assign a contiguous group of
four SGDK joy IDs to a multitap without checking the mapping.

Read [devices and SGDK behavior](references/devices.md) before configuring
mice, multitaps or device changes. It records the exact API, slot tables,
button aliases and important SGDK 2.11 limitations.

## Sample once, reuse throughout the frame

With the normal SGDK loop, `SYS_doVBlankProcess()` calls `JOY_update()`.
`JOY_readJoypad()` reads cached state; repeated calls do not poll the hardware.
Do not add a second `JOY_update()` or raw controller-port reads to that loop.

Take one game snapshot after the refresh and reuse it. Derive held, pressed
and released states, then map them to game actions. Keep the device check
with the binding: mouse buttons share bits with pad buttons.

Use [input patterns](references/input-patterns.md) for button edges, alternate
bindings, opposite directions, mouse movement and slower simulation steps.
Keep callbacks short. Do not perform VDP work, reset controllers or run game
logic inside the input callback.

## Start with the smallest useful example

The standalone [Input Lab](assets/input-lab/README.md) configures both ports,
displays all eight logical slots, shows held/edge masks, and consumes mouse
movement. Its small `input.c` is optional example code, not part of SGDK.
Adapt only what the game needs; no heap, string lookup or binding framework
is required for a handful of actions.

The example owns SGDK's single joy callback and mouse accumulators. Integrate
that ownership with existing input code rather than installing competing
handlers. Leave the project's toolchain pin unchanged. For a new scratch
project, the example documents the tested Docker image and build commands.

## Verify the behavior, not just compilation

Compile against the real pinned SDK. Test press/hold/release, alternate
bindings, opposite directions, baseline after reset, and each player's
independent state. For mouse input, test all supported buttons, both motion
axes, capture/mapping in the emulator and direct versus Team Player routing.

Use the [device test checklist](references/testing.md) for both multitaps,
mixed devices and hardware-specific checks. Record which configurations were
actually exercised. Host tests prove application snapshot logic with mocked
SDK calls; compiling a ROM proves API compatibility. Neither proves physical
device protocols, hotplug, emulator mapping or 50/60 Hz timing.

Do not promise automatic hotplug from `JOY_getPortType()` or
`JOY_getJoypadType()`. Use deliberate rediscovery during setup if needed,
explain its interruption, and account for the selected SGDK version's
startup and stale-state behavior.
