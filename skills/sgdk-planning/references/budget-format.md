# Optional budget worksheet

Use this when a few numbers would help compare design choices. A short table in
your game plan is enough to begin; you do not need a completed worksheet before
making a playable prototype.

Copy [budget-example.json](../assets/budget-example.json) into your project and
replace its illustrative values. Run the installed helper by its actual path:

```sh
python3 /path/to/sgdk-planning/scripts/budget_check.py budget.json
python3 /path/to/sgdk-planning/scripts/budget_check.py budget.json --json
```

Use `python` if that is your Python command. Python 3.10+ is sufficient; Docker
and extra packages are not needed. The helper only reads the worksheet.

## What the result means

| Status | Meaning |
| --- | --- |
| `over_limit` | At least one supplied quantity already exceeds its limit. Other costs may still be unknown. |
| `incomplete` | No known overage, but some costs, coverage or limits are still unknown. |
| `within_declared_limits` | The supplied quantities fit the declared limits. This does not prove the game will fit or run well. |

Exit codes are `1` for a known overage, `2` for invalid input or a file error,
and `0` otherwise. **An incomplete worksheet also exits with `0`.** Automation
that requires a complete declaration must inspect the JSON `status` too.

The helper adds supplied quantities. It does not inspect a ROM, SGDK project,
image or scene, infer missing allocations, check VRAM address overlaps, or
predict CPU use and frame rate. Keep evidence and timing probes in the game
plan. A `remaining` value is calculated room in your declaration, not measured
free memory.

## File structure

Use a JSON object with `schema_version: 1` and `video_mode: "H40"` or `"H32"`.
These modes describe standard non-interlaced tiled graphics. Other sections
are optional. Missing quantities and `null` mean **unknown**, not zero.

Quantities must be nonnegative whole numbers. Counts, alignments and supplied
budgets must be positive. Unknown fields and duplicate JSON keys are rejected
so a typo cannot silently remove a cost from the calculation.

### Main RAM and VRAM

`ram` and `vram` each accept `items` and `complete`:

```json
{
  "items": [
    {"name": "Enemy state", "bytes": 24, "count": 32},
    {"name": "Other allocations", "bytes": null}
  ],
  "complete": false
}
```

Each item needs a unique, nonempty `name` and one quantity: `bytes`, or `tiles`
for VRAM only. One 8×8 4bpp tile uses 32 bytes. `count` defaults to `1`; an
optional `note` can explain the estimate. Both memory totals are checked
against 65,536 bytes. Z80 RAM is separate and is not part of this worksheet.

`complete` defaults to `false`. Set it to `true` only when your list covers all
planned allocations and reservations, including SGDK, stack/headroom for RAM,
and tables, system graphics, fonts and sprite reservations for VRAM. It is your
declaration, not something the helper verifies. A `null` item keeps the result
incomplete even with `complete: true`.

Count disjoint allocations. If you include a reserved sprite VRAM pool, do not
add its occupied tiles again. Track occupancy separately in the plan. Shared
sprite definitions in ROM do not automatically share each instance's VRAM.
See [hardware budgets](hardware-budgets.md) for the pinned SGDK behavior.

### Palettes and sprites

`palette_banks` is the number of simultaneously used palette banks, checked
against four. Palette compatibility still needs an art review.

`sprites` accepts these independently supplied peak values:

| Field | H40 limit | H32 limit |
| --- | --- | --- |
| `total` | 80 hardware pieces | 64 hardware pieces |
| `per_scanline` | 20 hardware pieces | 16 hardware pieces |
| `pixels_per_scanline` | 320 pixels | 256 pixels |

Count exported hardware sprite pieces, not game objects. For scanline pixel
usage, count the widths of pieces intersecting the line, including transparent
parts. Check the crowded moment, not the average frame. The helper does not
derive these peaks from positions, clipping or image dimensions.

### ROM and padding

`rom` accepts:

- `bytes_before_padding`: complete ROM payload size, or `null`. This is not an
  ELF size or the sum of source PNG/WAV files.
- `alignment_bytes`: padding boundary, default `131072` (128 KiB), matching the
  pinned SGDK build. Final size rounds up to this boundary.
- `mapping`: `"standard"`, `"banked"` or `"unknown"`, default `"unknown"`.
- `cartridge_bytes`: optional physical cartridge capacity.

Standard mapping is checked against 4,194,304 bytes (4 MiB, or 32 Mbit), and a
smaller supplied cartridge capacity if present. A banked declaration needs a
cartridge capacity to provide a limit. It does not verify the SDK build,
header, mapper, cartridge or emulator support. Unknown mapping stays
incomplete. Padding alignment does not select a physical cartridge board.

### DMA transfers

`dma.peak_bytes` is your estimated or measured busiest frame's transfer bytes.
`dma.budget_bytes` is the transfer budget you chose for that display setup.
Either can be `null`. There is no automatic PAL/NTSC allowance: usable time
depends on the display configuration and other work.

A zero peak is valid. A zero budget is not; an SDK setting that disables a
software limit does not mean the hardware has unlimited bandwidth. Queue
entries, temporary buffers, Z80 bus interaction and transfer scheduling need
their own checks. See [hardware budgets](hardware-budgets.md#dma-and-changing-graphics).

## Machine-readable output

`--json` returns `status`, `video_mode`, `checks`, `unknowns` and `notes`.
Each check contains `name`, `known`, `limit`, `unit`, `complete`, `over_limit`
and `remaining`. An unknown limit produces `null` for `over_limit`.
`remaining` is `null` unless that check is complete. A partial `known` subtotal
may be zero even though unknown costs remain; never present it as the total.
