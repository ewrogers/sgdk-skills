"""Executable rules and native-asset regressions for the horizontal Carpet Lab."""
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
import wave

ROOT=Path(__file__).resolve().parents[1]
LAB=ROOT/'skills/sgdk-shmup/assets/carpet-lab'
FIXTURE=ROOT/'tests/fixtures/carpet'

def tool(name):
    # The example remains self-contained when copied out of this repository.
    sys.path.insert(0,str(LAB/'tools'))
    try:
        spec=importlib.util.spec_from_file_location('carpet_'+name,LAB/'tools'/(name+'.py'))
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
    finally: sys.path.pop(0)

class CarpetModelTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not compiler:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
            raise unittest.SkipTest('C17 compiler required')
        cls.scratch=tempfile.TemporaryDirectory(prefix='carpet host ');cls.addClassCleanup(cls.scratch.cleanup)
        project=Path(cls.scratch.name)/'standalone carpet lab';shutil.copytree(LAB,project)
        cls.binary=Path(cls.scratch.name)/('model.exe' if os.name=='nt' else 'model')
        command=[compiler,'-std=c17','-O2','-Wall','-Wextra','-Werror','-I',str(project/'inc')]
        command += [str(project/'src'/n) for n in ['game.c','weapons.c','combat.c','stage.c']]
        result=subprocess.run(command+[str(FIXTURE/'model_test.c'),str(FIXTURE/'collision_reference.c'),'-o',str(cls.binary)],capture_output=True,text=True,timeout=60)
        if result.returncode: raise RuntimeError(result.stdout+result.stderr)
    def check(self,name):
        result=subprocess.run([str(self.binary),name],capture_output=True,text=True,timeout=30)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr);self.assertEqual(result.stdout.strip(),name)
    def test_title_loadout_three_stages_bosses_clear_and_credits(self):self.check('flow')
    def test_gem_meter_upgrade_caps_barriers_and_life_loss(self):self.check('meter')
    def test_loadout_patterns_returning_piercing_seeking_and_pool_failure(self):self.check('weapons')
    def test_pal_ntsc_edges_pause_movement_bounds_focus_and_familiar_history(self):self.check('timing')
    def test_twelve_thousand_full_state_collision_oracle_comparisons(self):self.check('equivalence')
    def test_hundred_thousand_step_soak(self):self.check('soak')
    def test_continuous_ribbon_damage_cadence_release_pause_twin_and_guard(self):self.check('ribbon')
    def test_gem_drops_are_sparse_and_independent_of_reused_enemy_slots(self):self.check('drops')

class CarpetHudTests(unittest.TestCase):
    def test_fixed_bottom_rail_cached_uploads_boss_fill_and_menu_priority(self):
        compiler=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not compiler: self.skipTest('C17 compiler required')
        with tempfile.TemporaryDirectory(prefix='carpet hud ') as scratch:
            binary=Path(scratch)/('hud.exe' if os.name=='nt' else 'hud')
            result=subprocess.run([compiler,'-std=c17','-Wall','-Wextra','-Werror',
                '-I',str(FIXTURE/'ui'),'-I',str(LAB/'inc'),str(LAB/'src/ui.c'),
                *[str(LAB/'src'/n) for n in ['game.c','weapons.c','combat.c','stage.c']],
                str(FIXTURE/'ui/hud_test.c'),'-o',str(binary)],capture_output=True,text=True,timeout=30)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            result=subprocess.run([str(binary)],capture_output=True,text=True,timeout=10)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            self.assertEqual(result.stdout.strip(),'hud')

class CarpetBannerTests(unittest.TestCase):
    def test_long_stage_title_groups_glyphs_and_caches_uploads(self):
        compiler=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not compiler: self.skipTest('C17 compiler required')
        with tempfile.TemporaryDirectory(prefix='carpet banner ') as scratch:
            binary=Path(scratch)/('banner.exe' if os.name=='nt' else 'banner')
            result=subprocess.run([compiler,'-std=c17','-Wall','-Wextra','-Werror',
                '-I',str(FIXTURE/'sprites'),'-I',str(LAB/'inc'),str(LAB/'src/sprites.c'),
                str(FIXTURE/'sprites/banner_test.c'),'-o',str(binary)],capture_output=True,text=True,timeout=30)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            result=subprocess.run([str(binary)],capture_output=True,text=True,timeout=10)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            self.assertEqual(result.stdout.strip(),'banner')

