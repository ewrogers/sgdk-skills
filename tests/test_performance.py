"""Compare the lab's exact collision sets and compile the real SGDK workload."""
import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SKILL = ROOT / "skills/sgdk-performance"
LAB = SKILL / "assets/performance-lab"
IMAGE = os.environ.get("SGDK_IMAGE", "ewrogers/sgdk:2.11-gcc16.2.0-binutils2.47@sha256:fd7a891d37773f792aad528bb6640d63bc0bd8e86ed140b8ef43d296f035f51c")


class PerformanceBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = next((p for name in ["clang", "cc", "gcc"] if (p := shutil.which(name))), None)
        if not compiler:
            if os.environ.get("SGDK_REQUIRE_HOST_CC") == "1":
                raise RuntimeError("CI requires a C17 compiler for performance tests")
            raise unittest.SkipTest("Performance tests need a C17 compiler")
        cls.scratch = tempfile.TemporaryDirectory(prefix="sgdk performance host ")
        cls.addClassCleanup(cls.scratch.cleanup)
        copied = Path(cls.scratch.name) / "independent lab"
        shutil.copytree(LAB, copied)
        cls.binary = Path(cls.scratch.name) / ("performance-test.exe" if os.name == "nt" else "performance-test")
        result = subprocess.run([compiler, "-std=c17", "-O2", "-Wall", "-Wextra", "-Werror",
                                 "-I", str(copied / "inc"), str(copied / "src/workload.c"),
                                 str(ROOT / "tests/fixtures/performance/workload_test.c"),
                                 "-o", str(cls.binary)], capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise RuntimeError((result.stdout + result.stderr)[-6000:])

    def case(self, name):
        result = subprocess.run([str(self.binary), name], capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.strip(), name)

    def test_reset_capacity_and_idle_baseline(self): self.case("reset")
    def test_reflection_and_long_run_bounds(self): self.case("motion")
    def test_half_open_overlap_and_grid_edges(self): self.case("edges")
    def test_exact_collision_sets_over_multiple_capacities_and_ticks(self): self.case("equivalence")
    def test_grid_reduction_and_dense_worst_case(self): self.case("dense")
    def test_animation_has_a_fixed_tick_schedule(self): self.case("animation")
    def test_bounded_sample_window_warmup_and_refresh_accounting(self): self.case("window")
    def test_algorithm_choice_preserves_world_trajectory(self): self.case("repeat")


class PerformanceArtTests(unittest.TestCase):
    def test_reproducible_assets_and_overwrite_refusal(self):
        spec = importlib.util.spec_from_file_location("performance_art", SKILL / "scripts/make_lab_art.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        for name, data in module.assets().items():
            self.assertEqual(data, (LAB / "res" / name).read_bytes())
        with tempfile.TemporaryDirectory(prefix="sgdk performance art ") as folder:
            script = Path(folder) / "make_lab_art.py"
            shutil.copy2(SKILL / "scripts/make_lab_art.py", script)
            output = Path(folder) / "new art"
            result = subprocess.run([sys.executable, str(script), str(output)], capture_output=True)
            self.assertEqual(result.returncode, 0)
            (output / "orb.png").write_bytes(b"user art")
            result = subprocess.run([sys.executable, str(script), str(output)], capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual((output / "orb.png").read_bytes(), b"user art")


@unittest.skipUnless(os.environ.get("SGDK_SKILLS_INTEGRATION") == "1", "Docker integration is opt-in")
class PerformanceContainerTests(unittest.TestCase):
    def test_sdk_api_and_release_debug_roms(self):
        with tempfile.TemporaryDirectory(prefix="sgdk performance container ") as folder:
            project = Path(folder) / "standalone lab"
            shutil.copytree(LAB, project)
            base = ["docker", "run", "--rm"]
            if os.environ.get("SGDK_PLATFORM"):
                base += ["--platform", os.environ["SGDK_PLATFORM"]]
            if hasattr(os, "getuid"):
                base += ["--user", f"{os.getuid()}:{os.getgid()}"]
            base += ["--mount", f"type=bind,source={project},target=/workspace", IMAGE]
            (project / "probe.c").write_text('''#include <genesis.h>
_Static_assert(SUBTICKPERSECOND == 76800, "SGDK timer units");
u32 (*fps)(void) = SYS_getFPS;
u16 (*cpu)(void) = SYS_getCPULoad;
u32 (*subticks)(void) = getSubTick;
u16 (*queued_bytes)(void) = DMA_getQueueTransferSize;
u16 (*queued_ops)(void) = DMA_getQueueSize;
void (*queue_capacity)(u16) = DMA_setMaxQueueSize;
bool (*transfer)(TransferMethod, u8, void *, u16, u16, u16) = DMA_transfer;
void (*auto_animation)(Sprite *, bool) = SPR_setAutoAnimation;
void (*frame)(Sprite *, s16) = SPR_setFrame;
void (*load_cursor)(bool) = SYS_showFrameLoad;
void (*hide_cursor)(void) = SYS_hideFrameLoad;
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
            # Resource symbols are assembly labels; use the SDK's type from debug DWARF.
            result = subprocess.run(base + ["m68k-elf-gdb", "-batch", "out/rom.out",
                                           "-ex", "p ((SpriteDefinition*)&orb_image)->w",
                                           "-ex", "p ((SpriteDefinition*)&orb_image)->h",
                                           "-ex", "p ((SpriteDefinition*)&orb_image)->numAnimation",
                                           "-ex", "p ((SpriteDefinition*)&orb_image)->animations[0]->numFrame"],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-3000:])
            values = [int(line.split(" = ", 1)[1].split()[0]) for line in result.stdout.splitlines()
                      if line.startswith("$") and " = " in line]
            self.assertEqual(values, [8, 8, 1, 2])


if __name__ == "__main__":
    unittest.main()
