# Presenting an action through SGDK

## Choose automatic or manual advancement

SGDK automatic animation is useful for simple cosmetic loops. In the verified
implementation, `SPR_update()` advances those timers once per call. Calling
it once per VBlank means the same holds last longer at 50 Hz than 60 Hz.
ResComp's time labels do not make an overloaded or PAL loop automatically
normalize itself.

Animation Lab disables automatic playback with
`SPR_setAutoAnimation(sprite, FALSE)`, selects the logical pose with
`SPR_setFrame`, updates position/flip/visibility, and calls `SPR_update()` once
per displayed frame. Its 60-tick action scheduler can do two logical steps on
a PAL frame without submitting sprite graphics twice. Pose indices are checked
against the compiled resource in tests; they are not unchecked names or IDs.

Sources: [sprite API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sprite_eng.h),
[update/timer implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sprite_eng.c).

## Renderer state is not the combat controller

`SPR_setAnimAndFrame` changes state when its animation/frame differs; it is not
an unconditional restart of an identical pose. `SPR_isAnimationDone` is a
predicate, not a queued once-only event, and its behavior differs between manual
and automatic playback. Stopping/re-enabling automatic animation also does not
implement the Lab's exact remaining-hold pause semantics.

`SPR_setFrameChangeCallback` runs as the sprite engine processes frame changes
inside `SPR_update`. That is useful for presentation work, including custom tile
placement, but not an independent logical clock. Tile upload admission and frame
processing can lag the simulation. Do not make callback timing the authority
for damage, cooldowns or gameplay projectile creation. Keep callbacks short and
make actor/resource lifetimes explicit if they retain references.
The sprite's status flags are not yet fully updated inside that callback.

Source: [frame changes, callbacks and completion](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sprite_eng.c).

## Prepare and budget the actual frames

The Lab's hero is a 408×24 indexed PNG: seventeen 24×24 poses in one row.
ResComp frame dimensions are in 8-pixel tiles, so its resource is:

```text
SPRITE hero "hero.png" 3 3 NONE 0 NONE NONE
```

Here the hold value zero disables resource-driven automatic advancement,
collision is `NONE`, and cutting optimization is `NONE` so each pose uses one
24×24 hardware piece. The gameplay clips in `action.c` map those poses to their
own holds, events and boxes. Other projects can use separate rows/resources or
their own data exporter. Keep mappings stable and validate compiled frame counts
when an artist adds, removes or reorders poses.

Source: [ResComp resource syntax](https://github.com/Stephane-D/SGDK/blob/v2.11/bin/rescomp.txt).

The Lab reserves 72 sprite tiles with `SPR_initEx(72)`. Its six definitions need
at most 51 frame tiles together; hidden debug sprites can still own allocations.
Art is uncompressed to avoid decode work in this small example. Each displayed
pose change can upload tile data even though the logical controller is cheap.
Count frame tiles, hardware pieces, busiest scanline and simultaneous changes.
Sharing an image definition does not automatically share each instance's VRAM.

Text uses a retained 2,240-byte tilemap queued for VBlank. That buffer and sprite
upload/SAT work share a configured 4,096-byte DMA allowance. The Lab drains
queued reads before rewriting the tilemap. No active-display palette writes or
HBlank effects are needed. A larger game must budget its other consumers too;
this small training scene is not a throughput guarantee.

The Lab's finite PSG cues own tone channel zero without a music driver. In a
game where music also uses PSG, route cues through the audio owner instead of
copying those direct writes over the driver's channels.