class CarpetSoundTests(unittest.TestCase):
    def test_ribbon_grain_restart_release_pause_resume_and_sfx_priority(self):
        compiler=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not compiler: self.skipTest('C17 compiler required')
        with tempfile.TemporaryDirectory(prefix='carpet sound ') as scratch:
            binary=Path(scratch)/('audio.exe' if os.name=='nt' else 'audio')
            result=subprocess.run([compiler,'-std=c17','-Wall','-Wextra','-Werror',
                '-I',str(FIXTURE/'sound'),'-I',str(LAB/'inc'),str(LAB/'src/sound.c'),
                str(FIXTURE/'sound/audio_test.c'),'-o',str(binary)],capture_output=True,text=True,timeout=30)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            result=subprocess.run([str(binary)],capture_output=True,text=True,timeout=10)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            self.assertEqual(result.stdout.strip(),'audio')

class CarpetVramTests(unittest.TestCase):
    def test_linked_resource_layout_catches_later_stage_and_banner_overflows(self):
        checker=tool('check_vram');rom=bytearray(65536);symbols={};cursor=16
        def reserve(size):
            nonlocal cursor
            at=cursor;cursor+=size;return at
        def pointer(at,value):struct.pack_into('>I',rom,at,value)
        def word(at,value):struct.pack_into('>H',rom,at,value)
        def tiles(count):
            at=reserve(8);word(at+2,count);return at
        def image(name,count):
            at=reserve(12);pointer(at+4,tiles(count));symbols[name]=at
        def sprite(name,count):
            at=reserve(18);symbols[name+'_image']=at;word(at+8,1)
            animations=reserve(4);pointer(at+10,animations)
            animation=reserve(6);pointer(animations,animation);rom[animation]=1
            frames=reserve(4);pointer(animation+2,frames)
            frame=reserve(10);pointer(frames,frame);pointer(frame+2,tiles(count))
        for name,count in [('gradient_font',96),('hud_tiles',64),('health_tiles',11),('label_font',80),('title_font',40)]:symbols[name]=tiles(count)
        for stage,far,near in [(1,329,264),(2,433,256),(3,359,254)]:
            image('far_'+str(stage),far);image('near_'+str(stage),near)
        image('title_image',500)
        for name,count in [('hero',80),('teapot',9),('kite',9),('beetle',9),('sun',4),('moon',4),('storm',4),('ribbon',8),('button',8),('effects',60),('scarab',320),('serpent',320),('lantern',320)]:sprite(name,count)
        limit,rows=checker.analyze(rom,symbols,560,24)
        self.assertEqual(limit,880);self.assertEqual(rows[1],('Stage 2',862,555))
        with self.assertRaisesRegex(ValueError,'Stage 2: scenery'):
            checker.analyze(rom,symbols,640,24)
        with self.assertRaisesRegex(ValueError,'actors and banners'):
            checker.analyze(rom,symbols,560,32)
        image('title_image',1000)
        with self.assertRaisesRegex(ValueError,'Title: scenery'):
            checker.analyze(rom,symbols,560,24)

