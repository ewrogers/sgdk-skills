# PixelLab and SGDK: production notes

Working observations from Shooter Lab and Carpet of the Crescent, recorded on
2026-10-15. These are inputs for a future PixelLab/SGDK skill, not a finished
skill or a claim that every PixelLab model behaves the same way. Recheck tool
schemas when implementing that skill.

## What has worked

PixelLab has produced useful native-resolution art: 16×16 spells, 24×24 enemies,
32×32 characters and 64×64 bosses. Its small sprites can have clear silhouettes
and deliberate pixel clusters. We have not needed to generate a large painting
and shrink it into a sprite.

Shared palettes and consistent descriptions helped the courier, bosses and
scenery feel related. Palette-controlled generation proved more dependable
than naming hex colors in a prompt. Hovering animation preserved the basic
character designs and produced useful subtle scarf, carpet, wing and smoke
motion. Those observations apply to the frames inspected so far; they are not
a blanket guarantee of drift-free animations.

Generated art still needs visual review in the game. A technically valid sprite
can have poor contrast, a confusing silhouette or too much detail at its display
size. A palette preview alone cannot establish gameplay readability.

## Measured palette behavior

These counts come from the original downloaded PNG pixels before our importer
normalized them. Pixels with alpha below 128 are excluded from opaque-color
counts. “Outside palette” compares exact RGB values against our supplied or
requested 16-color plan.

| Original asset | Tool/control | Native size | Opaque colors | Opaque pixels outside plan |
| --- | --- | --- | ---: | ---: |
| Courier | Pixflux, forced palette image | 32×32 | 16 | 0 / 430 |
| Brasswing | Pixflux, forced palette image | 64×64 | 13 | 0 / 1,112 |
| Bazaar far layer | Pixflux, forced palette image | 256×192 | 6 | 0 / 49,152 |
| Revised bazaar foreground | Pixflux, forced palette image | 256×96 | 15 | 0 / 16,637 |
| Teapot | Pixen, colors named in text | 24×24 | 21 | 220 / 220 |
| Sun spell | Pixen, colors named in text | 16×16 | 21 | 57 / 57 |
| Moon spell | Pixen, colors named in text | 16×16 | 20 | 48 / 59 |
| Storm spell | Pixen, colors named in text | 16×16 | 26 | 44 / 56 |

This is a small sample, not a model benchmark. In the exposed Pixen tool schema
there was no forced-palette parameter; we supplied colors in its description.
Those results should not be presented as failures of a hard palette control
that was never available or used. Pixflux's forced palette matched exactly in
the samples above.

Matching RGB colors also does not establish palette-index compatibility. An
opaque black pixel is not interchangeable with a transparent pixel. Our
importer assigns transparency to index 0 and maps opaque sprite pixels only
to indices 1–15, even if their original RGB was black.

## Prompting versus generation limitations

Our first near-background requests mixed “bazaar scenery” with parallax,
empty-space and transparency requirements. One result was a perspective street
rather than a side-scrolling layer. Several environment requests returned fully
opaque images despite `no_background=true` and explicit empty-sky instructions.

A narrower request worked better: **one isolated continuous row of buildings,
strict orthographic side elevation, no road, no vanishing point, transparent
sky above the skyline**. The revised bazaar layer has 7,939 transparent pixels
in its 256×96 canvas.

More wording is not necessarily better. Establish the asset's role, projection,
silhouette and occupied area first. Add decoration afterward. For environment
cutouts, avoid asking for a complete scene and then trying to exclude most of
that scene in the same prompt.

A facing mismatch in this batch was our mistake: the written boss description
said left while the request's direction field said east. Do not blame the
service for conflicting controls. Validate prompt text and structured
parameters together before submission.

Composition, empty regions and seamless edges remain things to inspect rather
than assume. No systematic seam-quality test has been completed for these
backgrounds yet. Do not describe requested seamlessness as verified output.

## Useful tool distinctions

Observed MCP capabilities during this work:

- **Pixflux:** accepts a forced palette PNG, shading controls and image input.
  Its canvas has a minimum total area of 32×32, so a 16×16 request is not valid.
- **Pixen:** supports native 16×16 and 24×24 sprites. Below 32 pixels, the
  dimensions must be square; dimensions otherwise follow its multiple-of-four
  rules. The exposed schema did not offer hard palette enforcement.
