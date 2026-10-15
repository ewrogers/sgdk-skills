"""Test real mapper/SRAM adapters on the host and actual banked ROM placement."""
import importlib.util
import hashlib
from functools import reduce
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
LAB = ROOT / 'skills/sgdk-cartridge/assets/bank-lab'
FIXTURE = ROOT / 'tests/fixtures/cartridge'


def module(name, filename):
    spec = importlib.util.spec_from_file_location(name, LAB / 'tools' / filename)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


class CartridgeBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cc = next((p for name in ['clang', 'cc', 'gcc'] if (p := shutil.which(name))), None)
        if cc is None:
            if os.environ.get('SGDK_REQUIRE_HOST_CC') == '1': raise RuntimeError('C17 compiler required')
            raise unittest.SkipTest('Host C17 compiler needed')
        (ROOT / 'build').mkdir(exist_ok=True)
        cls.folder = tempfile.TemporaryDirectory(prefix='bank host ', dir=ROOT / 'build')
        cls.addClassCleanup(cls.folder.cleanup)
        cls.binary = Path(cls.folder.name) / ('bank.exe' if os.name == 'nt' else 'bank')
        r = subprocess.run([cc, '-std=c17', '-Wall', '-Wextra', '-Werror',
                            '-I', str(FIXTURE), '-I', str(LAB / 'inc'),
                            str(LAB / 'src/bank_model.c'), str(LAB / 'src/bank_io.c'),
                            str(FIXTURE / 'bank_test.c'), '-o', str(cls.binary)],
                           capture_output=True, text=True, timeout=60)
        if r.returncode: raise RuntimeError(r.stdout + r.stderr)

    def check(self, name):
        r = subprocess.run([str(self.binary), name], capture_output=True, text=True, timeout=15)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(r.stdout.strip(), name)

    def test_region_cadence_edges_and_bounds(self): self.check('model')
    def test_version_crc_all_single_bit_errors_and_validate_before_commit(self): self.check('record')
    def test_cross_bank_copy_remap_invalidates_view_but_not_owned_bytes(self): self.check('far')
    def test_audio_quiesces_before_mapping_and_reserves_window_during_far_reads(self): self.check('audio')
    def test_sram_closes_on_success_and_failure_without_resetting_mapper(self): self.check('storage')


