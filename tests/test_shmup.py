"""Run Shooter Lab's real model, asset checks and target-code probes."""
import hashlib
import importlib.util
import io
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
import wave
import zlib

ROOT = Path(__file__).resolve().parents[1]
LAB = ROOT / 'skills/sgdk-shmup/assets/shooter-lab'
FIXTURE = ROOT / 'tests/fixtures/shmup'


def load_generator(name):
    spec = importlib.util.spec_from_file_location(name, LAB / 'tools' / (name+'.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class ShooterBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler = next((p for name in ['clang','cc','gcc'] if (p := shutil.which(name))), None)
        if not cls.compiler:
            if os.environ.get('SGDK_REQUIRE_HOST_CC') == '1': raise RuntimeError('C17 compiler required')
            raise unittest.SkipTest('C17 compiler required')
        cls.scratch = tempfile.TemporaryDirectory(prefix='sgdk shooter host ')
        cls.addClassCleanup(cls.scratch.cleanup)
        project = Path(cls.scratch.name) / 'standalone shooter lab'
        shutil.copytree(LAB, project)
        cls.binary = Path(cls.scratch.name) / ('model.exe' if os.name=='nt' else 'model')
        command = [cls.compiler,'-std=c17','-O2','-Wall','-Wextra','-Werror','-I',str(project/'inc')]
        command += [str(project/'src'/name) for name in ['game.c','patterns.c','combat.c','weapons.c','stage.c','video_layout.c']]
        result = subprocess.run(command+[str(FIXTURE/'collision_reference.c'),str(FIXTURE/'model_test.c'),'-o',str(cls.binary)], capture_output=True,text=True,timeout=60)
        if result.returncode: raise RuntimeError(result.stdout+result.stderr)

    def check(self,name):
        result = subprocess.run([str(self.binary),name],capture_output=True,text=True,timeout=30)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        self.assertEqual(result.stdout.strip(),name)

    def test_h40_window_alignment_plane_ranges_and_overlap(self): self.check('layout')
    def test_pool_exhaustion_retirement_and_reuse(self): self.check('pools')
    def test_aim_ring_symmetry_fan_and_atomic_volley(self): self.check('patterns')
    def test_broad_phase_matches_original_substeps(self): self.check('equivalence')
    def test_focus_diagonals_and_bounds_exclude_hud(self): self.check('movement')
    def test_substep_hits_single_consumption_immunity_and_score_saturation(self): self.check('combat')
    def test_power_caps_surplus_score_bomb_edges_and_damage_downgrade(self): self.check('pickups')
    def test_authored_waves_boss_entry_clear_and_restart(self): self.check('waves')
    def test_boss_entry_damage_phase_change_and_single_defeat_award(self): self.check('boss')
    def test_shield_expiry_single_absorption_and_delayed_radial_bomb(self): self.check('weapons')
    def test_stage_flow_twin_orbits_upgrades_death_and_boss_guard_windows(self): self.check('stage2')
    def test_nominal_pal_ntsc_cadence_and_frozen_pause(self): self.check('clock')
    def test_authored_attack_scanline_scenarios_leave_headroom(self): self.check('pressure')
    def test_hundred_thousand_mixed_input_steps_stay_bounded(self): self.check('soak')

    def test_sound_priorities_pause_and_single_fanfare(self):
        binary = Path(self.scratch.name)/('sound.exe' if os.name=='nt' else 'sound')
        result = subprocess.run([self.compiler,'-std=c17','-Wall','-Wextra','-Werror','-I',str(FIXTURE),
            '-I',str(LAB/'inc'),str(LAB/'src/sound.c'),str(FIXTURE/'sound_test.c'),'-o',str(binary)],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        result = subprocess.run([str(binary)],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)


class ShooterAssetsTests(unittest.TestCase):
    def test_native_indexed_art_sizes_palettes_checksums_and_reproduction(self):
        sizes={'ship.png':(64,16),'enemies.png':(96,16),'boss.png':(224,32),
               'bullets.png':(40,8),'pickups.png':(64,16),'effects.png':(64,16),'utility.png':(128,32),'twins.png':(32,16),'font.png':(768,8),'stars.png':(256,256)}
        for name,data in load_generator('make_assets').assets().items():
            self.assertEqual(data,(LAB/'res'/name).read_bytes())
            offset,compressed=8,bytearray()
            while offset<len(data):
                size=struct.unpack_from('>I',data,offset)[0]
                kind,payload=data[offset+4:offset+8],data[offset+8:offset+8+size]
                self.assertEqual(zlib.crc32(kind+payload),struct.unpack_from('>I',data,offset+8+size)[0])
                if kind==b'IHDR':
                    width,height,depth,mode,_,_,_=struct.unpack('>IIBBBBB',payload)
                    self.assertEqual((width,height),sizes[name]);self.assertEqual((depth,mode),(8,3))
                elif kind==b'PLTE':
                    self.assertEqual(len(payload),48)
                    self.assertTrue(all(c in {0,36,73,109,146,182,219,255} for c in payload))
                elif kind==b'IDAT': compressed.extend(payload)
                offset+=size+12
            rows=zlib.decompress(compressed)
            self.assertEqual(len(rows),(width+1)*height)
            for y in range(height):
                self.assertEqual(rows[y*(width+1)],0)
                self.assertLess(max(rows[y*(width+1)+1:(y+1)*(width+1)]),16)

    def test_title_art_is_native_indexed_and_shares_ship_palette(self):
        data=(LAB/'res/title.png').read_bytes()
        cursor=8; chunks={}
        while cursor<len(data):
            size=struct.unpack_from('>I',data,cursor)[0]
            kind,payload=data[cursor+4:cursor+8],data[cursor+8:cursor+8+size]
            self.assertEqual(zlib.crc32(kind+payload),struct.unpack_from('>I',data,cursor+8+size)[0])
            chunks.setdefault(kind,bytearray()).extend(payload);cursor+=size+12
        self.assertEqual(struct.unpack('>IIBBBBB',chunks[b'IHDR']),(224,112,8,3,0,0,0))
        expected=bytes(c for rgb in load_generator('make_assets').PALETTES[1] for c in rgb)
        self.assertEqual(chunks[b'PLTE'],expected)
        rows=zlib.decompress(chunks[b'IDAT']); self.assertEqual(len(rows),225*112)
        for row in range(112):
            self.assertEqual(rows[row*225],0)
            self.assertLess(max(rows[row*225+1:(row+1)*225]),16)
        self.assertGreater(len(set(rows)),1)

    def test_audio_reproduction_pcm_format_and_vgm_waits_and_loop(self):
        for name,data in load_generator('make_audio').assets().items():
            self.assertEqual(data,(LAB/'res'/name).read_bytes())
            if name.endswith('.wav'):
                with wave.open(io.BytesIO(data)) as wav:
                    self.assertEqual((wav.getnchannels(),wav.getsampwidth(),wav.getframerate()),(1,2,13300))
                    self.assertGreater(wav.getnframes(),500)
            else:
                self.assertEqual(data[:4],b'Vgm ')
                self.assertEqual(struct.unpack_from('<I',data,4)[0],len(data)-4)
                cursor,frames,fm,psg=256,0,0,0
                while data[cursor]!=0x66:
                    command=data[cursor]
                    if command==0x52: fm+=1;cursor+=3
                    elif command==0x50: psg+=1;cursor+=2
                    elif command==0x62: frames+=1;cursor+=1
                    else: self.fail(f'Unexpected VGM command {command}')
                self.assertEqual(frames*735,struct.unpack_from('<I',data,0x18)[0])
                self.assertGreater(fm,100); self.assertGreater(psg,10)
                self.assertEqual(bool(struct.unpack_from('<I',data,0x1C)[0]),name in ('title.vgm','course.vgm','stage2.vgm','game_over.vgm','boss.vgm'))

    def test_voice_clips_are_bounded_pcm_and_not_silent(self):
        for name in ['bomb','power','shield','boss','over','twin']:
            with wave.open(str(LAB/'res'/('voice_'+name+'.wav'))) as wav:
                self.assertEqual((wav.getnchannels(),wav.getsampwidth(),wav.getframerate()),(1,2,13300))
                self.assertGreater(wav.getnframes(),3000)
                self.assertLess(wav.getnframes(),13300*3)
                samples=struct.unpack('<'+'h'*wav.getnframes(),wav.readframes(wav.getnframes()))
                self.assertGreater(max(samples)-min(samples),10000)

    def test_generators_refuse_to_overwrite_source_assets(self):
        for name in ['make_assets','make_audio']:
            with tempfile.TemporaryDirectory(prefix='shooter art ') as scratch:
                command=[sys.executable,str(LAB/'tools'/(name+'.py')),scratch]
                self.assertEqual(subprocess.run(command,capture_output=True).returncode,0)
                before={p.name:p.read_bytes() for p in Path(scratch).iterdir()}
                self.assertNotEqual(subprocess.run(command,capture_output=True).returncode,0)
                self.assertEqual(before,{p.name:p.read_bytes() for p in Path(scratch).iterdir()})


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class ShooterContainerTests(unittest.TestCase):
    def test_standalone_release_debug_and_executed_m68000_model_and_resources(self):
        from test_math import execute_sdk
        (ROOT/'build').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='shooter container ',dir=ROOT/'build') as scratch:
            project=Path(scratch)/'standalone shooter lab';shutil.copytree(LAB,project)
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'): base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'): base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                result=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                self.assertEqual(result.returncode,0,(result.stdout+result.stderr)[-6000:])
                return result.stdout+result.stderr
            hashes={}
            for mode in ['release','debug']:
                run('make','clean');log=run('make',mode);self.assertNotIn('warning:',log)
                rom=(project/'out/rom.bin').read_bytes();self.assertEqual(len(rom),262144)
                self.assertEqual(rom[0x100:0x104],b'SEGA')
                self.assertEqual(struct.unpack_from('>I',rom,0x1A4)[0],len(rom)-1)
                symbols=run('m68k-elf-nm','out/rom.out')
                for name in ['ship_image','boss_image','enemy_image','bullet_image','stars_image','title_image','title_music','stage_music','boss_music']:
                    self.assertRegex(symbols,r'\b'+name+r'\b')
                hashes[mode]=hashlib.sha256(rom).hexdigest()
            shutil.copyfile(FIXTURE/'target_probe.c',project/'src/main.c')
            for mode in ['release','debug']:
                run('make','clean');run('make',mode)
                code,values=execute_sdk((project/'out/rom.bin').read_bytes(),run('m68k-elf-nm','out/rom.out'))
                self.assertEqual(code,0,f'68000 probe failed at line {code}')
                self.assertEqual(len(values),32)
                for angle in range(16):
                    expected_x=math.trunc(round(math.sin(angle*math.pi/8)*256)*32/256)
                    expected_y=math.trunc(round(-math.cos(angle*math.pi/8)*256)*32/256)
                    self.assertEqual(values[angle*2:angle*2+2],[expected_x,expected_y])
            from shmup_profile import profile
            instructions=profile(project,run)
            self.assertLess(instructions['game_projectiles'],instructions['reference_game_projectiles'])
            output=ROOT/'build/shmup-integration' ;output.mkdir(parents=True,exist_ok=True)
            (output/'shmup-report.json').write_text(json.dumps({'passed':True,'hashes':hashes,
                'collision_instruction_proxy':instructions,
                'target_model':'executed release/debug on Unicorn 2.1.4 M68000'},indent=2)+'\n')


if __name__=='__main__': unittest.main()
