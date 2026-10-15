#!/usr/bin/env python3
"""Run SGDK in Docker without shell-specific path expansion or native toolchains."""
import argparse
import csv
import io
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


DEFAULT_IMAGE = "ewrogers/sgdk:latest"


def docker_command(image, platform=None, project=None):
    """Build an argv list; Docker's mount value is CSV, not shell syntax."""
    command = ["docker", "run", "--rm"]
    if image == DEFAULT_IMAGE:
        command.append("--pull=always")
    if platform:
        command.extend(["--platform", platform])
    if project is not None:
        folder = Path(project).resolve()
        if not folder.is_dir():
            raise ValueError(f"Project folder does not exist: {folder}")
        mount = io.StringIO()
        csv.writer(mount, lineterminator="").writerow(
            ["type=bind", f"source={folder}", "target=/workspace"]
        )
        command.extend(["--mount", mount.getvalue(), "--workdir", "/workspace"])
        # Native Windows has no Unix UID/GID; Docker Desktop maps its bind mount.
        if hasattr(os, "getuid") and hasattr(os, "getgid"):
            command.extend(["--user", f"{os.getuid()}:{os.getgid()}"])
    command.append(image)
    return command


def run(image, platform, project, arguments):
    return subprocess.run(docker_command(image, platform, project) + list(arguments)).returncode


def initialize(destination, image, platform):
    destination = Path(destination).resolve()
    if destination.exists():
        raise ValueError(f"Destination already exists; choose a new folder: {destination}")
    destination.parent.mkdir(parents=True, exist_ok=True)
    staging = Path(tempfile.mkdtemp(prefix=".sgdk-new-", dir=destination.parent))
    try:
        result = run(image, platform, staging, [
            "sh", "-c", 'cp -R "$GDK/sample/basics/hello-world/." /workspace/'
        ])
        if result:
            return result
        if not (staging / "src" / "main.c").is_file():
            raise ValueError("The container did not provide the expected SGDK example")
        ignore = staging / ".gitignore"
        if not ignore.exists():
            ignore.write_text("out/\n", encoding="utf-8")
        # Publish only the completed template. Never replace an existing project.
        if destination.exists():
            raise ValueError(f"Destination appeared during initialization: {destination}")
        staging.rename(destination)
        print(f"Created {destination} from SGDK's Hello World example", flush=True)
        return 0
    finally:
        if staging.exists():
            shutil.rmtree(staging)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", default=DEFAULT_IMAGE, help="Docker image tag or digest")
    parser.add_argument("--platform", choices=["linux/arm64", "linux/amd64"],
                        help="Override Docker's native architecture selection")
    parser.add_argument("--project", type=Path, help="SGDK project folder")
    commands = parser.add_subparsers(dest="operation", required=True)
    commands.add_parser("doctor", help="Show the actual SDK and tool versions")
    init = commands.add_parser("init", help="Copy Hello World into a new project folder")
    init.add_argument("destination", type=Path)
    build = commands.add_parser("build", help="Use SGDK's standard project makefile")
    build.add_argument("target", choices=["release", "debug", "clean"], default="release", nargs="?")
    execute = commands.add_parser("run", help="Run any tool through the container entrypoint")
    execute.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)
    if not args.image or args.image.startswith("-"):
        parser.error("--image must be a Docker image reference")
    if args.operation == "build" and args.project is None:
        parser.error("build requires --project before the subcommand")
    if args.operation == "run" and not args.command:
        parser.error("run requires a command, for example: run m68k-elf-gcc --version")
    if not shutil.which("docker"):
        print("Docker CLI was not found. Install Docker Desktop or Docker Engine and start it.", file=sys.stderr)
        return 127
    try:
        if args.operation == "init":
            return initialize(args.destination, args.image, args.platform)
        if args.operation == "doctor":
            return run(args.image, args.platform, args.project, ["doctor"])
        if args.operation == "build":
            return run(args.image, args.platform, args.project, ["build", args.target])
        return run(args.image, args.platform, args.project, args.command)
    except (OSError, ValueError) as error:
        print(f"SGDK: {error}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        return 130


if __name__ == "__main__":
    sys.exit(main())
