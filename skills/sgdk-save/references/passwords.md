# Passwords without battery-backed storage

## Make the code fit the game

A password can restore a checkpoint, lives and score without an SRAM cartridge.
It should encode only progress that makes sense to resume. It cannot restore an
arbitrary object graph or promise to prevent cheating.

Save Lab uses twelve characters from
`ABCDEFGHJKLMNPQRSTUVWXYZ23456789`. It omits `I`, `O`, `0` and `1` to reduce
lookalike mistakes. The decoder accepts lowercase ASCII too. Input is exactly
twelve characters, without separators; output is always uppercase.

```c
char code[SAVE_PASSWORD_LENGTH + 1];
SaveState candidate;
if (save_password_encode(&progress, code)) {
    /* Display code; the final byte is a C string terminator. */
}
if (save_password_decode(entered_code, &candidate)) {
    progress = candidate;  /* SRAM is unchanged until you explicitly save. */
} else {
    /* Keep the current progress and let the player correct the code. */
}
```

These are lab helpers, not SGDK functions. The API expects an ordinary terminated
C string with enough readable storage. Validate external buffer lengths before
passing raw data as a string; there is no allocator or unbounded text copy here.

## Bit layout

Each alphabet character carries five bits. The first eight characters encode
five bytes, most-significant bit first:

| Bits | Meaning |
| --- | --- |
| 0..3 | Version 1 |
| 4..9 | Level, 1..63 |
| 10..13 | Lives, 1..9 |
| 14 | Music option |
| 15..34 | Score, 0..999,999 |
| 35..39 | Reserved zeros |

The last four characters carry the same CRC-16 used by the SRAM example over
those five bytes, followed by four zero padding bits. Both reserved and padding
bits are checked. For level 7, lives 5, music on and score 123,400, the code is
`CHMD2SJAZ4UA`. No 64-bit arithmetic is needed to pack the fields on the 68000.

Unknown versions, out-of-range values, invalid characters, incorrect lengths,
noncanonical padding and CRC mismatches are rejected without changing the
caller's state. Every single-character substitution of the documented vector
is tested. This does not make passwords secret or rule out checksum collisions.

## Keep controller entry forgiving

The lab has a cursor moved with Left/Right. Up/Down cycles its character; A
applies the code and B cancels. It begins with the current state's generated
password so a roundtrip is easy to try. A rejected code stays in the editor for
correction. A game can instead use a character grid and erase button.

Keep decoding separate from saving and game transitions. Show a clear error,
leave progress alone on failure, and let the player try again. If the game's
password layout changes, retain an explicit old decoder or explain which
version a code belongs to; never reinterpret fields silently.

## Implementation

- [Fixed-size encoder, decoder and CRC](../assets/save-lab/src/save.c)
- [Controller entry example](../assets/save-lab/src/main.c)

The alphabet, twelve-character format and input layout are optional game design
choices. SGDK does not supply a universal password codec.
