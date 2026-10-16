# Validating dialogue

Test the progression model separately from SGDK drawing. Confirm should reveal
one page, then advance on a new press. Test a held button across opening,
choices, pause and return to gameplay. Include a last page, an explicit page
break, a long wrapped word, an unnamed speaker, no portrait and two retained
panes. Test choices with zero, one and several visible entries, flag changes,
replay and intentional fallback when all choices are hidden.

Reject unsupported glyphs/tags, mismatched style nesting, missing node targets,
invalid portrait IDs, oversized labels, empty trailing pages and excessive
fast-reveal work during authoring. Cyclic branches can be valid; validate their
intended exits in game-specific tests. Borrowed immutable C data is not an
untrusted binary-file parser. Adding runtime-loaded conversation data needs
bounds-checked decoding before pointer-based model access.

Compile the real renderer/resources with the selected SGDK. Check the linked
font length, portrait geometry, actual frame tile/piece counts, shared palettes
and reserved tile end. A nominal 32×32 source image is not proof of a particular
ResComp sprite layout. Check both normal and debug builds.

The repository's `tests/test_dialogue.py` runs portable C at `-O0` and `-O3`,
UBSan on Unix, 30,000 bounded graph steps, nine-slice geometry, nominal PAL/NTSC
clocks, pause and single-use input. It executes the real sound adapter against
a recording SDK to check cue priority and idle/paused ending behavior. Native
art/audio reproduction, palette/size checks, finite PCM endpoints, FM loop
commands, generated C strings and safe regeneration are tested separately.

Opt-in Docker checks build standalone release/debug ROMs and a direct-entry
probe against the actual SDK. Unicorn 2.1.4 executes the probe as base M68000,
checking authored progression, resources and target structure sizes. The probe
skips console startup and supplies SGDK 2.11's default tilemap boundary for its
placement check. It does not emulate VDP, DMA timing, Z80 playback or physical
input. CI requires matching ROM hashes and resource/structure reports on native
ARM64 and AMD64. Host helpers also run on Windows, macOS and Linux.

Finish with the [Lab playtest](../assets/dialogue-lab/README.md#playtest).
Local NTSC ares testing confirmed readable dialogue, controls, portraits and
the music/ending-cue fix. Nominal PAL timing tests do not establish PAL visual
layout or sound on a console. Check those separately, especially when mixing
dialogue transfers with a scrolling level, portrait streaming or other audio.
