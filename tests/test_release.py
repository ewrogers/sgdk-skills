"""Validate and package byte-identical SGDK releases, including a real Menu Lab build."""
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
import zipfile


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "skills/sgdk-release/scripts/release.py"
spec = importlib.util.spec_from_file_location("sgdk_release", SCRIPT)
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)


def seal(data):
    """Independent 16-bit-word oracle equivalent to SDK's folded 32-bit XOR."""
    data[0x18E:0x190] = b"\0\0"
    value = 0
    for offset in range(0, len(data), 2):
        value ^= (data[offset] << 8) | data[offset + 1]
    struct.pack_into(">H", data, 0x18E, value)
    return data


def rom(size=131072):
    data = bytearray(size)
    struct.pack_into(">II", data, 0, 0xE1000000, 0x200)
    data[0x100:0x110] = b"SEGA MEGA DRIVE "
    data[0x120:0x150] = b"RELEASE TEST".ljust(48)
    data[0x150:0x180] = b"RELEASE TEST".ljust(48)
    struct.pack_into(">II", data, 0x1A0, 0, 0xFFFFF)
    data[0x1B0:0x1B4] = b"RA\xF8\x20"
    struct.pack_into(">II", data, 0x1B4, 0x200000, 0x20FFFF)
    data[0x1F0:0x200] = b"JUE             "
    data[0x200:0x204] = b"\x4E\x71\x60\xFE"
    return seal(data)


def codes(report, level="error"):
    return {f["code"] for f in report["findings"] if f["level"] == level}


def elf_header():
    data = bytearray(52)
    data[:7] = b"\x7fELF\x01\x02\x01"
    struct.pack_into(">HHI", data, 16, 2, 4, 1)
    struct.pack_into(">H", data, 40, 52)
    return data


