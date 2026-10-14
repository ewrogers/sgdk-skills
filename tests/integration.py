#!/usr/bin/env python3
"""Exercise an independently copied project skill against the real SGDK image."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


DEFAULT_IMAGE = (
    "ewrogers/sgdk:2.11-gcc16.2.0-binutils2.47@"
    "sha256:7c43a41e5bfa177b59b938d56bf496419df6b55778a6b5c68c629591ccc21b34"
)
ROOT = Path(__file__).resolve().parents[1]


def check(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def verify_rom(path: Path) -> dict[str, str | int]:
    """Check SGDK 2.11's standard build, not arbitrary/custom SGDK ROM layouts."""
    data = bytearray(path.read_bytes())
    check(len(data) >= 512, "ROM is shorter than the cartridge header")
    check(data[0x100:0x104] == b"SEGA", "ROM has no SEGA cartridge header")
    check(len(data) % 131_072 == 0, "standard ROM lacks SGDK 2.11's 128 KiB padding")
    digest = hashlib.sha256(data).hexdigest()
    stored = int.from_bytes(data[0x18E:0x190], "big")
    data[0x18E:0x190] = b"\0\0"
    checksum = 0
    for offset in range(0, len(data), 4):
        checksum ^= int.from_bytes(data[offset:offset + 4], "big")
    check(stored == (checksum ^ (checksum >> 16)) & 0xFFFF,
          "ROM fails SGDK sizebnd's XOR checksum")
    return {"bytes": len(data), "sha256": digest}


class Runner:
    def __init__(self, helper: Path, output: Path, image: str, platform: str | None):
        self.helper = helper
        self.output = output
        self.image = image
        self.platform = platform
        self.steps: list[str] = []

    def run(self, name: str, args: list[str], *, project: Path | None = None,
            expected: int = 0) -> str:
        command = [sys.executable, str(self.helper), "--image", self.image]
        if self.platform:
            command += ["--platform", self.platform]
        if project:
            command += ["--project", str(project)]
        command += args
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                text=True, encoding="utf-8", errors="replace", timeout=600)
        log = self.output / f"{name}.log"
        log.write_text(result.stdout, encoding="utf-8")
        check(result.returncode == expected,
              f"{name}: expected exit {expected}, got {result.returncode}; see {log}")
        self.steps.append(name)
        print(f"PASS {name}", flush=True)
        return result.stdout

    def fails(self, name: str, args: list[str], *, project: Path | None = None) -> None:
        """Require a real nonzero result without coupling tests to argparse exit codes."""
        command = [sys.executable, str(self.helper), "--image", self.image]
        if self.platform:
            command += ["--platform", self.platform]
        if project:
            command += ["--project", str(project)]
        result = subprocess.run(command + args, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True, encoding="utf-8",
                                errors="replace", timeout=600)
        log = self.output / f"{name}.log"
        log.write_text(result.stdout, encoding="utf-8")
        check(result.returncode != 0, f"{name}: unexpectedly succeeded; see {log}")
        self.steps.append(name)
        print(f"PASS {name}", flush=True)


def run(args: argparse.Namespace) -> Path:
    args.output.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix="run-", dir=args.output)).resolve()
    # A user may install just one skill. Do not accidentally depend on this repo's
    # sibling skills, root Python imports or current working directory.
    installed = output / "installed skill" / "sgdk-project"
    shutil.copytree(ROOT / "skills" / "sgdk-project", installed)
    helper = installed / "scripts" / "sgdk.py"
    runner = Runner(helper, output, args.image, args.platform)
    project = output / "Hello World project"
    report: dict[str, object] = {
        "image": args.image, "platform": args.platform, "passed": False,
        "steps": runner.steps,
    }
    try:
        runner.run("init", ["init", str(project)])
        source = project / "src" / "main.c"
        check(source.is_file(), "init did not create src/main.c")
        original = source.read_bytes()
        upstream = runner.run("official-source", ["run", "python3", "-c",
            "import hashlib, os; from pathlib import Path; "
            "print(hashlib.sha256((Path(os.environ['GDK']) / "
            "'sample/basics/hello-world/src/main.c').read_bytes()).hexdigest())"],
            project=project)
        check(hashlib.sha256(original).hexdigest() in upstream.splitlines(),
              "initialized main.c differs from official SGDK Hello World")
        marker = project / "keep me.txt"
        marker.write_text("Do not overwrite an existing project.\n", encoding="utf-8")
        runner.fails("init-existing", ["init", str(project)])
        check(source.read_bytes() == original and marker.is_file(),
              "failed init modified the existing project")
        runner.run("doctor", ["doctor"], project=project)
        machine = runner.run("architecture", ["run", "uname", "-m"], project=project)
        expected_machine = {"linux/amd64": "x86_64", "linux/arm64": "aarch64"}
        if args.platform:
            check(expected_machine[args.platform] in machine.splitlines(),
                  f"container did not execute as requested platform {args.platform}")
        values = ["space preserved", "semi;colon", "$literal", 'quote"value']
        passed = runner.run("arguments", ["run", "python3", "-c",
            "import json,sys; print(json.dumps(sys.argv[1:]))", *values], project=project)
        check(json.dumps(values) in passed.splitlines(), "command arguments changed")
        runner.run("exit-status", ["run", "python3", "-c", "raise SystemExit(17)"],
                   project=project, expected=17)
        runner.run("release", ["build", "release"], project=project)
        report["release_rom"] = verify_rom(project / "out" / "rom.bin")
        if hasattr(os, "getuid"):
            check((project / "out" / "rom.bin").stat().st_uid == os.getuid(),
                  "build output is owned by a different host user")
        runner.run("clean", ["build", "clean"], project=project)
        check(not (project / "out" / "rom.bin").exists(), "clean left the release ROM")
        runner.run("debug", ["build", "debug"], project=project)
        report["debug_rom"] = verify_rom(project / "out" / "rom.bin")
        symbols = runner.run("debug-symbols", ["run", "m68k-elf-gdb", "--batch",
            "-ex", "file out/rom.out", "-ex", "info functions main"], project=project)
        check("main.c" in symbols and "main(" in symbols,
              "debug ELF did not preserve main's source/function symbols")
        runner.run("clean-before-invalid", ["build", "clean"], project=project)
        source.write_bytes(b'#error "INTENTIONAL_INTEGRATION_BUILD_FAILURE"\n' + original)
        runner.fails("invalid-c", ["build", "release"], project=project)
        invalid_log = (output / "invalid-c.log").read_text(encoding="utf-8")
        check("INTENTIONAL_INTEGRATION_BUILD_FAILURE" in invalid_log,
              "failure did not come from the intentional invalid C source")
        source.write_bytes(original)
        report["passed"] = True
    finally:
        (output / "report.json").write_text(json.dumps(report, indent=2) + "\n",
                                           encoding="utf-8")
        print(f"Report: {output / 'report.json'}", flush=True)
    return output


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", default=os.environ.get("SGDK_IMAGE", DEFAULT_IMAGE))
    parser.add_argument("--platform", choices=["linux/amd64", "linux/arm64"],
                        default=os.environ.get("SGDK_PLATFORM") or None)
    parser.add_argument("--output", type=Path, default=ROOT / "build" / "integration")
    args = parser.parse_args()
    try:
        run(args)
    except (AssertionError, OSError, subprocess.TimeoutExpired) as error:
        print(f"FAIL {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
