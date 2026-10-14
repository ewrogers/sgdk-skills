#!/usr/bin/env python3
"""Check declared static budgets, not game feasibility or runtime performance."""
import argparse
import json
from pathlib import Path
import sys


class BudgetError(ValueError):
    """A worksheet has invalid or ambiguous input."""


def object_fields(value, allowed, path):
    if not isinstance(value, dict):
        raise BudgetError(f"{path} must be an object")
    extra = set(value) - set(allowed)
    if extra:
        raise BudgetError(f"{path}: unknown field(s): {', '.join(sorted(extra))}")
    return value


def number(value, path, minimum=0):
    if value is None:
        return None
    if type(value) is not int or value < minimum:
        raise BudgetError(f"{path} must be an integer >= {minimum}, or null")
    return value


def check(name, known, limit, complete, unit="bytes"):
    complete = complete and limit is not None
    return {
        "name": name, "known": known, "limit": limit, "unit": unit,
        "complete": complete,
        "over_limit": known > limit if limit is not None else None,
        "remaining": limit - known if complete else None,
    }


def memory(section, name, unknowns):
    section = object_fields(section, ["items", "complete"], name)
    complete = section.get("complete", False)
    if type(complete) is not bool:
        raise BudgetError(f"{name}.complete must be true or false")
    if not complete:
        unknowns.append(f"{name}.coverage")
    items = section.get("items", [])
    if not isinstance(items, list):
        raise BudgetError(f"{name}.items must be an array")
    total = 0
    names = set()
    for index, item in enumerate(items):
        path = f"{name}.items[{index}]"
        fields = ["name", "bytes", "count", "note"]
        if name == "vram":
            fields.append("tiles")
        item = object_fields(item, fields, path)
        label = item.get("name")
        if not isinstance(label, str) or not label.strip():
            raise BudgetError(f"{path}.name must be nonempty text")
        if label.strip().casefold() in names:
            raise BudgetError(f"{path}: duplicate allocation name {label!r}")
        names.add(label.strip().casefold())
        if "note" in item and not isinstance(item["note"], str):
            raise BudgetError(f"{path}.note must be text")
        quantities = [field for field in ["bytes", "tiles"] if field in item]
        if len(quantities) != 1:
            raise BudgetError(f"{path} needs exactly one quantity: bytes or VRAM tiles")
        field = quantities[0]
        quantity = number(item[field], f"{path}.{field}")
        count = number(item.get("count", 1), f"{path}.count", minimum=1)
        if count is None:
            raise BudgetError(f"{path}.count must be a positive integer")
        if quantity is None:
            complete = False
            unknowns.append(f"{name}.{label.strip()}")
        else:
            total += quantity * count * (32 if field == "tiles" else 1)
    return check(name, total, 65536, complete)


def metric(value, path, unknowns, minimum=0):
    result = number(value, path, minimum)
    if result is None:
        unknowns.append(path)
    return result


