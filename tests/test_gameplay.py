"""Execute the gameplay model and compile its real SGDK adapter."""
import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SKILL = ROOT / "skills/sgdk-gameplay"
LAB = SKILL / "assets/gameplay-lab"
IMAGE = os.environ.get("SGDK_IMAGE", "ewrogers/sgdk:2.11-gcc16.2.0-binutils2.47@sha256:fd7a891d37773f792aad528bb6640d63bc0bd8e86ed140b8ef43d296f035f51c")


class GameplayBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = next((p for name in ["clang", "cc", "gcc"] if (p := shutil.which(name))), None)
        if not compiler:
            if os.environ.get("SGDK_REQUIRE_HOST_CC") == "1":
                raise RuntimeError("CI requires a C17 compiler for gameplay tests")
            raise unittest.SkipTest("Gameplay tests need a C17 compiler")
        cls.scratch = tempfile.TemporaryDirectory(prefix="sgdk gameplay host ")
        cls.addClassCleanup(cls.scratch.cleanup)
        copied = Path(cls.scratch.name) / "independent lab"
        shutil.copytree(LAB, copied)
        cls.binary = Path(cls.scratch.name) / ("game-test.exe" if os.name == "nt" else "game-test")
        result = subprocess.run([compiler, "-std=c17", "-Wall", "-Wextra", "-Werror",
                                 "-I", str(copied / "inc"), str(copied / "src/game.c"),
                                 str(ROOT / "tests/fixtures/gameplay/game_test.c"),
                                 "-o", str(cls.binary)], capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise RuntimeError((result.stdout + result.stderr)[-6000:])

    def case(self, name):
        result = subprocess.run([str(self.binary), name], capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.strip(), name)

    def test_reset_releases_all_game_slots(self): self.case("reset")
    def test_movement_diagonal_speed_and_opposing_inputs(self): self.case("movement")
    def test_map_bounds_and_half_open_tile_edges(self): self.case("bounds")
    def test_fractional_wall_contact_and_axis_sliding(self): self.case("wall_slide")
    def test_aabb_edges_empty_boxes_and_widened_sums(self): self.case("overlap")
    def test_full_pool_failure_preserves_state_and_reuses_a_slot(self): self.case("pool")
    def test_expiry_before_admission_and_new_shot_waits_one_tick(self): self.case("admission")
    def test_held_fire_respects_tick_cooldown(self): self.case("cooldown")
    def test_projectiles_substep_collide_and_score_once(self): self.case("sweep")
    def test_offscreen_bullets_keep_simulating(self): self.case("offscreen")
    def test_camera_clamps_without_changing_world_coordinates(self): self.case("camera")
    def test_50_and_60_refreshes_produce_equal_simulation_time(self): self.case("regions")
    def test_pause_resets_phase_and_invalid_refresh_is_rejected(self): self.case("pause")


class GameplayArtTests(unittest.TestCase):
    def test_generator_reproduces_assets_and_preserves_existing_art(self):
        spec = importlib.util.spec_from_file_location("gameplay_art", SKILL / "scripts/make_lab_art.py")
        generator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(generator)
        for name, data in generator.assets().items():
            self.assertEqual(data, (LAB / "res" / name).read_bytes())
        with tempfile.TemporaryDirectory(prefix="sgdk gameplay art ") as folder:
            root = Path(folder)
            script = root / "make_lab_art.py"
            shutil.copy2(SKILL / "scripts/make_lab_art.py", script)
            output = root / "new art"
            self.assertEqual(subprocess.run([sys.executable, str(script), str(output)], capture_output=True).returncode, 0)
            (output / "player.png").write_bytes(b"user art")
            before = {p.name: p.read_bytes() for p in output.iterdir()}
            self.assertNotEqual(subprocess.run([sys.executable, str(script), str(output)], capture_output=True).returncode, 0)
            self.assertEqual(before, {p.name: p.read_bytes() for p in output.iterdir()})


@unittest.skipUnless(os.environ.get("SGDK_SKILLS_INTEGRATION") == "1", "Docker integration is opt-in")
class GameplayContainerTests(unittest.TestCase):
    def test_actual_sdk_builds_both_modes(self):
        with tempfile.TemporaryDirectory(prefix="sgdk gameplay container ") as folder:
            project = Path(folder) / "standalone lab"
            shutil.copytree(LAB, project)
            base = ["docker", "run", "--rm"]
            if os.environ.get("SGDK_PLATFORM"):
                base += ["--platform", os.environ["SGDK_PLATFORM"]]
            if hasattr(os, "getuid"):
                base += ["--user", f"{os.getuid()}:{os.getgid()}"]
            base += ["--mount", f"type=bind,source={project},target=/workspace", IMAGE]
            (project / "api_probe.c").write_text('''#include <genesis.h>
_Static_assert(FIX32_FRAC_BITS == 10, "SGDK fix32 scaling");
_Static_assert(FIX32(1.5) == 1536, "Lab straight-line speed");
s32 (*to_pixels)(fix32) = F32_toInt;
fix32 (*multiply)(fix32, fix32) = F32_mul;
bool (*vblank)(void) = SYS_doVBlankProcess;
u32 (*fps)(void) = SYS_getFPS;
u16 (*load)(void) = SYS_getCPULoad;
Pool *(*create_pool)(u16, u16) = POOL_create;
void *(*allocate)(Pool *) = POOL_allocate;
void (*release)(Pool *, void *, bool) = POOL_release;
void (*reset)(Pool *, bool) = POOL_reset;
void (*destroy)(Pool *) = POOL_destroy;
Map *(*create_map)(const MapDefinition *, VDPPlane, u16) = MAP_create;
void (*scroll)(Map *, u32, u32) = MAP_scrollTo;
void (*place)(Sprite *, s16, s16) = SPR_setPosition;
void (*visible)(Sprite *, SpriteVisibility) = SPR_setVisibility;
''', encoding="utf-8")
            probe = subprocess.run(base + ["m68k-elf-gcc", "-m68000", "-std=gnu17", "-fno-builtin",
                                   "-Werror=implicit-function-declaration", "-Werror=incompatible-pointer-types",
                                   "-I/opt/sgdk/inc", "-I/opt/sgdk/res", "-fsyntax-only", "api_probe.c"],
                                   capture_output=True, text=True, timeout=60)
            self.assertEqual(probe.returncode, 0, (probe.stdout + probe.stderr)[-6000:])
            for mode in ["release", "debug"]:
                result = subprocess.run(base + ["build", mode], capture_output=True, text=True, timeout=240)
                self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-8000:])
                rom = (project / "out/rom.bin").read_bytes()
                self.assertEqual(rom[0x100:0x104], b"SEGA")
                self.assertEqual(len(rom) % 131072, 0)
                self.assertGreater(len(rom), 512)


if __name__ == "__main__":
    unittest.main()
