"""Dialogue authoring, portable behavior, native art/audio and actual SDK probes."""
import copy
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import wave

ROOT = Path(__file__).resolve().parents[1]
SKILL = ROOT / 'skills/sgdk-dialogue'
LAB = SKILL / 'assets/dialogue-lab'
FIXTURE = ROOT / 'tests/fixtures/dialogue'


def load(path):
    saved = {name: sys.modules.pop(name, None) for name in ['pixel_io', 'glyphs']}
    sys.path.insert(0, str(path.parent))
    try:
        spec = importlib.util.spec_from_file_location('dialogue_' + path.stem, path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module
    finally:
        sys.path.pop(0)
        for name, previous in saved.items():
            sys.modules.pop(name, None)
            if previous is not None: sys.modules[name] = previous


class DialogueBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler = next((p for name in ['clang', 'cc', 'gcc'] if (p := shutil.which(name))), None)
        if not cls.compiler:
            if os.environ.get('SGDK_REQUIRE_HOST_CC') == '1': raise RuntimeError('C17 compiler required')
            raise unittest.SkipTest('C17 compiler required')
        cls.scratch = tempfile.TemporaryDirectory(prefix='dialogue host ')
        cls.addClassCleanup(cls.scratch.cleanup)
        cls.project = Path(cls.scratch.name) / 'standalone lab'
        shutil.copytree(LAB, cls.project)
        cls.binaries = [cls.build('-O0'), cls.build('-O3')]

    @classmethod
    def build(cls, optimization, sanitizer=False):
        binary = Path(cls.scratch.name) / ('model-' + optimization + ('-san' if sanitizer else '') + ('.exe' if os.name == 'nt' else ''))
        command = [cls.compiler, '-std=c17', optimization, '-Wall', '-Wextra', '-Werror', '-I', str(cls.project / 'inc')]
        if sanitizer: command += ['-fsanitize=undefined', '-fno-sanitize-recover=all']
        command += [str(cls.project / 'src' / name) for name in ['dialogue.c', 'dialogue_data.c', 'panel.c', 'lab.c']]
        command += [str(FIXTURE / 'model_test.c'), '-o', str(binary)]
        result = subprocess.run(command, capture_output=True, text=True, timeout=60)
        if result.returncode: raise RuntimeError(result.stdout + result.stderr)
        return binary

    def check(self, case):
        for binary in self.binaries:
            result = subprocess.run([str(binary), case], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual(result.stdout.strip(), case)

    def test_timed_reveal_delays_styles_pages_speaker_panes_and_single_use_confirm(self): self.check('timing')
    def test_choices_wrap_filter_flags_branch_and_handle_all_hidden_options(self): self.check('choices')
    def test_invalid_graphs_tokens_fields_and_work_bounds_reject_safely(self): self.check('validation')
    def test_nine_slice_corners_edges_centers_small_sizes_and_screen_bounds(self): self.check('panels')
    def test_pal_ntsc_nominal_clock_pause_held_edges_replay_and_story_menu(self): self.check('clocks')
    def test_thirty_thousand_authored_graph_steps_preserve_bounds(self): self.check('authored')

    def test_sound_terminal_cue_is_once_and_pause_resume_does_not_replay_it(self):
        binary = Path(self.scratch.name) / ('sound.exe' if os.name == 'nt' else 'sound')
        command = [self.compiler, '-std=c17', '-O3', '-Wall', '-Wextra', '-Werror',
                   '-I', str(FIXTURE / 'sdk_stub'), '-I', str(self.project / 'inc')]
        command += [str(self.project / 'src' / name) for name in ['dialogue.c', 'dialogue_data.c', 'lab.c', 'sound.c']]
        command += [str(FIXTURE / 'sound_test.c'), '-o', str(binary)]
        result = subprocess.run(command, capture_output=True, text=True, timeout=60)
        self.assertEqual(result.returncode, 0, result.stderr)
        result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), 'sound lifecycle')

    @unittest.skipIf(os.name == 'nt', 'UBSan runs on Unix hosts')
    def test_corrected_model_with_undefined_behavior_sanitizer(self):
        binary = self.build('-O2', True)
        for case in ['timing', 'choices', 'validation', 'panels', 'clocks', 'authored']:
            result = subprocess.run([str(binary), case], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


class DialogueCompilerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.module = load(SKILL / 'scripts/compile_dialogue.py')
        cls.source = json.loads((LAB / 'res/conversations.json').read_text())

    def test_styles_delays_wrapping_long_words_and_automatic_pages(self):
        layout = self.module.layout
        tokens = layout('hello [color=gold][font=soft]world[/font][/color]', 5)
        self.assertEqual([value for op, value, _ in tokens if op == 'glyph'], list(map(ord, 'helloworld')))
        self.assertEqual([style for op, _, style in tokens if op == 'glyph'], [0]*5 + [4]*5)
        self.assertEqual(sum(op == 'newline' for op, _, _ in tokens), 1)
        tokens = layout('abcdefghijklmnopqrstuvw', 5)
        self.assertEqual(sum(op == 'page' for op, _, _ in tokens), 1)
        self.assertEqual(''.join(chr(value) for op, value, _ in tokens if op == 'glyph'), 'abcdefghijklmnopqrstuvw')
        tokens = layout('a[pause=350][speed=80]b[page]c[[d', 29)
        self.assertIn(('pause', 350, 0), tokens)
        self.assertIn(('speed', 80, 0), tokens)
        self.assertEqual(''.join(chr(value) for op, value, _ in tokens if op == 'glyph'), 'abc[d')

    def test_invalid_markup_limits_targets_flags_and_metadata_reject(self):
        for bad in ['[font=no]a[/font]', '[color=red]a[/color]', '[pause=0]a', '[speed=1001]a',
                    '[pause=5001]a', '[wat]a', '[color=gold]a', '[font=soft][color=gold]a[/font][/color]',
                    'a[page]', '[page]a', 'a[', 'café', '   ', 'a' + '[speed=1]'*256]:
            with self.subTest(text=bad[:60]), self.assertRaises(ValueError): self.module.layout(bad, 29)
        for change in [lambda d: d['nodes'].append(copy.deepcopy(d['nodes'][0])),
                       lambda d: d['nodes'][0].update(next='missing'),
                       lambda d: d['nodes'][0].update(speaker='missing'),
                       lambda d: d['nodes'][0].update(pane='middle'),
                       lambda d: d['nodes'][0].update(split='yes'),
                       lambda d: d['nodes'][0].update(unrecognized=1),
                       lambda d: d['nodes'][2]['choices'][0].update(set=['missing']),
                       lambda d: d['nodes'][2]['choices'][0].update(require=['compass'], forbid=['compass']),
                       lambda d: d['nodes'][2]['choices'][0].update(label=' '),
                       lambda d: d['speakers'][0].update(portrait=True),
                       lambda d: d['flags'].append('compass')]:
            document = copy.deepcopy(self.source); change(document)
            with self.assertRaises(ValueError): self.module.compile_program(document)

    def test_committed_data_matches_compiler_and_unsafe_regeneration_is_rejected(self):
        program = self.module.compile_program(self.source)
        for name, contents in self.module.emit(program).items(): self.assertEqual(contents, (LAB / name).read_text())
        with tempfile.TemporaryDirectory(prefix='dialogue author ') as scratch:
            destination = Path(scratch) / 'generated files'
            command = [sys.executable, str(SKILL / 'scripts/compile_dialogue.py'), str(LAB / 'res/conversations.json'), '--out', str(destination)]
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            files = {p: p.read_bytes() for p in destination.rglob('*') if p.is_file()}
            self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)
            self.assertEqual(files, {p: p.read_bytes() for p in files})
            bad = Path(scratch) / 'bad.json'; document = copy.deepcopy(self.source); document['nodes'][0]['next'] = 'missing'
            bad.write_text(json.dumps(document))
            result = subprocess.run([sys.executable, str(SKILL / 'scripts/compile_dialogue.py'), str(bad), '--out', str(destination), '--force'], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('ferry_welcome', result.stderr)
            self.assertEqual(files, {p: p.read_bytes() for p in files})

    def test_generated_c_preserves_quotes_backslashes_and_c17_trigraphs(self):
        compiler = next((p for n in ['clang', 'cc', 'gcc'] if (p := shutil.which(n))), None)
        if not compiler: self.skipTest('C17 compiler required')
        document = copy.deepcopy(self.source)
        title = '??/ "quote" \\ path'
        document['stories'][0]['title'] = title
        with tempfile.TemporaryDirectory(prefix='dialogue strings ') as scratch:
            root = Path(scratch)
            for name, content in self.module.emit(self.module.compile_program(document)).items():
                p = root / name; p.parent.mkdir(exist_ok=True); p.write_text(content)
            (root / 'main.c').write_text('#include "dialogue_data.h"\n#include <stdio.h>\nint main(void) { puts(lab_titles[0]); return 0; }\n')
            binary = root / ('probe.exe' if os.name == 'nt' else 'probe')
            result = subprocess.run([compiler, '-std=c17', '-Wall', '-Wextra', '-Werror', '-I', str(root / 'inc'),
                                     '-I', str(LAB / 'inc'), str(root / 'src/dialogue_data.c'), str(root / 'main.c'), '-o', str(binary)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(binary)], capture_output=True, text=True)
            self.assertEqual(result.stdout.strip(), title)


class DialogueAssetTests(unittest.TestCase):
    def test_native_indexed_assets_palette_transparency_and_reproducible_nine_slice_font_art(self):
        art = load(LAB / 'tools/make_art.py'); png = load(LAB / 'tools/pixel_io.py')
        for name, data in art.assets().items(): self.assertEqual(data, (LAB / 'res' / name).read_bytes())
        sizes = {'fonts.png': (768,48), 'panels.png': (24,24), 'icons.png': (32,8), 'world.png': (128,8),
                 'courier.png': (160,32), 'keeper.png': (160,32)}
        for name, dimensions in sizes.items():
            w, h, pixels = png.read_png(LAB / 'res' / name)
            self.assertEqual((w,h), dimensions)
            self.assertLessEqual(len({p[:3] for p in pixels}), 16)
            self.assertEqual((LAB / 'res' / name).read_bytes()[24:26], bytes([8,3]))
            if name in ['courier.png', 'keeper.png']:
                self.assertTrue(any(p[3] == 0 for p in pixels))
                self.assertTrue(any(p[3] == 255 for p in pixels))
                self.assertTrue({p[:3] for p in pixels if p[3]} <= {tuple(bytes.fromhex(c)) for c in art.PORTRAIT[1:]})
        with tempfile.TemporaryDirectory() as scratch:
            frame = Path(scratch) / 'wrong.png'; frame.write_bytes(png.indexed_png(16,16,[0]*256,art.PORTRAIT))
            with self.assertRaises(ValueError): art.pack_portrait([frame]*5)
        w, _, pixels = png.read_png(LAB / 'res/fonts.png')
        # The same A glyph has three ink colors and two distinct native faces.
        glyphs = [tuple(pixels[(bank*8+y)*w+33*8+x] for y in range(8) for x in range(8)) for bank in range(6)]
        self.assertEqual(len(set(glyphs)), 6)

    def test_finite_dialogue_cues_and_original_looped_fm_shanty_formats(self):
        audio = load(LAB / 'tools/make_audio.py')
        for name, data in audio.assets().items():
            self.assertEqual(data, (LAB / 'res' / name).read_bytes())
            if name.endswith('.wav'):
                with wave.open(io.BytesIO(data)) as wav:
                    self.assertEqual((wav.getnchannels(),wav.getsampwidth(),wav.getframerate()), (1,2,13300))
                    samples = struct.unpack('<'+'h'*wav.getnframes(),wav.readframes(wav.getnframes()))
                self.assertEqual((samples[0],samples[-1]),(0,0))
                self.assertLess(len(samples),13300)
                self.assertGreater(max(map(abs,samples)),1000)
            else:
                self.assertEqual(data[:4],b'Vgm ')
                self.assertEqual(struct.unpack_from('<I',data,4)[0],len(data)-4)
                self.assertGreater(struct.unpack_from('<I',data,0x1C)[0],0)
                self.assertEqual(struct.unpack_from('<I',data,0x0C)[0],0)  # FM music leaves PSG alone.
                cursor=256; waits=0
                while data[cursor] != 0x66:
                    if data[cursor] == 0x62: waits+=1; cursor+=1
                    else:
                        self.assertIn(data[cursor],(0x52,0x53)); cursor+=3
                self.assertEqual(waits,1728)
                self.assertEqual(waits*735,struct.unpack_from('<I',data,0x18)[0])
                self.assertEqual(waits*735,struct.unpack_from('<I',data,0x20)[0])


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION') == '1','Docker integration is opt-in')
class DialogueContainerTests(unittest.TestCase):
    def test_standalone_release_debug_resources_and_executed_m68000_contracts(self):
        from test_math import execute_sdk
        output=ROOT / 'build/dialogue-integration'; output.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='dialogue container ',dir=ROOT / 'build') as scratch:
            project=Path(scratch) / 'standalone dialogue lab'; shutil.copytree(LAB,project)
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'): base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'): base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                result=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                self.assertEqual(result.returncode,0,(result.stdout+result.stderr)[-5000:])
                return result.stdout+result.stderr
            hashes={}; geometry={}
            for kind in ['lab','probe']:
                if kind == 'probe': shutil.copyfile(FIXTURE / 'target_probe.c',project / 'src/main.c')
                for mode in ['release','debug']:
                    run('make','clean'); log=run('make',mode); (output / f'{kind}-{mode}.log').write_text(log)
                    self.assertFalse('warning:' in log,'\n'.join(l for l in log.splitlines() if 'warning:' in l))
                    rom=(project / 'out/rom.bin').read_bytes()
                    self.assertEqual(rom[0x100:0x104],b'SEGA'); self.assertEqual(len(rom),131072)
                    hashes[f'{kind}-{mode}']=hashlib.sha256(rom).hexdigest()
                    if kind == 'probe':
                        code, values=execute_sdk(rom,run('m68k-elf-nm','out/rom.out'))
                        self.assertEqual(code,0,f'M68000 contract failed at line {code}')
                        self.assertEqual(len(values),3); self.assertLessEqual(values[0],781)
                        geometry[mode]={'resident_end_tile':values[0],'dialogue_bytes':values[1],'lab_bytes':values[2]}
            (output / 'dialogue-report.json').write_text(json.dumps({'passed':True,'hashes':hashes,'geometry':geometry,
                'target':'Unicorn 2.1.4 M68000; no VDP timing/audio emulation'},indent=2)+'\n')


if __name__ == '__main__': unittest.main()
