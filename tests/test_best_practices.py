"""Behavioral examples for C contracts and SGDK resource ownership."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
EXAMPLES = ROOT / 'skills/sgdk-best-practices/assets/examples'
FIXTURE = ROOT / 'tests/fixtures/best_practices'


class BestPracticesBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler = next((p for name in ['clang', 'cc', 'gcc'] if (p := shutil.which(name))), None)
        if not cls.compiler:
            if os.environ.get('SGDK_REQUIRE_HOST_CC') == '1':
                raise RuntimeError('A C17 compiler is required')
            raise unittest.SkipTest('A C17 compiler is required')
        cls.scratch = tempfile.TemporaryDirectory(prefix='sgdk C contracts ')
        cls.addClassCleanup(cls.scratch.cleanup)
        cls.project = Path(cls.scratch.name) / 'standalone examples'
        shutil.copytree(EXAMPLES, cls.project)
        cls.binaries = []
        for optimization in ['-O0', '-O3']:
            cls.binaries.append(cls.build(optimization, []))

    @classmethod
    def build(cls, label, flags):
        binary = Path(cls.scratch.name) / (label + ('.exe' if os.name == 'nt' else ''))
        command = [cls.compiler, '-std=c17', '-Wall', '-Wextra', '-Werror', '-O3',
                   '-fstrict-aliasing', *flags]
        if label in ['-O0', '-O3']:
            command.append(label)
        command += ['-I', str(FIXTURE), '-I', str(cls.project / 'inc')]
        command += [str(cls.project / 'src' / name) for name in ['checked.c', 'checks.c', 'pair.c', 'upload.c']]
        command += [str(FIXTURE / 'contracts.c'), '-o', str(binary)]
        result = subprocess.run(command, capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        return binary

    def check(self, case):
        for binary in self.binaries:
            with self.subTest(optimization=binary.name):
                result = subprocess.run([str(binary), case], capture_output=True, text=True, timeout=30)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(result.stdout.strip(), case)

    def test_integer_promotions_saturation_shift_bounds_and_wrapping_elapsed(self): self.check('numbers')
    def test_offset_byte_records_roundtrip_reject_malformed_and_preserve_destinations(self): self.check('packets')
    def test_partial_sprite_acquisition_cleanup_reentry_and_repeatable_close(self): self.check('ownership')
    def test_dma_copy_lifetime_failures_retry_and_nested_interrupt_pairing(self): self.check('uploads')

    @unittest.skipIf(os.name == 'nt', 'UBSan runtime is checked on Unix hosts')
    def test_corrected_examples_with_undefined_behavior_sanitizer(self):
        binary = self.build('ubsan', ['-fsanitize=undefined', '-fno-sanitize-recover=all'])
        for case in ['numbers', 'packets', 'ownership', 'uploads']:
            result = subprocess.run([str(binary), case], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_documented_host_example_builds_from_standalone_copy(self):
        binary = Path(self.scratch.name) / ('example.exe' if os.name == 'nt' else 'example')
        result = subprocess.run([self.compiler, '-std=c17', '-O2', '-Wall', '-Wextra', '-Werror',
                                 '-DBP_HOST_MAIN', '-I', str(self.project / 'inc'),
                                 str(self.project / 'src/checked.c'), str(self.project / 'src/checks.c'),
                                 '-o', str(binary)], capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.strip(), 'C contracts OK')


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION') == '1', 'Docker integration is opt-in')
class BestPracticesContainerTests(unittest.TestCase):
    def test_actual_sdk_compilation_and_executed_m68000_dma_contracts(self):
        from test_math import execute_sdk
        output = ROOT / 'build/best-practices-integration'
        output.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='best practices container ', dir=ROOT / 'build') as scratch:
            project = Path(scratch) / 'standalone examples'
            shutil.copytree(EXAMPLES, project)
            base = ['docker', 'run', '--rm']
            if os.environ.get('SGDK_PLATFORM'):
                base += ['--platform', os.environ['SGDK_PLATFORM']]
            if hasattr(os, 'getuid'):
                base += ['--user', f'{os.getuid()}:{os.getgid()}']
            base += ['--mount', f'type=bind,source={project},target=/workspace',
                     os.environ.get('SGDK_IMAGE', 'ewrogers/sgdk:latest')]

            def run(*args):
                result = subprocess.run(base + list(args), capture_output=True, text=True, timeout=240)
                self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-6000:])
                return result.stdout + result.stderr

            hashes = {}
            for kind in ['example', 'sdk-probe']:
                if kind == 'sdk-probe':
                    shutil.copyfile(FIXTURE / 'target_probe.c', project / 'src/main.c')
                for mode in ['release', 'debug']:
                    run('make', 'clean')
                    log = run('make', mode)
                    (output / f'{kind}-{mode}.log').write_text(log)
                    self.assertFalse('warning:' in log, '\n'.join(line for line in log.splitlines() if 'warning:' in line))
                    rom = (project / 'out/rom.bin').read_bytes()
                    self.assertEqual(rom[0x100:0x104], b'SEGA')
                    code, values = execute_sdk(rom, run('m68k-elf-nm', 'out/rom.out'))
                    self.assertEqual(code, 0, f'{kind}/{mode} failed at line {code}')
                    self.assertEqual(values, [])
                    hashes[f'{kind}-{mode}'] = hashlib.sha256(rom).hexdigest()
            (output / 'best-practices-report.json').write_text(json.dumps({
                'passed': True, 'hashes': hashes,
                'execution': 'Unicorn 2.1.4 M68000 direct entry, no VDP transfer or interrupt scheduling'
            }, indent=2) + '\n')


if __name__ == '__main__':
    unittest.main()
