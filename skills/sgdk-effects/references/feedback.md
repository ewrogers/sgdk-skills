# Camera shake and hit feedback

## Shake the view, not the world

Keep the authoritative camera/player positions independent of a short-lived
render offset. Apply the same offset to the world plane and world sprites, then
cull using that rendered camera. Leave screen-space HUD/menu coordinates fixed.
Clamp the final camera or provide extra streamed margins so shake does not expose
uninitialized map edges. Multiple cameras need independent offsets and budgets.

Effects Lab has a deterministic 18-tick shake envelope with a two-pixel maximum
offset. It clamps horizontal world coverage and restores the baseline when the
envelope expires. Disabling shake clears the current envelope immediately.
Pause stops particle ages, movement, animation and the envelope, while Start
still resumes and held buttons are not replayed as new presses.

The SGDK adapter uses `VDP_setHorizontalScrollVSync(BG_B, -camera_x)` and
`VDP_setVerticalScrollVSync(BG_B, camera_y)`. Both defer scroll changes to frame
processing. BG_A remains the fixed HUD. Check the [versioned scroll implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp_bg.c)
for the selected SDK; do not accidentally scroll every plane.

## Flashing, fades and clarity

A sprite has palette selection and binary transparent index zero, not an alpha
value. A fade may use authored frames, sparse pixels, visibility patterns or a
palette change, each with different visual and budget costs. Dithering is an
optional style choice, not a requirement. Avoid assuming shadow/highlight is a
per-sprite opacity feature.

For hit flashes, prefer an already reserved palette variant when appropriate.
Changing a shared CRAM line changes every user of that line. Track who owns the
palette, restore it on interruption/exit, and perform ordinary palette transfers
in VBlank or while display is disabled. HBlank palette tricks add timing/CRAM-dot
risks and are not needed for routine impact feedback.

Preserve readability: decoration should not hide enemy shots or a player's
silhouette. Consider reduced shake, flashes and weather settings; those must
leave hit detection and scoring unchanged. The Lab exposes a shake toggle but
does not claim a comprehensive accessibility mode or photosensitivity safety.

## Useful regression checks

- Fill pools and retry; confirm deterministic priority replacement and reuse.
- Exercise concentrated and distributed rectangles at frame/scanline limits.
- Compare rendered selections with a simple independent counter implementation.
- Vary the rendering budget and confirm simulation/RNG remain unchanged.
- Test world-space trails while moving the camera and stopping the emitter.
- Check every frame's actual linked tiles, palette and hardware-piece geometry.
- Pause during bursts, shake and weather; resume without repeating a held press.
- Repeat initialization/cleanup, shorten the SAT chain and inject upload failure.
- Compare nominal PAL/NTSC elapsed time, then playtest actual regional displays.

Host recording adapters do not emulate the VDP. Target CPU execution does not
establish scanline timing, physical-console appearance or sustained 60 FPS.
