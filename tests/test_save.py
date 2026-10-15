"""Execute Save Lab's byte format and transactions; build its real SGDK adapter."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SKILL = ROOT / "skills/sgdk-save"
LAB = SKILL / "assets/save-lab"
FIXTURE = ROOT / "tests/fixtures/save"
IMAGE = os.environ.get("SGDK_IMAGE", "ewrogers/sgdk:latest")


class SaveBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = next((p for name in ["clang", "cc", "gcc"] if (p := shutil.which(name))), None)
        if not compiler:
            if os.environ.get("SGDK_REQUIRE_HOST_CC") == "1":
                raise RuntimeError("CI requires a C17 compiler for save tests")
            raise unittest.SkipTest("Save tests need a C17 compiler")
        cls.scratch = tempfile.TemporaryDirectory(prefix="sgdk save host ")
        cls.addClassCleanup(cls.scratch.cleanup)
        copied = Path(cls.scratch.name) / "independent lab"
        shutil.copytree(LAB, copied)
        suffix = ".exe" if os.name == "nt" else ""
        cls.binary = Path(cls.scratch.name) / ("save-test" + suffix)
        cls.adapter = Path(cls.scratch.name) / ("storage-test" + suffix)
        common = [compiler, "-std=c17", "-Wall", "-Wextra", "-Werror", "-I", str(copied / "inc")]
        for binary, sources in [
            (cls.binary, [str(copied / "src/save.c"), str(FIXTURE / "save_test.c")]),
            (cls.adapter, ["-I", str(FIXTURE), str(copied / "src/save.c"),
                           str(copied / "src/storage.c"), str(FIXTURE / "storage_test.c")]),
        ]:
            result = subprocess.run(common + sources + ["-o", str(binary)],
                                    capture_output=True, text=True, timeout=60)
            if result.returncode:
                raise RuntimeError((result.stdout + result.stderr)[-6000:])

    def case(self, name):
        result = subprocess.run([str(self.binary), name], capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.strip(), name)

    def test_crc_known_vector(self): self.case("crc")
    def test_empty_and_garbage_preserve_caller_state(self): self.case("empty")
    def test_explicit_bytes_bounds_and_slot_rotation(self): self.case("roundtrip")
    def test_every_single_bit_error_recovers_previous_slot(self): self.case("recovery")
    def test_every_interrupted_write_preserves_previous_save(self): self.case("interrupted")
    def test_unknown_valid_version_is_never_overwritten(self): self.case("versions")
    def test_invalid_values_and_schema_are_rejected(self): self.case("schema")
    def test_sequence_wrap_ties_and_ambiguous_ordering(self): self.case("ordering")
    def test_io_failures_bounds_readback_and_erase(self): self.case("io")
    def test_every_read_failure_before_and_after_commit(self): self.case("verification")
    def test_password_golden_vector_and_roundtrips(self): self.case("password")
    def test_password_typos_lengths_and_alphabet(self): self.case("password_bad")

    def test_sram_adapter_locks_on_success_and_failure(self):
        result = subprocess.run([str(self.adapter)], capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


@unittest.skipUnless(os.environ.get("SGDK_SKILLS_INTEGRATION") == "1", "Docker integration is opt-in")
class SaveContainerTests(unittest.TestCase):
    def test_sdk_api_header_mapping_and_both_build_modes(self):
        with tempfile.TemporaryDirectory(prefix="sgdk save container ") as folder:
            project = Path(folder) / "standalone lab"
            shutil.copytree(LAB, project)
            base = ["docker", "run", "--rm"]
            if os.environ.get("SGDK_PLATFORM"):
                base += ["--platform", os.environ["SGDK_PLATFORM"]]
            if hasattr(os, "getuid"):
                base += ["--user", f"{os.getuid()}:{os.getgid()}"]
            base += ["--mount", f"type=bind,source={project},target=/workspace", IMAGE]
            (project / "probe.c").write_text('''#include <genesis.h>
_Static_assert(SRAM_BASE == 0x200001, "odd-address SRAM");
void (*enable)(void) = SRAM_enable;
void (*readonly)(void) = SRAM_enableRO;
void (*disable)(void) = SRAM_disable;
u8 (*read_byte)(u32) = SRAM_readByte;
void (*write_byte)(u32, u8) = SRAM_writeByte;
u16 (*read_word)(u32) = SRAM_readWord;
u32 (*read_long)(u32) = SRAM_readLong;
void (*write_word)(u32, u16) = SRAM_writeWord;
void (*write_long)(u32, u32) = SRAM_writeLong;
''', encoding="utf-8")
            result = subprocess.run(base + ["m68k-elf-gcc", "-m68000", "-std=gnu17", "-fno-builtin",
                                    "-Werror=implicit-function-declaration", "-Werror=incompatible-pointer-types",
                                    "-I/opt/sgdk/inc", "-I/opt/sgdk/res", "-fsyntax-only", "probe.c"],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-6000:])
            for mode in ["release", "debug"]:
                result = subprocess.run(base + ["build", mode], capture_output=True, text=True, timeout=240)
                self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-8000:])
                rom = (project / "out/rom.bin").read_bytes()
                self.assertEqual(rom[0x100:0x104], b"SEGA")
                self.assertEqual(rom[0x1B0:0x1B4], b"RA\xF8\x20")
                self.assertEqual(int.from_bytes(rom[0x1B4:0x1B8], "big"), 0x200001)
                self.assertEqual(int.from_bytes(rom[0x1B8:0x1BC], "big"), 0x20FFFF)
                self.assertEqual(len(rom) % 131072, 0)
                self.assertLess(len(rom), 0x200000, "Lab code and constants must stay below SRAM window")


if __name__ == "__main__":
    unittest.main()
