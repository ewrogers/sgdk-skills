"""Exercise bounded raster tables and split behavior, then inspect target code."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
LAB = ROOT / "skills/sgdk-raster/assets/raster-lab"
FIXTURE = ROOT / "tests/fixtures/raster"


class RasterBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = next((p for name in ["clang", "cc", "gcc"] if (p := shutil.which(name))), None)
        if compiler is None:
            if os.environ.get("SGDK_REQUIRE_HOST_CC") == "1":
                raise RuntimeError("CI requires a C17 compiler for raster behavior tests")
            raise unittest.SkipTest("Raster tests need clang, cc or gcc")
        (ROOT / "build").mkdir(exist_ok=True)
        cls.folder = tempfile.TemporaryDirectory(prefix="raster host ", dir=ROOT / "build")
        cls.addClassCleanup(cls.folder.cleanup)
        project = Path(cls.folder.name) / "copied raster lab"
        shutil.copytree(LAB, project)
        cls.binary = Path(cls.folder.name) / ("raster-test.exe" if os.name == "nt" else "raster-test")
        r = subprocess.run([compiler, "-std=c17", "-Wall", "-Wextra", "-Werror", "-DRASTER_TEST",
                            "-I", str(FIXTURE), "-I", str(project / "inc"),
                            str(project / "src/raster_model.c"), str(project / "src/water_split.c"),
                            str(FIXTURE / "raster_test.c"), "-o", str(cls.binary)],
                           capture_output=True, text=True, timeout=60)
        if r.returncode:
            raise RuntimeError((r.stdout + r.stderr)[-6000:])

    def check(self, case):
        r = subprocess.run([str(self.binary), case], capture_output=True, text=True, timeout=10)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(r.stdout.strip(), case)

    def test_region_clock_input_edges_pause_camera_and_waterline_bounds(self): self.check("model")
    def test_wave_tables_all_phases_and_quantized_palette_bounds(self): self.check("tables")
    def test_one_shot_split_copied_palettes_and_vdp_state_restoration(self): self.check("split")
    def test_late_setup_delayed_interrupt_and_bounded_fifo_timeout(self): self.check("failure")


@unittest.skipUnless(os.environ.get("SGDK_SKILLS_INTEGRATION") == "1", "Docker integration is opt-in")
class RasterContainerTests(unittest.TestCase):
    def test_real_api_both_build_modes_interrupt_abi_and_region_headers(self):
        (ROOT / "build").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="raster container ", dir=ROOT / "build") as folder:
            project = Path(folder) / "raster lab"
            shutil.copytree(LAB, project)
            base = ["docker", "run", "--rm"]
            if os.environ.get("SGDK_PLATFORM"):
                base += ["--platform", os.environ["SGDK_PLATFORM"]]
            if hasattr(os, "getuid"):
                base += ["--user", f"{os.getuid()}:{os.getgid()}"]
            base += ["--mount", f"type=bind,source={project},target=/workspace",
                     os.environ.get("SGDK_IMAGE", "ewrogers/sgdk:latest")]
            def run(*args):
                r = subprocess.run(base + list(args), capture_output=True, text=True, timeout=240)
                self.assertEqual(r.returncode, 0, (r.stdout + r.stderr)[-6000:])
                return r.stdout
            # Fixture constants and signatures are checked against the actual SDK.
            header = (FIXTURE / "genesis.h").read_text()
            checks = re.findall(r"^#define (VDP_\w+_FLAG) (.+)$", header, re.M)
            probe = "#include <genesis.h>\n" + "\n".join(
                f'_Static_assert({name} == {value}, "{name}");' for name, value in checks)
            probe += "\n" + "\n".join(re.findall(r"^(?:void|u8) [^\n]+\);$", header, re.M))
            probe += "\nvoid (*scroll)(VDPPlane, u16, s16*, u16, TransferMethod) = VDP_setHorizontalScrollLine;\n"
            (project / "api_probe.c").write_text(probe)
            run("m68k-elf-gcc", "-m68000", "-std=gnu17", "-fno-builtin",
                "-Werror=incompatible-pointer-types", "-I/opt/sgdk/inc", "-I/opt/sgdk/res",
                "-fsyntax-only", "api_probe.c")
            for mode in ["release", "debug"]:
                run("build", mode)
                rom = (project / "out/rom.bin").read_bytes()
                self.assertEqual(rom[0x100:0x104], b"SEGA")
                self.assertEqual(len(rom) % 131072, 0)
                assembly = run("m68k-elf-objdump", "-d", "out/rom.out")
                match = re.search(r"<water_split_interrupt>:\n(.*?)(?=\n\n)", assembly, re.S)
                self.assertIsNotNone(match)
                code = match.group(1)
                self.assertRegex(code, r"\brte\b")
                self.assertNotRegex(code, r"\b(?:rts|jsr|bsr\w*)\b")
                self.assertGreaterEqual(len(re.findall(r"\bdbf\b", code)), 2)
                self.assertGreaterEqual(len(re.findall(r"movel %d[0-7],%a[0-7]@", code)), 3)
            # Change only this disposable project's generated SDK header.
            header_file = project / "src/boot/rom_head.c"
            original = header_file.read_text()
            self.assertIn('"JUE             "', original)
            for region in ["U", "E"]:
                header_file.write_text(original.replace('"JUE             "', '"' + region.ljust(16) + '"'))
                run("build", "clean")
                run("build", "release")
                self.assertEqual((project / "out/rom.bin").read_bytes()[0x1F0:0x200], region.encode().ljust(16))


if __name__ == "__main__":
    unittest.main()
