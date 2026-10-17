"""Co-op session/camera contracts and actual SGDK builds; no adapter emulation."""
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
LAB=ROOT/'skills/sgdk-multiplayer/assets/coop-lab';FIXTURE=ROOT/'tests/fixtures/multiplayer'
def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);sys.path.insert(0,str(path.parent))
    try:spec.loader.exec_module(m)
    finally:sys.path.pop(0)
    return m
def compile_host(cc,path,mode):
    args=[cc,'-DCP_HOST','-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'inc'),'-I',str(FIXTURE),str(LAB/'src/coop.c'),str(FIXTURE/'host.c'),'-o',str(path)]
    if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
    r=subprocess.run(args,capture_output=True,text=True,timeout=60)
    if r.returncode:raise RuntimeError(r.stdout+r.stderr)
class MultiplayerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cc=next((p for n in ['clang','cc','gcc'] if(p:=shutil.which(n))),None)
        if not cls.cc:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
            raise unittest.SkipTest('C17 compiler required')
        cls.scratch=tempfile.TemporaryDirectory(prefix='coop contracts ');cls.addClassCleanup(cls.scratch.cleanup);cls.binaries=[]
        for mode in ['-O0','-O3']:
            path=Path(cls.scratch.name)/(mode+('.exe' if os.name=='nt' else ''));compile_host(cls.cc,path,mode);cls.binaries.append(path)
    def test_routes_join_edges_capacity_pause_respawn_tether_pickups_and_20000_frames(self):
        traces=[]
        for path in self.binaries:
            r=subprocess.run([str(path)],capture_output=True,text=True,timeout=60);self.assertEqual(r.returncode,0,r.stderr);traces.append(r.stdout)
        self.assertEqual(traces[0],traces[1])
    def test_1000_camera_cases_against_independent_bounding_box_oracle(self):
        rng=random.Random(20261016);cases=[];expected=[]
        for _ in range(1000):
            mask=rng.randrange(16);old=(rng.randrange(193),rng.randrange(201));points=[(rng.randrange(24,489),rng.randrange(24,360))for _ in range(4)]
            active=[p for i,p in enumerate(points)if mask&(1<<i)]
            if active:
                x=max(0,min(192,(min(p[0]for p in active)+max(p[0]for p in active)-320)//2))
                y=max(0,min(200,(min(p[1]for p in active)+max(p[1]for p in active)-184)//2));expected.append((x,y))
            else:expected.append(old)
            cases.append((mask,*old,*(v for p in points for v in p)))
        data=''.join(' '.join(map(str,row))+'\n' for row in cases)
        for path in self.binaries:
            r=subprocess.run([str(path),'camera'],input=data,capture_output=True,text=True,timeout=30);self.assertEqual(r.returncode,0,r.stderr)
            self.assertEqual([tuple(map(int,row.split()))for row in r.stdout.splitlines()],expected)
class MultiplayerAssetsTests(unittest.TestCase):
    def test_native_role_palettes_sheets_and_standalone_copy(self):
        from test_localization import pixels
        art=load('coop_asset_art',LAB/'tools/coop_art.py');assets=art.assets();self.assertEqual(assets,art.assets())
        specs={'hero':(192,96),'moth':(64,16),'mote':(32,8),'beacon':(64,24),'world':(512,384),'title':(320,224)}
        for name,shape in specs.items():
            w,h,rows=pixels(assets[name+'.png']);self.assertEqual((w,h),shape);self.assertLess(max(v for row in rows for v in row),16)
        for pal in [art.WORLD,art.HERO,art.PROPS,art.UI]:self.assertEqual(len(pal),16);self.assertTrue(all(0<=v<=7 for rgb in pal for v in rgb))
        self.assertEqual(len(art.font_words()),768)
        self.assertEqual(len(art.world_rows()),24);self.assertTrue(all(len(row)==32 for row in art.world_rows()))
        prepare=load('coop_asset_prepare',LAB/'tools/prepare.py')
        with tempfile.TemporaryDirectory(prefix='coop copy ') as scratch:
            first=prepare.prepare(Path(scratch)/'first copy')
            with self.assertRaises(ValueError):prepare.prepare(first)
            r=subprocess.run([sys.executable,str(first/'tools/prepare.py'),str(Path(scratch)/'second copy')],capture_output=True,text=True,timeout=30);self.assertEqual(r.returncode,0,r.stderr)
            for name in ['res/world.png','res/hero.png','inc/font.h','res/courtyard.vgm','res/join.wav']:
                self.assertEqual((first/name).read_bytes(),(Path(scratch)/'second copy'/name).read_bytes())
        with self.assertRaises(ValueError):prepare.prepare(LAB/'new output')
@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class MultiplayerContainerTests(unittest.TestCase):
    def test_roms_base_m68000_session_contracts_and_compiled_resources(self):
        from test_streaming import execute
        prepare=load('coop_container_prepare',LAB/'tools/prepare.py');art=load('coop_container_art',LAB/'tools/coop_art.py')
        out=ROOT/'build/multiplayer-integration';out.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='coop target ',dir=ROOT/'build') as scratch:
            project=prepare.prepare(Path(scratch)/'standalone coop lab')
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
                        tiles=values[4:6];self.assertEqual(values[6:8],[64,48]);self.assertLess(sum(tiles)+5+128+96,1024)
                        at=8
                        for name,w,h,pieces,count,pal in [('hero',16,24,1,4,art.HERO),('moth',16,16,1,1,art.PROPS),('mote',8,8,1,1,art.PROPS),('beacon',16,24,1,1,art.PROPS)]:
                            self.assertEqual(values[at:at+5],[w,h,pieces,w*h//64,count]);at+=5
                            self.assertEqual(values[at:at+count],[12]*4 if name=='hero'else[4]);at+=count
                            self.assertEqual(values[at:at+16],[(b<<9)|(g<<5)|(r<<1)for r,g,b in pal]);at+=16
                        self.assertEqual(values[at:at+1536],[half for v in art.font_words()for half in(v>>16,v&65535)]);at+=1536
                        self.assertEqual(values[at:],list(range(8)))
            (out/'multiplayer-report.json').write_text(json.dumps({'passed':True,'hashes':hashes,'geometry':geometry,'trace_digest':trace,
                'budgets':{'world_title_tiles':tiles,'live_sprite_tiles':72,'sprite_reservation':128,'hardware_sprites':20,'conservative_scanline_pieces':20,'conservative_scanline_pixels':256,'palettes':4,'hud_ram_bytes':400},
                'execution':'Base M68000 model and compiled resources; no VDP/audio/peripheral protocol emulation','physical_multitaps':'not tested'},indent=2)+'\n')
if __name__=='__main__':unittest.main()
