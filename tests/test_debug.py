"""ROM inspection tests; opt in to actual SGDK compilation with the documented env."""

import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SKILL = ROOT / "skills" / "sgdk-debug"
SCRIPT = SKILL / "scripts" / "rom_report.py"
SPEC = importlib.util.spec_from_file_location("rom_report", SCRIPT)
ROM_REPORT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ROM_REPORT)
IMAGE = os.environ.get(
    "SGDK_IMAGE",
    "ewrogers/sgdk:latest",
)


def make_rom(length=1024):
    data = bytearray(length)
    struct.pack_into(">II", data, 0, 0x01000000, 0x200)
    data[0x100:0x110] = b"SEGA MEGA DRIVE "
    data[0x120:0x150] = b"SGDK TEST".ljust(48)
    data[0x150:0x180] = b"SGDK TEST".ljust(48)
    data[0x180:0x18E] = b"GM 00000000-00"
    struct.pack_into(">IIII", data, 0x1A0, 0, length - 1, 0xFF0000, 0xFFFFFF)
    data[0x1F0:0x200] = b"JUE".ljust(16)
    return data


class RomReportTests(unittest.TestCase):
    def test_reads_big_endian_header_and_real_file_size(self):
        rom = make_rom()
        report = ROM_REPORT.inspect_rom(rom)
        self.assertEqual(report["initial_sp"], "0x01000000")
        self.assertEqual(report["reset_pc"], "0x00000200")
        self.assertEqual(report["rom_end"], "0x000003FF")
        self.assertEqual(report["ram_start"], "0x00FF0000")
        self.assertEqual(report["ram_end"], "0x00FFFFFF")
        self.assertEqual(report["region"], "JUE")
        self.assertEqual(report["overseas_title"], "SGDK TEST")
        self.assertEqual(report["file_bytes"], len(rom))
        self.assertEqual(report["sha256"], hashlib.sha256(rom).hexdigest())
        self.assertEqual(report["warnings"], [])

    def test_rejects_truncated_header(self):
        with self.assertRaisesRegex(ValueError, "512-byte"):
            ROM_REPORT.inspect_rom(b"SEGA")

    def test_rejects_wrong_format(self):
        with self.assertRaisesRegex(ValueError, "raw .bin"):
            ROM_REPORT.inspect_rom(bytes(1024))

    def test_fixed_upstream_header_range_is_advisory(self):
        rom = make_rom()
        struct.pack_into(">I", rom, 0x1A4, 0xFFFFF)
        report = ROM_REPORT.inspect_rom(rom)
        self.assertEqual(report["file_bytes"], 1024)
        self.assertEqual(len(report["warnings"]), 1)
        self.assertIn("fixed range", report["warnings"][0])

    def test_large_rom_is_advisory_and_needs_mapping_review(self):
        report = ROM_REPORT.inspect_rom(make_rom(4 * 1024 * 1024 + 1))
        self.assertEqual(len(report["warnings"]), 1)
        self.assertIn("mapper/header", report["warnings"][0])

    def test_four_mib_boundary_does_not_require_a_mapper_warning(self):
        self.assertEqual(ROM_REPORT.inspect_rom(make_rom(4 * 1024 * 1024))["warnings"], [])

    def test_bad_vector_warns_without_claiming_emulation(self):
        rom = make_rom()
        struct.pack_into(">I", rom, 4, 0x4001)
        warnings = ROM_REPORT.inspect_rom(rom)["warnings"]
        self.assertEqual(len(warnings), 2)
        self.assertIn("odd", warnings[0])
        self.assertIn("outside", warnings[1])

    def test_non_ascii_metadata_is_safe_to_display(self):
        rom = make_rom()
        rom[0x150] = 0xFF
        self.assertTrue(ROM_REPORT.inspect_rom(rom)["overseas_title"].startswith("\uFFFD"))

    def test_cli_accepts_spaces_and_does_not_modify_rom(self):
        with tempfile.TemporaryDirectory(prefix="sgdk debug ") as folder:
            path = Path(folder) / "my game.bin"
            original = bytes(make_rom())
            path.write_bytes(original)
            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(path), "--json"],
                text=True, capture_output=True, check=True,
            )
            self.assertEqual(json.loads(result.stdout)["file_bytes"], 1024)
            self.assertEqual(path.read_bytes(), original)

    def test_cli_missing_file_is_actionable_nonzero(self):
        with tempfile.TemporaryDirectory() as folder:
            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(Path(folder) / "missing.bin")],
                text=True, capture_output=True,
            )
        self.assertEqual(result.returncode, 2)
        self.assertIn("error:", result.stderr)
        self.assertEqual(result.stdout, "")

    def test_cli_invalid_rom_is_nonzero(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "bad.bin"
            path.write_bytes(b"bad")
            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(path)], text=True, capture_output=True,
            )
        self.assertEqual(result.returncode, 2)
        self.assertIn("512-byte", result.stderr)


