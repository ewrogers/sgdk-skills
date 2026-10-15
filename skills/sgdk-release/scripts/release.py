#!/usr/bin/env python3
"""Inspect standard linear SGDK ROMs and package an existing build without modifying it."""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import re
import struct
import sys
import tempfile
import zipfile


ALIGNMENT = 128 * 1024
LINEAR_LIMIT = 4 * 1024 * 1024


def inspect_rom(data):
    """SGDK 2.11 sizebnd checksum, not the conventional Genesis word-sum."""
    report = {"valid": False, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest(),
              "profile": "sgdk-linear", "findings": []}

    def finding(level, code, message):
        report["findings"].append({"level": level, "code": code, "message": message})

    if len(data) < 512:
        finding("error", "short-header", "ROM is shorter than its 512-byte vector/header area.")
        return report
    if len(data) > LINEAR_LIMIT:
        finding("error", "mapping", "ROM exceeds the 4 MiB linear window; review its mapper with the selected SDK and cartridge. This helper does not validate banked ROMs.")
        return report
    if len(data) % ALIGNMENT:
        finding("error", "alignment", "Standard SGDK output must be padded to a 128 KiB boundary. Rebuild with the selected SDK; do not pad or trim an already checksummed file by hand.")
    if data[0x100:0x104] != b"SEGA":
        finding("error", "signature", "Missing SEGA header at 0x100; supply the raw cartridge .bin, not an ELF, ZIP or copier-header image.")
    console = data[0x100:0x110].decode("ascii", errors="replace").strip()
    if "SSF" in console or "32X" in console or "MEGAWIFI" in console:
        finding("error", "mapping", "This helper covers ordinary linear Mega Drive cartridges, not SSF, 32X or MegaWiFi mappings.")
    sp, pc = struct.unpack_from(">II", data)
    sp_address, pc_address = sp & 0xFFFFFF, pc & 0xFFFFFF
    # SGDK's stock __stack is 0xE1000000: low 24 bits wrap to zero.
    # Predecrement stack writes then land at the top of mirrored work RAM.
    if sp & 1 or (sp_address != 0 and not 0xFF0000 <= sp_address <= 0xFFFFFF):
        finding("error", "stack-vector", "Initial stack must be even and point into work RAM or its wrapped top. Inspect the linker script and reset vector.")
    if pc & 1 or not 512 <= pc_address < len(data):
        finding("error", "reset-vector", "Reset address must be even and inside the supplied linear ROM after its header.")
    rom_start, rom_end = struct.unpack_from(">II", data, 0x1A0)
    if rom_start != 0 or not 511 <= rom_end < LINEAR_LIMIT:
        finding("error", "rom-range", "Header ROM range is inconsistent with an ordinary zero-based linear cartridge.")
    elif rom_end != len(data) - 1:
        finding("warning", "header-size", "Header ROM end differs from file size. SGDK's stock header uses a fixed 1 MiB range; review it for your intended cartridge without truncating the file.")
    region = data[0x1F0:0x200].decode("ascii", errors="replace").strip()
    if not region or any(c not in "JUE0123456789ABCDEFabcdef " for c in region):
        finding("warning", "region", "Region field is empty or unfamiliar. Review it against the selected SDK and intended targets.")
    stored = struct.unpack_from(">H", data, 0x18E)[0]
    if len(data) % 4 == 0:
        checksum = 0
        for (word,) in struct.iter_unpack(">I", data):
            checksum ^= word
        # sizebnd clears the checksum field before XOR/folding all padded bytes.
        expected = ((checksum ^ (checksum >> 16)) & 0xFFFF) ^ stored
        report["checksum"] = {"scheme": "sgdk-xor-fold-32", "stored": stored, "expected": expected}
        if stored != expected:
            finding("error", "checksum", "SGDK XOR-folded checksum does not match. Rebuild; do not substitute a generic Genesis word-sum checksum.")
    else:
        finding("error", "checksum-length", "SGDK's checksum needs complete 32-bit words; this ROM appears truncated or is not standard SDK output.")
    report["header"] = {"console": console, "domestic_title": data[0x120:0x150].decode("ascii", errors="replace").strip(),
                        "overseas_title": data[0x150:0x180].decode("ascii", errors="replace").strip(),
                        "region": region, "rom_start": rom_start, "rom_end": rom_end,
                        "stack_vector": sp, "reset_vector": pc}
    if data[0x1B0:0x1B2] == b"RA":
        start, end = struct.unpack_from(">II", data, 0x1B4)
        report["header"]["sram"] = {"descriptor": data[0x1B0:0x1B4].hex(), "start": start, "end": end}
        if start > end:
            finding("error", "sram-range", "SRAM start exceeds its end; inspect the project's ROM header.")
        elif start < len(data):
            finding("warning", "sram-overlap", "Declared SRAM overlaps ROM addresses. Verify mapper switching, code/data placement and audio access before shipping.")
    report["valid"] = not any(f["level"] == "error" for f in report["findings"])
    return report


