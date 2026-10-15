"""Run Enemy Lab's actual C model and compile its standalone SGDK adapter."""
import importlib.util
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
import zlib

ROOT=Path(__file__).resolve().parents[1]
LAB=ROOT/'skills/sgdk-ai/assets/enemy-lab'
SOURCES=['game.c','world.c','navigation.c','enemy.c','senses.c','shots.c']


class EnemyBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cc=next((p for name in ['clang','cc','gcc'] if (p:=shutil.which(name))),None)
        if not cc:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1': raise RuntimeError('A C17 compiler is required')
            raise unittest.SkipTest('A C17 compiler is required')
        (ROOT/'build').mkdir(exist_ok=True)
        cls.folder=tempfile.TemporaryDirectory(prefix='enemy host ',dir=ROOT/'build')
        cls.addClassCleanup(cls.folder.cleanup)
        cls.binary=Path(cls.folder.name)/('enemy.exe' if os.name=='nt' else 'enemy')
        r=subprocess.run([cc,'-std=c17','-O2','-Wall','-Wextra','-Werror','-I',str(LAB/'inc'),
                          *[str(LAB/'src'/name) for name in SOURCES],
                          str(ROOT/'tests/fixtures/ai/model_test.c'),'-o',str(cls.binary)],
                         capture_output=True,text=True,timeout=60)
        if r.returncode: raise RuntimeError(r.stdout+r.stderr)

    def check(self,name):
        r=subprocess.run([str(self.binary),name],capture_output=True,text=True,timeout=30)
        self.assertEqual(r.returncode,0,r.stdout+r.stderr)
        self.assertEqual(r.stdout.strip(),name)

    def test_all_exits_reachable_level_progression_caught_retries_and_game_over(self): self.check('flow')
    def test_world_sliding_half_open_edges_and_symmetric_supercover_sight(self): self.check('world')
    def test_cones_occlusion_ranges_hysteresis_and_stable_player_selection(self): self.check('senses')
    def test_all_walkable_pairs_match_independent_shortest_path_oracle(self): self.check('paths')
    def test_fair_queue_node_budgets_and_stale_result_cancellation(self): self.check('scheduler')
    def test_attack_windup_dodge_recovery_invulnerability_and_lost_target(self): self.check('states')
    def test_eight_direction_projectiles_pool_limits_hits_walls_and_retirement(self): self.check('shots')
    def test_input_edges_join_pause_and_nominal_pal_ntsc_timing(self): self.check('clock')
    def test_off_grid_stun_recovery_collision_safe_long_run_and_reset(self): self.check('run')


class EnemyViewTests(unittest.TestCase):
    def test_graphical_hud_palette_roles_facing_alert_stun_warning_and_level_panels(self):
        cc=next((p for name in ['clang','cc','gcc'] if (p:=shutil.which(name))),None)
        if not cc:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1': self.fail('C17 compiler required')
            self.skipTest('C17 compiler required')
        (ROOT/'build').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='enemy view ',dir=ROOT/'build') as folder:
            binary=Path(folder)/('view.exe' if os.name=='nt' else 'view')
            fixture=ROOT/'tests/fixtures/ai'
            p=subprocess.run([cc,'-std=c17','-O2','-Wall','-Wextra','-Werror',
                              '-I',str(fixture),'-I',str(LAB/'inc'),
                              *[str(LAB/'src'/name) for name in [*SOURCES,'view.c','text.c']],
                              str(fixture/'view_test.c'),'-o',str(binary)],capture_output=True,text=True,timeout=60)
            self.assertEqual(p.returncode,0,p.stdout+p.stderr)
            p=subprocess.run([str(binary)],capture_output=True,text=True,timeout=15)
            self.assertEqual(p.returncode,0,p.stdout+p.stderr)
            self.assertEqual(p.stdout.strip(),'view')


