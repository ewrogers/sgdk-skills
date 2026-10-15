---
name: sgdk-scenes
description: Build SGDK game flow with title screens, character select, levels, pause, continue and credits; manage fades, input handoff, audio and resource cleanup between screens. Use when a scene transition leaves stale graphics or callbacks, repeats a held button, leaks memory, loses progress or needs a safe loading fallback.
---

# SGDK Scenes

Make each screen own its resources and decide exactly when gameplay stops,
loading happens and input becomes active again. SGDK supplies the hardware
services; a scene enum and update loop are ordinary game code.

## Choose a small flow

1. List the screens and what survives between them: player choices, options,
   score and checkpoint. Keep that session data separate from sprites, maps,
   callbacks and temporary buffers.
2. Pick what Continue restores. A level-start checkpoint is simpler than a
   full mid-frame save. Validate the complete record before changing the live
   session, and preserve the previous valid slot if a write fails.
3. Decide whether pause freezes gameplay only or also animation and sound.
   Keep input polling and VBlank service running. See
   [timing, input and saves](references/game-flow.md).
4. Pull `ewrogers/sgdk:latest`, run `doctor`, and verify APIs against its
   headers/source. The linked references were checked with SGDK 2.11.

## Give transitions one owner

- Request the next screen from game code. Apply that request in the main loop,
  after the current update finishes; avoid resource changes inside IRQ or
  joypad callbacks.
- Stop outgoing producers, fade out, then drain the final queued transfers
  while their source data still exists. SGDK clears the fade flag before its
  last queued palette transfer has been flushed.
- Hide the display, remove only callbacks the screen owns, and release its
  maps, sprites and scratch allocations. Clear stale handles.
- Reset the planes, Window, scrolling and palettes needed by the next screen.
  Build the incoming screen while hidden; flush its initial sprite/graphics
  uploads before fading in.
- Check each allocation. Clean up partial entry before trying a safe title or
  error screen. Do not repeatedly retry a failing load forever.

Follow [resource ownership and cleanup](references/transitions.md) for the
actual APIs and their limits. Do not cancel the global DMA queue or reset
unrelated services merely to make a transition appear to work.

## Prove the flow

The standalone [Transition Lab](assets/transition-lab/README.md) includes
Title, Continue, Options, Credits, two-player character selection, four rooms
and pause. Its boss encounter exercises bounded projectiles, dropped coins,
effects and a final reward without creating another scene framework.

Test both success and failure:

- Either player joins first; simultaneous joins and held buttons behave once.
- Countdown, manual animation and pause follow the intended clock at 50/60 Hz.
- Fades drain before resource cleanup; repeated transitions return to a
  comparable heap baseline; partial loads clean up before fallback.
- Missing, corrupt and interrupted saves preserve the older valid checkpoint.
- Emulator checks cover menus, both players, music, fades, Continue and the
  complete game. Console timing and SRAM retention need separate tests.

Report the ownership rules, checkpoint policy, tool versions and tests actually
performed. A host fixture checks ordering and game behavior; compiling a ROM
checks API compatibility. Neither proves VDP timing or real hardware behavior.
