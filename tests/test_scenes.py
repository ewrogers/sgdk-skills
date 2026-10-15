"""Exercise ordinary C scene flow, real cleanup adapters and standalone ROM builds."""
import importlib.util
import io
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
import wave
import zlib

ROOT=Path(__file__).resolve().parents[1]
LAB=ROOT/'skills/sgdk-scenes/assets/transition-lab'
FIXTURE=ROOT/'tests/fixtures/scenes'


class SceneBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cc=next((p for name in ['clang','cc','gcc'] if (p:=shutil.which(name))),None)
        if cc is None:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1': raise RuntimeError('Host C17 compiler required')
            raise unittest.SkipTest('Host C17 compiler needed')
        (ROOT/'build').mkdir(exist_ok=True)
        cls.folder=tempfile.TemporaryDirectory(prefix='scene host ',dir=ROOT/'build')
        cls.addClassCleanup(cls.folder.cleanup)
        cls.binary=Path(cls.folder.name)/('scenes.exe' if os.name=='nt' else 'scenes')
        sources=[LAB/'src'/name for name in ['lab.c','level.c','path.c','arena.c','scene_switch.c','checkpoint.c','storage.c','scene_video.c','view.c','text.c']]
        sources+=sorted(FIXTURE.glob('*.c'))
        r=subprocess.run([cc,'-std=c17','-Wall','-Wextra','-Werror','-I',str(FIXTURE),'-I',str(LAB/'inc'),
                          *map(str,sources),'-o',str(cls.binary)],capture_output=True,text=True,timeout=60)
        if r.returncode: raise RuntimeError(r.stdout+r.stderr)

    def check(self,name):
        r=subprocess.run([str(self.binary),name],capture_output=True,text=True,timeout=15)
        self.assertEqual(r.returncode,0,r.stdout+r.stderr)
        self.assertEqual(r.stdout.strip(),name)

    def test_title_continue_options_and_credits_routes(self): self.check('routes')
    def test_first_join_either_player_simultaneous_join_and_ten_seconds_both_regions(self): self.check('join')
    def test_countdown_skips_snap_to_whole_seconds_and_hold_does_not_retrigger(self): self.check('skip')
    def test_directions_pause_coop_coin_ownership_and_room_clear(self): self.check('play')
    def test_boss_scatter_bounce_pause_ownership_smoke_mega_coin_and_credits(self): self.check('boss')
    def test_boss_routes_around_multiple_solid_blocks_without_crossing_them(self): self.check('path')
    def test_distinct_room_layouts_have_reachable_coins_and_solid_wall_edges(self): self.check('levels')
    def test_top_down_radial_scatter_has_no_downward_bias_and_settles_near_hit(self): self.check('scatter')
    def test_boss_laser_windup_eight_directions_pool_hits_pause_and_retirement(self): self.check('lasers')
    def test_level_start_checkpoints_survive_completion_and_boss_victory(self): self.check('start-checkpoints')
    def test_transition_order_busy_requests_partial_entry_and_safe_fallback(self): self.check('switch')
    def test_bounded_text_and_number_formatting(self): self.check('text')
    def test_checkpoint_format_single_bit_corruption_and_validate_before_commit(self): self.check('checkpoint')
    def test_every_interrupted_write_keeps_prior_slot_and_ambiguous_sequence_rejected(self): self.check('recovery')
    def test_final_fade_dma_drains_before_sprite_cleanup_and_loading(self): self.check('video')
    def test_sram_adapter_closes_on_missing_invalid_and_successful_saves(self): self.check('storage')
    def test_text_countdown_colors_banner_pause_and_directional_sprite_rows(self): self.check('view')
    def test_repeated_partial_and_complete_scene_cleanup_returns_to_baseline(self): self.check('rollback')




