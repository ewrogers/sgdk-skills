#!/usr/bin/env python3
"""Capture bounded console-video evidence through FFmpeg AVFoundation on macOS.

Use an explicitly selected capture card, not a desktop window. No audio,
automatic camera selection, USB reset, or game input is performed.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys


def validate(device, frames, sample_fps, input_fps, size, after, timeout):
    if not device or ":" in device or any(ord(c) < 32 for c in device):
        raise ValueError("select an explicit video device name/index without ':' or control characters")
    if (device.startswith(("default", "none", "Capture screen ")) or
            (re.match(r"\s*[+-]?\d", device) and not re.fullmatch(r"[0-9]+", device))):
        raise ValueError("use an explicit capture-card name or numeric video index, not a default/screen selector")
    if not 1 <= frames <= 16:
        raise ValueError("frames must be between 1 and 16")
    for name, value, low, high in [("sample fps", sample_fps, 0.1, 10),
                                    ("input fps", input_fps, 1, 120),
                                    ("after", after, 0, 30), ("timeout", timeout, 1, 60)]:
        if not math.isfinite(value) or not low <= value <= high:
            raise ValueError(f"{name} must be finite and between {low} and {high}")
    if not re.fullmatch(r"[1-9][0-9]{0,3}x[1-9][0-9]{0,3}", size):
        raise ValueError("size must be WIDTHxHEIGHT")
    if after + (frames - 1) / sample_fps >= timeout:
        raise ValueError("requested frame sampling cannot fit within the capture timeout")


def frame_command(ffmpeg, device, output, frames=1, sample_fps=4.0,
                  input_fps=30.0, size="1920x1080", after=0.5):
    command = [ffmpeg, "-hide_banner", "-nostdin", "-f", "avfoundation",
               "-framerate", str(input_fps), "-video_size", size,
               "-pixel_format", "uyvy422", "-i", device + ":none", "-an"]
    if after:
        command.extend(["-ss", str(after)])
    if frames > 1:
        command.extend(["-vf", f"fps={sample_fps}"])
    command.extend(["-c:v", "png", "-frames:v", str(frames), "-start_number", "0",
                    "-n", str(output / "frame-%02d.png")])
    return command


def png_record(path):
    data = path.read_bytes()
    if (len(data) < 45 or data[:8] != b"\x89PNG\r\n\x1a\n" or
            data[12:16] != b"IHDR" or data[-12:] != bytes.fromhex("0000000049454e44ae426082")):
        raise RuntimeError(f"missing or incomplete PNG: {path}")
    width, height = struct.unpack(">II", data[16:24])
    return {"file": path.name, "width": width, "height": height,
            "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def capture(ffmpeg, device, output, frames=1, sample_fps=4.0, input_fps=30.0,
            size="1920x1080", after=0.5, timeout=20.0):
    validate(device, frames, sample_fps, input_fps, size, after, timeout)
    if sys.platform != "darwin":
        raise ValueError("this capture backend requires macOS AVFoundation; no camera fallback")
    executable = shutil.which(ffmpeg)
    if executable is None:
        raise ValueError("FFmpeg is required on the host; no software is installed automatically")
    output = Path(output).resolve()
    # Each run gets a fresh folder, so an old image cannot masquerade as capture success.
    output.mkdir(parents=True, exist_ok=False)
    command = frame_command(executable, device, output, frames, sample_fps, input_fps, size, after)
    started = datetime.now(timezone.utc).isoformat()
    log = output / "capture.log"
    try:
        with log.open("xb") as stream:
            result = subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT, timeout=timeout)
    except subprocess.TimeoutExpired as error:
        raise RuntimeError(f"capture timed out; partial output is unverified; see {log}") from error
    if result.returncode:
        raise RuntimeError(f"capture failed with exit {result.returncode}; see {log}")
    files = [output / f"frame-{i:02d}.png" for i in range(frames)]
    version = subprocess.run([executable, "-version"], capture_output=True, text=True, timeout=5)
    report = {"source": "capture card via AVFoundation", "device": device,
              "started_at_utc": started, "finished_at_utc": datetime.now(timezone.utc).isoformat(),
              "requested_size": size, "requested_input_fps": input_fps,
              "sample_fps": sample_fps if frames > 1 else None, "after_seconds": after,
              "audio": False, "command": command,
              "ffmpeg": version.stdout.splitlines()[0] if version.stdout else "unknown",
              "frames": [png_record(p) for p in files], "runtime_verified": False,
              "observation": "unreviewed; inspect images, including signal/error screens"}
    (output / "capture.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return output / "capture.json"


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ffmpeg", default="ffmpeg", help="Host FFmpeg executable")
    parser.add_argument("--device", required=True, help="Explicit capture-card video name or index")
    parser.add_argument("--output", required=True, type=Path, help="New evidence folder; existing paths refused")
    parser.add_argument("--frames", type=int, default=1, help="1..16 PNGs (default: 1)")
    parser.add_argument("--sample-fps", type=float, default=4.0, help="Burst sampling rate, not game/VBlank timing")
    parser.add_argument("--input-fps", type=float, default=30.0)
    parser.add_argument("--size", default="1920x1080")
    parser.add_argument("--after", type=float, default=0.5, help="Skip this many input seconds before saving")
    parser.add_argument("--timeout", type=float, default=20.0)
    args = parser.parse_args(argv)
    try:
        report = capture(args.ffmpeg, args.device, args.output, args.frames, args.sample_fps,
                         args.input_fps, args.size, args.after, args.timeout)
        print(report)
        return 0
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"Console capture: {error}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("Console capture: interrupted; output is unverified", file=sys.stderr)
        return 130


if __name__ == "__main__":
    sys.exit(main())
