---
name: sgdk-dialogue
description: Build SGDK dialogue boxes with gradual text, inline styles, animated portraits, choices and branching. Use for NPC conversations, radio exchanges and story panels with bounded layout and predictable controller input.
---

# SGDK Dialogue

Keep authored conversation data, progression and VDP drawing separate. SGDK
provides tile, sprite, input and audio APIs; it does not provide this dialogue
format or a built-in rich-text system. Adapt the included patterns to the
project rather than imposing a new game framework.

- [Authoring and branching](references/authoring.md): graph schema, flags,
  paging, text timing and the optional JSON-to-C helper.
- [Rendering and budgets](references/rendering.md): nine-slice panels, resident
  font styles, animated portraits, split panes and DMA ownership.
- [Validation](references/validation.md): executable checks and playtesting.
- [Dialogue Lab](assets/dialogue-lab/README.md): a self-contained example with
  three conversations, portraits, choices and a quiet looping soundtrack.

Before adding a conversation, establish the visible text area, supported glyphs,
who pauses gameplay, and how confirm/back interact with the existing controls.
Separate “reveal this page” from “advance” so one press cannot skip a choice.
Use press edges for confirmation; give held choice navigation an explicit repeat
policy if the game needs it. Freeze reveal delays and portrait animation with
its owning conversation when paused.

Compile wrapping and markup into bounded tokens where practical. Validate node
IDs, branch targets, page bounds and supported styles before running them.
Keep conversation and string storage alive while the model borrows it. Decide
which flags survive an exchange, a room change or a save; avoid hiding all
choices without an intentional fallback. Give every retained pane clear speaker
ownership and distinguish another page from the end of a conversation.

Reserve UI tiles and palette roles with the rest of the game. Preload a small
portrait set or schedule larger sets explicitly; do not stream arbitrary art
and rewrite every text row each frame. Queue copies for mutable row data, and
drain pending transfers before releasing or repurposing their sources.

Pull `ewrogers/sgdk:latest` before build validation. Verify unfamiliar APIs in
the selected image's `inc/vdp_tile.h`, `inc/vdp_spr.h`, `inc/vdp_pal.h`,
`inc/joy.h`, `inc/sys.h`, `inc/snd/xgm2.h` and `bin/rescomp.txt`. References
use SGDK 2.11; check changes when the image updates. Use `sgdk-ui`, `sgdk-input`,
`sgdk-audio` or `sgdk-scenes` when available for broader work in those areas;
the included example builds independently.

The Lab's harbor setting, portraits, colors and music demonstrate the features.
Follow the user's art, audio and narrative direction for their game.
