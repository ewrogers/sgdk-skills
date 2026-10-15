"""Execute Tiled collision import, world logic and the real SGDK streaming adapter."""
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
import xml.etree.ElementTree as ET
import zlib

ROOT = Path(__file__).resolve().parents[1]
LAB = ROOT / "skills/sgdk-levels/assets/level-lab"
FIXTURE = ROOT / "tests/fixtures/levels"
IMAGE = os.environ.get("SGDK_IMAGE", "ewrogers/sgdk:latest")


def importer():
    spec = importlib.util.spec_from_file_location("level_import", LAB / "tools/tiled_collision.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def scratch(prefix):
    (ROOT / "build").mkdir(exist_ok=True)
    return tempfile.TemporaryDirectory(prefix=prefix, dir=ROOT / "build")


class TiledImportTests(unittest.TestCase):
    def setUp(self):
        self.scratch = scratch("levels import ")
        self.addCleanup(self.scratch.cleanup)
        self.project = Path(self.scratch.name) / "independent lab"
        shutil.copytree(LAB, self.project)
        self.path = self.project / "res/outpost.tmx"
        self.module = importer()

    def edit(self, action):
        tree = ET.parse(self.path)
        action(tree.getroot())
        tree.write(self.path)

    def reject(self, action):
        self.edit(action)
        with self.assertRaises((ValueError, OSError, ET.ParseError)):
            self.module.read_collision(self.path)

    def test_bitset_matches_every_authored_cell_and_stays_in_rom_source(self):
        data = self.module.read_collision(self.path)
        cells = [int(v) for v in ET.parse(self.path).find("layer[@name='Collision']/data").text.split(",")]
        self.assertEqual((data.columns, data.rows, data.cell, len(data.bits)), (64, 64, 16, 512))
        for index, gid in enumerate(cells):
            self.assertEqual(bool(data.bits[index // 8] & (1 << (index % 8))), bool(gid))
        header, source = self.project / "inc/level_data.h", self.project / "src/level_data.c"
        self.module.export(data, header, source)
        original = (header.read_bytes(), source.read_bytes())
        self.module.export(data, header, source)
        self.assertEqual(original, (header.read_bytes(), source.read_bytes()))

    def test_transformed_nonzero_gids_preserve_occupancy(self):
        data = self.module.read_collision(self.path)
        def flip(root):
            node = root.find("layer[@name='Collision']/data")
            node.text = ",".join(str(int(v) | 0xC0000000) if int(v) else "0" for v in node.text.split(","))
        self.edit(flip)
        self.assertEqual(self.module.read_collision(self.path).bits, data.bits)

    def test_external_firstgid_is_not_a_tile_index(self):
        before = self.module.read_collision(self.path)
        def move(root):
            root.find("tileset").set("firstgid", "101")
            node = root.find("layer[@name='Collision']/data")
            node.text = ",".join(str(int(v)+100) if int(v) else "0" for v in node.text.split(","))
        self.edit(move)
        self.assertEqual(before.bits, self.module.read_collision(self.path).bits)

    def test_rejects_infinite_nonorthogonal_and_wrong_cell_geometry(self):
        for key, value in [("infinite", "1"), ("orientation", "isometric"), ("tilewidth", "8"),
                           ("tileheight", "32"), ("width", "3000")]:
            with self.subTest(key=key):
                original = self.path.read_bytes()
                self.reject(lambda root: root.set(key, value))
                self.path.write_bytes(original)

    def test_rejects_shifted_nested_duplicate_and_mismatched_layers(self):
        for operation in [
            lambda root: root.find("layer[@name='Collision']").set("offsetx", "1"),
            lambda root: root.find("layer[@name='Collision']").set("width", "63"),
            lambda root: ET.SubElement(root, "layer", name="Collision"),
            lambda root: root.remove(root.find("layer[@name='Collision']")),
            lambda root: ET.SubElement(ET.SubElement(root, "group"), "layer", name="Collision"),
        ]:
            original = self.path.read_bytes()
            self.reject(operation)
            self.path.write_bytes(original)

    def test_rejects_compressed_chunks_bad_counts_ids_and_rotation(self):
        for value in ["0", "999", "-1", "4294967296", "536870913", "268435457", "2147483648", "wrong"]:
            original = self.path.read_bytes()
            def replace(root):
                node = root.find("layer[@name='Collision']/data")
                node.text = value if value == "0" else value + "," + node.text.split(",", 1)[1]
            self.reject(replace)
            self.path.write_bytes(original)
        self.reject(lambda root: root.find("layer[@name='Collision']/data").set("compression", "zlib"))

    def test_missing_tsx_and_cli_failure_preserve_previous_outputs(self):
        (self.project / "res/outpost.tsx").unlink()
        header = self.project / "inc/level_data.h"
        header.write_text("previous valid output")
        result = subprocess.run([sys.executable, "tools/tiled_collision.py", "res/outpost.tmx"],
                                cwd=self.project, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Collision import failed", result.stderr)
        self.assertEqual(header.read_text(), "previous valid output")


class LevelBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = next((p for name in ["clang", "cc", "gcc"] if (p := shutil.which(name))), None)
        if not compiler:
            if os.environ.get("SGDK_REQUIRE_HOST_CC") == "1":
                raise RuntimeError("CI requires a C17 compiler for level tests")
            raise unittest.SkipTest("Level tests need a C17 compiler")
        cls.scratch = scratch("levels host ")
        cls.addClassCleanup(cls.scratch.cleanup)
        copied = Path(cls.scratch.name) / "standalone lab"
        shutil.copytree(LAB, copied)
        mod = importer()
        mod.export(mod.read_collision(copied / "res/outpost.tmx"),
                   copied / "inc/level_data.h", copied / "src/level_data.c")
        suffix = ".exe" if os.name == "nt" else ""
        cls.world = Path(cls.scratch.name) / ("world-test" + suffix)
        cls.view = Path(cls.scratch.name) / ("view-test" + suffix)
        common = [compiler, "-std=c17", "-O2", "-Wall", "-Wextra", "-Werror", "-I", str(copied / "inc")]
        for binary, sources in [
            (cls.world, [str(copied / "src/world.c"), str(copied / "src/level_data.c"), str(FIXTURE / "world_test.c")]),
            (cls.view, ["-I", str(FIXTURE), str(copied / "src/level_view.c"), str(FIXTURE / "view_test.c")]),
        ]:
            result = subprocess.run(common + sources + ["-o", str(binary)],
                                    capture_output=True, text=True, timeout=60)
            if result.returncode:
                raise RuntimeError((result.stdout + result.stderr)[-6000:])

    def case(self, name):
        result = subprocess.run([str(self.world), name], capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.strip(), name)

    def test_fractional_edges_substeps_and_wall_sliding(self): self.case("collision")
    def test_camera_limits_small_worlds_and_invalid_warps(self): self.case("camera")
    def test_pal_ntsc_motion_diagonals_opposites_and_pause(self): self.case("timing")
    def test_authored_corridors_beacons_and_long_run_bounds(self): self.case("authored")
    def test_facing_walk_idle_phase_and_pause(self): self.case("animation")

    def test_streaming_ownership_teardown_failures_and_vram_boundaries(self):
        result = subprocess.run([str(self.view)], capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.strip(), "view")


class LevelArtTests(unittest.TestCase):
    def test_indexed_art_uses_the_planned_hardware_palettes(self):
        palettes = json.loads((LAB / "res/palette.json").read_text())
        for name, size in [("outpost.png", (128, 48)), ("scout.png", (128, 48))]:
            colors = palettes["terrain" if name == "outpost.png" else "robot"]["rgb"]
            expected = b"".join(bytes.fromhex(c) for c in colors)
            self.assertEqual(len(colors), 16)
            self.assertTrue(all(v % 34 == 0 for v in expected))
            data = (LAB / "res" / name).read_bytes()
            chunks, offset = {}, 8
            while offset < len(data):
                length = int.from_bytes(data[offset:offset+4], "big")
                kind = data[offset+4:offset+8]
                payload = data[offset+8:offset+8+length]
                self.assertEqual(zlib.crc32(kind+payload), int.from_bytes(data[offset+8+length:offset+12+length], "big"))
                chunks[kind] = payload
                offset += 12 + length
            self.assertEqual(struct.unpack(">II", chunks[b"IHDR"][:8]), size)
            self.assertIn(chunks[b"IHDR"][8], [4, 8])
            self.assertEqual(chunks[b"IHDR"][9], 3)
            self.assertEqual(chunks[b"PLTE"], expected)
            if name == "scout.png": self.assertEqual(chunks[b"tRNS"], b"\0")


@unittest.skipUnless(os.environ.get("SGDK_SKILLS_INTEGRATION") == "1", "Docker integration is opt-in")
class LevelContainerTests(unittest.TestCase):
    def test_tiled_resources_api_both_build_modes_and_linked_collision(self):
        with scratch("levels container ") as folder:
            project = Path(folder) / "standalone lab"
            shutil.copytree(LAB, project)
            base = ["docker", "run", "--rm"]
            if os.environ.get("SGDK_PLATFORM"):
                base += ["--platform", os.environ["SGDK_PLATFORM"]]
            if hasattr(os, "getuid"):
                base += ["--user", f"{os.getuid()}:{os.getgid()}"]
            base += ["--mount", f"type=bind,source={project},target=/workspace", IMAGE]
            (project / "probe.c").write_text('''#include <genesis.h>
Map *(*create)(const MapDefinition *, VDPPlane, u16) = MAP_create;
void (*scroll)(Map *, u32, u32, bool) = MAP_scrollToEx;
void (*release_map)(Map *) = MAP_release;
u16 (*load_tiles)(const TileSet *, u16, TransferMethod) = VDP_loadTileSet;
void (*window)(u16, u16) = VDP_setWindowVPos;
void (*manual_frame)(Sprite *, s16, s16) = SPR_setAnimAndFrame;
void (*auto_frame)(Sprite *, bool) = SPR_setAutoAnimation;
void (*flip)(Sprite *, bool) = SPR_setHFlip;
_Static_assert(FIX32_FRAC_BITS == 10, "fixed-point scale");
''', encoding="utf-8")
            commands = [base + ["m68k-elf-gcc", "-m68000", "-std=gnu17", "-fno-builtin",
                "-Werror=implicit-function-declaration", "-Werror=incompatible-pointer-types",
                "-I/opt/sgdk/inc", "-I/opt/sgdk/res", "-fsyntax-only", "probe.c"],
                base + ["python3", "tools/tiled_collision.py", "res/outpost.tmx"]]
            for command in commands:
                result = subprocess.run(command, capture_output=True, text=True, timeout=60)
                self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-5000:])
            for mode in ["release", "debug"]:
                result = subprocess.run(base + ["build", mode], capture_output=True, text=True, timeout=240)
                self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-8000:])
                rom = (project / "out/rom.bin").read_bytes()
                self.assertEqual(rom[0x100:0x104], b"SEGA")
                self.assertEqual(len(rom) % 131072, 0)
            result = subprocess.run(base + ["m68k-elf-gdb", "-batch", "out/rom.out",
                "-ex", "p ((MapDefinition*)&outpost_map)->w", "-ex", "p ((MapDefinition*)&outpost_map)->h",
                "-ex", "p ((Palette*)&terrain_palette)->length",
                "-ex", "p ((SpriteDefinition*)&scout_image)->w",
                "-ex", "p ((SpriteDefinition*)&scout_image)->h",
                "-ex", "p ((SpriteDefinition*)&scout_image)->numAnimation",
                "-ex", "p ((SpriteDefinition*)&scout_image)->animations[0]->numFrame",
                "-ex", "dump binary memory collision.bin occupancy occupancy+512"],
                capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-3000:])
            values = [int(line.split(" = ", 1)[1].split()[0]) for line in result.stdout.splitlines()
                      if line.startswith("$") and " = " in line]
            self.assertEqual(values, [8, 8, 16, 16, 16, 3, 8])  # Map dimensions are 128px blocks.
            self.assertEqual((project / "collision.bin").read_bytes(), importer().read_collision(project / "res/outpost.tmx").bits)


if __name__ == "__main__":
    unittest.main()