def read_rom(path):
    # Bound file reads too: the helper deliberately supports only linear ROMs.
    if Path(path).stat().st_size > LINEAR_LIMIT:
        raise ValueError("ROM exceeds 4 MiB; banked cartridge validation needs a mapper-specific workflow.")
    with Path(path).open("rb") as stream:
        data = stream.read(LINEAR_LIMIT + 1)
    if len(data) > LINEAR_LIMIT:
        raise ValueError("ROM grew beyond the 4 MiB linear limit while being read.")
    return data


def package(rom, output, name, version, readme, license_file, credits=None, symbols=None, build_info=None):
    data = read_rom(rom)
    report = inspect_rom(data)
    if not report["valid"]:
        errors = "; ".join(f["message"] for f in report["findings"] if f["level"] == "error")
        raise ValueError("ROM validation failed: " + errors)
    if not name.strip():
        raise ValueError("Give the game a nonempty name.")
    slug = re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-")
    if not slug or len(slug) > 80:
        raise ValueError("Game name needs an ASCII letter/digit and an archive basename of at most 80 characters.")
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]{0,63}", version) or version.endswith("."):
        raise ValueError("Version must be a short filename-safe label, such as 0.1.0 or 1.0-rc1.")
    files = {f"{slug}-{version}.bin": data}
    for archive_name, source in [("README.txt", readme), ("LICENSE.txt", license_file),
                                 ("CREDITS.txt", credits), (f"{slug}-{version}.elf", symbols),
                                 ("BUILD-INFO.txt", build_info)]:
        if source is None:
            continue
        content = Path(source).read_bytes()
        if not content.strip():
            raise ValueError(f"{archive_name} is empty; supply the actual release document or omit the optional file.")
        if archive_name.endswith(".elf"):
            if (len(content) < 52 or content[:7] != b"\x7fELF\x01\x02\x01" or
                    struct.unpack_from(">H", content, 18)[0] != 4 or
                    struct.unpack_from(">H", content, 40)[0] != 52):
                raise ValueError("Symbols must have a big-endian ELF32 M68000 header, usually out/rom.out from this exact build.")
        files[archive_name] = content
    manifest = {"format": 1, "name": name, "version": version, "rom_validation": report,
                "files": {n: {"bytes": len(b), "sha256": hashlib.sha256(b).hexdigest()} for n, b in sorted(files.items())}}
    if symbols is not None:
        manifest["symbol_note"] = "ELF supplied by caller; basic ELF32/M68000 header checks do not establish correspondence to this ROM. Preserve exact-build provenance."
    files["manifest.json"] = (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode("utf-8")
    buffer = io.BytesIO()
    # Stored ZIP entries avoid zlib-version differences, host timestamps and modes.
    with zipfile.ZipFile(buffer, "w", compression=zipfile.ZIP_STORED) as archive:
        for filename, content in sorted(files.items()):
            entry = zipfile.ZipInfo(filename, date_time=(1980, 1, 1, 0, 0, 0))
            entry.create_system = 3
            entry.external_attr = 0o100644 << 16
            archive.writestr(entry, content)
    output = Path(output)
    # Publish a finished file exclusively. Neither a failed write nor a retry
    # replaces an existing release. Temporary file stays on the same filesystem.
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=output.parent, prefix=".sgdk-release-", delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(buffer.getvalue())
        os.link(temporary, output)
    finally:
        if temporary is not None:
            temporary.unlink()
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    inspect = commands.add_parser("inspect", help="Validate a raw standard SGDK .bin without changing it")
    inspect.add_argument("rom", type=Path)
    inspect.add_argument("--json", action="store_true")
    bundle = commands.add_parser("package", help="Create a new ZIP after validation; never overwrite")
    for flag in ["rom", "output", "readme", "license"]:
        bundle.add_argument("--" + flag, required=True, type=Path)
    for flag in ["name", "version"]:
        bundle.add_argument("--" + flag, required=True)
    for flag in ["credits", "symbols", "build-info"]:
        bundle.add_argument("--" + flag, type=Path)
    args = parser.parse_args()
    try:
        if args.command == "inspect":
            report = inspect_rom(read_rom(args.rom))
            if args.json:
                print(json.dumps(report, indent=2, sort_keys=True))
            else:
                print(f"{'PASS' if report['valid'] else 'FAIL'}: {report['bytes']:,} bytes | SHA-256 {report['sha256']}")
                for f in report["findings"]:
                    print(f"{f['level'].upper()} [{f['code']}]: {f['message']}")
                print("Checks cover standard SGDK file structure, not gameplay, region compatibility or hardware behavior.")
            return 0 if report["valid"] else 1
        manifest = package(args.rom, args.output, args.name, args.version, args.readme,
                           args.license, args.credits, args.symbols, args.build_info)
        print(f"Created {args.output}: {args.output.stat().st_size:,} bytes")
        for f in manifest["rom_validation"]["findings"]:
            print(f"{f['level'].upper()} [{f['code']}]: {f['message']}")
        return 0
    except (OSError, ValueError, zipfile.BadZipFile) as error:
        print(f"Release failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
