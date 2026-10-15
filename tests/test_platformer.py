"""Exercise the real Platform Lab model, native art and compiled 68000 code."""
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
import xml.etree.ElementTree as ET
import zlib

ROOT = Path(__file__).resolve().parents[1]
LAB = ROOT / 'skills/sgdk-platformer/assets/platform-lab'
FIXTURE = ROOT / 'tests/fixtures/platformer'


class PlatformBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = next((p for name in ['clang', 'cc', 'gcc'] if (p := shutil.which(name))), None)
        if not compiler:
            if os.environ.get('SGDK_REQUIRE_HOST_CC') == '1':
                raise RuntimeError('A C17 compiler is required')
            raise unittest.SkipTest('A C17 compiler is required')
        cls.scratch = tempfile.TemporaryDirectory(prefix='sgdk platform host ')
        cls.addClassCleanup(cls.scratch.cleanup)
        project = Path(cls.scratch.name) / 'standalone platform lab'
        shutil.copytree(LAB, project)
        cls.binary = Path(cls.scratch.name) / ('model.exe' if os.name == 'nt' else 'model')
        command = [compiler, '-std=c17', '-O2', '-Wall', '-Wextra', '-Werror', '-I', str(project / 'inc')]
        command += [str(project / 'src' / name) for name in ['game.c', 'terrain.c', 'enemies.c']]
        result = subprocess.run(command + [str(FIXTURE / 'model_test.c'), '-o', str(cls.binary)],
                                capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)

    def check(self, name):
        result = subprocess.run([str(self.binary), name], capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.strip(), name)

    def test_half_open_bounds_walls_and_bounded_substeps(self): self.check('geometry')
    def test_uphill_downhill_slope_attachment_without_embedding(self): self.check('slopes')
    def test_one_way_ascending_landing_and_deliberate_drop(self): self.check('oneway')
    def test_variable_jump_coyote_buffer_wall_jump_and_single_air_jump(self): self.check('jumps')
    def test_moving_platform_carry_jump_drop_and_static_support_identity(self): self.check('platform')
    def test_charge_threshold_complete_dash_and_breakable_terrain(self): self.check('charge')
    def test_ledge_patrol_falling_walker_hopper_flyer_armor_stomp_and_damage(self): self.check('enemies')
    def test_powers_secret_goal_checkpoint_damage_and_fall_reset(self): self.check('progression')
    def test_pal_ntsc_equal_nominal_steps_and_frozen_pause(self): self.check('clock')
    def test_hundred_thousand_mixed_input_steps_remain_collision_safe(self): self.check('soak')


    def test_sound_events_priorities_pause_ready_edge_and_single_fanfare(self):
        compiler = next(shutil.which(name) for name in ['clang', 'cc', 'gcc'] if shutil.which(name))
        binary = Path(self.scratch.name) / ('sound.exe' if os.name == 'nt' else 'sound')
        result = subprocess.run([compiler, '-std=c17', '-Wall', '-Wextra', '-Werror',
            '-I', str(FIXTURE), '-I', str(LAB / 'inc'), str(LAB / 'src/sound.c'),
            str(FIXTURE / 'sound_test.c'), '-o', str(binary)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)


class PlatformAssetsTests(unittest.TestCase):
    def test_native_art_reproduction_palettes_png_integrity_and_no_overwrite(self):
        spec = importlib.util.spec_from_file_location('platform_assets', LAB / 'tools/make_assets.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        expected_sizes = {'course.png': (112, 16), 'hero.png': (160, 24),
                          'enemies.png': (320, 16), 'items.png': (208, 16)}
        for name, data in module.assets().items():
            self.assertEqual(data, (LAB / name).read_bytes(), name)
            if not name.endswith('.png'):
                continue
            offset, compressed = 8, bytearray()
            while offset < len(data):
                size = struct.unpack_from('>I', data, offset)[0]
                kind, payload = data[offset+4:offset+8], data[offset+8:offset+8+size]
                self.assertEqual(zlib.crc32(kind+payload), struct.unpack_from('>I', data, offset+8+size)[0])
                if kind == b'IHDR':
                    width, height, depth, mode, _, _, _ = struct.unpack('>IIBBBBB', payload)
                    self.assertEqual((width, height), expected_sizes[Path(name).name])
                    self.assertEqual((depth, mode), (8, 3))
                elif kind == b'PLTE':
                    self.assertEqual(len(payload), 48)
                    self.assertTrue(all(c in {0,36,73,109,146,182,219,255} for c in payload))
                elif kind == b'IDAT': compressed.extend(payload)
                offset += size+12
            pixels = zlib.decompress(compressed)
            self.assertEqual(len(pixels), (width+1)*height)
            for y in range(height):
                self.assertEqual(pixels[y*(width+1)], 0)
                self.assertLess(max(pixels[y*(width+1)+1:(y+1)*(width+1)]), 16)
        with tempfile.TemporaryDirectory(prefix='native platform assets ') as scratch:
            command = [sys.executable, str(LAB / 'tools/make_assets.py'), scratch]
            self.assertEqual(subprocess.run(command, capture_output=True).returncode, 0)
            before = {p.relative_to(scratch): p.read_bytes() for p in Path(scratch).rglob('*') if p.is_file()}
            self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)
            self.assertEqual(before, {p.relative_to(scratch): p.read_bytes() for p in Path(scratch).rglob('*') if p.is_file()})

    def test_audio_reproduction_waveform_bounds_and_vgm_timing_and_loop(self):
        import io
        import wave
        spec = importlib.util.spec_from_file_location('platform_audio', LAB / 'tools/make_audio.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        for name, data in module.assets().items():
            self.assertEqual(data, (LAB / 'res' / name).read_bytes())
            if name.endswith('.wav'):
                with wave.open(io.BytesIO(data)) as audio:
                    self.assertEqual((audio.getnchannels(), audio.getsampwidth(), audio.getframerate()), (1,2,13300))
                    self.assertGreater(audio.getnframes(), 1000)
                    samples = struct.unpack('<'+'h'*audio.getnframes(), audio.readframes(audio.getnframes()))
                    self.assertGreater(max(samples), 1000)
                    self.assertLess(min(samples), -1000)
            else:
                self.assertEqual(data[:4], b'Vgm ')
                self.assertEqual(struct.unpack_from('<I', data, 4)[0], len(data)-4)
                cursor, frames, fm, psg = 256, 0, 0, 0
                while data[cursor] != 0x66:
                    command = data[cursor]
                    if command == 0x52: fm += 1; cursor += 3
                    elif command == 0x50: psg += 1; cursor += 2
                    elif command == 0x62: frames += 1; cursor += 1
                    else: self.fail(f'Unexpected VGM command {command}')
                self.assertEqual(frames*735, struct.unpack_from('<I', data, 0x18)[0])
                self.assertGreater(fm, 100)
                self.assertGreater(psg, 10)
                loop = struct.unpack_from('<I', data, 0x1C)[0]
                self.assertEqual(bool(loop), name == 'course.vgm')
                if loop: self.assertLess(loop+0x1C, len(data))
        with tempfile.TemporaryDirectory(prefix='platform audio ') as scratch:
            command = [sys.executable, str(LAB / 'tools/make_audio.py'), scratch]
            self.assertEqual(subprocess.run(command, capture_output=True).returncode, 0)
            before = {p.name:p.read_bytes() for p in Path(scratch).iterdir()}
            self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)
            self.assertEqual(before, {p.name:p.read_bytes() for p in Path(scratch).iterdir()})

    def test_every_tiled_cell_matches_collision_source_and_backing_is_padded(self):
        spec = importlib.util.spec_from_file_location('platform_layout', LAB / 'tools/make_assets.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        root = ET.parse(LAB / 'res/course.tmx').getroot()
        self.assertEqual((root.get('width'), root.get('height')), ('128', '32'))
        self.assertEqual((root.get('tilewidth'), root.get('tileheight')), ('16', '16'))
        gids = [int(v) for v in root.find('layer/data').text.strip().split(',')]
        rows, symbols = module.layout(), '.#-/\\*?'
        self.assertEqual(len(gids), 128*32)
        for y in range(32):
            for x in range(128):
                expected = symbols.index(rows[y][x])+1 if y<18 and x<80 else 1
                self.assertEqual(gids[y*128+x], expected)


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION') == '1', 'Docker integration is opt-in')
class PlatformContainerTests(unittest.TestCase):
    def test_standalone_release_debug_and_actual_m68000_movement(self):
        from test_math import execute_sdk
        (ROOT / 'build').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='platform container ', dir=ROOT / 'build') as scratch:
            project = Path(scratch) / 'standalone platform lab'
            shutil.copytree(LAB, project)
            base = ['docker', 'run', '--rm']
            if os.environ.get('SGDK_PLATFORM'): base += ['--platform', os.environ['SGDK_PLATFORM']]
            if hasattr(os, 'getuid'): base += ['--user', f'{os.getuid()}:{os.getgid()}']
            base += ['--mount', f'type=bind,source={project},target=/workspace', os.environ.get('SGDK_IMAGE', 'ewrogers/sgdk:latest')]
            def run(*args):
                result = subprocess.run(base+list(args), capture_output=True, text=True, timeout=240)
                self.assertEqual(result.returncode, 0, (result.stdout+result.stderr)[-6000:])
                return result.stdout+result.stderr
            hashes = {}
            for mode in ['release', 'debug']:
                run('make', 'clean')
                log = run('make', mode)
                self.assertNotIn('warning:', log)
                rom = (project / 'out/rom.bin').read_bytes()
                self.assertEqual(len(rom), 262144)
                self.assertEqual(rom[0x100:0x104], b'SEGA')
                self.assertEqual(struct.unpack_from('>I', rom, 0x1A4)[0], len(rom)-1)
                symbols = run('m68k-elf-nm', 'out/rom.out')
                for name in ['hero_image', 'enemy_image', 'item_image', 'course_map', 'course_map_tileset0']:
                    self.assertRegex(symbols, r'\b'+name+r'\b')
                hashes[mode] = hashlib.sha256(rom).hexdigest()
            shutil.copyfile(FIXTURE / 'target_probe.c', project / 'src/main.c')
            for mode in ['release', 'debug']:
                run('make', 'clean'); run('make', mode)
                code, values = execute_sdk((project / 'out/rom.bin').read_bytes(), run('m68k-elf-nm', 'out/rom.out'))
                self.assertEqual(code, 0, f'Target model failed at probe line {code}')
                self.assertEqual(values, [])
            output = ROOT / 'build/platformer-integration'
            output.mkdir(parents=True, exist_ok=True)
            (output / 'platformer-report.json').write_text(json.dumps({'passed': True, 'hashes': hashes,
                'target_model': 'executed release/debug on Unicorn 2.1.4 M68000'}, indent=2)+'\n')


if __name__ == '__main__': unittest.main()
