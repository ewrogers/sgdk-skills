# Informed UI correction

This revision follows user feedback about flashing text, an overwhelming desk
and a guide that did not explain the game clearly. It is separate from the frozen
first submission and is not another independent-agent result.

The original dispatch model is copied from `../submission/src/desk.c` by the test
driver. The UI adds three panels, selected-row highlights, a concrete next-action
prompt and a three-page guide. All original gameplay controls remain:

- D-pad Up/Down: tram. Left/Right: bay.
- C: dispatch. B: clear bay. A: hold/run.
- Start: guide. In the guide, Left/Right changes page, C advances/returns,
  and Start closes. Help restores the prior held/running state.

Bay rows show the selected tram's required upper/lower lines (U/L). Red letters
mark conflicts, blue dotted rails preview its route, and amber rails show traffic.
The prompt names the blocking line and wait; the guide includes the full route
table. Host tests cover all 144 candidate/active pairs and staggered releases.
The user confirmed the panelled desk was clearer and no longer flashed; route
hints follow their subsequent feedback about the misleading linked-bay wording.
The user then confirmed that these route conflicts are clear in ares.

Screen contents are composed in RAM. Changed complete rows are queued for
VBlank; `view_complete` runs only after the queue drains. At most 56 operations /
4,480 bytes are queued, within the selected SDK defaults. This small ROM has no
other DMA producer. Staged/shown maps use 8,960 bytes plus metadata. The recording
test checks source lifetimes, no intermediate blank screen, unchanged-row elision,
visible bounds and guide/clock behavior; it does not emulate VDP scanout.

Run from the repository root:

```sh
python -m unittest discover -s tests -p test_design_adaptation_ui.py -v
```

With `SGDK_SKILLS_INTEGRATION=1` and the appropriate `SGDK_PLATFORM`, the driver
uses the project helper to initialize a fresh SDK project, overlays the frozen
model/header plus this revision, and builds PAL/NTSC release ROMs. They are saved
under `build/design-adaptation-ui-integration/last-service-U.bin` and
`last-service-E.bin`. Generated projects, ROMs and logs remain ignored.

Original UI/game code follows the repository MIT license. The SDK's default font,
startup and runtime retain the license recorded in `../submission/LICENSE-SGDK.txt`.
