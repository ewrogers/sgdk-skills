# Check translated layouts

A sentence that fits in English may occupy another line in French or Spanish.
Changing the font can change that again. Measure the renderer's output in the
actual box; source byte counts and a translation-length percentage are only
rough hints.

## Define the box and substitutions

For fixed 8x8 fonts, record width in glyph cells, rows per page and maximum
pages. For proportional fonts, use measured advances/pixel bounds instead.
Include portraits, nameplates, choice cursors and page indicators in the screen
layout; fitting the text alone does not prove those elements stay separate.

Test maximum permitted runtime substitutions as well as the authored string.
An eight-glyph accented name is eight cells, even if its UTF-8 representation
takes more bytes. A grouped number may be longer than its digits. Language Lab
uses a nonbreaking space in French numbers, so a group cannot split across rows.

Specify handling for explicit line breaks, long words and blank lines. The
Lab wraps at ordinary spaces, preserves hard breaks and rejects a word wider
than the box. It has no automatic hyphenation or Japanese line-breaking rules.
Its katakana text includes authored spaces/breaks. Real Japanese text needs
appropriate punctuation/line-start rules; do not infer full support from that
small example.

Do not silently clip a translation to make a build pass. Revise the layout,
choose approved shorter wording, add pages, or adopt a suitable font/layout
system. Keep that choice consistent with the game's design and translator's
intent.

## Change language without breaking UI state

Keep semantic menu selections, not an index into a translated list that may
have changed order. Preserve quantities, names and pending choices. Recompute
layout when the language changes. For dialogue, choose whether to return to
the start of the current message or preserve a semantic progress marker;
raw glyph offsets can land inside a different word or page.

Language Lab keeps the current menu/keyboard/quantity and restarts reflowed
dialogue at page one. Each draw constructs the full 40x28 tilemap, so shorter
text replaces older letters. A changed screen queues copied data for VBlank;
idle frames do not upload it again. For a larger game, dirty fields/regions
may be preferable to this small Lab's whole-screen update.

## Test the implementation and the artifact

- Compile every catalog. Reject missing keys, duplicate IDs, placeholder drift,
  unknown glyphs, invalid controls, font overflow and expanded layout overflow.
  Check NFC-equivalent input and reordered source lists for stable output.
- Execute formatting/wrapping on actual game code. Include zero/max values,
  short buffers, maximum names, exact line/page boundaries and long words.
  Compare runtime line widths with the compiler's layout report.
- Compile the real ResComp atlas with optimization disabled. Check its tile
  count and linked pixels; a source PNG check alone cannot detect reordered
  resource tiles. Run relevant rules on base M68000 as well as host when useful.
- Check every screen/language in an emulator, including transitions and reset.
  Native-speaker and hardware/display review remain separate acceptance work.

The Lab's CPU probe preserves its direct-entry ABI with GCC's `noipa` attribute.
SGDK startup ignores `main`'s result, so LTO can otherwise specialize an entry
whose return register a test harness tries to inspect. The probe checks logic
and linked data; it does not emulate the VDP or physical SRAM writes.

## Sources and example

- [Language Lab compiler and controls](../assets/language-lab/README.md).
- [SGDK 2.11 copied tilemap transfers](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp_tile.c)
  and [DMA queue implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/dma.c).
- [GCC function attributes](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html)
  documents `noipa`; it is used for the test entry, not as a game optimization default.
