#!/usr/bin/env python3
"""Read a Mega Drive ROM header without modifying the ROM or needing Docker."""

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path


def inspect_rom(data):
    """Return header facts and advisory warnings, not an emulator validation."""
    if len(data) < 0x200:
        raise ValueError("ROM is shorter than its 512-byte vector table and header")
    if data[0x100:0x104] != b"SEGA":
        raise ValueError("missing SEGA header at 0x100; expected a raw .bin ROM")

    def text_at(start, length):
        return data[start:start + length].decode("ascii", errors="replace").strip(" \x00")

    initial_sp, reset_pc = struct.unpack_from(">II", data)
    rom_start, rom_end, ram_start, ram_end = struct.unpack_from(">IIII", data, 0x1A0)
    warnings = []
    if len(data) > 4 * 1024 * 1024:
        warnings.append("ROM exceeds the normal 4 MiB cartridge window; verify mapper/header, emulator and cartridge support.")
    declared_bytes = rom_end - rom_start + 1 if rom_end >= rom_start else None
    if declared_bytes != len(data):
        warnings.append("Header ROM range differs from file size; SGDK sample headers may use a fixed range.")
    if reset_pc & 1:
        warnings.append("Reset vector is odd; a 68000 instruction address must be even.")
    if not rom_start <= reset_pc < rom_start + len(data):
        warnings.append("Reset vector is outside the file's declared starting address and byte extent.")
    return {
        "file_bytes": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "console": text_at(0x100, 16),
        "domestic_title": text_at(0x120, 48),
        "overseas_title": text_at(0x150, 48),
        "serial": text_at(0x180, 14),
        "region": text_at(0x1F0, 16),
        "initial_sp": f"0x{initial_sp:08X}",
        "reset_pc": f"0x{reset_pc:08X}",
        "rom_start": f"0x{rom_start:08X}",
        "rom_end": f"0x{rom_end:08X}",
        "ram_start": f"0x{ram_start:08X}",
        "ram_end": f"0x{ram_end:08X}",
        "warnings": warnings,
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path, help="raw, uncompressed Mega Drive ROM (.bin)")
    parser.add_argument("--json", action="store_true", help="emit a machine-readable report")
    args = parser.parse_args(argv)
    try:
        report = inspect_rom(args.rom.read_bytes())
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        print(f"ROM: {args.rom.name} ({report['file_bytes']:,} bytes)")
        print(f"Title: {report['overseas_title'] or report['domestic_title'] or '(unnamed)'}")
        print(f"Region: {report['region'] or '(unspecified)'}")
        print(f"Reset PC: {report['reset_pc']}  Initial SP: {report['initial_sp']}")
        print(f"SHA-256: {report['sha256']}")
        for warning in report["warnings"]:
            print(f"Note: {warning}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
