"""Combat resolution contracts and actual SGDK resources; no VDP/audio emulation."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import random
import shutil
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
LAB=ROOT/'skills/sgdk-combat/assets/combat-lab';FIXTURE=ROOT/'tests/fixtures/combat'
def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);sys.path.insert(0,str(path.parent))
    try:spec.loader.exec_module(m)
    finally:sys.path.pop(0)
    return m
def compile_host(cc,path,mode):
    args=[cc,'-DCC_HOST','-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'inc'),'-I',str(FIXTURE),str(LAB/'src/combat.c'),str(FIXTURE/'host.c'),'-o',str(path)]
    if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
    r=subprocess.run(args,capture_output=True,text=True,timeout=60)
    if r.returncode:raise RuntimeError(r.stdout+r.stderr)
class CombatTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cc=next((p for n in ['clang','cc','gcc'] if(p:=shutil.which(n))),None)
        if not cls.cc:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
            raise unittest.SkipTest('C17 compiler required')
        cls.scratch=tempfile.TemporaryDirectory(prefix='combat contracts ');cls.addClassCleanup(cls.scratch.cleanup);cls.binaries=[]
        for mode in ['-O0','-O3']:
            path=Path(cls.scratch.name)/(mode+('.exe' if os.name=='nt' else ''));compile_host(cls.cc,path,mode);cls.binaries.append(path)
    def test_damage_guard_trade_identity_clock_pool_and_20000_frames(self):
        traces=[]
        for path in self.binaries:
            r=subprocess.run([str(path)],capture_output=True,text=True,timeout=60);self.assertEqual(r.returncode,0,r.stderr);traces.append(r.stdout)
        self.assertEqual(traces[0],traces[1])
class CombatAssetsTests(unittest.TestCase):
    def test_native_role_palettes_sheets_and_standalone_copy(self):
        from test_localization import pixels
        art=load('combat_asset_art',LAB/'tools/combat_art.py');assets=art.assets();self.assertEqual(assets,art.assets())
        specs={'fighters':(480,160),'bolts':(32,16),'sparks':(64,32),'swords':(96,192),'court':(320,224),'title':(320,224)}
        for name,shape in specs.items():
            w,h,rows=pixels(assets[name+'.png']);self.assertEqual((w,h),shape);self.assertLess(max(v for row in rows for v in row),16)
        for pal in [art.WORLD,art.ACTOR,art.PROPS,art.UI]:self.assertEqual(len(pal),16);self.assertTrue(all(0<=v<=7 for rgb in pal for v in rgb))
        self.assertEqual(len(art.font_words()),768)
        prepare=load('combat_asset_prepare',LAB/'tools/prepare.py')
        with tempfile.TemporaryDirectory(prefix='combat copy ') as scratch:
            first=prepare.prepare(Path(scratch)/'first copy')
            with self.assertRaises(ValueError):prepare.prepare(first)
            r=subprocess.run([sys.executable,str(first/'tools/prepare.py'),str(Path(scratch)/'second copy')],capture_output=True,text=True,timeout=30);self.assertEqual(r.returncode,0,r.stderr)
            for name in ['res/court.png','res/fighters.png','inc/font.h','res/court.vgm','res/impact.wav']:
                self.assertEqual((first/name).read_bytes(),(Path(scratch)/'second copy'/name).read_bytes())
        with self.assertRaises(ValueError):prepare.prepare(LAB/'new output')
    def test_swing_damage_boxes_follow_native_leading_blade_in_all_directions(self):
        cc=next((p for n in ['clang','cc','gcc'] if(p:=shutil.which(n))),None)
        if not cc:self.skipTest('C17 compiler required')
        art=load('combat_swing_art',LAB/'tools/combat_art.py')
        with tempfile.TemporaryDirectory(prefix='combat swing ') as scratch:
            binary=Path(scratch)/('swing.exe' if os.name=='nt' else 'swing');compile_host(cc,binary,'-O3')
            centers=[]
            for face in range(4):
                for frame in range(4):
                    b=list(map(int,subprocess.check_output([str(binary),'swing',str(face),str(frame)],text=True).split()))
                    x,y,w,h,ox,oy=b;self.assertTrue(-10<=x<x+w<=34 and -10<=y<y+h<=34)
                    canvas=art.sword(face,frame);leading=[(i%24,i//24)for i,p in enumerate(canvas.p)if p==5]
                    self.assertTrue(leading)
                    tx,ty=[(8,1),(22,7),(22,17),(8,22)][frame]
                    expected=[(23-ty,tx),(tx,ty),(ty,23-tx),(23-tx,23-ty)][face]
                    self.assertEqual(canvas.p[expected[1]*24+expected[0]],5)
                    self.assertTrue(all(x<=px<x+w and y<=py<y+h for px,py in leading),(face,frame,b))
                    if frame==1:centers.append((ox+x+w/2,oy+y+h/2))
            # Down/right/up/left rotate in the intended screen directions.
            self.assertGreater(centers[0][1],centers[2][1])
            self.assertGreater(centers[1][0],centers[3][0])
@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class CombatContainerTests(unittest.TestCase):
    def test_roms_base_m68000_session_contracts_and_compiled_resources(self):
        from test_streaming import execute
        prepare=load('combat_container_prepare',LAB/'tools/prepare.py');art=load('combat_container_art',LAB/'tools/combat_art.py')
        out=ROOT/'build/combat-integration';out.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='combat target ',dir=ROOT/'build') as scratch:
            project=prepare.prepare(Path(scratch)/'standalone combat lab')
            cc=next((p for n in ['clang','cc','gcc'] if(p:=shutil.which(n))),None);self.assertIsNotNone(cc)
            host=Path(scratch)/'host';compile_host(cc,host,'-O3');trace=int(subprocess.check_output([str(host),'trace'],text=True))
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                r=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240);self.assertEqual(r.returncode,0,(r.stdout+r.stderr)[-5000:]);return r.stdout+r.stderr
            hashes={};geometry=None;tiles=None
            for kind in ['rom','pal','ntsc','probe']:
                header=(LAB/'src/boot/rom_head.c').read_text()
                if kind in ('pal','ntsc'):header=header.replace('.region="JUE"','.region="'+('E'if kind=='pal'else'U')+'"')
                (project/'src/boot/rom_head.c').write_text(header)
                if kind=='probe':shutil.copyfile(FIXTURE/'probe.c',project/'src/main.c');shutil.copyfile(FIXTURE/'checks.h',project/'inc/checks.h')
                for mode in (['release','debug']if kind in ('rom','probe')else['release']):
                    run('make','clean');log=run('make',mode);(out/f'{kind}-{mode}.log').write_text(log)
                    self.assertEqual([line for line in log.splitlines()if 'warning:'in line],[])
                    rom=(project/'out/rom.bin').read_bytes();self.assertEqual(len(rom),262144)
                    # The small probe shares metadata but is a CPU fixture, not a release candidate.
                    if kind!='probe':self.assertEqual(int.from_bytes(rom[0x1a4:0x1a8],'big'),len(rom)-1)
                    self.assertEqual(rom[0x1f0:0x1f3].rstrip(b'\0 '),{'pal':b'E','ntsc':b'U'}.get(kind,b'JUE'))
                    hashes[kind+'-'+mode]=hashlib.sha256(rom).hexdigest()
                    if kind=='probe':
                        code,values=execute(rom,run('m68k-elf-nm','--defined-only','out/rom.out'));self.assertEqual(code,0,f'Target contract failed at {code}')
                        geometry=values[:2];self.assertEqual(values[2]<<16|values[3],trace)
                        tiles=values[4:6];self.assertEqual(values[6:8],[40,28]);self.assertLess(sum(tiles)+28+160+96,1024)
                        at=8
                        for name,w,h,pieces,count,pal in [('fighters',24,32,1,5,art.ACTOR),('bolts',8,8,1,2,art.PROPS),('sparks',16,16,1,2,art.PROPS),('swords',24,24,1,8,art.PROPS)]:
                            self.assertEqual(values[at:at+5],[w,h,pieces,w*h//64,count]);at+=5
                            self.assertEqual(values[at:at+count],[20]*5 if name=='fighters'else[4]*8 if name=='swords'else[4]*2);at+=count
                            self.assertEqual(values[at:at+16],[(b<<9)|(g<<5)|(r<<1)for r,g,b in pal]);at+=16
                        self.assertEqual(values[at:at+1536],[half for v in art.font_words()for half in(v>>16,v&65535)]);at+=1536
                        self.assertEqual(values[at:],[])
            (out/'combat-report.json').write_text(json.dumps({'passed':True,'hashes':hashes,'geometry':geometry,'trace_digest':trace,
                'budgets':{'court_title_tiles':tiles,'live_sprite_tiles':154,'sprite_reservation':160,'hardware_sprites':22,'admitted_scanline_pieces':20,'admitted_scanline_pixels':320,'palettes':4,'hud_ram_bytes':320,'max_contact_batch':34,'contact_capacity':40},
                'execution':'Base M68000 model and compiled resources; no VDP/audio/peripheral protocol emulation','physical_hardware':'not tested'},indent=2)+'\n')
if __name__=='__main__':unittest.main()
