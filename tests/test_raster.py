"""Exercise bounded raster tables and split behavior, then inspect target code."""
import os
import importlib.util
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import struct
import zlib
import unittest


ROOT = Path(__file__).resolve().parents[1]
LAB = ROOT / "skills/sgdk-raster/assets/raster-lab"
SPLIT_LAB = ROOT / "skills/sgdk-raster/assets/split-lab"
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
                            str(project / "src/water_fx.c"),
                            str(FIXTURE / "raster_test.c"), "-o", str(cls.binary)],
                           capture_output=True, text=True, timeout=60)
        if r.returncode:
            raise RuntimeError((r.stdout + r.stderr)[-6000:])
        split = Path(cls.folder.name) / "copied split lab"
        shutil.copytree(SPLIT_LAB, split)
        cls.split_binary = Path(cls.folder.name) / ("split-test.exe" if os.name == "nt" else "split-test")
        r = subprocess.run([compiler, "-std=c17", "-Wall", "-Wextra", "-Werror", "-DRASTER_TEST",
                            "-I", str(FIXTURE), "-I", str(split / "inc"),
                            str(split / "src/split_model.c"), str(split / "src/split_video.c"),
                            str(FIXTURE / "split_test.c"), "-o", str(cls.split_binary)],
                           capture_output=True, text=True, timeout=60)
        if r.returncode:
            raise RuntimeError((r.stdout + r.stderr)[-6000:])

    def check(self, case):
        binary = self.split_binary if case.startswith("split-") else self.binary
        r = subprocess.run([str(binary), case], capture_output=True, text=True, timeout=10)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(r.stdout.strip(), case)

    def test_region_clock_input_edges_pause_camera_and_waterline_bounds(self): self.check("model")
    def test_wave_tables_all_phases_and_quantized_palette_bounds(self): self.check("tables")
    def test_one_shot_split_copied_palettes_no_blanking_and_increment_restoration(self): self.check("split")
    def test_late_setup_delayed_interrupt_and_bounded_fifo_timeout(self): self.check("failure")
    def test_bubble_pool_splash_crossings_pause_and_region_cadence(self): self.check("effects")
    def test_early_vint_restore_moving_waterline_frame_snapshot_and_teardown(self): self.check("frame-restore")
    def test_split_independent_players_cpu_route_pause_and_regions(self): self.check("split-model")
    def test_split_camera_offsets_and_software_sprite_clipping(self): self.check("split-views")
    def test_split_vsram_hud_handoff_and_bounded_failures(self): self.check("split-video")


