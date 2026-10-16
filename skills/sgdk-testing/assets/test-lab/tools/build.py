#!/usr/bin/env python3
"""Prepare a new Test Lab project and build it with the SGDK Docker image."""
import argparse
import os
from pathlib import Path
import subprocess
from prepare import prepare


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--platform', choices=['linux/arm64', 'linux/amd64'])
    parser.add_argument('--mode', choices=['release', 'debug'], default='release')
    parser.add_argument('--region', choices=['auto', 'ntsc', 'pal'], default='auto',
                        help='Header hint for emulator selection; runtime still detects the console')
    args = parser.parse_args()
    output = prepare(args.output)
    if args.region != 'auto':
        # Runtime timing is detected with SYS_isPAL, never forced by this flag.
        rom_header = output / 'src/boot/rom_head.c'
        source = rom_header.read_text(encoding='utf-8')
        rom_header.write_text(source.replace('.region = "JUE"',
            '.region = "' + ('E' if args.region == 'pal' else 'U') + '"'), encoding='utf-8')
    command = ['docker', 'run', '--rm']
    if args.platform: command += ['--platform', args.platform]
    if hasattr(os, 'getuid'): command += ['--user', f'{os.getuid()}:{os.getgid()}']
    command += ['--mount', f'type=bind,source={output},target=/workspace', 'ewrogers/sgdk:latest']
    return subprocess.run(command + ['make', args.mode]).returncode


if __name__ == '__main__': raise SystemExit(main())