class RomValidationTests(unittest.TestCase):
    def test_stock_header_size_and_stack_alias_are_accepted_without_mutation(self):
        data = rom()
        before = bytes(data)
        report = release.inspect_rom(data)
        self.assertTrue(report["valid"])
        self.assertEqual(codes(report, "warning"), {"header-size"})
        self.assertEqual(report["header"]["sram"]["start"], 0x200000)
        self.assertEqual(report["checksum"]["stored"], report["checksum"]["expected"])
        self.assertEqual(data, before)
        self.assertEqual(report["sha256"], hashlib.sha256(before).hexdigest())

    def test_checksum_covers_header_vectors_payload_and_final_padding(self):
        for offset in [0, 0x110, 0x18E, 0x200, 10000, 131071]:
            with self.subTest(offset=offset):
                data = rom()
                data[offset] ^= 1
                self.assertIn("checksum", codes(release.inspect_rom(data)))

    def test_known_xor_checksum_and_not_a_generic_additive_checksum(self):
        data = rom()
        data[0x200:0x208] = bytes.fromhex("12345678 9abcdef0")
        base = release.inspect_rom(data)["checksum"]["expected"]
        data[0x204:0x208] = b"\0\0\0\0"
        other = release.inspect_rom(data)["checksum"]["expected"]
        self.assertEqual(base ^ other, 0x9ABC ^ 0xDEF0)
        seal(data)
        self.assertTrue(release.inspect_rom(data)["valid"])
        additive = sum(struct.unpack(">" + "H" * ((len(data) - 512) // 2), data[512:])) & 0xFFFF
        self.assertNotEqual(additive, struct.unpack_from(">H", data, 0x18E)[0])

    def test_weak_checksum_collision_is_not_mistaken_for_release_identity(self):
        first = rom()
        second = bytearray(first)
        second[10000] ^= 1
        second[10004] ^= 1
        # XOR checks cannot detect cancelling changes. SHA-256 identifies bytes.
        self.assertTrue(release.inspect_rom(second)["valid"])
        self.assertNotEqual(release.inspect_rom(first)["sha256"], release.inspect_rom(second)["sha256"])

    def test_short_truncated_unaligned_and_non_rom_input_fail(self):
        for data, expected in [(b"", "short-header"), (rom()[:-1], "alignment"),
                               (rom()[:-4], "alignment"), (b"\0" * 131072, "signature")]:
            with self.subTest(expected=expected):
                self.assertIn(expected, codes(release.inspect_rom(data)))

    def test_linear_size_boundary_and_special_mappings(self):
        self.assertTrue(release.inspect_rom(rom(4 * 1024 * 1024))["valid"])
        self.assertIn("mapping", codes(release.inspect_rom(bytes(4 * 1024 * 1024 + 4))))
        for signature in [b"SEGA SSF", b"SEGA 32X", b"SEGA MEGAWIFI"]:
            data = rom()
            data[0x100:0x110] = signature.ljust(16)
            self.assertIn("mapping", codes(release.inspect_rom(seal(data))))

    def test_reset_stack_vectors_and_rom_range_fail_helpfully(self):
        for offset, value, expected in [(0, 0xFF0001, "stack-vector"), (0, 0xA00000, "stack-vector"),
                                        (4, 0x201, "reset-vector"), (4, 0x100, "reset-vector"),
                                        (4, 131072, "reset-vector"), (0x1A0, 4, "rom-range"),
                                        (0x1A4, 0x400000, "rom-range")]:
            with self.subTest(value=value):
                data = rom()
                struct.pack_into(">I", data, offset, value)
                self.assertIn(expected, codes(release.inspect_rom(seal(data))))
        for sp in [0xE1000000, 0xFFFF00, 0xE0FFFFFE]:
            data = rom()
            struct.pack_into(">I", data, 0, sp)
            self.assertTrue(release.inspect_rom(seal(data))["valid"])

    def test_regions_and_sram_overlap_are_review_warnings(self):
        for region in [b"JUE", b"U", b"E", b"A", b"4"]:
            data = rom()
            data[0x1F0:0x200] = region.ljust(16)
            self.assertNotIn("region", codes(release.inspect_rom(seal(data)), "warning"))
        data = rom()
        data[0x1F0:0x200] = b"?".ljust(16)
        self.assertIn("region", codes(release.inspect_rom(seal(data)), "warning"))
        data = rom(0x220000)
        report = release.inspect_rom(data)
        self.assertTrue(report["valid"])
        self.assertIn("sram-overlap", codes(report, "warning"))
        struct.pack_into(">II", data, 0x1B4, 0x210000, 0x200000)
        self.assertIn("sram-range", codes(release.inspect_rom(seal(data))))


class PackageTests(unittest.TestCase):
    def setUp(self):
        (ROOT / "build").mkdir(exist_ok=True)
        self.folder = tempfile.TemporaryDirectory(prefix="release with spaces ", dir=ROOT / "build")
        self.addCleanup(self.folder.cleanup)
        self.root = Path(self.folder.name)
        self.rom = self.root / "source rom.bin"
        self.rom.write_bytes(rom())
        self.readme = self.root / "readme.txt"
        self.readme.write_bytes(b"Test release\r\nA selects.\r\n")
        self.license = self.root / "license.txt"
        self.license.write_text("Original project license and required notices.\n", encoding="utf-8")
        self.output = self.root / "release.zip"

    def package(self, **kwargs):
        return release.package(self.rom, self.output, "Menu Lab", "0.1.0", self.readme, self.license, **kwargs)

    def test_exact_rom_documents_manifest_hashes_and_reproducible_archive(self):
        before = self.rom.read_bytes()
        credits = self.root / "credits"
        credits.write_text("Original test assets.\n", encoding="utf-8")
        symbols = self.root / "rom.out"
        symbols.write_bytes(elf_header())
        self.package(credits=credits, symbols=symbols)
        first = self.output.read_bytes()
        with zipfile.ZipFile(self.output) as archive:
            self.assertEqual(archive.read("menu-lab-0.1.0.bin"), before)
            self.assertEqual(archive.read("README.txt"), self.readme.read_bytes())
            self.assertEqual(archive.read("menu-lab-0.1.0.elf"), symbols.read_bytes())
            manifest = json.loads(archive.read("manifest.json"))
            for name, record in manifest["files"].items():
                content = archive.read(name)
                self.assertEqual(record, {"bytes": len(content), "sha256": hashlib.sha256(content).hexdigest()})
            self.assertNotIn(str(self.root), archive.read("manifest.json").decode())
            self.assertTrue(all(i.date_time == (1980, 1, 1, 0, 0, 0) for i in archive.infolist()))
            self.assertTrue(all(i.compress_type == zipfile.ZIP_STORED for i in archive.infolist()))
        self.assertEqual(self.rom.read_bytes(), before)
        os.utime(self.rom, (1234567890, 1234567890))
        self.output = self.root / "second.zip"
        self.package(credits=credits, symbols=symbols)
        self.assertEqual(first, self.output.read_bytes())

    def test_no_overwrite_and_cleanup_of_temporary_files(self):
        self.output.write_bytes(b"existing release")
        with self.assertRaises(FileExistsError):
            self.package()
        self.assertEqual(self.output.read_bytes(), b"existing release")
        self.assertFalse(list(self.root.glob(".sgdk-release-*")))

    def test_failed_validation_or_missing_documents_never_produces_archive(self):
        bad = bytearray(self.rom.read_bytes()); bad[-1] ^= 1
        self.rom.write_bytes(bad)
        with self.assertRaisesRegex(ValueError, "checksum"):
            self.package()
        self.assertFalse(self.output.exists())
        self.rom.write_bytes(rom())
        self.license.unlink()
        with self.assertRaises(OSError):
            self.package()
        self.assertFalse(self.output.exists())
        self.license.write_bytes(b" \n")
        with self.assertRaisesRegex(ValueError, "empty"):
            self.package()
        self.assertFalse(self.output.exists())

    def test_unsafe_version_and_non_elf_symbols_fail_without_output(self):
        for version in ["../escape", "1/2", "", "a" * 65, "1."]:
            with self.subTest(version=version), self.assertRaises(ValueError):
                release.package(self.rom, self.output, "Menu", version, self.readme, self.license)
        symbols = self.root / "wrong.out"
        symbols.write_bytes(b"not an ELF")
        with self.assertRaisesRegex(ValueError, "ELF"):
            self.package(symbols=symbols)
        for offset, value in [(4, 2), (5, 1), (19, 62), (41, 0)]:
            wrong = elf_header(); wrong[offset] = value; symbols.write_bytes(wrong)
            with self.subTest(offset=offset), self.assertRaisesRegex(ValueError, "ELF"):
                self.package(symbols=symbols)
        self.assertFalse(self.output.exists())

    def test_copied_standalone_cli_json_errors_and_paths_with_spaces(self):
        helper = self.root / "release helper.py"
        shutil.copy2(SCRIPT, helper)
        def run(*args):
            return subprocess.run([sys.executable, str(helper), *map(str, args)], cwd=self.root,
                                  capture_output=True, text=True, timeout=20)
        result = run("inspect", self.rom, "--json")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(json.loads(result.stdout)["valid"])
        result = run("package", "--rom", self.rom, "--output", self.output,
                     "--name", "A Game", "--version", "1.0", "--readme", self.readme, "--license", self.license)
        self.assertEqual(result.returncode, 0, result.stderr)
        result = run("inspect", self.root / "missing.bin")
        self.assertEqual(result.returncode, 1)
        data = bytearray(self.rom.read_bytes()); data[-1] ^= 1; self.rom.write_bytes(data)
        result = run("inspect", self.rom, "--json")
        self.assertEqual(result.returncode, 1)
        self.assertIn("checksum", codes(json.loads(result.stdout)))
        oversized = self.root / "oversized.bin"
        with oversized.open("wb") as stream:
            stream.truncate(release.LINEAR_LIMIT + 1)
        result = run("inspect", oversized)
        self.assertEqual(result.returncode, 1)
        self.assertIn("4 MiB", result.stderr)


@unittest.skipUnless(os.environ.get("SGDK_SKILLS_INTEGRATION") == "1", "Docker integration is opt-in")
class ReleaseContainerTests(unittest.TestCase):
    def test_real_sizebnd_oracle_and_menu_lab_release_package(self):
        (ROOT / "build").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="release container ", dir=ROOT / "build") as folder:
            project = Path(folder) / "menu lab"
            shutil.copytree(ROOT / "skills/sgdk-ui/assets/menu-lab", project)
            base = ["docker", "run", "--rm"]
            if os.environ.get("SGDK_PLATFORM"):
                base += ["--platform", os.environ["SGDK_PLATFORM"]]
            if hasattr(os, "getuid"):
                base += ["--user", f"{os.getuid()}:{os.getgid()}"]
            base += ["--mount", f"type=bind,source={project},target=/workspace", os.environ.get("SGDK_IMAGE", "ewrogers/sgdk:latest")]
            def run(*args):
                r = subprocess.run(base + list(args), capture_output=True, text=True, timeout=240)
                self.assertEqual(r.returncode, 0, (r.stdout + r.stderr)[-6000:])
                return r.stdout
            for size in [0x20000 - 4, 0x20000, 0x60000]:
                fixture = rom(size)
                fixture[-16:] = bytes(range(16))
                fixture[0x18E:0x190] = b"\xFF\xFF"
                source = project / "oracle.bin"
                source.write_bytes(fixture)
                run("java", "-jar", "/opt/sgdk/bin/sizebnd.jar", "oracle.bin", "-sizealign", "131072", "-checksum")
                actual = source.read_bytes()
                self.assertTrue(release.inspect_rom(actual)["valid"])
                padded = fixture + bytes((-len(fixture)) % release.ALIGNMENT)
                self.assertEqual(actual, bytes(seal(padded)))
            run("build", "release")
            binary = project / "out/rom.bin"
            snapshot = binary.read_bytes()
            self.assertTrue(release.inspect_rom(snapshot)["valid"])
            output = project / "menu-release.zip"
            release.package(binary, output, "Menu Lab", "0.1.0", project / "README.md", ROOT / "LICENSE",
                            symbols=project / "out/rom.out")
            with zipfile.ZipFile(output) as archive:
                self.assertEqual(archive.read("menu-lab-0.1.0.bin"), snapshot)
                self.assertEqual(archive.read("menu-lab-0.1.0.elf"), (project / "out/rom.out").read_bytes())
                self.assertTrue(release.inspect_rom(archive.read("menu-lab-0.1.0.bin"))["valid"])
            # Debug is not our performance baseline, but its injected symbols
            # and extra padding must still pass the same physical-file checks.
            run("build", "debug")
            self.assertTrue(release.inspect_rom(binary.read_bytes())["valid"])


if __name__ == "__main__":
    unittest.main()
