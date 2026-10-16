#!/usr/bin/env python3
"""Exercise discovery and project-local copying through the actual skills CLI."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
SKILLS = ["sgdk-best-practices", "sgdk-planning", "sgdk-project", "sgdk-assets", "sgdk-graphics", "sgdk-raster", "sgdk-audio", "sgdk-input", "sgdk-ui", "sgdk-levels", "sgdk-gameplay", "sgdk-platformer", "sgdk-shmup", "sgdk-math", "sgdk-ai", "sgdk-scenes", "sgdk-save", "sgdk-performance", "sgdk-debug", "sgdk-release", "sgdk-cartridge"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cli-version", default="1.7.2")
    args = parser.parse_args()
    scratch_root = ROOT / "build" / "distribution"
    scratch_root.mkdir(parents=True, exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix="run-", dir=scratch_root))
    environment = dict(os.environ, DO_NOT_TRACK="1")
    # npx is a .cmd launcher on native Windows; this smoke test runs on Linux.
    base = ["npx", "--yes", f"skills@{args.cli_version}", "add", str(ROOT)]
    for name, arguments in [
        ("discover", ["--list"]),
        ("install", ["--skill", *SKILLS, "--agent", "codex", "--copy", "--yes"]),
    ]:
        with (scratch / f"{name}.log").open("w", encoding="utf-8") as log:
            result = subprocess.run(base + arguments, cwd=scratch, env=environment,
                                    stdout=log, stderr=subprocess.STDOUT, timeout=240)
        if result.returncode:
            raise RuntimeError(f"{name} failed; see {scratch / (name + '.log')}")
    for name in SKILLS:
        installed = [p.parent for p in scratch.rglob("SKILL.md") if p.parent.name == name]
        if not installed:
            raise RuntimeError(f"CLI did not install {name}; see {scratch / 'install.log'}")
        source = ROOT / "skills" / name
        for file in source.rglob("*"):
            if not file.is_file() or "__pycache__" in file.parts:
                continue
            relative = file.relative_to(source)
            if not any((destination / relative).is_file() and
                       (destination / relative).read_bytes() == file.read_bytes()
                       for destination in installed):
                raise RuntimeError(f"CLI lost or changed {name}/{relative}")
    print(f"Discovered and copied all {len(SKILLS)} skills with their resources: {scratch}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
