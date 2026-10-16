# Prompting and choosing the tool

Consult the [live MCP docs](https://api.pixellab.ai/mcp/docs) and current tool
schema. The following was checked on 2026-10-15. Tool names describe the MCP
interface, not REST paths. Use MCP to generate; download only the returned
asset URLs. Jobs are asynchronous. Keep their IDs, wait for completion and
read the reported cost/frame count before queuing more work.

## Match controls to the asset

| Tool | Useful controls and observed limits |
| --- | --- |
| `create_image_pixen` | Low-cost small sprites. Axes 16–768, multiples of four, area up to 512×512; below 32 either axis forces a square canvas. No forced-palette argument in this schema. |
| `create_image_pixflux` | Palette PNG, image input, facing/view and shading. Axes 16–400, total area at least 1,024 pixels. A 16×16 or 32×16 request is invalid. |
| `create_image_pro` | Hex palette, maximum colors and labelled references. Live validation requires axes at least 16. Small canvases return many candidates: both our 16×16 and 32×16 calls returned 64 for ten generations each. |
| `create_image_pro_flash` | Single-image creation and optional style reference. Query `get_pro_flash_capabilities` for current native/custom sizes and provisional costs; not evaluated in this study. |
| `create_topdown_tileset` / `create_sidescroller_tileset` | Dedicated connected terrain, 16- or 32-pixel authored tiles. No hard palette field in the tested schemas. Keep placement metadata. |
| `pixelart_workbench` | Free exact drawing/editing on a native grid. Read `describe start`, then the relevant topic before use. We drew an exact 8×8 tile here. This is authored geometry, not an 8×8 generative model. |

Do not promise native AI-generated 8×8 art with the tested tools. Pro rejected
8×8 before generation. Prefer exact pixel drawing for tiny tiles/icons, or an
artist-approved pixel-preserving crop when the complete subject already fits.
Never generate at 32×32, shrink it to 8×8 and call that native generation.

A schema can lag its server. Our top-down call advertised `seed` in the tool
schema but rejected it; the successful retry omitted it. Record such a failure
and adapt to the live response rather than repeatedly paying for new jobs.
A seed aids controlled comparisons but does not guarantee future identical
outputs across service/model changes.

## Build a compact prompt

State the subject and its function, then projection/facing, native dimensions,
occupied area/anchor, major identifying shapes, and only the relevant style
constraints. Put palette colors in a supported palette field. Describe a few
unwanted failure modes rather than a long generic negative-prompt list.
Structured direction and written facing must agree. Exact coordinates in text
are a preference to inspect, not a bounding-box enforcement API.

These are templates; replace the creative details with the game's brief:

- **Character:** “One [character], strict [view], facing [direction]. Native
  [W×H] canvas, complete body with [margin], feet at [anchor]. [Two or three
  identifying shapes]. [Project outline/shading]. No ground or unrelated props.”
- **Small object:** “One [item], [orientation], complete [silhouette] inside
  [occupied dimensions]. Native [canvas]. [Readable feature]. Transparent
  surroundings; no detached particles or cast shadow unless the brief needs them.”
- **UI:** “One empty [panel/button], native [dimensions], front view. [Frame
  thickness/corners] and quiet interior. Symmetric [edges]. Text added separately.”
- **Parallax cutout:** “One continuous row of [structures], strict orthographic
  side elevation. Native [canvas]. Transparent sky above the silhouette. No
  road or vanishing point.” Request a layer, not an entire scene with exclusions.
- **Full backdrop/title:** “[Setting] at native [dimensions]. [Large compositional
  masses], quiet [region] for the title/menu added later. [Project style].”
  Add sparse dithering only when the visual brief benefits; inspect any invented
  text or signature-like marks even after explicitly requesting none.
- **Tileset:** Describe each terrain material and the boundary separately.
  Prefer simple connected shapes first; decorate after testing representative
  corners and repeated interiors. Collision is authored separately from texture.

## Iterate on the demonstrated failure

In our robot trial, “strict side view” plus numeric occupied bounds still
produced a front-facing pose. Naming visible anatomy helped the next result:
“visor only on the right, backpack on the left, both toes point right.” This
is a small observation, not a guaranteed formula; the retry also used a new seed.

Our more specific key prompt improved some shape details but increased colors
from 30 to 32. Color names and “three shades” were soft guidance. The Pro
palette call returned five opaque colors in the selected candidate; the cost
bought a candidate set, not a guarantee that every icon had good margins.
We chose candidate 5 after checking its silhouette and border.

The wide Pro reference trial returned 64 panel-like crops rather than a
convincing closed chest. Reject that result for this role. The accepted 32×16
chest is a literal crop of the empty rows around the successful 32×32 source;
all subject pixels survive. Label that operation honestly.

## Cohesion and animation

Use an approved base asset and explicit style/reference controls where the
chosen tool supports them. Say which traits to copy: color relationships,
outline weight and cluster density are different from copying the subject.
A reference cannot replace the target canvas, anchor or projection contract.

For `animate_image`, describe motion rather than redesigning appearance.
Our four-frame request returned a nominal source frame plus four generated
frames. The tool described frame zero as unchanged, but comparison found fourteen
visible pixel changes (including transparency) and normalized hidden RGB. Keep
your actual input separately and compare it; the prepared sheet uses that actual
original at the start. Record the selected playback order, and compare
occupied bounds and feet across the loop. The robot's bottom bound stayed at
row 29, but its body/limbs moved more than a subtle idle request suggested.
This clip suits an active hover/fidget better than a restrained idle; decide
with the artist before integrating it as a character's default pose.

Batch palette reduction across all directions/frames, or use the same explicit
palette for each. Never independently auto-quantize each frame: it can cause
color flicker. Color correction does not fix identity changes or bad motion.
