"""Check Math Lab against independent arithmetic oracles and the actual SDK."""
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[1]
LAB = ROOT / 'skills/sgdk-math/assets/math-lab'
FIXTURE = ROOT / 'tests/fixtures/math'


class MathBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = next((p for name in ['clang', 'cc', 'gcc'] if (p := shutil.which(name))), None)
        if not compiler:
            if os.environ.get('SGDK_REQUIRE_HOST_CC') == '1':
                raise RuntimeError('A C17 compiler is required')
            raise unittest.SkipTest('A C17 compiler is required')
        cls.scratch = tempfile.TemporaryDirectory(prefix='sgdk math host ')
        cls.addClassCleanup(cls.scratch.cleanup)
        project = Path(cls.scratch.name) / 'independent math lab'
        shutil.copytree(LAB, project)
        cls.binary = Path(cls.scratch.name) / ('model.exe' if os.name == 'nt' else 'model')
        # MSVC's CRT includes the math functions; there is no separate m.lib.
        math_library = [] if os.name == 'nt' else ['-lm']
        result = subprocess.run([compiler, '-std=c17', '-O2', '-Wall', '-Wextra', '-Werror',
                                 '-I', str(project / 'inc'), str(project / 'src/demo.c'),
                                 str(FIXTURE / 'model_test.c'), *math_library, '-o', str(cls.binary)],
                                capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)

    def check(self, name):
        result = subprocess.run([str(self.binary), name], capture_output=True, text=True, timeout=20)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.strip(), name)

    def test_perspective_matches_rational_oracle_and_rejects_near_plane_and_zero(self): self.check('projection')
    def test_interpolation_endpoints_clamps_and_full_range_squared_distance(self): self.check('helpers')
    def test_three_formations_match_independent_trigonometry_and_repeat(self): self.check('patterns')
    def test_pal_ntsc_equal_nominal_updates_and_pause_preserves_state(self): self.check('clock')
    def test_input_edges_speed_limits_pattern_wrap_and_repeatable_reset(self): self.check('input')
    def test_hundred_thousand_updates_keep_stars_bounded_without_pool_growth(self): self.check('soak')


class MathArtTests(unittest.TestCase):
    def test_original_art_is_reproducible_indexed_bounded_and_cannot_be_overwritten(self):
        spec = importlib.util.spec_from_file_location('math_art', LAB / 'tools/make_art.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        for name, data in module.assets().items():
            self.assertEqual(data, (LAB / 'res' / name).read_bytes())
            offset = 8
            compressed = bytearray()
            while offset < len(data):
                size = struct.unpack_from('>I', data, offset)[0]
                kind = data[offset + 4:offset + 8]
                chunk = data[offset + 8:offset + 8 + size]
                self.assertEqual(zlib.crc32(kind + chunk), struct.unpack_from('>I', data, offset + 8 + size)[0])
                if kind == b'IHDR':
                    width, height, depth, mode, _, _, _ = struct.unpack('>IIBBBBB', chunk)
                    self.assertEqual((depth, mode), (8, 3))
                elif kind == b'PLTE':
                    self.assertEqual(len(chunk), 48)
                elif kind == b'IDAT':
                    compressed.extend(chunk)
                offset += size + 12
            rows = zlib.decompress(compressed)
            self.assertEqual(len(rows), (width + 1) * height)
            for row in range(height):
                self.assertEqual(rows[row * (width + 1)], 0)
                self.assertLess(max(rows[row * (width + 1) + 1:(row + 1) * (width + 1)]), 16)
        with tempfile.TemporaryDirectory(prefix='math source art ') as scratch:
            command = [sys.executable, str(LAB / 'tools/make_art.py'), scratch]
            self.assertEqual(subprocess.run(command, capture_output=True).returncode, 0)
            before = {p.name: p.read_bytes() for p in Path(scratch).iterdir()}
            self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)
            self.assertEqual(before, {p.name: p.read_bytes() for p in Path(scratch).iterdir()})


