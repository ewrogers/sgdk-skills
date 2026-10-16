#!/usr/bin/env python3
"""Compile bounded replay tapes, run host logic and compare checkpoint reports."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FIELDS = ('tick', 'active', 'x', 'y', 'score', 'fired', 'paused', 'digest')


def integer(value, low, high):
    if type(value) is not int or not low <= value <= high:
        raise ValueError(f'Expected integer in {low}..{high}')
    return value


def load_tape(path):
    data = json.loads(Path(path).read_text(encoding='utf-8'))
    if not isinstance(data, dict) or set(data) != {'version', 'name', 'seed', 'tick_hz', 'runs'}:
        raise ValueError('Replay fields must be version, name, seed, tick_hz and runs')
    if type(data['version']) is not int or data['version'] != 1 or type(data['tick_hz']) is not int or data['tick_hz'] != 60:
        raise ValueError('Replay requires version 1 and 60 simulation ticks/second')
    if (not isinstance(data['name'], str) or not data['name'].isascii() or
            not 1 <= len(data['name']) <= 48 or
            any(not (c.isalnum() or c in '-_') for c in data['name'])):
        raise ValueError('Replay name must be a 1..48 character ASCII slug')
    integer(data['seed'], 0, 65535)
    runs = data['runs']
    if not isinstance(runs, list) or not 1 <= len(runs) <= 128:
        raise ValueError('Replay needs 1..128 input runs')
    total = 0
    for run in runs:
        if not isinstance(run, dict) or set(run) != {'ticks', 'held'}:
            raise ValueError('Each run needs ticks and held')
        total += integer(run['ticks'], 1, 720)
        integer(run['held'], 0, 255)
    if total > 720:
        raise ValueError('Replay exceeds 720 simulation ticks')
    canonical = json.dumps(data, sort_keys=True, separators=(',', ':')).encode('ascii')
    return data, total, hashlib.sha256(canonical).hexdigest()


def header(path):
    data, total, digest = load_tape(path)
    lines = ['/* Validated replay data; regenerate from JSON, do not edit. */',
             '#ifndef LAB_REPLAY_DATA_H', '#define LAB_REPLAY_DATA_H', '#include "lab.h"',
             f'#define REPLAY_SEED {data["seed"]}u', f'#define REPLAY_TICKS {total}u',
             f'#define REPLAY_COUNT {len(data["runs"])}u',
             '#define REPLAY_NAME ' + json.dumps(data['name']),
             '#define REPLAY_SHA ' + json.dumps(digest),
             'static const LabRun reference_runs[REPLAY_COUNT] = {']
    lines.extend(f'    {{{run["ticks"]}, {run["held"]}}},' for run in data['runs'])
    return '\n'.join(lines + ['};', '#endif', ''])


def load_trace(path):
    data = json.loads(Path(path).read_text(encoding='utf-8'))
    required = {'format', 'replay', 'replay_sha256', 'seed', 'tick_hz', 'total_ticks', 'render_hz', 'frames', 'checkpoints'}
    if not isinstance(data, dict) or set(data) != required or data['format'] != 'sgdk-testlab-trace-v1':
        raise ValueError('Unknown or incomplete trace format')
    integer(data['seed'], 0, 65535)
    if (type(data['tick_hz']) is not int or data['tick_hz'] != 60 or
            type(data['render_hz']) is not int or data['render_hz'] not in (50, 60)):
        raise ValueError('Invalid simulation/render rate')
    total = integer(data['total_ticks'], 1, 720)
    integer(data['frames'], 1, 720)
    if data['frames'] != (total * data['render_hz'] + 59) // 60:
        raise ValueError('Trace frame count does not match its declared schedule')
    if not isinstance(data['replay'], str) or not data['replay']:
        raise ValueError('Missing replay identity')
    sha = data['replay_sha256']
    if not isinstance(sha, str) or len(sha) != 64 or any(c not in '0123456789abcdef' for c in sha):
        raise ValueError('Invalid replay SHA-256')
    points = data['checkpoints']
    ticks = list(range(0, total + 1, 60))
    if ticks[-1] != total: ticks.append(total)
    if not isinstance(points, list) or len(points) != len(ticks):
        raise ValueError('Missing checkpoints')
    for tick, point in zip(ticks, points):
        if not isinstance(point, dict) or set(point) != set(FIELDS):
            raise ValueError('Incomplete checkpoint')
        for name in FIELDS:
            integer(point[name], -32768 if name in ('x', 'y') else 0,
                    4294967295 if name == 'digest' else 32767 if name in ('x', 'y') else 65535)
        if point['tick'] != tick or point['active'] > tick or point['paused'] not in (0, 1):
            raise ValueError('Invalid checkpoint timeline')
    return data


def compare(expected, actual):
    for key in ('format', 'replay', 'replay_sha256', 'seed', 'tick_hz', 'total_ticks'):
        if expected[key] != actual[key]:
            raise ValueError(f'Replay identity mismatch: {key}')
    for left, right in zip(expected['checkpoints'], actual['checkpoints']):
        for key in FIELDS:
            if left[key] != right[key]:
                return {'passed': False, 'tick': left['tick'], 'field': key,
                        'expected': left[key], 'actual': right[key]}
    return {'passed': True, 'checkpoints': len(expected['checkpoints'])}


def run_host(tape, hz, fault, compiler):
    with tempfile.TemporaryDirectory(prefix='sgdk test lab ') as directory:
        directory = Path(directory)
        (directory / 'replay_data.h').write_text(header(tape), encoding='utf-8')
        binary = directory / ('runner.exe' if os.name == 'nt' else 'runner')
        command = [compiler, '-std=c17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-I', str(ROOT / 'inc'), '-I', str(directory),
                   str(ROOT / 'src/lab.c'), str(ROOT / 'tools/host.c'), '-o', str(binary)]
        subprocess.run(command, check=True, capture_output=True, text=True, timeout=60)
        result = subprocess.run([str(binary), str(hz), str(int(fault))], check=True,
                                capture_output=True, text=True, timeout=15)
        return json.loads(result.stdout)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    compile_parser = commands.add_parser('compile', help='Validate JSON and emit a C header')
    compile_parser.add_argument('tape', type=Path)
    compile_parser.add_argument('--output', type=Path, required=True)
    run = commands.add_parser('run', help='Run the portable game logic; does not emulate the console')
    run.add_argument('--tape', type=Path, default=ROOT / 'replays/courier.json')
    run.add_argument('--hz', type=int, choices=[50, 60], default=60)
    run.add_argument('--fault', action='store_true', help='Inject a movement regression')
    run.add_argument('--cc', default=next((p for n in ['clang', 'cc', 'gcc'] if (p := shutil.which(n))), None))
    run.add_argument('--output', type=Path, required=True)
    check = commands.add_parser('compare', help='Exit 1 on divergence, 2 on invalid input')
    check.add_argument('expected', type=Path)
    check.add_argument('actual', type=Path)
    args = parser.parse_args()
    try:
        if args.command == 'compare':
            result = compare(load_trace(args.expected), load_trace(args.actual))
            print(json.dumps(result))
            return 0 if result['passed'] else 1
        if args.output.exists():
            raise ValueError('Output already exists; choose a new path')
        if args.command == 'compile': content = header(args.tape)
        else:
            if not args.cc: raise ValueError('A C17 compiler is required; use --cc')
            content = json.dumps(run_host(args.tape, args.hz, args.fault, args.cc), indent=2) + '\n'
        # Exclusive create, including if another process creates it while compiling.
        with args.output.open('x', encoding='utf-8') as destination: destination.write(content)
        print(args.output)
        return 0
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        if isinstance(error, subprocess.CalledProcessError): print(error.stderr, file=sys.stderr)
        return 2


if __name__ == '__main__': raise SystemExit(main())
