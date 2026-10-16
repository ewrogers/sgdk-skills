# Authoring conversations

The helper and C model in this skill are original example code, not SGDK APIs.
Start with the [Lab's JSON](../assets/dialogue-lab/res/conversations.json),
[public model](../assets/dialogue-lab/inc/dialogue.h) and
[compiler](../scripts/compile_dialogue.py).

## Graph and ownership

A program contains `flags`, `speakers`, `stories` and `nodes`. IDs use lowercase
letters, digits and underscores, beginning with a letter. Stories name an entry
node. A speaker has an ID, a display name (possibly empty), and portrait `0`, `1`
or `null`. Those portrait IDs refer to the Lab's two resident resources; expand
the renderer and validation together when adding more.

Each node specifies its speaker, text and optional next node. `pane` is `top`
or `bottom` (default bottom); `split: true` retains the other pane's last page.
Otherwise a new speaker clears the other pane. `nameplate: false` hides the
speaker label independently of portrait selection. No portrait gives text more
columns. An absent/null `next` ends the exchange after its final page is accepted.

A node may offer up to three choices. Each choice has `label`, `target`, and
optional flag lists: `require`, `forbid`, `set`. Required flags must all be set;
forbidden flags must all be clear. Selecting a visible choice sets its flags
and enters its target. Up/Down wrap over visible choices. If none are visible,
confirmation follows `next`, or ends if there is no next. Author that fallback
deliberately; the validator does not infer narrative intent or prove termination
of cyclic graphs.

The model borrows immutable compiled nodes, token arrays and strings. Keep them
in ROM or another valid lifetime for the whole exchange. `dlg_start` takes
initial flags; `dialogue.flags` holds the result. The Lab resets flags on replay.
A game can instead copy selected results to its quest/save state. These sixteen
flag bits are this example's capacity, not a console or SGDK restriction.

## Text, styles and pacing

The helper supports printable ASCII plus newlines. Its artwork uses uppercase
glyph shapes, including for lowercase character slots. Other languages need an
explicit encoding, glyph atlas and width policy; byte-by-byte UTF-8 is not a
working localization strategy.

| Markup | Effect |
| --- | --- |
| `[color=gold]key[/color]` | White, gold or cyan ink. |
| `[font=soft]whisper[/font]` | Normal or the Lab's slanted/bold alternate face. |
| `[pause=350]` | Wait 350 ms before continuing (1–5,000). |
| `[speed=30]` | Set subsequent glyph delay in ms (0–1,000). |
| `[br]` or a newline | Begin a text row. |
| `[page]` | Wait for confirmation before a new page. |
| `[[` | A literal opening bracket. |

Styles nest with matching closing tags. The helper wraps words to 29 columns
with portraits or 34 without, four rows per page, splitting oversized words.
It inserts page boundaries and rejects empty trailing pages, unknown tags,
unsupported glyphs and excessive work. Styling changes ink/font tiles, not a
new CRAM palette for each word. It does not substitute runtime variables or
execute expressions. For a variable name or number, build bounded glyph tokens
before opening the page, then validate the new layout and keep its storage
alive; never splice unchecked strings into fixed cell buffers.

Confirm during typing reveals only the current page and skips cosmetic delays.
A second press advances or selects. Default delay is 30 ms per glyph; a speed
change lasts for the rest of that node unless changed again. The next node
starts at the default. The active pane shows an animated arrow for another
page/node and a square at a terminal page. Choices have their own cursor.

## Compile and integrate

Run from the installed skill folder, using a fresh destination first:

```sh
python scripts/compile_dialogue.py assets/dialogue-lab/res/conversations.json
python scripts/compile_dialogue.py assets/dialogue-lab/res/conversations.json --out /path/to/new-project
```

The first command validates only. The second writes `inc/dialogue_data.h` and
`src/dialogue_data.c`; deliberate regeneration needs `--force`. Invalid input
is rejected before either file is written. Treat ordinary filesystem write
failures separately; output is not a transactional project update.

The helper permits at most 64 nodes, 16 flags, 16 speakers, eight story entries,
three choices/node and 2,048 tokens/node. Each page has at most 256 actions so
fast reveal cannot traverse unlimited tags in one frame. Titles, labels and
names have explicit lengths. The Lab menu is laid out for its three stories;
adding more entries also needs a scrollable or paged menu.

Call `dlg_step` with elapsed milliseconds and button press edges. The Lab
accumulates fractional milliseconds for nominal 50/60 Hz display frames and
samples cached pad input once. It freezes its clock while paused. This is not
a missed-frame recovery clock; integrate the game's timing policy when frames
can be skipped. `elapsed_ms` is capped at 1,000 and work remains bounded. Events
are cleared every step, including after completion, so an ending sound or
quest effect is not retriggered by idle frames.
