"""Replay the frozen agent submission; creative adaptation is a manual assessment."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

from test_streaming import load

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / 'tests/fixtures/design-adaptation'
SUBMISSION = FIXTURE / 'submission'


def host_run(directory, source, mode):
    cc = next((path for name in ('clang', 'cc', 'gcc')
               if (path := shutil.which(name))), None)
    if not cc:
        if os.environ.get('SGDK_REQUIRE_HOST_CC') == '1':
            raise RuntimeError('C17 compiler required')
        raise unittest.SkipTest('C17 compiler required')
    binary = Path(directory) / (Path(source).stem + mode +
                                ('.exe' if os.name == 'nt' else ''))
    command = [cc, '-std=c17', mode, '-Wall', '-Wextra', '-Werror', '-pedantic',
               '-I', str(SUBMISSION / 'inc'), '-I', str(FIXTURE),
               str(SUBMISSION / 'src/desk.c'), str(source), '-o', str(binary)]
    if os.name != 'nt':
        command += ['-fsanitize=undefined', '-fno-sanitize-recover=all']
    result = subprocess.run(command, capture_output=True, text=True, timeout=60)
    if result.returncode:
        raise AssertionError(result.stdout + result.stderr)
    result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=60)
    if result.returncode:
        raise AssertionError(result.stdout + result.stderr)
    return result.stdout.strip()


class DesignAdaptationContracts(unittest.TestCase):
    def test_original_submitted_conflicts_capacity_hold_help_retry_and_regions(self):
        with tempfile.TemporaryDirectory(prefix='adaptation submitted ') as directory:
            self.assertEqual(host_run(directory, SUBMISSION / 'tests/test_desk.c', '-O0'),
                             host_run(directory, SUBMISSION / 'tests/test_desk.c', '-O3'))

    def test_independent_topology_oracle_clear_reject_help_and_regional_trace(self):
        with tempfile.TemporaryDirectory(prefix='adaptation review ') as directory:
            self.assertEqual(host_run(directory, FIXTURE / 'review_host.c', '-O0'),
                             host_run(directory, FIXTURE / 'review_host.c', '-O3'))


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION') == '1',
                     'Docker integration is opt-in')
class DesignAdaptationContainer(unittest.TestCase):
    def test_first_submission_regions_actual_sdk_mapping_and_m68000_contracts(self):
        from test_streaming import execute
        output = ROOT / 'build/design-adaptation-integration'
        output.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='adapted dispatch ', dir=ROOT / 'build') as directory:
            directory = Path(directory)
            skills = directory / '.agents/skills'
            for name in ('sgdk-project', 'sgdk-release'):
                shutil.copytree(ROOT / 'skills' / name, skills / name,
                                ignore=shutil.ignore_patterns('__pycache__'))
            helper = skills / 'sgdk-project/scripts/sgdk.py'
            base = [sys.executable, str(helper)]
            if os.environ.get('SGDK_PLATFORM'):
                base += ['--platform', os.environ['SGDK_PLATFORM']]

            def run(args, name):
                with (output / (name + '.log')).open('w') as log:
                    result = subprocess.run(args, stdout=log, stderr=subprocess.STDOUT,
                                            timeout=240)
                text = (output / (name + '.log')).read_text()
                self.assertEqual(result.returncode, 0, text[-4000:])
                return text

            project = directory / 'Last Service project'
            run(base + ['init', str(project)], 'init')
            shutil.copytree(SUBMISSION / 'src', project / 'src', dirs_exist_ok=True)
            shutil.copytree(SUBMISSION / 'inc', project / 'inc', dirs_exist_ok=True)
            header_path = project / 'src/boot/rom_head.c'
            header = header_path.read_text()
            hashes = {}
            inspector = load('adaptation_release', skills / 'sgdk-release/scripts/release.py')
            for region in ('JUE', 'U', 'E'):
                self.assertIn('"JUE', header)
                header_path.write_text(re.sub(r'"JUE\s*"', '"' + region.ljust(16) + '"', header))
                run(base + ['--project', str(project), 'build', 'clean'], region + '-clean')
                log = run(base + ['--project', str(project), 'build', 'release'], region)
                self.assertNotIn('warning:', log)
                data = (project / 'out/rom.bin').read_bytes()
                hashes[region] = hashlib.sha256(data).hexdigest()
                report = inspector.inspect_rom(data)
                self.assertTrue(report['valid'])
                self.assertEqual(report['findings'], [])
                self.assertEqual(report['header']['region'], region)
            # Include the exact submitted main.c to exercise its static edges()
            # against actual SDK masks. Unused view code is linker-collected.
            shutil.copyfile(SUBMISSION / 'src/main.c', project / 'inc/submitted_main.c')
            shutil.copyfile(FIXTURE / 'review_checks.h', project / 'inc/review_checks.h')
            shutil.copyfile(FIXTURE / 'review_probe.c', project / 'src/main.c')
            run(base + ['--project', str(project), 'build', 'clean'], 'probe-clean')
            log = run(base + ['--project', str(project), 'build', 'release'], 'probe')
            self.assertNotIn('warning:', log)

            def probe(name):
                symbols = run(base + ['--project', str(project), 'run',
                                     'm68k-elf-nm', '--defined-only',
                                     '/workspace/out/rom.out'], name + '-symbols')
                data = (project / 'out/rom.bin').read_bytes()
                return execute(data, symbols, instruction_budget=100000000)

            code, values = probe('probe')
            self.assertEqual(code, 0, f'M68000 review contract failed at line {code}')
            trace = int(host_run(directory, FIXTURE / 'review_host.c', '-O3'))
            self.assertEqual(values[:3], [0xD15C, trace >> 16, trace & 65535])
            # Negative control: the exact target adapter must catch C mapped to A.
            mapping = (project / 'inc/submitted_main.c').read_text()
            self.assertIn('if(p&BUTTON_C)e|=K_C;', mapping)
            (project / 'inc/submitted_main.c').write_text(
                mapping.replace('if(p&BUTTON_C)e|=K_C;', 'if(p&BUTTON_C)e|=K_A;'))
            run(base + ['--project', str(project), 'build', 'clean'], 'wrong-map-clean')
            run(base + ['--project', str(project), 'build', 'release'], 'wrong-map')
            wrong_code, wrong_values = probe('wrong-map')
            self.assertNotEqual(wrong_code, 0)
            self.assertEqual(wrong_values, [0xD15C])
            (output / 'design-adaptation-report.json').write_text(json.dumps({
                'passed': True, 'hashes': hashes, 'trace': trace,
                'target_layout': values[3:], 'wrong_mapping_detected': True,
                'evidence': 'Frozen submission; host and base M68000 contracts; real SDK mapping. Creative adaptation is manually assessed.'
            }, indent=2) + '\n')
