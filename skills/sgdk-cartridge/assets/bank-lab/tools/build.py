#!/usr/bin/env python3
"""Build Bank Lab with a private bank-enabled copy of the selected SGDK."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import struct
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
ROM_BYTES = 0x600000
SDK_PARTS = ['bin', 'inc', 'src', 'res', 'common.mk', 'makelib.gen', 'makefile.gen', 'md.ld']


def sdk_key(source, compiler):
    digest = hashlib.sha256(platform.machine().encode() + compiler.encode() + Path(__file__).read_bytes())
    # Include compiler policy wrappers and the executable, not just --version.
    for executable in [shutil.which('m68k-elf-gcc'),
                       subprocess.check_output(['m68k-elf-gcc', '-print-prog-name=cc1'], text=True).strip()]:
        if executable and Path(executable).is_file(): digest.update(Path(executable).read_bytes())
    for name in SDK_PARTS:
        path = source / name
        files = sorted(path.rglob('*')) if path.is_dir() else [path]
        for file in files:
            if not file.is_file() or file.suffix in {'.o', '.d', '.o80', '.rs'}: continue
            digest.update(str(file.relative_to(source)).encode())
            digest.update(file.read_bytes())
    return digest.hexdigest()[:20]


def linker_script(original):
    needle = '*(.rodata_binf)'
    if original.count(needle) != 1 or '  .data 0xE0FF0000 :' not in original:
        raise ValueError('Selected SGDK linker changed; review the Bank Lab placement adapter')
    script = original.replace(needle, '*(EXCLUDE_FILE (*stage_a.o *stage_b.o *music_a.o *music_b.o) .rodata_binf)')
    # ResComp binary input sections have no ALLOC flag. BYTE forces these
    # standalone output sections into the load image; it is trailing padding.
    sections = '''
  .bank_stage_a 0x400000 : { KEEP(*stage_a.o(.rodata_binf)) BYTE(0) } > rom
  .bank_probe 0x47FFF0 : { KEEP(*(.bank_probe)) } > rom
  .bank_music_a 0x480100 : { KEEP(*music_a.o(.rodata_binf)) __music_a_end = .; BYTE(0) } > rom
  .bank_stage_b 0x500000 : { KEEP(*stage_b.o(.rodata_binf)) BYTE(0) } > rom
  .bank_music_b 0x580000 : { KEEP(*music_b.o(.rodata_binf)) __music_b_end = .; BYTE(0) } > rom

'''
    script = script.replace('  .data 0xE0FF0000 :', sections + '  .data 0xE0FF0000 :')
    return script + '''
ASSERT(SIZEOF(.text) + SIZEOF(.data) <= 0x80000, "Common code/assets/data image exceed fixed bank zero")
ASSERT(SIZEOF(.bank_stage_a) > 0 && SIZEOF(.bank_stage_a) <= 0x7FFF0, "Stage A exceeds its graphics bank")
ASSERT(SIZEOF(.bank_stage_b) > 0 && SIZEOF(.bank_stage_b) <= 0x80000, "Stage B exceeds its graphics bank")
ASSERT(SIZEOF(.bank_probe) == 32, "Boundary probe must cross two banks by 16 bytes each")
ASSERT(stage_music_a == 0x480100 && SIZEOF(.bank_music_a) > 0 && SIZEOF(.bank_music_a) <= 0x7FF00, "Music A must fit its pinned bank")
ASSERT(stage_music_b == 0x580000 && SIZEOF(.bank_music_b) > 0 && SIZEOF(.bank_music_b) <= 0x80000, "Music B must fit its pinned bank")
'''


def run(command, log, cwd=ROOT):
    with log.open('w', encoding='utf-8') as output:
        result = subprocess.run(command, cwd=cwd, stdout=output, stderr=subprocess.STDOUT)
    if result.returncode:
        print('\n'.join(log.read_text(errors='replace').splitlines()[-25:]), file=sys.stderr)
        raise RuntimeError(f'Build failed ({result.returncode}); see {log.relative_to(ROOT)}')


def prepare_sdk(source, mode, jobs):
    compiler = subprocess.check_output(['m68k-elf-gcc', '--version'], text=True)
    sdk = ROOT / 'out/bank-sdk' / sdk_key(source, compiler)
    if not sdk.exists():
        print('Preparing a private bank-enabled SGDK (installed SDK stays unchanged).', flush=True)
        sdk.mkdir(parents=True)
        for name in SDK_PARTS:
            src, dst = source / name, sdk / name
            if src.is_dir(): shutil.copytree(src, dst, symlinks=True)
            else: shutil.copy2(src, dst)
        (sdk / 'lib').mkdir()
        config = sdk / 'inc/config.h'
        text = config.read_text()
        old = '#define ENABLE_BANK_SWITCH      0'
        if text.count(old) != 1: raise ValueError('Review the selected SDK bank-switch configuration')
        config.write_text(text.replace(old, '#define ENABLE_BANK_SWITCH      1'))
        (sdk / 'md.ld').write_text(linker_script((source / 'md.ld').read_text()))
        # Discard any precompiled objects inherited from the published SDK.
        run(['make', '-f', str(sdk / 'makelib.gen'), 'clean'], ROOT / 'out/sdk-clean.log', sdk)
    library = sdk / 'lib' / ('libmd_debug.a' if mode == 'debug' else 'libmd.a')
    if not library.exists():
        print(f'Building bank-enabled SGDK {mode} library; later builds reuse it.', flush=True)
        run(['make', '-f', str(sdk / 'makelib.gen'), 'cleanobj', 'cleandep'], ROOT / 'out/sdk-objects.log', sdk)
        run(['make', '-f', str(sdk / 'makelib.gen'), f'-j{jobs}', mode, 'EXTRA_FLAGS=-std=gnu17'],
            ROOT / f'out/sdk-{mode}.log', sdk)
    return sdk


def finish_rom(path):
    data = bytearray(path.read_bytes())
    if len(data) > ROM_BYTES: raise ValueError('ROM exceeds the declared twelve-bank lab layout')
    if data[0x100:0x110] != b'SEGA SSF        ': raise ValueError('ROM needs the SEGA SSF mapper header')
    if struct.unpack_from('>I', data, 0x1A4)[0] != ROM_BYTES - 1:
        raise ValueError('ROM header end does not match the lab cartridge size')
    data.extend(b'\xff' * (ROM_BYTES - len(data)))
    path.write_bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['release', 'debug', 'clean'], nargs='?', default='release')
    parser.add_argument('--jobs', type=int, default=2)
    args = parser.parse_args()
    if args.jobs < 1 or args.jobs > 16: parser.error('--jobs must be between 1 and 16')
    if args.mode == 'clean':
        shutil.rmtree(ROOT / 'out', ignore_errors=True)
        print('Removed Bank Lab output and private SDK cache.')
        return 0
    source = Path(os.environ.get('GDK', '/opt/sgdk'))
    if not (source / 'inc/genesis.h').is_file() or not shutil.which('m68k-elf-gcc'):
        parser.error('Run this helper inside ewrogers/sgdk:latest; see the lab README')
    (ROOT / 'out').mkdir(exist_ok=True)
    sdk = prepare_sdk(source, args.mode, args.jobs)
    stamp = ROOT / 'out/build-selection.json'
    selection = {'sdk': sdk.name, 'mode': args.mode}
    if not stamp.exists() or json.loads(stamp.read_text()) != selection:
        run(['make', '-f', str(sdk / 'makefile.gen'), 'clean'], ROOT / 'out/app-clean.log')
    stamp.write_text(json.dumps(selection))
    print(f'Building Bank Lab {args.mode}.', flush=True)
    run(['make', '-f', str(sdk / 'makefile.gen'), args.mode, 'EXTRA_FLAGS=-std=gnu17'],
        ROOT / f'out/build-{args.mode}.log')
    finish_rom(ROOT / 'out/rom.bin')
    run(['java', '-jar', str(sdk / 'bin/sizebnd.jar'), 'out/rom.bin', '-sizealign', '131072', '-checksum'],
        ROOT / 'out/checksum.log')
    symbols = subprocess.check_output(['m68k-elf-nm', '-n', 'out/rom.out'], text=True, cwd=ROOT)
    values = {}
    for line in symbols.splitlines():
        parts = line.split()
        if len(parts) == 3:
            try: values[parts[2]] = int(parts[0], 16)
            except ValueError: pass
    selected = ['_stext', '_sdata', '_bend', 'hero_image', 'click_sound',
                'stage_a_palette', 'stage_b_palette', 'stage_a_sprite', 'stage_b_sprite',
                'stage_music_a', 'stage_music_b', '__music_a_end', '__music_b_end', 'bank_edge_probe']
    report = {'rom_bytes': (ROOT / 'out/rom.bin').stat().st_size, 'mode': args.mode,
              'compiler': subprocess.check_output(['m68k-elf-gcc', '--version'], text=True).splitlines()[0],
              'symbols': {name: values[name] for name in selected if name in values}}
    (ROOT / 'out/bank-layout.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f"ROM: {report['rom_bytes']:,} bytes (48 Mbit; deliberately sparse bank proof)")
    print(f"Fixed common image: {values['_stext'] + values['_sdata']:,} / 524,288 bytes")
    print('Art banks: 8 / 10   Music banks: 9 / 11   Audio window: 3')
    return 0


if __name__ == '__main__':
    try: sys.exit(main())
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print(error, file=sys.stderr); sys.exit(1)