class RasterAssetTests(unittest.TestCase):
    def test_asset_generator_copies_standalone_and_refuses_overwrites(self):
        (ROOT / "build").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="water assets ", dir=ROOT / "build") as folder:
            script = Path(folder) / "make_assets.py"
            shutil.copy(LAB / "tools/make_assets.py", script)
            output = Path(folder) / "generated art"
            import sys
            command = [sys.executable, str(script), str(output)]
            result = subprocess.run(command, capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)
            files = {p.name: p.read_bytes() for p in output.iterdir()}
            self.assertEqual(set(files), {"water_surface.png", "water_bubble.png", "water_splash.png", "music.vgm"})
            for name, data in files.items(): self.assertEqual(data, (LAB / "res" / name).read_bytes())
            result = subprocess.run(command, capture_output=True, text=True, timeout=10)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(files, {p.name: p.read_bytes() for p in output.iterdir()})

    def test_original_assets_reproduce_and_surface_covers_handoff_in_every_frame(self):
        spec = importlib.util.spec_from_file_location("raster_assets", LAB / "tools/make_assets.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        self.assertEqual(module.music(), (LAB / "res/music.vgm").read_bytes())
        for name, generated in [("water_bubble.png", module.bubble()), ("water_splash.png", module.splash())]:
            self.assertEqual(generated, (LAB / "res" / name).read_bytes())
        png = module.surface()
        self.assertEqual(png, (LAB / "res/water_surface.png").read_bytes())
        offset, compressed = 8, bytearray()
        while offset < len(png):
            size = struct.unpack_from(">I", png, offset)[0]
            kind, data = png[offset + 4:offset + 8], png[offset + 8:offset + 8 + size]
            self.assertEqual(zlib.crc32(kind + data), struct.unpack_from(">I", png, offset + 8 + size)[0])
            if kind == b"IHDR": self.assertEqual(struct.unpack(">IIBBBBB", data), (128, 16, 8, 3, 0, 0, 0))
            if kind == b"PLTE": self.assertEqual(len(data), 48)
            if kind == b"IDAT": compressed.extend(data)
            offset += size + 12
        rows = zlib.decompress(compressed)
        self.assertEqual(len(rows), 129 * 16)
        for y in range(16):
            self.assertEqual(rows[y * 129], 0)
            pixels = rows[y * 129 + 1:(y + 1) * 129]
            self.assertLessEqual(max(pixels), 5)
            if y >= 3: self.assertNotIn(0, pixels)

    def test_underwater_phrase_has_valid_loop_timing_soft_pads_and_spaced_bells(self):
        data = (LAB / "res/music.vgm").read_bytes()
        self.assertEqual(data[:4], b"Vgm ")
        self.assertEqual(struct.unpack_from("<I", data, 4)[0] + 4, len(data))
        self.assertEqual(struct.unpack_from("<I", data, 0x18)[0], 720 * 735)
        loop = struct.unpack_from("<I", data, 0x1C)[0] + 0x1C
        offset, frames, notes, attacks = 256, 0, [], {}
        while data[offset] != 0x66:
            command = data[offset]
            if command == 0x52:
                reg, value = data[offset + 1:offset + 3]
                if reg == 0x28 and value & 0xF0: notes.append((frames, value & 7))
                if reg in (0x50, 0x51, 0x52): attacks[reg] = value
                if reg == 0x2B: self.assertEqual(value, 0) # No sampled percussion.
                offset += 3
            elif command == 0x50: offset += 2
            elif command == 0x62: frames += 1; offset += 1
            else: self.fail(f"Unexpected VGM command {command:#x}")
        self.assertEqual(offset, len(data) - 1)
        self.assertEqual(frames, 720)
        self.assertEqual(data[loop:loop + 3], bytes([0x52, 0x28, 0]))
        self.assertEqual(notes, [(frame, ch) for phrase in range(6)
                                for frame, ch in [(phrase * 120, 0), (phrase * 120, 1), (phrase * 120 + 30, 2)]])
        self.assertEqual(attacks, {0x50: 12, 0x51: 12, 0x52: 31})


@unittest.skipUnless(os.environ.get("SGDK_SKILLS_INTEGRATION") == "1", "Docker integration is opt-in")
class RasterContainerTests(unittest.TestCase):
    def test_real_api_both_build_modes_interrupt_abi_and_region_headers(self):
        self.verify_lab(LAB, "water_split_interrupt")

    def test_split_lab_both_modes_interrupt_abi_and_region_headers(self):
        self.verify_lab(SPLIT_LAB, "split_video_interrupt")

    def verify_lab(self, lab, handler):
        (ROOT / "build").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="raster container ", dir=ROOT / "build") as folder:
            project = Path(folder) / "raster lab"
            shutil.copytree(lab, project)
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
                match = re.search(r"<" + handler + r">:\n(.*?)(?=\n\n)", assembly, re.S)
                self.assertIsNotNone(match)
                code = match.group(1)
                self.assertRegex(code, r"\brte\b")
                self.assertNotRegex(code, r"\b(?:rts|jsr|bsr\w*)\b")
                self.assertGreaterEqual(len(re.findall(r"\bdbf\b", code)), 2)
                self.assertGreaterEqual(len(re.findall(r"movel %d[0-7],%a[0-7]@", code)), 2)
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
