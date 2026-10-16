# PixelLab → Mega Drive prompt study

Open [the comparison gallery](gallery.html) locally. It shows exact prompts and
controls, original images, assessments, additional work and the prepared asset
chosen for each role. Preview at native size or an integer zoom. Both Pro
batches include all 64 candidates in row-major contact sheets; key candidate 5
was selected, and the wide chest batch was rejected.

The 2026-10-15 study used **42.3 generations** against an allowance of 100,
measured from account usage 41 to 83.3 after completion. There are no pending
jobs and no additional generation is needed to build or test these files.
Costs and exact tool receipts are in [requests.json](requests.json). Those are
observations for this run; future tool pricing and behavior may change.

## Coverage and findings

| Trial | What it established |
| --- | --- |
| 8×8 masonry | Pro rejected 8×8 generation. PixelLab workbench produced an exact native tile from a grid recipe, free. |
| 16×16 objects/character | Specific wording helped some shapes but did not enforce colors. Pro palette controls gave a usable six-color-budget key; select among candidates. |
| 32×32 robot | Numeric bounds/facing requests were not hard constraints. Describing visible anatomy improved the profile. |
| 32×16 chest | The Pro reference batch missed the role. A pixel-preserving crop of empty rows around a successful 32×32 source gave the usable prop. |
| 64×32 UI | Ornate and minimalist examples differ. Palette conversion is necessary for the unforced minimalist frame; neither is a validated nine-slice kit. |
| 256×128 background | More specific composition was quieter, but the output was not seamless despite that request. |
| 320×224 title illustration | Useful negative space; invented signature-like pixels required an explicit native edit. Text/logo should be separate assets. |
| 320×192 painted-pixel background | Compare clean bands and 2×2/4×4 reductions. Dithering is optional; 2×2 suited this image, while 4×4 felt busier. |
| 16×16 top-down terrain | Returned corner/bounding-box metadata made native map reconstruction possible. First path shape exposed missing configurations; axis-aligned edits fixed the preview. |
| 32×32 side terrain | Strong edges, but palette consolidation was still needed. Authored cells decompose into 8×8 hardware patterns. |
| Five-frame 32×32 animation | Four requested frames yielded the nominal source plus four generated frames; frame 0 changed fourteen visible pixels. Keep the actual input separately. The motion exceeded “subtle breathing”; treat it as a fidget candidate. |

These are small practical trials, not a controlled benchmark. Paired initial
prompts shared seeds, but also changed structured controls. Later corrective
prompts sometimes used new seeds. Do not claim that a phrase alone caused an
improvement or that one failed batch establishes a model-wide limitation.

The styles and palette choices here are illustrative. Preserve the artist's
existing brief and palette in a real project. The terrain and desert examples
have their own palette plans; they are not all intended to share one scene.

## Files and reproducibility

- `raw/`: original selected outputs, both full candidate overviews, tile metadata
  and the final map's corner layout. Source frames remain native.
- `contracts/`: expected dimensions, palette and transparency roles.
- `reports/`: source/prepared pixel statistics, changed RGB counts and exact
  title-corner edit coordinates. Isolated/edge/seam counts are review hints.
- `final/`: prepared indexed exports, including alternatives that were not
  selected for their original role. Consult the gallery before choosing one.
- `reviews.json`: assessments and selection decisions, separate from generated data.

From this directory:

```sh
python rebuild.py
python rebuild.py --out /path/to/new-folder
python build_gallery.py
```

The first command checks all prepared assets reproduce offline. The second
requires a new directory. The third regenerates the local HTML gallery from
committed requests, reviews and reports. No service calls or image packages are
needed. `rebuild.py` uses helpers inside this installed skill only. The map and
animation sheet checks read committed prepared tiles/frames as inputs; the
individual outputs are verified as part of the same run.

The 32×16 crop removes only transparent rows; the title correction clears the
small generated mark in a recorded uniform corner. Nothing is smooth-resized.
The 8×8 grid recipe is recorded explicitly in `requests.json`.

The indexed outputs and the gallery are not an emulator or hardware playtest.
Repository integration checks compile representative assets through the real
SDK and execute resource assertions as M68000 code. Follow
[the integration guide](../../references/integration.md) for boundaries and
remaining scene/scanline/visual checks. The test ROM is a direct-entry CPU
probe, not a playable showcase ROM.

Original generated artwork, exact pixel work, code and notes are distributed
with this repository's MIT license. PixelLab service terms still govern use
of its service. No Lospec artist palette was copied for this study; a real
project choosing one should retain its name, author and source link.
