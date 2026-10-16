#!/usr/bin/env python3
"""Copy Test Lab to a fresh project and generate bounded replay/art resources."""
import argparse
import json
from pathlib import Path
import shutil
import sys
from art import assets
from qa import header, load_tape, load_trace

ROOT = Path(__file__).resolve().parents[1]


def prepare(output):
    output = Path(output).resolve()
    if output.exists(): raise ValueError('Choose a new output directory')
    # Parse everything before creating or copying the destination.
    tape = ROOT / 'replays/courier.json'
    _, ticks, sha = load_tape(tape)
    expected = load_trace(ROOT / 'replays/courier.expected.json')
    if expected['replay_sha256'] != sha or expected['total_ticks'] != ticks:
        raise ValueError('Baseline belongs to a different tape; review it before building')
    # Copy only source inputs, so output may safely live under this Lab's build/.
    output.mkdir(parents=True)
    for name in ('inc', 'src', 'tools', 'replays'):
        shutil.copytree(ROOT / name, output / name, ignore=shutil.ignore_patterns('__pycache__'))
    for name in ('Makefile', 'README.md'): shutil.copyfile(ROOT / name, output / name)
    (output / 'inc/replay_data.h').write_text(header(tape), encoding='utf-8')
    points = expected['checkpoints']
    (output / 'inc/expected_data.h').write_text(
        '#ifndef LAB_EXPECTED_DATA_H\n#define LAB_EXPECTED_DATA_H\n'
        f'#define EXPECTED_COUNT {len(points)}u\n'
        'static const u32 expected_hashes[EXPECTED_COUNT] = {\n' +
        ''.join(f'    {p["digest"]}u,\n' for p in points) + '};\n#endif\n', encoding='utf-8')
    resources = output / 'res'
    resources.mkdir(exist_ok=True)
    for name, data in assets().items(): (resources / name).write_bytes(data)
    (resources / 'resources.res').write_text(
        'SPRITE courier_image "courier.png" 2 2 NONE 0\n'
        'SPRITE mote_image "mote.png" 1 1 NONE 0\n'
        'SPRITE bolt_image "bolt.png" 1 1 NONE 0\n', encoding='utf-8')
    return output


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    try: print(prepare(args.output))
    except (OSError, ValueError) as error:
        print(error, file=sys.stderr); raise SystemExit(2)