class SceneAssetTests(unittest.TestCase):
    @staticmethod
    def assets_module():
        spec=importlib.util.spec_from_file_location('scene_art',LAB/'tools/make_assets.py')
        module=importlib.util.module_from_spec(spec); spec.loader.exec_module(module); return module

    def test_generated_art_and_audio_reproduce_without_overwriting(self):
        module=self.assets_module(); generated=module.assets()
        for name,data in generated.items():
            stored=(LAB/'res'/name).read_bytes()
            if name.endswith('.wav'):
                # libm may round a synthesized sample one integer apart across hosts.
                with wave.open(io.BytesIO(data)) as a,wave.open(io.BytesIO(stored)) as b:
                    self.assertEqual(a.getparams(),b.getparams())
                    sa=struct.unpack('<'+'h'*a.getnframes(),a.readframes(a.getnframes()))
                    sb=struct.unpack('<'+'h'*b.getnframes(),b.readframes(b.getnframes()))
                    self.assertLessEqual(max(abs(x-y) for x,y in zip(sa,sb)),1)
            else: self.assertEqual(data,stored,name)
        (ROOT/'build').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='scene art ',dir=ROOT/'build') as folder:
            output=Path(folder)/'new assets'
            cmd=[sys.executable,str(LAB/'tools/make_assets.py'),str(output)]
            self.assertEqual(subprocess.run(cmd,capture_output=True).returncode,0)
            self.assertNotEqual(subprocess.run(cmd,capture_output=True).returncode,0)
            self.assertEqual(generated,{p.name:p.read_bytes() for p in output.iterdir()})

    def test_image_dimensions_shared_effect_palette_and_index_budgets(self):
        expected={'hero.png':(128,48),'coins.png':(32,8),'boss.png':(128,96),
                  'smoke.png':(128,32),'mega.png':(128,32),'laser.png':(48,16),'title.png':(160,56)}
        palettes={}
        for name,size in expected.items():
            data=(LAB/'res'/name).read_bytes(); pos=8; compressed=bytearray(); palette=None
            while pos<len(data):
                count=struct.unpack_from('>I',data,pos)[0]; kind=data[pos+4:pos+8]; chunk=data[pos+8:pos+8+count]
                self.assertEqual(zlib.crc32(kind+chunk),struct.unpack_from('>I',data,pos+8+count)[0])
                if kind==b'IHDR': self.assertEqual(struct.unpack('>IIBBBBB',chunk),(*size,4 if name=='hero.png' else 8,3,0,0,0))
                elif kind==b'PLTE': palette=chunk; self.assertLessEqual(len(chunk),48)
                elif kind==b'IDAT': compressed.extend(chunk)
                pos+=count+12
            self.assertIsNotNone(palette); palettes[name]=palette
            # Generated/indexed sheets use no filtering; hero comes from an existing sheet.
            if name!='hero.png':
                pixels=zlib.decompress(compressed); width,height=size
                self.assertEqual(len(pixels),(width+1)*height)
                for y in range(height):
                    self.assertEqual(pixels[y*(width+1)],0)
                    self.assertLess(max(pixels[y*(width+1)+1:(y+1)*(width+1)]),16)
        for name in ['boss.png','smoke.png','mega.png','laser.png']: self.assertEqual(palettes[name],palettes['coins.png'])

    def test_each_scene_music_has_distinct_phrase_timing_and_boss_percussion(self):
        module=self.assets_module(); names=['menu','room_one','room_two','pause','room_three','boss','victory']
        phrases=[]
        for kind,name in enumerate(names):
            data=module.music(kind); phrases.append(data)
            self.assertEqual(data,(LAB/'res'/f'{name}.vgm').read_bytes())
            self.assertEqual(struct.unpack_from('<I',data,4)[0]+4,len(data))
            pos=256; frames=0; notes=[]; psg=[]
            while data[pos]!=0x66:
                command=data[pos]
                if command==0x52:
                    register,value=data[pos+1:pos+3]
                    if register==0x28 and value&0xF0: notes.append((frames,value&7))
                    pos+=3
                elif command==0x50: psg.append((frames,data[pos+1])); pos+=2
                elif command==0x62: frames+=1; pos+=1
                else: self.fail(f'Unexpected VGM command {command:#x}')
            self.assertEqual(pos,len(data)-1)
            self.assertEqual(struct.unpack_from('<I',data,0x18)[0],frames*735)
            self.assertEqual(struct.unpack_from('<I',data,0x1C)[0]==0,name=='victory')
            self.assertTrue(notes)
            if name=='pause': self.assertEqual(sorted(set(t for t,ch in notes if ch==0))[:4],[0,18,30,48])
            if name=='boss':
                self.assertIn((0,2),notes); self.assertIn((0,0xE4),psg); self.assertIn((3,0xFF),psg)
        self.assertEqual(len(set(phrases)),len(names))


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class SceneContainerTests(unittest.TestCase):
    def test_standalone_release_debug_real_api_sprite_and_audio_resources(self):
        import hashlib
        (ROOT/'build').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='scenes container ',dir=ROOT/'build') as folder:
            project=Path(folder)/'standalone transition lab'; shutil.copytree(LAB,project)
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'): base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'): base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                r=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                self.assertEqual(r.returncode,0,(r.stdout+r.stderr)[-6000:]); return r.stdout+r.stderr
            # Check every fake adapter declaration against the installed SDK.
            declarations=re.findall(r'^(?:void|bool|u8|u16|Sprite \*) [^\n]+\);$',(FIXTURE/'genesis.h').read_text(),re.M)
            (project/'api_probe.c').write_text('#include <genesis.h>\n'+'\n'.join(declarations))
            run('m68k-elf-gcc','-m68000','-std=gnu17','-fno-builtin','-I/opt/sgdk/inc','-I/opt/sgdk/res',
                '-Werror=incompatible-pointer-types','-fsyntax-only','api_probe.c')
            hashes={}
            for mode in ['release','debug']:
                run('build','clean'); log=run('build',mode)
                self.assertNotIn('warning:',re.sub(r'\x1b\[[0-9;]*m','',log))
                rom=(project/'out/rom.bin').read_bytes(); self.assertEqual(len(rom),262144)
                self.assertEqual(rom[0x100:0x104],b'SEGA')
                self.assertEqual(struct.unpack_from('>I',rom,0x1A4)[0],len(rom)-1)
                self.assertEqual(rom[0x1B0:0x1B4],b'RA\xF8\x20')
                self.assertEqual(struct.unpack_from('>II',rom,0x1B4),(0x200001,0x20FFFF))
                symbols=run('m68k-elf-nm','-n','out/rom.out')
                for name in ['hero_image','coin_image','boss_image','smoke_image','mega_image','laser_image','title_art',
                             'menu_music','room_one_music','room_two_music','room_three_music','boss_music','pause_music','victory_music',
                             'join_sound','coin_sound','clear_sound','hit_sound','poof_sound','laser_sound']:
                    self.assertRegex(symbols,r'\b'+name+r'\b')
                hashes[mode]=hashlib.sha256(rom).hexdigest()
            out=ROOT/'build/scenes-integration'; out.mkdir(parents=True,exist_ok=True)
            (out/'scenes-report.json').write_text(json.dumps({'passed':True,'hashes':hashes},indent=2)+'\n')


if __name__=='__main__': unittest.main()
