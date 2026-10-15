"""Run the actual input adapter with a fake SDK, then opt in to real ROM builds."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
LAB = ROOT / "skills/sgdk-input/assets/input-lab"
FIXTURE = ROOT / "tests/fixtures/input"
IMAGE = os.environ.get(
    "SGDK_IMAGE",
    "ewrogers/sgdk:2.11-gcc16.2.0-binutils2.47@sha256:fd7a891d37773f792aad528bb6640d63bc0bd8e86ed140b8ef43d296f035f51c",
)


class InputBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = next((path for name in ["clang", "cc", "gcc"]
                         if (path := shutil.which(name))), None)
        if compiler is None:
            if os.environ.get("SGDK_REQUIRE_HOST_CC") == "1":
                raise RuntimeError("CI requires a C17 compiler for input behavior tests")
            raise unittest.SkipTest("Input behavior tests need clang, cc or gcc")
        cls.folder = tempfile.TemporaryDirectory(prefix="sgdk input host ")
        cls.addClassCleanup(cls.folder.cleanup)
        project = Path(cls.folder.name) / "copied lab"
        shutil.copytree(LAB, project)
        cls.binary = Path(cls.folder.name) / ("input-test.exe" if os.name == "nt" else "input-test")
        command = [compiler, "-std=c17", "-Wall", "-Wextra", "-Werror",
                   "-I", str(FIXTURE), "-I", str(project / "inc"),
                   str(project / "src/input.c"), str(FIXTURE / "input_test.c"),
                   "-o", str(cls.binary)]
        result = subprocess.run(command, capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise RuntimeError((result.stdout + result.stderr)[-6000:])

    def check_case(self, name):
        result = subprocess.run([str(self.binary), name], capture_output=True,
                                text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.strip(), name)

    def test_press_hold_release(self): self.check_case("edges")
    def test_repeated_consumers_share_snapshot(self): self.check_case("snapshot")
    def test_alternate_bindings_have_one_action_edge(self): self.check_case("bindings")
    def test_opposing_directions_cancel(self): self.check_case("axes")
    def test_all_eight_players_remain_independent(self): self.check_case("all_players")
    def test_initial_held_button_is_not_a_press(self): self.check_case("initial_hold")
    def test_six_button_chord_press_and_release_together(self): self.check_case("six_button_changes")
    def test_rescan_installs_handler_and_rebaselines(self): self.check_case("rescan")
    def test_pad_mouse_and_unknown_port_configuration(self): self.check_case("configuration")
    def test_ea_adapter_reserves_both_ports(self): self.check_case("ea_adapter")
    def test_mouse_deltas_are_consumed_once(self): self.check_case("mouse")
    def test_team_player_slot_mapping_and_y_direction(self): self.check_case("team_player")
    def test_type_change_discards_old_motion_and_edges(self): self.check_case("type_change")
    def test_all_mega_mouse_buttons(self): self.check_case("mouse_buttons")
    def test_invalid_slot_is_neutral_without_sdk_access(self): self.check_case("invalid_slot")


@unittest.skipUnless(os.environ.get("SGDK_SKILLS_INTEGRATION") == "1",
                     "Docker integration is opt-in")
class InputContainerTests(unittest.TestCase):
    def test_lab_builds_and_mock_api_matches_real_sdk(self):
        with tempfile.TemporaryDirectory(prefix="sgdk input container ") as folder:
            project = Path(folder) / "input lab"
            shutil.copytree(LAB, project)
            base = ["docker", "run", "--rm"]
            if os.environ.get("SGDK_PLATFORM"):
                base += ["--platform", os.environ["SGDK_PLATFORM"]]
            if hasattr(os, "getuid"):
                base += ["--user", f"{os.getuid()}:{os.getgid()}"]
            base += ["--mount", f"type=bind,source={project},target=/workspace", IMAGE]

            def run(*args):
                result = subprocess.run(base + list(args), capture_output=True,
                                        text=True, timeout=240)
                self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-8000:])
                return result.stdout

            # Verify the host mock's symbols and signatures against real headers.
            assertions = []
            for name, value in re.findall(r"^#define ((?:JOY|PORT|BUTTON)_\w+) ([\w]+)$",
                                          (FIXTURE / "genesis.h").read_text(), re.M):
                assertions.append(f'_Static_assert({name} == {value}, "{name}");')
            probe = "#include <genesis.h>\n" + "\n".join(assertions) + "\n" + (
                "u8 (*port_probe)(u16) = JOY_getPortType;\n"
                "u8 (*type_probe)(u16) = JOY_getJoypadType;\n"
                "u16 (*buttons_probe)(u16) = JOY_readJoypad;\n"
                "s16 (*x_probe)(u16) = JOY_readJoypadX;\n"
                "s16 (*y_probe)(u16) = JOY_readJoypadY;\n"
                "s16 (*write_x_probe)(u16, u16) = JOY_writeJoypadX;\n"
                "s16 (*write_y_probe)(u16, u16) = JOY_writeJoypadY;\n"
                "void (*support_probe)(u16, u16) = JOY_setSupport;\n"
                "void (*callback_probe)(JoyEventCallback *) = JOY_setEventHandler;\n"
                "void (*reset_probe)(void) = JOY_reset;\n"
            )
            (project / "api_probe.c").write_text(probe, encoding="utf-8")
            flags = ["m68k-elf-gcc", "-m68000", "-std=gnu17", "-fno-builtin",
                     "-Werror=implicit-function-declaration", "-Werror=incompatible-pointer-types",
                     "-I/opt/sgdk/inc", "-I/opt/sgdk/res", "-Iinc", "-fsyntax-only"]
            run(*flags, "api_probe.c", "src/input.c", "src/main.c")
            for mode in ["release", "debug"]:
                with self.subTest(mode=mode):
                    run("build", mode)
                    rom = (project / "out/rom.bin").read_bytes()
                    self.assertGreater(len(rom), 512)
                    self.assertEqual(rom[0x100:0x104], b"SEGA")
                    self.assertEqual(len(rom) % 131072, 0)
                    run("build", "clean")


if __name__ == "__main__":
    unittest.main()
