---
name: sgdk-pixellab
description: Generate and refine Mega Drive game art through PixelLab MCP with native pixel dimensions, cohesive palettes, prompt iteration and SGDK asset validation. Use for new sprites, UI, tilesets, maps and title art, rather than importing existing art alone.
---

# SGDK PixelLab

Turn the game's art brief into a measurable asset contract before spending
credits. Preserve the artist's style; the included study demonstrates several
styles and does not define how another game should look.

## Before generating

Look for existing palettes, assets, camera conventions and art references in
the project. If palette/style preferences remain unclear, ask the artist.
When they have none, propose a few suitable [Lospec palettes](https://lospec.com/palette-list)
with previews. Read [palette planning](references/palettes-and-validation.md)
before converting one: a sixteen-color palette is not automatically a suitable
Mega Drive palette or a complete scene palette plan.

Establish the native canvas and occupied area, facing/projection, frame anchor,
transparency, palette role and intended background. A single object, terrain
sheet and full title illustration need different prompts. Read
[prompting and tool selection](references/prompting.md) for templates and the
live-tool limitations found in our trials.

Use the available PixelLab MCP tools and their current schemas. Check the
[official MCP docs](https://api.pixellab.ai/mcp/docs), tool receipts and account
balance when costs matter. Respect the user's generation allowance, including
candidate batches, animations and cleanup calls; stop before exceeding it.
Submit a small representative batch, inspect it, then change the specific
failure. More prompt prose is not a substitute for hard palette controls.
Do not repeatedly regenerate an almost-correct icon when a few exact edits
would finish it. Poll existing jobs instead of submitting duplicates.

## Review and finish

Inspect the original at actual size and an integer nearest-neighbor zoom.
Verify dimensions, silhouette, perspective, facing, alpha, palette and frame
alignment. Test sprites over their intended scenery. Preserve raw outputs,
exact requests, selected candidates and edits so the artist can see how the
final asset was obtained. Treat tile-edge similarity and isolated-pixel counts
as review hints; neither establishes that art is seamless or wrong.

For large gradients, compare clean color bands, authored dithering and gentle
ordered palette reduction when useful. Choose what looks right for the game.
Dithering is optional: it can obscure silhouettes, create scrolling shimmer,
look noisy and increase unique tiles. Avoid making it a default for sprites,
UI, flat areas or backgrounds where the undithered version reads better.

Do not smooth-resize larger art into the working dimensions. Regenerate or
rework on the native grid. Cropping empty canvas or adding transparent padding
is different from rescaling: verify that no subject pixels are lost and update
anchors deliberately. Quantize all related frames against the same explicit
palette, preserve opaque colors independently of transparent index zero, and
review the converted image again.

The self-contained [asset helper](scripts/art_pipeline.py) inspects, prepares
and validates indexed exports without resizing. Read its
[contract and validation guide](references/palettes-and-validation.md) before
using it; it supports a deliberately limited PNG subset and one palette line
per export. It does not certify visual quality or an entire game's graphics
budget. Use the real ResComp and linked-resource checks after preparation.

## Evidence and integration

[The prompt study](assets/prompt-study/README.md) includes exact requests,
raw outputs, all Pro candidate overviews, assessments, explicit corrections,
final assets and a local comparison gallery. Its findings are a small sample,
not a benchmark or a promise of repeatability.

Use `ewrogers/sgdk:latest`, pulled before validation, and verify resource syntax
in the selected image's `bin/rescomp.txt`. Budget actual frame tiles, palettes,
VRAM, transfers and hardware sprite pieces separately. Read
[integration checks](references/integration.md) for the tested boundaries.
`sgdk-assets`, `sgdk-graphics` and `sgdk-levels` can help with wider integration
when available; no other installed skill is required by these helpers.