class CarpetAssetTests(unittest.TestCase):
    def test_native_dimensions_four_palette_plan_and_animation_cells(self):
        art=tool('import_art');pal=tool('art_palette')
        plans={'hero':(160,32,pal.ACTORS),'boss_scarab':(320,64,pal.ACTORS),
               'boss_serpent':(320,64,pal.ACTORS),'boss_lantern':(320,64,pal.ACTORS),
               'enemy_teapot':(24,24,pal.ACTORS),'enemy_kite':(24,24,pal.ACTORS),'enemy_beetle':(24,24,pal.ACTORS),
               'spell_sun':(16,16,pal.FX),'spell_moon':(16,16,pal.FX),'spell_storm':(16,16,pal.FX),'title':(256,144,pal.FAR[0])}
        for i in range(3):plans['far'+str(i+1)]=(256,192,pal.FAR[i]);plans['near'+str(i+1)]=(256,96,pal.NEAR[i])
        for name,(width,height,colors) in plans.items():
            path=LAB/'res'/(name+'.png');w,h,pixels=art.read_png(path)
            self.assertEqual((w,h),(width,height));self.assertEqual(path.read_bytes()[25],3)
            self.assertTrue(set(p[:3] for p in pixels)<=set(tuple(bytes.fromhex(c)) for c in colors))
            self.assertLessEqual(len(set(p[:3] for p in pixels)),16)
            self.assertTrue(all(v in {0,36,73,109,146,182,219,255} for c in colors for v in bytes.fromhex(c)))
    def test_gradient_font_has_opaque_panel_background_and_exact_reproduction(self):
        art=tool('import_art');ui=tool('make_ui')
        for name,data in ui.assets().items():self.assertEqual(data,(LAB/'res'/name).read_bytes())
        w,h,pixels=art.read_png(LAB/'res/font.png');self.assertEqual((w,h),(768,8))
        # Blank font cells must cover the panel, not reveal BG_B title art.
        panel=tuple(bytes.fromhex(tool('art_palette').FX[1]))
        self.assertTrue(all(pixels[y*w+x][:3]==panel for y in range(8) for x in range(8)))
        self.assertNotIn((0,0,0),set(p[:3] for p in pixels))
        self.assertGreater(len(set(p[:3] for p in pixels)),3)
        w,h,pixels=art.read_png(LAB/'res/hud.png')
        self.assertEqual((w,h),(256,16))
        self.assertTrue(all(pixels[y*w][:3]==panel for y in (0,15)))
        w,h,pixels=art.read_png(LAB/'res/health.png')
        self.assertEqual((w,h),(88,8))
        w,h,pixels=art.read_png(LAB/'res/label_font.png')
        self.assertEqual(h,32)
        self.assertTrue(all(pixels[y*w+x][:3]==panel for y in (0,1,2,3,12,13,14,15) for x in range(w)))
        selected=tuple(bytes.fromhex(tool('art_palette').FX[2]))
        self.assertTrue(all(pixels[y*w+x][:3]==selected for y in (16,17,18,19,28,29,30,31) for x in range(w)))
        w,h,pixels=art.read_png(LAB/'res/effects.png')
        # Every gem spin frame is one connected shape, with no detached sparkle.
        for frame in range(4,8):
            body={(x,y) for y in range(16) for x in range(16) if pixels[y*w+frame*16+x][:3]!=(0,0,0)}
            remaining=set(body);pending=[remaining.pop()]
            while pending:
                x,y=pending.pop()
                for neighbor in [(x-1,y),(x+1,y),(x,y-1),(x,y+1)]:
                    if neighbor in remaining:remaining.remove(neighbor);pending.append(neighbor)
            self.assertFalse(remaining)
            self.assertTrue(all(0<x<15 and 0<y<15 for x,y in body))
        outline=tuple(bytes.fromhex(tool('art_palette').FX[12]))
        black=tuple(bytes.fromhex(tool('art_palette').FX[0]))
        for frame in range(4,8):
            for y in range(1,14):
                occupied=[x for x in range(2,13) if pixels[y*w+frame*16+x][:3]!=black]
                # Ignore the detached sparkle above/right of the main gem.
                body=[x for x in occupied if abs(x-7)<=5 and not (y<=2 and x>=12)]
                if body:
                    self.assertEqual(pixels[y*w+frame*16+min(body)][:3],outline)
                    self.assertEqual(pixels[y*w+frame*16+max(body)][:3],outline)
    def test_importer_transparency_palette_order_and_bad_checksum(self):
        art=tool('import_art');pal=tool('art_palette').FX
        with tempfile.TemporaryDirectory(prefix='carpet art ') as scratch:
            source=Path(scratch)/'source.png';target=Path(scratch)/'indexed.png'
            source.write_bytes(art.indexed_png(2,1,[0,15],pal));self.assertEqual(art.normalize(source,target,pal,True),(2,1,2))
            # An indexed file without tRNS is opaque; index0 black is remapped
            # to a nontransparent palette entry rather than punched out.
            _,_,pixels=art.read_png(target);self.assertNotEqual(pixels[0][:3],(0,0,0))
            data=bytearray(source.read_bytes());data[20]^=1;source.write_bytes(data)
            with self.assertRaisesRegex(ValueError,'checksum'):art.read_png(source)
    def test_original_music_effect_formats_loops_and_reproduction(self):
        audio=tool('make_audio')
        for name,data in audio.assets().items():
            self.assertEqual(data,(LAB/'res'/name).read_bytes())
            if name.endswith('.vgm'):
                self.assertEqual(data[:4],b'Vgm ');self.assertEqual(struct.unpack_from('<I',data,4)[0],len(data)-4)
                self.assertEqual(bool(struct.unpack_from('<I',data,0x1C)[0]),name!='clear.vgm')
                cursor=256;frames=0
                while data[cursor]!=0x66:
                    command=data[cursor];self.assertIn(command,(0x52,0x53,0x50,0x62))
                    if command==0x62:frames+=1;cursor+=1
                    else:cursor+=3 if command in (0x52,0x53) else 2
                self.assertEqual(frames*735,struct.unpack_from('<I',data,0x18)[0])
            else:
                with wave.open(str(LAB/'res'/name)) as wav:
                    self.assertEqual((wav.getnchannels(),wav.getsampwidth(),wav.getframerate()),(1,2,13300))
                    self.assertGreater(wav.getnframes(),500)
        self.assertNotEqual(audio.music('title'),audio.music('loadout'))
        self.assertEqual(len(set(audio.music('boss'+str(i)) for i in (1,2,3))),3)