def assess(document):
    document = object_fields(document, ["schema_version", "video_mode", "ram", "vram",
                                        "palette_banks", "sprites", "rom", "dma"], "worksheet")
    if type(document.get("schema_version")) is not int or document["schema_version"] != 1:
        raise BudgetError("schema_version must be 1")
    mode = document.get("video_mode")
    if mode not in ["H32", "H40"]:
        raise BudgetError("video_mode must be H32 or H40 (standard non-interlaced tiled graphics)")
    unknowns = []
    checks = [memory(document.get(name, {}), name, unknowns) for name in ["ram", "vram"]]
    palettes = metric(document.get("palette_banks"), "palette_banks", unknowns)
    checks.append(check("palette_banks", palettes or 0, 4, palettes is not None, "banks"))
    sprites = object_fields(document.get("sprites", {}), ["total", "per_scanline", "pixels_per_scanline"], "sprites")
    limits = [80, 20, 320] if mode == "H40" else [64, 16, 256]
    for field, name, limit, unit in zip(
        ["total", "per_scanline", "pixels_per_scanline"],
        ["sprites_total", "sprites_per_scanline", "sprite_pixels_per_scanline"],
        limits, ["hardware sprites", "hardware sprites", "pixels"],
    ):
        value = metric(sprites.get(field), f"sprites.{field}", unknowns)
        checks.append(check(name, value or 0, limit, value is not None, unit))

    rom = object_fields(document.get("rom", {}), ["bytes_before_padding", "alignment_bytes",
                                                "cartridge_bytes", "mapping"], "rom")
    raw = metric(rom.get("bytes_before_padding"), "rom.bytes_before_padding", unknowns)
    alignment = number(rom.get("alignment_bytes", 131072), "rom.alignment_bytes", minimum=1)
    if alignment is None:
        raise BudgetError("rom.alignment_bytes must be a positive integer")
    cart = number(rom.get("cartridge_bytes"), "rom.cartridge_bytes", minimum=1)
    mapping = rom.get("mapping", "unknown")
    if mapping not in ["standard", "banked", "unknown"]:
        raise BudgetError("rom.mapping must be standard, banked or unknown")
    if mapping == "unknown":
        unknowns.append("rom.mapping")
    if mapping == "banked" and cart is None:
        unknowns.append("rom.cartridge_bytes")
    padded = ((raw + alignment - 1) // alignment) * alignment if raw is not None else 0
    limit = min(4194304, cart) if mapping == "standard" and cart is not None else cart
    if mapping == "standard" and cart is None:
        limit = 4194304
    checks.append(check("rom", padded, limit, raw is not None and mapping != "unknown"))

    dma = object_fields(document.get("dma", {}), ["peak_bytes", "budget_bytes"], "dma")
    peak = metric(dma.get("peak_bytes"), "dma.peak_bytes", unknowns)
    budget = metric(dma.get("budget_bytes"), "dma.budget_bytes", unknowns, minimum=1)
    checks.append(check("dma", peak or 0, budget, peak is not None and budget is not None))
    status = "over_limit" if any(c["over_limit"] for c in checks) else (
        "incomplete" if unknowns else "within_declared_limits"
    )
    notes = [
        "Checks only supplied static quantities; no CPU/FPS, runtime memory safety, layout or game-feasibility prediction.",
        "Memory items must be disjoint. Count a reserved pool OR its occupants, not both; shared sprite definitions do not imply shared per-instance VRAM.",
        "Remaining values apply only to declared complete checks; they are not measured free memory. Estimates still need proof.",
        "DMA uses your declared transfer budget, not a universal PAL/NTSC bandwidth. Queue/temp-buffer capacity and bus timing need separate checks.",
    ]
    if mapping == "banked":
        notes.append("Banking is a declaration, not verified SDK/library/header, cartridge or emulator mapper support.")
    return {"status": status, "video_mode": mode, "checks": checks,
            "unknowns": unknowns, "notes": notes}


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise BudgetError(f"Duplicate JSON key: {key}")
        result[key] = value
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("worksheet", type=Path)
    parser.add_argument("--json", action="store_true", help="emit a machine-readable report")
    args = parser.parse_args(argv)
    try:
        document = json.loads(args.worksheet.read_text(encoding="utf-8"), object_pairs_hook=unique_object)
        report = assess(document)
    except (OSError, ValueError) as error:
        print(f"Budget worksheet: {error}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2, ensure_ascii=True))
    else:
        print(f"Static budget check: {report['status']} ({report['video_mode']})")
        for item in report["checks"]:
            limit = f"{item['limit']:,}" if item["limit"] is not None else "unknown"
            state = "OVER" if item["over_limit"] else ("declared" if item["complete"] else "partial")
            print(f"  {item['name']}: {item['known']:,} / {limit} {item['unit']} ({state})")
        if report["unknowns"]:
            print("Unknowns: " + ", ".join(report["unknowns"]))
        print("Static accounting only; not a performance or feasibility verdict.")
    return 1 if report["status"] == "over_limit" else 0


if __name__ == "__main__":
    sys.exit(main())
