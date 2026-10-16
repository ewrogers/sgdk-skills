# Catalogs, substitutions and language preferences

Keep a text key for each complete message. A key such as `item_found_many`
is easier to maintain than scattered language branches in the game loop.
Translations can change word order, punctuation and spacing; concatenating
“found” + quantity + item usually preserves the wrong grammar somewhere.

## Keep IDs separate from array order

Use explicit stable IDs when messages or language choices enter saves, content
scripts or other persistent formats. Reordering a JSON list must not turn a
saved French preference into Spanish. Build lookup tables or an ID-to-index
mapping, and define what an unknown ID does.

Language Lab persists the language's explicit ID, then maps that ID to a table
index. Invalid stored IDs are rejected and the caller chooses English. That
default is a project policy, not SGDK behavior. Merge a real game's preference
into its existing versioned save/options record; do not claim an unowned SRAM
range for a second save system.

The Lab's six-byte record has a CRC and commit marker, written last. Interrupted
writes can lose the preference and return to the default; they cannot become a
valid partial selection. It deliberately does not provide old-slot recovery or
campaign persistence. Use a transactional save scheme when retaining the older
record matters.

## Author Unicode, emit the renderer's representation

UTF-8 is useful for catalogs and translator tools. It need not be the runtime
encoding. The Lab normalizes source text to NFC, validates an authored glyph
for every character, then emits compact glyph IDs and control tokens. For
example, precomposed `é` and `e` plus a combining acute accent compile to the
same tile. The runtime does not decode arbitrary UTF-8 or combine accents.

Choose a policy for user-entered or externally supplied text too. A fixed
glyph keyboard can guarantee coverage. A free-form name needs decoding,
normalization, length/character validation and a deliberate unsupported-glyph
policy. Do not cut a UTF-8 name at an arbitrary byte boundary.

## Validate the message contract

Name the allowed substitutions and their types. Language Lab supports only
`{name}` and `{count}`; a translation must use the same placeholder set as the
default. It rejects format specifiers and conversions rather than forwarding
them into C formatting. Literal punctuation such as `%` is ordinary text.

Bound each substitution before measuring or rendering it. The Lab permits
eight name glyphs and unsigned 16-bit counts. Its maximum grouped count takes
six glyphs. The compiler checks those expansions, and the runtime refuses
too-small output buffers without changing them.

Select grammatical variants deliberately. The Lab demonstrates `found_one`
versus `found_many`; that is not a universal plural rule. Languages may need
zero/few/many forms, classifiers, grammatical gender or different item names.
Use a plural/message system appropriate to the requested locales rather than
extending English suffix concatenation.

Number separators also belong to locale policy. The examples use comma,
nonbreaking space, period or no grouping. They do not implement currencies,
dates, decimal fractions or a complete internationalization library.

## Make fallback an explicit decision

Fail missing translations by default while preparing a release. During partial
translation work, an explicit per-message fallback can be useful. Report which
language/key fell back, keep its placeholders/layout checked and make release
completeness visible. A default-language fallback must not silently disguise a
missing localization as a passed translation check.

The [catalog compiler](../assets/language-lab/tools/catalog.py) demonstrates
these decisions in a small standard-library helper. Its schema, limits and
codec belong to the example, not to SGDK. Adapt it to an existing catalog
instead of forcing another game's data into the Lab's exact shape.

## Sources

- [SGDK 2.11 text implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp_bg.c)
  maps bytes to glyph tiles; it supplies no catalog or Unicode conversion.
- [Unicode normalization specification](https://www.unicode.org/reports/tr15/)
  defines NFC. Normalization is distinct from translation, shaping and font coverage.
- [SGDK 2.11 SRAM API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sram.h)
  exposes byte offsets and read/write enable operations. Save formats remain game code.
