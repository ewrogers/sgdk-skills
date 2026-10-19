"""Behavior checks for the installed project helper without requiring Docker."""
import contextlib
import csv
import importlib.util
import io
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("sgdk_project", ROOT / "skills/sgdk-project/scripts/sgdk.py")
sgdk = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(sgdk)


class ProjectTests(unittest.TestCase):
    def test_mount_keeps_spaces_commas_and_shell_characters_in_one_argument(self):
        with tempfile.TemporaryDirectory(prefix="sgdk test,") as temporary:
            folder = Path(temporary) / "game; literal"
            folder.mkdir()
            command = sgdk.docker_command("image:test", "linux/amd64", folder)
            mount = command[command.index("--mount") + 1]
            fields = next(csv.reader([mount]))
            self.assertEqual(fields, ["type=bind", f"source={folder.resolve()}", "target=/workspace"])
            self.assertEqual(command[-1], "image:test")
            self.assertEqual(command[command.index("--platform") + 1], "linux/amd64")

    def test_missing_project_is_rejected_before_running_docker(self):
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaisesRegex(ValueError, "does not exist"):
                sgdk.docker_command("image", project=Path(temporary) / "missing")

    def test_no_mount_or_unix_identity_is_needed_for_doctor(self):
        self.assertEqual(sgdk.docker_command("image"), ["docker", "run", "--rm", "image"])

    def test_latest_is_pulled_for_each_explicit_platform_without_substitution(self):
        for platform in [None, "linux/arm64", "linux/amd64"]:
            command = sgdk.docker_command(sgdk.DEFAULT_IMAGE, platform)
            self.assertEqual(command[-1], "ewrogers/sgdk:latest")
            self.assertIn("--pull=always", command)
            if platform:
                self.assertEqual(command[command.index("--platform") + 1], platform)

    def test_custom_local_image_does_not_force_a_registry_pull(self):
        command = sgdk.docker_command("my-local-build:test", "linux/arm64")
        self.assertEqual(command[-1], "my-local-build:test")
        self.assertNotIn("--pull=always", command)

    def test_native_windows_does_not_require_unix_uid(self):
        with tempfile.TemporaryDirectory() as temporary:
            # The same branch is naturally exercised on Windows CI.
            with patch.object(sgdk, "os", spec=[]):
                self.assertNotIn("--user", sgdk.docker_command("image", project=temporary))

    def test_run_preserves_arguments_and_exit_status(self):
        with patch.object(sgdk.subprocess, "run", return_value=subprocess.CompletedProcess([], 17)) as run:
            result = sgdk.run("image", None, None, ["tool", "two words", "semi;colon"])
        self.assertEqual(result, 17)
        self.assertEqual(run.call_args.args[0][-3:], ["tool", "two words", "semi;colon"])
        self.assertNotIn("shell", run.call_args.kwargs)

    def test_initializer_refuses_existing_project(self):
        with tempfile.TemporaryDirectory() as temporary:
            file = Path(temporary) / "main.c"
            file.write_text("keep me", encoding="utf-8")
            with patch.object(sgdk, "run") as run:
                with self.assertRaisesRegex(ValueError, "already exists"):
                    sgdk.initialize(temporary, "image", None)
                run.assert_not_called()
            self.assertEqual(file.read_text(), "keep me")

    def test_failed_copy_leaves_no_partial_destination(self):
        with tempfile.TemporaryDirectory() as temporary:
            destination = Path(temporary) / "game"
            with patch.object(sgdk, "run", return_value=23):
                self.assertEqual(sgdk.initialize(destination, "image", None), 23)
            self.assertFalse(destination.exists())
            self.assertEqual(list(Path(temporary).iterdir()), [])

    def test_successful_copy_publishes_complete_example(self):
        def copy_example(image, platform, project, arguments):
            (project / "src").mkdir()
            (project / "src/main.c").write_text("int main(void) { return 0; }", encoding="utf-8")
            return 0

        with tempfile.TemporaryDirectory() as temporary:
            destination = Path(temporary) / "new game"
            with patch.object(sgdk, "run", side_effect=copy_example), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(sgdk.initialize(destination, "image", None), 0)
            self.assertTrue((destination / "src/main.c").exists())
            self.assertEqual((destination / ".gitignore").read_text(), "out/\n")
            self.assertEqual(list(Path(temporary).iterdir()), [destination])

    def test_missing_docker_explains_prerequisite(self):
        stderr = io.StringIO()
        with patch.object(sgdk.shutil, "which", return_value=None), contextlib.redirect_stderr(stderr):
            self.assertEqual(sgdk.main(["doctor"]), 127)
        self.assertIn("Docker CLI", stderr.getvalue())

    def test_build_requires_explicit_project(self):
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit) as error:
                sgdk.main(["build"])
        self.assertEqual(error.exception.code, 2)

    def test_failed_build_never_invokes_uploader(self):
        with patch.object(sgdk, "run", return_value=19) as build:
            with patch.object(sgdk.subprocess, "run") as upload:
                self.assertEqual(sgdk.deploy("image", None, "project", "unused"), 19)
                upload.assert_not_called()
        self.assertEqual(build.call_args.args[-1], ["build", "release"])

    def test_successful_build_uses_installed_host_client_and_hard_reset_by_default(self):
        with tempfile.TemporaryDirectory(prefix="game with spaces ") as temporary:
            with patch.object(sgdk, "run", return_value=0):
                with patch.object(sgdk.subprocess, "run", return_value=subprocess.CompletedProcess([], 7)) as upload:
                    self.assertEqual(sgdk.deploy("image", None, temporary, "/dev/explicit"), 7)
            args = upload.call_args.args[0]
            self.assertEqual(args[1], str(Path(sgdk.__file__).with_name("everdrive.py")))
            self.assertEqual(args[-3:], [str(Path(temporary).resolve() / "out/rom.bin"), "--reset", "hard"])
            self.assertNotIn("shell", upload.call_args.kwargs)

    def test_explicit_soft_override_is_forwarded(self):
        with patch.object(sgdk, "run", return_value=0):
            with patch.object(sgdk.subprocess, "run", return_value=subprocess.CompletedProcess([], 0)) as upload:
                sgdk.deploy("image", None, ".", "COM4", reset="soft")
        self.assertEqual(upload.call_args.args[0][-2:], ["--reset", "soft"])

    def test_invalid_usb_timeout_is_rejected_before_build(self):
        for value in ["nan", "inf", "0", "-1"]:
            with patch.object(sgdk, "run") as build, contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit):
                    sgdk.main(["--project", ".", "deploy", "--port", "unused", "--timeout", value])
                build.assert_not_called()


if __name__ == "__main__":
    unittest.main()