@unittest.skipUnless(os.environ.get("SGDK_SKILLS_INTEGRATION") == "1", "Docker integration is opt-in")
class DebugContainerTests(unittest.TestCase):
    def test_profiling_fixture_and_exact_elf_source_lookup(self):
        with tempfile.TemporaryDirectory(prefix="sgdk debug ") as folder:
            project = Path(folder)
            (project / "src").mkdir()
            (project / "res").mkdir()
            shutil.copyfile(SKILL / "assets" / "profile.c", project / "src" / "main.c")
            base = ["docker", "run", "--rm"]
            if os.environ.get("SGDK_PLATFORM"):
                base.extend(["--platform", os.environ["SGDK_PLATFORM"]])
            if hasattr(os, "getuid"):
                base.extend(["--user", f"{os.getuid()}:{os.getgid()}"])
            base.extend(["--mount", f"type=bind,source={project},target=/workspace", IMAGE])

            def run(*args):
                result = subprocess.run(base + list(args), text=True, capture_output=True, timeout=240)
                self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-8000:])
                return result.stdout

            # Compile the maintained sample against the actual SDK and link a ROM.
            run("build", "debug")
            report = ROM_REPORT.inspect_rom((project / "out" / "rom.bin").read_bytes())
            self.assertEqual(report["console"].split()[0], "SEGA")
            self.assertGreater(report["file_bytes"], 0x200)
            symbols = run("m68k-elf-nm", "-n", "out/rom.out")
            address = next(line.split()[0] for line in symbols.splitlines() if line.split()[-1:] == ["profile_sample"])
            resolved = run("m68k-elf-addr2line", "-e", "out/rom.out", "-f", address)
            self.assertIn("profile_sample", resolved)
            self.assertIn("main.c:", resolved)
            self.assertNotIn("??", resolved)
            sections = run("m68k-elf-size", "-A", "out/rom.out")
            self.assertIn(".text", sections)
            self.assertIn(".bss", sections)

            # Verify documented API types rather than trusting spelling alone.
            probe = project / "src" / "api_probe.c"
            probe.write_text(
                "#include <genesis.h>\n#include <kdebug.h>\n"
                "u32 (*fps_probe)(void) = SYS_getFPS;\n"
                "u16 (*load_probe)(void) = SYS_getCPULoad;\n"
                "void (*show_probe)(bool) = SYS_showFrameLoad;\n"
                "void (*hide_probe)(void) = SYS_hideFrameLoad;\n"
                "void (*alert_probe)(const char *) = KDebug_Alert;\n"
                "void (*number_probe)(u32) = KDebug_AlertNumber;\n"
                "void (*start_probe)(void) = KDebug_StartTimer;\n"
                "void (*stop_probe)(void) = KDebug_StopTimer;\n",
                encoding="utf-8",
            )
            run("m68k-elf-gcc", "-m68000", "-std=gnu17", "-fno-builtin", "-Werror=incompatible-pointer-types", "-I/opt/sgdk/inc", "-I/opt/sgdk/res", "-fsyntax-only", "src/api_probe.c")


if __name__ == "__main__":
    unittest.main()