def execute_sdk(rom, symbols):
    """Execute real 68000 SDK math without emulating VDP behavior or startup."""
    from unicorn import Uc, UC_ARCH_M68K, UC_MODE_BIG_ENDIAN, UC_HOOK_MEM_WRITE
    from unicorn.m68k_const import UC_CPU_M68K_M68000, UC_M68K_REG_A7, UC_M68K_REG_D0, UC_M68K_REG_PC, UC_M68K_REG_SR
    entries = [int(line.split()[0], 16) for line in symbols.splitlines()
               if len(line.split()) == 3 and (line.split()[2] == 'main' or line.split()[2].startswith('main.'))]
    if len(entries) != 1:
        raise AssertionError('Expected exactly one SDK probe entry point')
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
    def record(machine, access, address, size, value, data):
        if address == 0xC00000:
            if size != 2:
                raise AssertionError('Math probe must emit 16-bit results')
            raw = value & 65535
            values.append(raw if raw < 32768 else raw - 65536)
    machine.hook_add(UC_HOOK_MEM_WRITE, record)
    machine.emu_start(entries[0], 0x300000, count=2_000_000)
    if machine.reg_read(UC_M68K_REG_PC) != 0x300000:
        raise AssertionError('SDK math probe did not finish within its instruction limit')
    return machine.reg_read(UC_M68K_REG_D0), values


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION') == '1', 'Docker integration is opt-in')
class MathContainerTests(unittest.TestCase):
    def test_standalone_rom_and_executed_sdk_math_in_release_and_debug(self):
        # Require the executor in integration runs rather than silently skipping
        # the SDK checks when CI is missing a dependency.
        import unicorn
        self.assertEqual(unicorn.__version__, '2.1.4')
        (ROOT / 'build').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='math container ', dir=ROOT / 'build') as scratch:
            project = Path(scratch) / 'standalone math lab'
            shutil.copytree(LAB, project)
            base = ['docker', 'run', '--rm']
            if os.environ.get('SGDK_PLATFORM'):
                base += ['--platform', os.environ['SGDK_PLATFORM']]
            if hasattr(os, 'getuid'):
                base += ['--user', f'{os.getuid()}:{os.getgid()}']
            base += ['--mount', f'type=bind,source={project},target=/workspace', os.environ.get('SGDK_IMAGE', 'ewrogers/sgdk:latest')]
            def run(*args):
                result = subprocess.run(base + list(args), capture_output=True, text=True, timeout=240)
                self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-6000:])
                return result.stdout + result.stderr
            hashes = {}
            for mode in ['release', 'debug']:
                run('make', 'clean')
                log = run('make', mode)
                self.assertNotIn('warning:', log)
                rom = (project / 'out/rom.bin').read_bytes()
                self.assertEqual(len(rom), 131072)
                self.assertEqual(rom[0x100:0x104], b'SEGA')
                self.assertEqual(struct.unpack_from('>I', rom, 0x1A4)[0], len(rom) - 1)
                symbols = run('m68k-elf-nm', 'out/rom.out')
                for resource in ['star_image', 'orb_image']:
                    self.assertRegex(symbols, r'\b' + resource + r'\b')
                hashes[mode] = hashlib.sha256(rom).hexdigest()
            # Replace only the disposable project's entry point. Test the
            # selected SDK libraries, not a host mock or copied math routines.
            shutil.copyfile(FIXTURE / 'sdk_probe.c', project / 'src/main.c')
            for mode in ['release', 'debug']:
                run('make', 'clean')
                run('make', mode)
                code, values = execute_sdk((project / 'out/rom.bin').read_bytes(), run('m68k-elf-nm', 'out/rom.out'))
                self.assertEqual(code, 0, f'SDK arithmetic contract failed at case {code}')
                self.assertEqual(len(values), 360)
                for angle, value in enumerate(values):
                    self.assertLessEqual(abs(value - math.sin(math.radians(angle)) * 64), 1.01)
                self.assertEqual([values[i] for i in [0, 90, 180, 270]], [0, 64, 0, -64])
            output = ROOT / 'build/math-integration'
            output.mkdir(parents=True, exist_ok=True)
            (output / 'math-report.json').write_text(json.dumps({'passed': True, 'hashes': hashes,
                'sdk_math': 'executed release/debug on Unicorn 2.1.4 M68000'}, indent=2) + '\n')


if __name__ == '__main__':
    unittest.main()
