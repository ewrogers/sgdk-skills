"""Bounded, explicit video evidence; automated tests never open capture devices."""
import base64
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[1]
HELPER = ROOT / "skills/sgdk-testing/scripts/capture_console.py"
SPEC = importlib.util.spec_from_file_location("console_capture", HELPER)
capture = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(capture)
PNG = base64.b64decode("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII=")


class CaptureTests(unittest.TestCase):
    def test_named_video_input_has_no_audio_and_no_shell(self):
        command = capture.frame_command("ffmpeg", "Elgato HD60 X", Path("space path"))
        self.assertEqual(command[command.index("-i") + 1], "Elgato HD60 X:none")
        self.assertIn("-an", command)
        self.assertEqual(command[command.index("-video_size") + 1], "1920x1080")
        self.assertEqual(command[-1], str(Path("space path/frame-%02d.png")))
        self.assertIn("-n", command)

    def test_validation_precedes_device_access(self):
        for bad in [dict(device="camera:microphone"), dict(device=""), dict(frames=0),
                    dict(device="default"), dict(device="none"), dict(device="0wrong"),
                    dict(device="Capture screen 0"),
                    dict(frames=17), dict(sample_fps=float("nan")), dict(timeout=float("inf")),
                    dict(after=-1), dict(size="1920x1080;bad")]:
            values = dict(device="Elgato", frames=1, sample_fps=4, input_fps=30,
                          size="1920x1080", after=0.5, timeout=20)
            values.update(bad)
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                capture.validate(**values)

    def test_existing_evidence_is_never_overwritten(self):
        with tempfile.TemporaryDirectory() as temporary:
            file = Path(temporary) / "keep.txt"
            file.write_text("keep")
            with patch.object(capture.sys, "platform", "darwin"), patch.object(capture.shutil, "which", return_value="ffmpeg"):
                with patch.object(capture.subprocess, "run") as run:
                    with self.assertRaises(FileExistsError):
                        capture.capture("ffmpeg", "Elgato", temporary)
                    run.assert_not_called()
            self.assertEqual(file.read_text(), "keep")

    def test_known_impossible_sampling_deadline_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "cannot fit"):
            capture.validate("Elgato", 16, 0.1, 30, "1920x1080", 0.5, 20)

    def test_success_records_fresh_frame_hashes_without_claiming_runtime_pass(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "capture with spaces"

            def ffmpeg(command, **kwargs):
                if "-version" in command:
                    return subprocess.CompletedProcess(command, 0, stdout="ffmpeg test version\n")
                self.assertEqual(kwargs["timeout"], 20)
                self.assertNotIn("shell", kwargs)
                for i in range(2):
                    (output / f"frame-{i:02d}.png").write_bytes(PNG)
                return subprocess.CompletedProcess(command, 0)

            with patch.object(capture.sys, "platform", "darwin"), patch.object(capture.shutil, "which", return_value="ffmpeg"):
                with patch.object(capture.subprocess, "run", side_effect=ffmpeg):
                    report = capture.capture("ffmpeg", "Elgato", output, frames=2)
            data = json.loads(report.read_text())
            self.assertEqual(len(data["frames"]), 2)
            self.assertEqual(data["frames"][0]["width"], 1)
            self.assertEqual(data["frames"][0]["sha256"], capture.hashlib.sha256(PNG).hexdigest())
            self.assertFalse(data["runtime_verified"])
            self.assertFalse(data["audio"])

    def test_failure_timeout_and_missing_frames_do_not_publish_success_manifest(self):
        for failure in [subprocess.CompletedProcess([], 1), subprocess.TimeoutExpired([], 1),
                        subprocess.CompletedProcess([], 0)]:
            with self.subTest(failure=failure), tempfile.TemporaryDirectory() as temporary:
                output = Path(temporary) / "capture"
                with patch.object(capture.sys, "platform", "darwin"), patch.object(capture.shutil, "which", return_value="ffmpeg"):
                    kwargs = {"side_effect": failure} if isinstance(failure, Exception) else {"return_value": failure}
                    with patch.object(capture.subprocess, "run", **kwargs):
                        with self.assertRaises((RuntimeError, OSError)):
                            capture.capture("ffmpeg", "Elgato", output)
                self.assertFalse((output / "capture.json").exists())

    def test_unsupported_host_does_not_fall_back_to_a_webcam(self):
        with patch.object(capture.sys, "platform", "linux"), patch.object(capture.subprocess, "run") as run:
            with self.assertRaisesRegex(ValueError, "macOS"):
                capture.capture("ffmpeg", "Elgato", "unused")
            run.assert_not_called()

    def test_copied_helper_has_no_repository_or_optional_python_dependencies(self):
        with tempfile.TemporaryDirectory() as temporary:
            helper = Path(temporary) / "capture console.py"
            shutil.copyfile(HELPER, helper)
            result = subprocess.run([sys.executable, str(helper), "--help"], cwd=temporary,
                                    capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("--device", result.stdout)


if __name__ == "__main__":
    unittest.main()
