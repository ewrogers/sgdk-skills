"""Execute the installed Menu Lab model/renderer and build its real font/ROM."""
import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SKILL = ROOT / "skills/sgdk-ui"
LAB = SKILL / "assets/menu-lab"
FIXTURE = ROOT / "tests/fixtures/ui"
IMAGE = os.environ.get("SGDK_IMAGE", "ewrogers/sgdk:latest")


def font_module():
    spec = importlib.util.spec_from_file_location("menu_font", SKILL / "scripts/make_lab_font.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class MenuBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = next((p for name in ["clang", "cc", "gcc"] if (p := shutil.which(name))), None)
        if not compiler:
            if os.environ.get("SGDK_REQUIRE_HOST_CC") == "1":
                raise RuntimeError("CI requires a C17 compiler for UI tests")
            raise unittest.SkipTest("UI tests need a C17 compiler")
        # Keep all disposable outputs in the ignored build directory.
        (ROOT / "build").mkdir(exist_ok=True)
        cls.scratch = tempfile.TemporaryDirectory(prefix="ui host ", dir=ROOT / "build")
        cls.addClassCleanup(cls.scratch.cleanup)
        copied = Path(cls.scratch.name) / "independent lab"
        shutil.copytree(LAB, copied)
        suffix = ".exe" if os.name == "nt" else ""
        cls.model = Path(cls.scratch.name) / ("menu-test" + suffix)
        cls.view = Path(cls.scratch.name) / ("view-test" + suffix)
        common = [compiler, "-std=c17", "-O2", "-Wall", "-Wextra", "-Werror", "-I", str(copied / "inc")]
        for binary, sources in [
            (cls.model, [str(copied / "src/menu.c"), str(FIXTURE / "menu_test.c")]),
            (cls.view, ["-I", str(FIXTURE), str(copied / "src/menu.c"),
                        str(copied / "src/view.c"), str(FIXTURE / "view_test.c")]),
        ]:
            result = subprocess.run(common + sources + ["-o", str(binary)],
                                    capture_output=True, text=True, timeout=60)
            if result.returncode:
                raise RuntimeError((result.stdout + result.stderr)[-6000:])

    def case(self, name):
        result = subprocess.run([str(self.model), name], capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.strip(), name)

    def test_cursor_wrap_opposites_and_repeat_schedule(self): self.case("navigation")
    def test_options_font_choice_persistence_and_credits(self): self.case("options")
    def test_actions_are_edges_and_scores_are_bounded(self): self.case("edges")
    def test_pause_resume_restart_results_and_title_transitions(self): self.case("transitions")
    def test_nominal_pal_ntsc_round_duration(self): self.case("timing")
    def test_marker_bounds_and_scroll_wrap(self): self.case("movement")
    def test_text_padding_clipping_encoding_and_number_capacity(self): self.case("formatting")

    def test_dirty_rendering_fixed_hud_font_palette_ownership_and_stale_text(self):
        result = subprocess.run([str(self.view)], capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.strip(), "view")


class MenuFontTests(unittest.TestCase):
    def test_original_ascii_glyph_slots_and_reproducibility(self):
        module = font_module()
        self.assertEqual(module.font_png(), (LAB / "res/font.png").read_bytes())
        pixels = module.pixels()
        self.assertEqual(len(pixels), 128 * 48)
        self.assertEqual(set(pixels), {0, 15})
        def glyph(character):
            slot = ord(character) - 32
            return [pixels[(slot // 16 * 8 + y) * 128 + slot % 16 * 8:
                           (slot // 16 * 8 + y) * 128 + slot % 16 * 8 + 8] for y in range(8)]
        self.assertEqual(glyph(" "), [bytes(8)] * 8)
        self.assertEqual(glyph("A")[0], bytes([0, 0, 15, 15, 15, 0, 0, 0]))
        self.assertEqual(glyph("a"), glyph("A"))
        self.assertNotEqual(glyph("1"), glyph("2"))

    def test_standalone_generator_refuses_to_overwrite(self):
        (ROOT / "build").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="ui art ", dir=ROOT / "build") as folder:
            script = Path(folder) / "font.py"
            shutil.copy2(SKILL / "scripts/make_lab_font.py", script)
            output = Path(folder) / "new font.png"
            result = subprocess.run([sys.executable, str(script), str(output)], capture_output=True)
            self.assertEqual(result.returncode, 0)
            self.assertEqual(output.read_bytes(), font_module().font_png())
            output.write_bytes(b"user font")
            result = subprocess.run([sys.executable, str(script), str(output)], capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(output.read_bytes(), b"user font")


@unittest.skipUnless(os.environ.get("SGDK_SKILLS_INTEGRATION") == "1", "Docker integration is opt-in")
class MenuContainerTests(unittest.TestCase):
    def test_real_apis_both_build_modes_and_exact_compiled_font(self):
        (ROOT / "build").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="ui container ", dir=ROOT / "build") as folder:
            project = Path(folder) / "standalone lab"
            shutil.copytree(LAB, project)
            base = ["docker", "run", "--rm"]
            if os.environ.get("SGDK_PLATFORM"):
                base += ["--platform", os.environ["SGDK_PLATFORM"]]
            if hasattr(os, "getuid"):
                base += ["--user", f"{os.getuid()}:{os.getgid()}"]
            base += ["--mount", f"type=bind,source={project},target=/workspace", IMAGE]
            (project / "probe.c").write_text('''#include <genesis.h>
_Static_assert(FONT_LEN == 96, "ASCII font slots");
_Static_assert(TILE_USER_INDEX == 16, "SDK system tiles");
_Static_assert(TILE_ATTR(PAL1, TRUE, FALSE, FALSE) == 0xA000, "text attributes");
_Static_assert(RGB24_TO_VDPCOLOR(0xCCDDEE) == 0x0EEE, "foreground quantization");
_Static_assert(RGB24_TO_VDPCOLOR(0x88BBFF) == 0x0EC8, "accent quantization");
void (*draw)(VDPPlane, const char *, u16, u16, u16, TransferMethod) = VDP_drawTextEx;
void (*clear)(VDPPlane, u16, u16, u16) = VDP_clearTextBG;
u16 (*font)(const TileSet *, TransferMethod) = VDP_loadFont;
void (*text_plane)(VDPPlane) = VDP_setTextPlane;
void (*text_palette)(u16) = VDP_setTextPalette;
void (*text_priority)(u16) = VDP_setTextPriority;
void (*window_h)(u16, u16) = VDP_setWindowHPos;
void (*window_v)(u16, u16) = VDP_setWindowVPos;
u16 (*pad)(u16) = JOY_readJoypad;
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
                self.assertEqual(len(rom) % 131072, 0)
            result = subprocess.run(base + ["m68k-elf-gdb", "-batch", "out/rom.out",
                "-ex", "p ((TileSet*)&lab_font)->numTile", "-ex", "p ((TileSet*)&lab_font)->compression",
                "-ex", "dump binary memory font.bin ((TileSet*)&lab_font)->tiles ((TileSet*)&lab_font)->tiles+768"],
                capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-3000:])
            values = [int(line.split(" = ", 1)[1].split()[0]) for line in result.stdout.splitlines()
                      if line.startswith("$") and " = " in line]
            self.assertEqual(values, [96, 0])
            pixels = font_module().pixels()
            expected = bytearray()
            for slot in range(96):
                for y in range(8):
                    start = (slot // 16 * 8 + y) * 128 + slot % 16 * 8
                    expected.extend((pixels[start+x] << 4) | pixels[start+x+1] for x in range(0, 8, 2))
            self.assertEqual((project / "font.bin").read_bytes(), expected)


if __name__ == "__main__":
    unittest.main()