class CartridgeAssetTests(unittest.TestCase):
    def test_generator_reproduces_stage_art_palettes_and_original_fm(self):
        assets = module('bank_assets', 'make_assets.py').assets()
        for name, data in assets.items():
            source = LAB / 'res' / name
            checked_in = source.read_text().encode() if source.suffix == '.inc' else source.read_bytes()
            self.assertEqual(data, checked_in, name)
        self.assertNotEqual(assets['music_a.vgm'], assets['music_b.vgm'])
        for name in ['a', 'b']:
            music = assets[f'music_{name}.vgm']
            self.assertEqual(music[:4], b'Vgm ')
            self.assertEqual(struct.unpack_from('<I', music, 4)[0] + 4, len(music))
            palette = struct.unpack('>16H', assets[f'stage_{name}_palette.bin'])
            self.assertTrue(all(not (color & ~0xEEE) for color in palette))
        (ROOT / 'build').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='bank art ', dir=ROOT / 'build') as tmp:
            output = Path(tmp) / 'original art'
            command = [sys.executable, str(LAB / 'tools/make_assets.py'), str(output)]
            self.assertEqual(subprocess.run(command, capture_output=True).returncode, 0)
            self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)
            self.assertEqual(assets, {p.name: p.read_bytes() for p in output.iterdir()})

    def test_rom_finishing_rejects_bad_headers_and_oversize(self):
        helper = module('bank_build', 'build.py')
        (ROOT / 'build').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='bank finish ', dir=ROOT / 'build') as tmp:
            file = Path(tmp) / 'rom.bin'
            data = bytearray(512); file.write_bytes(data)
            with self.assertRaises(ValueError): helper.finish_rom(file)
            data[0x100:0x110] = b'SEGA SSF        '
            struct.pack_into('>I', data, 0x1A4, helper.ROM_BYTES - 1); file.write_bytes(data)
            helper.finish_rom(file)
            self.assertEqual(file.stat().st_size, helper.ROM_BYTES)
            self.assertEqual(file.read_bytes()[512:], b'\xff' * (helper.ROM_BYTES - 512))
            with file.open('ab') as out: out.write(b'\0')
            with self.assertRaises(ValueError): helper.finish_rom(file)
        with self.assertRaises(ValueError): helper.linker_script('unexpected SDK layout')


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION') == '1', 'Docker integration is opt-in')
class CartridgeContainerTests(unittest.TestCase):
    def test_bank_enabled_sdk_real_sections_payloads_and_sram_api(self):
        (ROOT / 'build').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='cartridge container ', dir=ROOT / 'build') as tmp:
            project = Path(tmp) / 'standalone bank lab'
            shutil.copytree(LAB, project)
            base = ['docker', 'run', '--rm']
            if os.environ.get('SGDK_PLATFORM'): base += ['--platform', os.environ['SGDK_PLATFORM']]
            if hasattr(os, 'getuid'): base += ['--user', f'{os.getuid()}:{os.getgid()}']
            base += ['--mount', f'type=bind,source={project},target=/workspace',
                     os.environ.get('SGDK_IMAGE', 'ewrogers/sgdk:latest')]
            def run(*args, success=True):
                r = subprocess.run(base + list(args), capture_output=True, text=True, timeout=300)
                self.assertEqual(r.returncode == 0, success, (r.stdout + r.stderr)[-6000:])
                return r.stdout + r.stderr
            # Compile the fixture's declarations against the actual SDK, not just a fake.
            fixture = (FIXTURE / 'genesis.h').read_text()
            declarations = re.findall(r'^(?:void|u8) [^\n]+\);$', fixture, re.M)
            (project / 'api_probe.c').write_text('#include <genesis.h>\n' + '\n'.join(declarations))
            run('m68k-elf-gcc', '-m68000', '-std=gnu17', '-fno-builtin', '-I/opt/sgdk/inc',
                '-I/opt/sgdk/res', '-fsyntax-only', 'api_probe.c')
            # The normal bank-disabled library must not silently build this example.
            run('build', 'release', success=False)
            hashes = {}
            for mode in ['release', 'debug', 'release']:
                run('python3', 'tools/build.py', mode, '--jobs', '4')
                report = json.loads((project / 'out/bank-layout.json').read_text())
                values = report['symbols']; rom = (project / 'out/rom.bin').read_bytes()
                digest = hashlib.sha256(rom).hexdigest()
                if mode in hashes: self.assertEqual(hashes[mode], digest, 'Mode switching changed the release ROM')
                hashes[mode] = digest
                self.assertEqual(len(rom), 0x600000)
                self.assertEqual(rom[0x100:0x110], b'SEGA SSF        ')
                self.assertEqual(struct.unpack_from('>I', rom, 0x1A4)[0], len(rom) - 1)
                self.assertEqual(rom[0x1B0:0x1B4], b'RA\xf8\x20')
                self.assertEqual(struct.unpack_from('>II', rom, 0x1B4), (0x200001, 0x203FFF))
                # SGDK sizebnd uses XOR-fold over all bytes with this field cleared.
                cleared = bytearray(rom); cleared[0x18E:0x190] = b'\0\0'
                checksum = reduce(int.__xor__, struct.unpack(f'>{len(rom)//2}H', cleared), 0)
                self.assertEqual(checksum, struct.unpack_from('>H', rom, 0x18E)[0])
                self.assertLessEqual(values['_stext'] + values['_sdata'], 0x80000)
                for name in ['hero_image', 'click_sound', 'stage_a_sprite', 'stage_b_sprite']:
                    self.assertLess(values[name], 0x80000)
                self.assertEqual(values['bank_edge_probe'], 0x47FFF0)
                self.assertEqual(rom[0x47FFF0:0x480010], b'BANK EIGHT END!!BANK NINE START!')
                for letter, bank, music_addr in [('a', 8, 0x480100), ('b', 10, 0x580000)]:
                    address = values[f'stage_{letter}_palette']
                    self.assertEqual(address, bank * 0x80000)
                    # These checks catch symbols whose non-ALLOC sections were omitted by objcopy.
                    self.assertEqual(rom[address:address+32], (project / f'res/stage_{letter}_palette.bin').read_bytes())
                    self.assertEqual(rom[address+32:address+48], b'BANK LAB STAGE ' + letter.upper().encode())
                    self.assertEqual(values[f'stage_music_{letter}'], music_addr)
                    self.assertGreater(values[f'__music_{letter}_end'], music_addr)
                    self.assertLessEqual(values[f'__music_{letter}_end'] - music_addr,
                                         0x80000 - music_addr % 0x80000)
                    self.assertNotEqual(rom[music_addr:music_addr+256], b'\0' * 256)
                symbols = {}
                for line in run('m68k-elf-nm', '-n', 'out/rom.out').splitlines():
                    fields = line.split()
                    if len(fields) == 3: symbols[fields[2]] = int(fields[0], 16)
                for letter, start in [('a', 0x400000), ('b', 0x500000)]:
                    for suffix in ['floor_data'] + [f'sprite_animation0_frame{i}_tileset_data' for i in range(4)]:
                        label = f'stage_{letter}_{suffix}'
                        self.assertTrue(start <= symbols[label] < start + 0x80000)
                        count = symbols[label + '_size']
                        self.assertNotEqual(rom[symbols[label]:symbols[label]+count], b'\0' * count)
                        # Resident TileSet descriptors must point at the actual FAR tiles.
                        metadata = symbols[f'stage_{letter}_floor' if suffix == 'floor_data' else label[:-5]]
                        self.assertEqual(struct.unpack_from('>I', rom, metadata + 4)[0], symbols[label])
                sections = run('m68k-elf-objdump', '-h', 'out/rom.out')
                for name in ['stage_a', 'stage_b', 'music_a', 'music_b', 'probe']:
                    self.assertRegex(sections, r'\.bank_' + name + r'[^\n]+\n\s+CONTENTS, ALLOC, LOAD')
                sdk = next((project / 'out/bank-sdk').iterdir())
                self.assertIn('#define ENABLE_BANK_SWITCH      1', (sdk / 'inc/config.h').read_text())
                assembly = run('m68k-elf-objdump', '-dr', str(Path('/workspace/out/bank-sdk') / sdk.name / 'lib/libmd.a'))
                self.assertIn('SYS_getFarData', assembly)
            # Exercise the actual linker's fixed-bank budget, not a prose check.
            (project / 'src/too_big.s').write_text('.section .text.keepboot,"ax"\n.space 0x80000,1\n')
            failure = run('python3', 'tools/build.py', 'release', success=False)
            self.assertIn('Common code/assets/data image exceed fixed bank zero', failure)
            destination = ROOT / 'build/cartridge-integration'
            destination.mkdir(parents=True, exist_ok=True)
            (destination / 'bank-report.json').write_text(json.dumps({'passed': True, 'hashes': hashes}, indent=2) + '\n')


if __name__ == '__main__': unittest.main()
