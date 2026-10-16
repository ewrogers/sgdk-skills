"""Prove replay behavior, useful failures, portable QA and executed target logic."""
import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
LAB = ROOT / 'skills/sgdk-testing/assets/test-lab'
FIXTURE = ROOT / 'tests/fixtures/testing'


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    sys.path.insert(0, str(path.parent))
    try: spec.loader.exec_module(result)
    finally: sys.path.pop(0)
    return result


class ReplayTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler = next((p for n in ['clang', 'cc', 'gcc'] if (p := shutil.which(n))), None)
        if not cls.compiler:
            if os.environ.get('SGDK_REQUIRE_HOST_CC') == '1': raise RuntimeError('C17 compiler required')
            raise unittest.SkipTest('C17 compiler required')
        cls.scratch = tempfile.TemporaryDirectory(prefix='sgdk replay tests ')
        cls.addClassCleanup(cls.scratch.cleanup)
        cls.project = Path(cls.scratch.name) / 'independent test lab'
        shutil.copytree(LAB, cls.project)
        cls.qa = module('testing_qa', cls.project / 'tools/qa.py')
        cls.binary = Path(cls.scratch.name) / ('contracts.exe' if os.name == 'nt' else 'contracts')
        subprocess.run([cls.compiler, '-std=c17', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-I', str(cls.project / 'inc'), str(cls.project / 'src/lab.c'),
                        str(FIXTURE / 'contracts.c'), '-o', str(cls.binary)],
                       check=True, capture_output=True, text=True, timeout=60)

    def case(self, name):
        result = subprocess.run([str(self.binary), name], capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_held_buttons_emit_one_edge_and_pause_does_not_advance_gameplay(self): self.case('edges')
    def test_recording_capacity_merge_end_of_tape_and_rejected_input_are_safe(self): self.case('tape')
    def test_player_slides_along_wall_collects_coins_and_respects_bounds(self): self.case('motion')
    def test_full_projectile_pool_declines_fire_and_expired_slots_are_reused(self): self.case('pools')
    def test_nominal_pal_ntsc_schedules_and_invalid_rates_preserve_phase(self): self.case('clock')
    def test_hundred_thousand_ticks_remain_bounded_repeatable_and_resettable(self): self.case('soak')

    @unittest.skipIf(os.name == 'nt', 'UBSan runtime checked on Unix hosts')
    def test_rule_boundaries_under_undefined_behavior_sanitizer(self):
        binary = Path(self.scratch.name) / 'sanitized'
        subprocess.run([self.compiler, '-std=c17', '-O2', '-fsanitize=undefined',
                        '-fno-sanitize-recover=all', '-I', str(self.project / 'inc'),
                        str(self.project / 'src/lab.c'), str(FIXTURE / 'contracts.c'), '-o', str(binary)],
                       check=True, capture_output=True, text=True, timeout=60)
        for case in ['edges', 'tape', 'motion', 'pools', 'clock', 'soak']:
            result = subprocess.run([str(binary), case], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_standalone_cli_runs_both_regions_matches_golden_and_catches_deliberate_bug(self):
        with tempfile.TemporaryDirectory(prefix='QA reports ') as scratch:
            outputs = []
            for hz, fault in [(60, False), (50, False), (60, True)]:
                report = Path(scratch) / f'{hz}-{fault}.json'
                command = [sys.executable, str(self.project / 'tools/qa.py'), 'run', '--cc', self.compiler,
                           '--hz', str(hz), '--output', str(report)] + (['--fault'] if fault else [])
                result = subprocess.run(command, capture_output=True, text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stderr)
                outputs.append(report)
            golden = self.project / 'replays/courier.expected.json'
            for report, code in zip(outputs, [0, 0, 1]):
                result = subprocess.run([sys.executable, str(self.project / 'tools/qa.py'), 'compare',
                                         str(golden), str(report)], capture_output=True, text=True, timeout=15)
                self.assertEqual(result.returncode, code, result.stdout + result.stderr)
                if code: self.assertEqual(json.loads(result.stdout)['tick'], 60)
            reports = [self.qa.load_trace(path) for path in outputs]
            self.assertEqual([p['frames'] for p in reports[:2]], [720, 600])
            last = reports[0]['checkpoints'][-1]
            self.assertEqual({k: last[k] for k in ('active', 'score', 'fired', 'x', 'y')},
                             {'active': 660, 'score': 2, 'fired': 7, 'x': 16, 'y': 180})
            before = outputs[0].read_bytes()
            result = subprocess.run([sys.executable, str(self.project / 'tools/qa.py'), 'run',
                                     '--output', str(outputs[0])], capture_output=True)
            self.assertEqual(result.returncode, 2)
            self.assertEqual(outputs[0].read_bytes(), before)

    def test_malformed_tapes_cannot_emit_headers_or_replace_an_existing_one(self):
        source = json.loads((LAB / 'replays/courier.json').read_text())
        variants = []
        for path, value in [('seed', -1), ('seed', True), ('version', 2), ('tick_hz', 50),
                            ('name', 'quote"injection'), ('runs', []), ('runs', [{'ticks': 721, 'held': 0}]),
                            ('runs', [{'ticks': 0, 'held': 0}]), ('runs', [{'ticks': 1, 'held': 256}]),
                            ('runs', [{'ticks': True, 'held': 0}]),
                            ('runs', [{'ticks': 1, 'held': 0}] * 129)]:
            item = copy.deepcopy(source); item[path] = value; variants.append(item)
        with tempfile.TemporaryDirectory() as scratch:
            tape = Path(scratch) / 'bad.json'; output = Path(scratch) / 'tape.h'
            for item in variants:
                tape.write_text(json.dumps(item))
                result = subprocess.run([sys.executable, str(self.project / 'tools/qa.py'), 'compile',
                                         str(tape), '--output', str(output)], capture_output=True)
                self.assertEqual(result.returncode, 2, item)
                self.assertFalse(output.exists())
            output.write_text('keep me')
            result = subprocess.run([sys.executable, str(self.project / 'tools/qa.py'), 'compile',
                                     str(LAB / 'replays/courier.json'), '--output', str(output)], capture_output=True)
            self.assertEqual(result.returncode, 2)
            self.assertEqual(output.read_text(), 'keep me')

    def test_empty_truncated_reordered_or_foreign_reports_never_pass(self):
        good = json.loads((LAB / 'replays/courier.expected.json').read_text())
        variants = []
        for key, value in [('checkpoints', []), ('checkpoints', good['checkpoints'][:-1]),
                           ('checkpoints', list(reversed(good['checkpoints']))), ('format', 'unknown'),
                           ('frames', 1), ('seed', True)]:
            item = copy.deepcopy(good); item[key] = value; variants.append(item)
        with tempfile.TemporaryDirectory() as scratch:
            bad = Path(scratch) / 'bad.json'
            for item in variants:
                bad.write_text(json.dumps(item))
                with self.assertRaises(ValueError): self.qa.load_trace(bad)
        foreign = copy.deepcopy(good); foreign['seed'] += 1
        with self.assertRaises(ValueError): self.qa.compare(good, foreign)
        changed = copy.deepcopy(good); changed['checkpoints'][3]['digest'] ^= 1
        self.assertEqual(self.qa.compare(good, changed)['tick'], 180)

    def test_partial_final_frame_is_emitted_and_truncated_target_output_is_rejected(self):
        with tempfile.TemporaryDirectory() as scratch:
            tape = Path(scratch) / 'short.json'
            tape.write_text(json.dumps({'version': 1, 'name': 'short', 'seed': 0,
                                       'tick_hz': 60, 'runs': [{'ticks': 7, 'held': 8}]}))
            reports = [self.qa.run_host(tape, hz, False, self.compiler) for hz in (50, 60)]
            self.assertEqual([r['frames'] for r in reports], [6, 7])
            self.assertEqual([p['tick'] for p in reports[0]['checkpoints']], [0, 7])
            self.assertTrue(self.qa.compare(*reports)['passed'])
            m68k = module('host_testing_m68k', self.project / 'tools/m68k.py')
            with self.assertRaises(ValueError): m68k.traces([], tape)

    def test_prepare_is_standalone_reproducible_and_refuses_overwrite(self):
        prepare = module('testing_prepare', self.project / 'tools/prepare.py')
        with tempfile.TemporaryDirectory(prefix='prepare QA ') as scratch:
            one = prepare.prepare(Path(scratch) / 'one')
            two = prepare.prepare(Path(scratch) / 'two')
            for relative in ['res/courier.png', 'res/mote.png', 'inc/replay_data.h', 'inc/expected_data.h']:
                self.assertEqual((one / relative).read_bytes(), (two / relative).read_bytes())
            before = (one / 'src/main.c').read_bytes()
            with self.assertRaises(ValueError): prepare.prepare(one)
            self.assertEqual((one / 'src/main.c').read_bytes(), before)
        nested = prepare.prepare(self.project / 'build/nested output')
        self.assertTrue((nested / 'tools/qa.py').is_file())
        self.assertFalse((nested / 'build').exists())


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION') == '1', 'Docker integration is opt-in')
class TestingContainerTests(unittest.TestCase):
    def test_release_debug_rom_and_m68000_logic_match_both_region_baselines(self):
        prepare = module('container_testing_prepare', LAB / 'tools/prepare.py')
        qa = module('container_testing_qa', LAB / 'tools/qa.py')
        m68k = module('container_testing_m68k', LAB / 'tools/m68k.py')
        output = ROOT / 'build/testing-integration'; output.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='testing container ', dir=ROOT / 'build') as scratch:
            project = prepare.prepare(Path(scratch) / 'standalone test lab')
            base = ['docker', 'run', '--rm']
            if os.environ.get('SGDK_PLATFORM'): base += ['--platform', os.environ['SGDK_PLATFORM']]
            if hasattr(os, 'getuid'): base += ['--user', f'{os.getuid()}:{os.getgid()}']
            base += ['--mount', f'type=bind,source={project},target=/workspace', os.environ.get('SGDK_IMAGE', 'ewrogers/sgdk:latest')]

            def run(*arguments):
                result = subprocess.run(base + list(arguments), capture_output=True, text=True, timeout=240)
                self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-5000:])
                return result.stdout + result.stderr

            hashes = {}
            golden = qa.load_trace(LAB / 'replays/courier.expected.json')
            for kind in ['rom', 'probe']:
                if kind == 'probe': shutil.copyfile(project / 'tools/target_probe.c', project / 'src/main.c')
                for mode in ['release', 'debug']:
                    run('make', 'clean'); log = run('make', mode)
                    (output / f'{kind}-{mode}.log').write_text(log)
                    self.assertNotIn('warning:', log)
                    rom = (project / 'out/rom.bin').read_bytes()
                    self.assertEqual(rom[0x100:0x104], b'SEGA')
                    self.assertLessEqual(len(rom), 131072)
                    symbols = run('m68k-elf-nm', 'out/rom.out')
                    hashes[f'{kind}-{mode}'] = hashlib.sha256(rom).hexdigest()
                    if kind == 'probe':
                        reports = m68k.traces(m68k.execute(rom, symbols), project / 'replays/courier.json')
                        for hz, report in reports.items():
                            path = output / f'{mode}-{hz}.json'; path.write_text(json.dumps(report, indent=2) + '\n')
                            self.assertTrue(qa.compare(golden, qa.load_trace(path))['passed'])
            # A failed target must not pass just because its trace words exist.
            entry = project / 'src/main.c'
            entry.write_text(entry.read_text().replace('return 0;', 'return 17;'))
            run('make', 'release')
            with self.assertRaisesRegex(ValueError, 'Probe failed'):
                m68k.execute((project / 'out/rom.bin').read_bytes(), run('m68k-elf-nm', 'out/rom.out'))
            (output / 'testing-report.json').write_text(json.dumps({
                'passed': True, 'hashes': hashes, 'final_digest': golden['checkpoints'][-1]['digest'],
                'execution': 'Host and Unicorn M68000 logic; nominal 50/60 Hz schedules; no console device emulation'
            }, indent=2) + '\n')


if __name__ == '__main__': unittest.main()