- **Pro:** its exposed schema accepts explicit hex palette colors and a maximum
  color count, and returns multiple candidates at small sizes. We have not
  evaluated those results in this Carpet Lab batch.
- **Animate Image:** returns the unchanged input plus generated frames. A
  request for four generated frames therefore returns five images. Do not
  silently omit the input frame or assume the returned count is four.
- Jobs are asynchronous. One request hit an eight-active-job limit; waiting
  and retrying resolved it. Use the job wait/status tools rather than blindly
  resubmitting requests or assuming a failed job consumed a generation.

Store generation IDs, prompts, dimensions, intended palettes and any chosen
frames. Generation IDs are provenance; they are not a substitute for committing
the final assets needed to build the project.

## Console preparation we still perform

1. **Plan the whole scene.** Allocate the four VDP palette lines across world,
   actors, UI and effects before generating isolated assets. Each line has
   sixteen entries; sprite transparency uses entry 0, leaving fifteen opaque
   colors. Do not give every sprite an independent sixteen-color palette.
2. **Choose actual VDP colors.** Use colors aligned with the console's channel
   levels. Distinguish the logical RGB representation from the observed analog
   display ramp; do not claim equal numerical steps look equally bright.
3. **Generate at native resolution.** Reject the wrong dimensions. If art must
   change scale, rework or regenerate it at the target resolution rather than
   applying smooth image resizing.
4. **Inspect alpha and composition.** Check that supposedly empty areas really
   are transparent, that outlines survive, and that the subject is not cropped.
   Avoid a blanket color key that removes legitimate interior pixels.
5. **Export indexed PNGs.** Preserve the intended palette order and reserve
   transparent index 0. Validate opaque color counts and alpha decisions.
   Palette conversion can merge useful shades, so review the converted result.
6. **Pack animation without resizing.** Require consistent frame dimensions and
   anchors. Assemble the native frames and keep a stable order. Review loops
   for shifting bodies, edge artifacts and inconsistent silhouettes.
7. **Compile with ResComp.** Check actual tile counts, palette reuse and hardware
   sprite decomposition. A 64×64 boss needs multiple hardware pieces; a logical
   image or animation frame is not one hardware sprite by definition.
8. **Budget the busy frame.** Check main RAM, VRAM, resident animation frames,
   DMA, total sprites and scanline pressure separately. H40's 80-sprite total
   does not remove its 20-sprite and 320-sprite-pixel per-line limits.
9. **Playtest.** Check sprites against the actual scenery, moving parallax,
   projectile visibility, animation timing and audio. A valid PNG and successful
   ROM build do not prove those qualities.

The current importer uses a fixed alpha threshold and nearest-palette mapping,
with no resizing or dithering. Those are explicit choices for these examples,
not automatically the best choices for every game. It refuses to overwrite an
existing export directory. Its PNG reader validates checksums and supported
formats; meaningful tests are still required before treating it as a reusable
skill helper.

## Suggested feedback for PixelLab

The most useful improvement for this use case would be a console-target export
profile with:

- Hard palette enforcement available on the small-sprite generator too.
- Reserved transparent index and separate handling for opaque black.
- Exact native frame dimensions, anchors and optional tile-grid constraints.
- A report showing color counts, off-palette pixels and alpha usage.
- Reliable isolated foreground-layer generation with explicit empty regions.
- Seam previews and edge validation for scrolling backgrounds.
- Animation contact sheets and anchor/occupied-bounds comparisons.

The distinction matters: the service is already producing attractive pixel art.
The missing convenience is a reliable path from that artwork to a constrained
console asset, with explicit checks rather than additional prompt prose.

## Starting point for the future skill

Compose it with `sgdk-assets`, `sgdk-graphics` and `sgdk-performance` where
available. Keep the PixelLab-specific generation workflow separate from the
hardware accounting and actual SGDK resource API.

Use asset-specific prompt templates for actors, small projectiles, background
cutouts and full backdrops. Include a request lint step for conflicting facing,
invalid canvas sizes and unsupported palette controls. Follow generation with
validation and a human-visible preview; never mark art compatible solely because
the prompt requested compatibility.

Current local implementation and provenance are under
`skills/sgdk-shmup/assets/carpet-lab/tools/` and
`skills/sgdk-shmup/assets/carpet-lab/res/art-provenance.json`.
