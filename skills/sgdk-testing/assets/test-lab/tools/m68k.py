#!/usr/bin/env python3
"""Execute the dedicated probe's logic on a base M68000, without console devices."""
import argparse
import json
from pathlib import Path
import sys
from qa import load_tape


def execute(rom, symbols):
    from unicorn import Uc, UcError, UC_ARCH_M68K, UC_MODE_BIG_ENDIAN, UC_HOOK_MEM_WRITE
    from unicorn.m68k_const import (UC_CPU_M68K_M68000, UC_M68K_REG_A7,
                                   UC_M68K_REG_D0, UC_M68K_REG_PC, UC_M68K_REG_SR)
    entries = [int(parts[0], 16) for line in symbols.splitlines()
               if len(parts := line.split()) == 3 and
               (parts[2] == 'main' or parts[2].startswith('main.'))]
    if len(entries) != 1 or not 512 <= len(rom) <= 0x300000:
        raise ValueError('Expected a small dedicated probe ROM and one main entry')
    machine = Uc(UC_ARCH_M68K, UC_MODE_BIG_ENDIAN)
    machine.ctl_set_cpu_model(UC_CPU_M68K_M68000)
    machine.mem_map(0, 0x400000)
    machine.mem_map(0xC00000, 0x1000)
    machine.mem_map(0xE0FF0000, 0x10000)
    machine.mem_write(0, rom)
    machine.mem_write(0xE0FFFFE0, (0x300000).to_bytes(4, 'big'))
    machine.reg_write(UC_M68K_REG_SR, 0x2700)
    machine.reg_write(UC_M68K_REG_A7, 0xE0FFFFE0)
    values = []

    def record(cpu, access, address, size, value, user):
        if address == 0xC00000:
            if size != 2: raise ValueError('Probe output must use 16-bit words')
            if len(values) >= 236: raise ValueError('Probe exceeded its bounded output')
            values.append(value & 65535)

    machine.hook_add(UC_HOOK_MEM_WRITE, record)
    try: machine.emu_start(entries[0], 0x300000, count=3_000_000)
    except UcError as error: raise ValueError(f'M68000 probe execution failed: {error}') from error
    # This SGDK toolchain's int is 16 bits; only D0.w carries int's return value.
    if machine.reg_read(UC_M68K_REG_PC) != 0x300000 or machine.reg_read(UC_M68K_REG_D0) & 65535:
        raise ValueError(f'Probe failed or exceeded its instruction budget: '
                         f'PC={machine.reg_read(UC_M68K_REG_PC):08X}, '
                         f'D0={machine.reg_read(UC_M68K_REG_D0):08X}, words={len(values)}')
    return values


def traces(values, tape):
    data, total, sha = load_tape(tape)
    ticks = list(range(0, total + 1, 60))
    if ticks[-1] != total: ticks.append(total)
    if len(values) != (len(ticks) * 9 + 1) * 2: raise ValueError('Incomplete probe output')
    reports = {}
    for hz in (50, 60):
        points = []
        for tick in ticks:
            words, values = values[:9], values[9:]
            signed = lambda x: x if x < 32768 else x - 65536
            point = dict(zip(('tick', 'active', 'x', 'y', 'score', 'fired', 'paused'), words[:7]))
            point.update(x=signed(words[2]), y=signed(words[3]), digest=(words[7] << 16) | words[8])
            if point['tick'] != tick: raise ValueError('Probe checkpoints out of order')
            points.append(point)
        frames, values = values[0], values[1:]
        reports[hz] = {'format': 'sgdk-testlab-trace-v1', 'replay': data['name'],
                       'replay_sha256': sha, 'seed': data['seed'], 'tick_hz': 60,
                       'total_ticks': total, 'render_hz': hz,
                       'frames': frames, 'checkpoints': points}
    return reports


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    parser.add_argument('--symbols', type=Path, required=True)
    parser.add_argument('--tape', type=Path, default=Path(__file__).resolve().parents[1] / 'replays/courier.json')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        if args.output.exists(): raise ValueError('Choose a new report directory')
        reports = traces(execute(args.rom.read_bytes(), args.symbols.read_text()), args.tape)
        args.output.mkdir(parents=True)
        for hz, report in reports.items():
            (args.output / f'{hz}.json').write_text(json.dumps(report, indent=2) + '\n')
        print('M68000 logic traces written; VDP, interrupts, audio and real-time load were not emulated')
        return 0
    except (OSError, ValueError, ImportError) as error:
        print(error, file=sys.stderr); return 2


if __name__ == '__main__': raise SystemExit(main())