class EnemyAssetTests(unittest.TestCase):
    def test_generated_indexed_sheets_reproduce_and_reject_overwrites(self):
        (ROOT/'build').mkdir(exist_ok=True)
        spec=importlib.util.spec_from_file_location('enemy_art',LAB/'tools/make_assets.py')
        module=importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
        dimensions={'marker.png':(8,8),'shot.png':(24,8),'pulse.png':(256,64),'status.png':(48,16),'tries.png':(32,16),'exit.png':(32,16)}
        for name,data in module.assets().items():
            self.assertEqual(data,(LAB/'res'/name).read_bytes())
            pos=8; payload=bytearray()
            while pos<len(data):
                count=struct.unpack_from('>I',data,pos)[0]; kind=data[pos+4:pos+8]; chunk=data[pos+8:pos+8+count]
                self.assertEqual(zlib.crc32(kind+chunk),struct.unpack_from('>I',data,pos+8+count)[0])
                if kind==b'IHDR': self.assertEqual(struct.unpack('>IIBBBBB',chunk),(*dimensions[name],8,3,0,0,0))
                elif kind==b'PLTE': self.assertEqual(len(chunk),48)
                elif kind==b'IDAT': payload.extend(chunk)
                pos+=count+12
            width,height=dimensions[name]; rows=zlib.decompress(payload)
            for y in range(height):
                self.assertEqual(rows[y*(width+1)],0)
                self.assertLess(max(rows[y*(width+1)+1:(y+1)*(width+1)]),16)
        with tempfile.TemporaryDirectory(prefix='enemy art ',dir=ROOT/'build') as folder:
            out=Path(folder)/'assets'
            cmd=[sys.executable,str(LAB/'tools/make_assets.py'),str(out)]
            self.assertEqual(subprocess.run(cmd,capture_output=True).returncode,0)
            self.assertNotEqual(subprocess.run(cmd,capture_output=True).returncode,0)


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class EnemyContainerTests(unittest.TestCase):
    def test_standalone_real_sdk_release_debug_symbols_and_resources(self):
        import hashlib
        (ROOT/'build').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='enemy container ',dir=ROOT/'build') as folder:
            project=Path(folder)/'standalone enemy lab'; shutil.copytree(LAB,project)
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'): base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'): base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                p=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                self.assertEqual(p.returncode,0,(p.stdout+p.stderr)[-5000:]); return p.stdout+p.stderr
            declarations=re.findall(r'^(?:void|bool|Sprite \*) [^\n]+\);$',(ROOT/'tests/fixtures/ai/genesis.h').read_text(),re.M)
            (project/'api_probe.c').write_text('#include <genesis.h>\n'+'\n'.join(declarations))
            run('m68k-elf-gcc','-m68000','-std=gnu17','-fno-builtin','-I/opt/sgdk/inc','-I/opt/sgdk/res',
                '-Werror=incompatible-pointer-types','-fsyntax-only','api_probe.c')
            hashes={}
            for mode in ['release','debug']:
                run('make','clean'); log=run('make',mode)
                self.assertIn('-fno-strict-aliasing',log)
                self.assertNotIn('warning:',re.sub(r'\x1b\[[0-9;]*m','',log))
                rom=(project/'out/rom.bin').read_bytes(); self.assertEqual(len(rom),131072)
                self.assertEqual(rom[0x100:0x104],b'SEGA')
                self.assertEqual(struct.unpack_from('>I',rom,0x1A4)[0],len(rom)-1)
                self.assertNotEqual(rom[0x1B0:0x1B2],b'RA')
                symbols=run('m68k-elf-nm','-n','out/rom.out')
                for name in ['hero_image','pulse_image','shot_image','marker_image','status_image','tries_image','exit_image','arena_music','hit_sound','stun_sound','alert_sound','fire_sound']:
                    self.assertRegex(symbols,r'\b'+name+r'\b')
                hashes[mode]=hashlib.sha256(rom).hexdigest()
            out=ROOT/'build/ai-integration'; out.mkdir(parents=True,exist_ok=True)
            (out/'ai-report.json').write_text(json.dumps({'passed':True,'hashes':hashes},indent=2)+'\n')


if __name__=='__main__': unittest.main()
