"""Execute lab behavior, validate original art, and inspect compiled resources."""
import importlib.util
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib


ROOT = Path(__file__).resolve().parents[1]
SKILL = ROOT / "skills/sgdk-graphics"
LAB = SKILL / "assets/graphics-lab"
FIXTURE = ROOT / "tests/fixtures/graphics"
IMAGE = os.environ.get(
    "SGDK_IMAGE",
    "ewrogers/sgdk:latest",
)


class GraphicsBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = next((p for name in ["clang", "cc", "gcc"] if (p := shutil.which(name))), None)
        if compiler is None:
            if os.environ.get("SGDK_REQUIRE_HOST_CC") == "1":
                raise RuntimeError("CI requires a C17 compiler for graphics behavior tests")
            raise unittest.SkipTest("Graphics behavior tests need clang, cc or gcc")
        cls.folder = tempfile.TemporaryDirectory(prefix="sgdk graphics host ")
        cls.addClassCleanup(cls.folder.cleanup)
        project = Path(cls.folder.name) / "copied lab"
        shutil.copytree(LAB, project)
        cls.binary = Path(cls.folder.name) / ("graphics-test.exe" if os.name == "nt" else "graphics-test")
        result = subprocess.run(
            [compiler, "-std=c17", "-Wall", "-Wextra", "-Werror", "-I", str(FIXTURE),
             "-I", str(project / "inc"), str(project / "src/demo.c"),
             str(FIXTURE / "graphics_test.c"), "-o", str(cls.binary)],
            capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise RuntimeError((result.stdout + result.stderr)[-6000:])

    def check_case(self, name):
        result = subprocess.run([str(self.binary), name], capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.strip(), name)

    def test_exact_fit_uses_post_sprite_init_budget(self): self.check_case("allocation")
    def test_widened_budget_arithmetic(self): self.check_case("overflow")
    def test_reject_one_tile_over_budget(self): self.check_case("one_tile_over")
    def test_failed_image_load_ends_owned_engine(self): self.check_case("draw_failure")
    def test_failed_sprite_allocation_is_not_dereferenced(self): self.check_case("sprite_failure")
    def test_invalid_and_duplicate_initialization(self): self.check_case("initialization")
    def test_pause_edges_and_one_update_per_step(self): self.check_case("pause_edges")
    def test_one_shot_completion_latches_and_replays(self): self.check_case("completion")
    def test_manual_pause_does_not_count_completion(self): self.check_case("paused_completion")
    def test_fade_edges_and_busy_guard(self): self.check_case("fade")
    def test_palette_cycle_copies_temporary_data_and_avoids_fading_slot(self): self.check_case("cycle")
    def test_parallax_wrap_preserves_half_speed_phase(self): self.check_case("scroll")
    def test_actor_clamps_and_opposite_directions_cancel(self): self.check_case("movement")
    def test_teardown_flushes_before_release_and_can_restart(self): self.check_case("teardown")


class GraphicsArtTests(unittest.TestCase):
    def test_generator_reproduces_indexed_art_and_valid_crcs(self):
        spec = importlib.util.spec_from_file_location("graphics_art", SKILL / "scripts/make_lab_art.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        expected = {"far.png": (512, 256), "near.png": (512, 256), "hero.png": (64, 32)}
        for name, data in module.images().items():
            with self.subTest(name=name):
                self.assertEqual(data, (LAB / "res" / name).read_bytes())
                self.assertEqual(data[:8], b"\x89PNG\r\n\x1a\n")
                offset, chunks = 8, {}
                while offset < len(data):
                    size = struct.unpack_from(">I", data, offset)[0]
                    kind, payload = data[offset + 4:offset + 8], data[offset + 8:offset + 8 + size]
                    crc = struct.unpack_from(">I", data, offset + 8 + size)[0]
                    self.assertEqual(crc, zlib.crc32(kind + payload))
                    chunks[kind] = payload
                    offset += size + 12
                width, height, depth, color, comp, filt, interlace = struct.unpack(">IIBBBBB", chunks[b"IHDR"])
                self.assertEqual((width, height), expected[name])
                self.assertEqual((depth, color, comp, filt, interlace), (8, 3, 0, 0, 0))
                self.assertEqual(len(chunks[b"PLTE"]), 16 * 3)
                pixels = zlib.decompress(chunks[b"IDAT"])
                self.assertEqual(len(pixels), (width + 1) * height)
                self.assertLess(max(pixels), 16)
                self.assertTrue(all(pixels[y * (width + 1)] == 0 for y in range(height)))

    def test_standalone_generator_never_replaces_existing_art(self):
        with tempfile.TemporaryDirectory(prefix="sgdk art ") as folder:
            root = Path(folder)
            script = root / "make_lab_art.py"
            shutil.copy2(SKILL / "scripts/make_lab_art.py", script)
            output = root / "new art"
            result = subprocess.run([sys.executable, str(script), str(output)], capture_output=True, timeout=20)
            self.assertEqual(result.returncode, 0, result.stderr)
            (output / "hero.png").write_bytes(b"user work")
            before = {p.name: p.read_bytes() for p in output.iterdir()}
            result = subprocess.run([sys.executable, str(script), str(output)], capture_output=True, timeout=20)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(before, {p.name: p.read_bytes() for p in output.iterdir()})


@unittest.skipUnless(os.environ.get("SGDK_SKILLS_INTEGRATION") == "1", "Docker integration is opt-in")
class GraphicsContainerTests(unittest.TestCase):
    def test_real_api_builds_and_resource_frame_holds(self):
        with tempfile.TemporaryDirectory(prefix="sgdk graphics container ") as folder:
            project = Path(folder) / "graphics lab"
            shutil.copytree(LAB, project)
            base = ["docker", "run", "--rm"]
            if os.environ.get("SGDK_PLATFORM"):
                base += ["--platform", os.environ["SGDK_PLATFORM"]]
            if hasattr(os, "getuid"):
                base += ["--user", f"{os.getuid()}:{os.getgid()}"]
            base += ["--mount", f"type=bind,source={project},target=/workspace", IMAGE]

            def run(*args):
                result = subprocess.run(base + list(args), capture_output=True, text=True, timeout=240)
                self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-8000:])
                return result.stdout

            constants = re.findall(r"^#define ((?:PAL|BUTTON_)\w*) (0x[0-9A-Fa-f]+|[0-9]+)$",
                                   (FIXTURE / "genesis.h").read_text(), re.M)
            assertions = [f'_Static_assert({name} == {value}, "{name}");' for name, value in constants]
            assertions += [f'_Static_assert({name} == {value}, "{name}");' for name, value in
                           [("CPU", 0), ("DMA", 1), ("DMA_QUEUE", 2), ("DMA_QUEUE_COPY", 3),
                            ("BG_A", 0), ("BG_B", 1), ("WINDOW", 2)]]
            # Redeclare every fake function against the actual headers. A changed
            # return type matters even if this particular caller ignores it.
            declarations = re.findall(r"^(?:void|bool|Sprite \*)[^\n]*\);$",
                                      (FIXTURE / "genesis.h").read_text(), re.M)
            # Also verify guides' APIs beyond those called by this small lab.
            probe = "#include <genesis.h>\n" + "\n".join(assertions + declarations) + "\n" + """
Map *(*create_map)(const MapDefinition *, VDPPlane, u16) = MAP_create;
void (*scroll_map)(Map *, u32, u32) = MAP_scrollTo;
void (*release_map)(Map *) = MAP_release;
void (*scroll_lines)(VDPPlane, u16, s16 *, u16, TransferMethod) = VDP_setHorizontalScrollLine;
void (*scroll_tiles)(VDPPlane, u16, s16 *, u16, TransferMethod) = VDP_setHorizontalScrollTile;
void (*scroll_columns)(VDPPlane, u16, s16 *, u16, TransferMethod) = VDP_setVerticalScrollTile;
void (*scroll_vertical)(VDPPlane, s16) = VDP_setVerticalScrollVSync;
void (*sprite_init)(u16) = SPR_initEx;
Sprite *(*sprite_safe)(const SpriteDefinition *, s16, s16, u16) = SPR_addSpriteSafe;
void (*anim)(Sprite *, s16) = SPR_setAnim;
void (*frame)(Sprite *, s16) = SPR_setFrame;
void (*next_frame)(Sprite *) = SPR_nextFrame;
bool (*anim_done)(Sprite *) = SPR_isAnimationDone;
u16 (*free_tiles)(void) = SPR_getFreeVRAM;
u16 (*largest_block)(void) = SPR_getLargestFreeVRAMBlock;
u16 (*used_pieces)(void) = SPR_getUsedVDPSprite;
void (*fade_to)(u16, const u16 *, u16, bool) = PAL_fadeToPalette;
void (*colors)(u16, const u16 *, u16, TransferMethod) = PAL_setColors;
"""
            (project / "api_probe.c").write_text(probe, encoding="utf-8")
            run("m68k-elf-gcc", "-m68000", "-std=gnu17", "-fno-builtin",
                "-Werror=implicit-function-declaration", "-Werror=incompatible-pointer-types",
                "-I/opt/sgdk/inc", "-I/opt/sgdk/res", "-fsyntax-only", "api_probe.c")
            for mode in ["release", "debug"]:
                with self.subTest(mode=mode):
                    run("build", mode)
                    rom = (project / "out/rom.bin").read_bytes()
                    self.assertEqual(rom[0x100:0x104], b"SEGA")
                    self.assertGreater(len(rom), 512)
                    self.assertEqual(len(rom) % 131072, 0)
            # Read target ELF data, not a second hand-written timing table in C.
            commands = ["m68k-elf-gdb", "--batch", "-ex", "file out/rom.out"]
            expressions = ["((SpriteDefinition*)&hero)->numAnimation"]
            for animation in range(2):
                expressions.append(f"((SpriteDefinition*)&hero)->animations[{animation}]->numFrame")
                for frame in range(4):
                    expressions.append(f"((SpriteDefinition*)&hero)->animations[{animation}]->frames[{frame}]->timer")
            expressions += ["((SpriteDefinition*)&hero)->w", "((SpriteDefinition*)&hero)->h"]
            for expression in expressions:
                commands += ["-ex", f'printf "VALUE:%d\\n", {expression}']
            output = run(*commands)
            values = [int(x) for x in re.findall(r"VALUE:(\d+)", output)]
            self.assertEqual(values, [2, 4, 18, 4, 4, 4, 4, 5, 5, 5, 10, 16, 16])


if __name__ == "__main__":
    unittest.main()