class CarpetHeaderTests(unittest.TestCase):
    def test_final_extent_and_invalid_input_preserves_file(self):
        finalize=tool('finalize_rom').finalize
        with tempfile.TemporaryDirectory() as scratch:
            path=Path(scratch)/'rom.bin';data=bytearray(393216)
            data[0x100:0x104]=b'SEGA';data[0x18E:0x190]=b'\x12\x34'
            path.write_bytes(data);finalize(path)
            expected=bytearray(data);struct.pack_into('>I',expected,0x1A4,len(data)-1)
            self.assertEqual(path.read_bytes(),expected)
            finalize(path);self.assertEqual(path.read_bytes(),expected)
            for invalid in [b'',b'SEGA',bytes(131072),bytes(data[:-1])]:
                path.write_bytes(invalid)
                with self.assertRaises(ValueError):finalize(path)
                self.assertEqual(path.read_bytes(),invalid)

@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class CarpetContainerTests(unittest.TestCase):
    def test_standalone_release_debug_vram_and_executed_m68000_contracts(self):
        from test_math import execute_sdk
        (ROOT/'build').mkdir(exist_ok=True)
        output=ROOT/'build/carpet-integration';output.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='carpet container ',dir=ROOT/'build') as scratch:
            project=Path(scratch)/'standalone carpet lab';shutil.copytree(LAB,project)
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                result=subprocess.run(base+list(args),capture_output=True,text=True,timeout=300)
                self.assertEqual(result.returncode,0,(result.stdout+result.stderr)[-6000:])
                return result.stdout+result.stderr
            hashes={};budgets={}
            checker=tool('check_vram')
            for mode in ['release','debug']:
                run('make','clean');log=run('make',mode);(output/(mode+'.log')).write_text(log)
                self.assertNotIn('warning:',log)
                rom=(project/'out/rom.bin').read_bytes();self.assertEqual(rom[0x100:0x104],b'SEGA')
                self.assertEqual(len(rom)%131072,0);self.assertLessEqual(len(rom),1048576)
                self.assertEqual(struct.unpack_from('>I',rom,0x1A4)[0],len(rom)-1)
                symbols=run('m68k-elf-nm','out/rom.out');entries={}
                for line in symbols.splitlines():
                    fields=line.split()
                    if len(fields)==3:
                        try:entries[fields[2]]=int(fields[0],16)
                        except ValueError:pass
                limit,rows=checker.analyze(rom,entries,560,24)
                budgets[mode]={'scenery_limit':limit,'scenes':rows,'rom_bytes':len(rom)}
                hashes[mode]=hashlib.sha256(rom).hexdigest()
            shutil.copyfile(FIXTURE/'target_probe.c',project/'src/main.c')
            for mode in ['release','debug']:
                run('make','clean');log=run('make',mode);(output/('probe-'+mode+'.log')).write_text(log)
                self.assertNotIn('warning:',log)
                code,values=execute_sdk((project/'out/rom.bin').read_bytes(),run('m68k-elf-nm','out/rom.out'))
                self.assertEqual(code,0,f'68000 Carpet contract failed at line {code}')
                self.assertEqual(values,[v for i in range(16) for v in (round(math.sin(i*math.pi/8)*256),round(-math.cos(i*math.pi/8)*256))])
            (output/'carpet-report.json').write_text(json.dumps({'passed':True,'hashes':hashes,'budgets':budgets,
                'target_model':'executed release/debug on Unicorn 2.1.4 M68000'},indent=2)+'\n')


if __name__=='__main__':unittest.main()
