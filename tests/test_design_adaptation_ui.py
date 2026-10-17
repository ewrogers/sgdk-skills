"""Informed usability revision; keep the blind first submission untouched."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from test_streaming import load

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / 'tests/fixtures/design-adaptation'
REVISION = FIXTURE / 'revision'


def populate(project):
    for source in (FIXTURE / 'submission', REVISION):
        for name in ('src', 'inc'):
            shutil.copytree(source / name, project / name, dirs_exist_ok=True)


class DeskUIContracts(unittest.TestCase):
    def test_paged_help_preserves_hold_bounded_layouts_and_completed_row_transfers(self):
        cc = next((p for n in ('clang', 'cc', 'gcc') if (p := shutil.which(n))), None)
        if not cc:
            if os.environ.get('SGDK_REQUIRE_HOST_CC') == '1':
                raise RuntimeError('C17 compiler required')
            self.skipTest('C17 compiler required')
        with tempfile.TemporaryDirectory(prefix='desk usability ') as scratch:
            for mode in ('-O0', '-O3'):
                binary = Path(scratch) / ('checks' + mode + ('.exe' if os.name == 'nt' else ''))
                args = [cc, '-std=c17', mode, '-Wall', '-Wextra', '-Werror', '-pedantic',
                        '-I', str(FIXTURE / 'submission/inc'), '-I', str(REVISION / 'inc'),
                        str(FIXTURE / 'submission/src/desk.c'),
                        str(REVISION / 'src/desk_ui.c'), str(REVISION / 'src/desk_view.c'),
                        str(REVISION / 'checks.c'), '-o', str(binary)]
                if os.name != 'nt': args += ['-fsanitize=undefined', '-fno-sanitize-recover=all']
                result = subprocess.run(args, capture_output=True, text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stderr)
                result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION') == '1',
                     'Docker integration is opt-in')
class DeskUIContainer(unittest.TestCase):
    def test_revised_ui_builds_with_real_sdk_and_valid_region_headers(self):
        output = ROOT / 'build/design-adaptation-ui-integration'
        output.mkdir(parents=True, exist_ok=True)
        helper = ROOT / 'skills/sgdk-project/scripts/sgdk.py'
        base = [sys.executable, str(helper)]
        if os.environ.get('SGDK_PLATFORM'): base += ['--platform', os.environ['SGDK_PLATFORM']]
        inspector = load('desk_ui_release', ROOT / 'skills/sgdk-release/scripts/release.py')
        with tempfile.TemporaryDirectory(prefix='revised dispatch ', dir=ROOT / 'build') as scratch:
            project = Path(scratch) / 'Last Service'

            def run(args, name):
                with (output / (name + '.log')).open('w') as log:
                    result = subprocess.run(args, stdout=log, stderr=subprocess.STDOUT, timeout=240)
                text = (output / (name + '.log')).read_text()
                self.assertEqual(result.returncode, 0, text[-4000:])
                return text

            run(base + ['init', str(project)], 'init')
            populate(project)
            header_path = project / 'src/boot/rom_head.c'
            header = header_path.read_text()
            hashes = {}
            for region in ('U', 'E'):
                header_path.write_text(header.replace('"JUE             "', '"' + region.ljust(16) + '"'))
                run(base + ['--project', str(project), 'build', 'clean'], region + '-clean')
                log = run(base + ['--project', str(project), 'build', 'release'], region)
                self.assertNotIn('warning:', log)
                data = (project / 'out/rom.bin').read_bytes()
                report = inspector.inspect_rom(data)
                self.assertTrue(report['valid'])
                self.assertEqual(report['findings'], [])
                self.assertEqual(report['header']['region'], region)
                hashes[region] = hashlib.sha256(data).hexdigest()
                (output / ('last-service-' + region + '.bin')).write_bytes(data)
            (output / 'desk-ui-report.json').write_text(json.dumps({
                'passed': True, 'hashes': hashes,
                'max_row_operations': 56, 'max_frame_bytes': 4480,
                'evidence': 'Informed UI revision, not the blind first submission. Builds do not certify flicker or usability.'
            }, indent=2) + '\n')
