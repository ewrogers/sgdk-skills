"""Projection, independent projected-hull oracle, ordering and actual SGDK builds."""
import hashlib
import importlib.util
import itertools
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
LAB=ROOT/'skills/sgdk-isometric/assets/isometric-lab';FIXTURE=ROOT/'tests/fixtures/isometric'
SPECS=[('hero',24,32,16,1,1),('deck',32,24,1,1,4),('post',16,40,1,2,4),('ramp_l',32,48,1,2,1),('ramp_r',32,48,1,2,1),('tower',32,64,1,2,2),('tree',32,48,1,2,2),('fountain',32,32,3,1,1),('marker',16,8,1,1,1)]
def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);sys.path.insert(0,str(path.parent))
    try:spec.loader.exec_module(m)
    finally:sys.path.pop(0)
    return m

def compile_host(cc,binary,mode):
    args=[cc,'-DIL_HOST','-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'inc'),'-I',str(FIXTURE),str(LAB/'src/iso.c'),str(LAB/'src/world.c'),str(FIXTURE/'host.c'),'-o',str(binary)]
    if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
    r=subprocess.run(args,capture_output=True,text=True,timeout=60)
    if r.returncode:raise RuntimeError(r.stdout+r.stderr)

def hull(box):
    # Independent monotone-chain convex hull of the eight projected 3-D corners.
    x0,y0,z0,x1,y1,z1=box
    points=sorted(set((2*(x-y),x+y-2*z) for x,y,z in itertools.product((x0,x1),(y0,y1),(z0,z1))))
    def cross(a,b,c):return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    lower=[];upper=[]
    for p in points:
        while len(lower)>1 and cross(lower[-2],lower[-1],p)<=0:lower.pop()
        lower.append(p)
    for p in reversed(points):
        while len(upper)>1 and cross(upper[-2],upper[-1],p)<=0:upper.pop()
        upper.append(p)
    return lower[:-1]+upper[:-1]

def projected_overlap(a,b):
    for polygon in (a,b):
        for p,q in zip(polygon,polygon[1:]+polygon[:1]):
            nx,ny=q[1]-p[1],p[0]-q[0];va=[x*nx+y*ny for x,y in a];vb=[x*nx+y*ny for x,y in b]
            if max(va)<=min(vb) or max(vb)<=min(va):return False
    return True

class IsometricTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cls.cc:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
            raise unittest.SkipTest('C17 compiler required')
        cls.scratch=tempfile.TemporaryDirectory(prefix='isometric contracts ');cls.addClassCleanup(cls.scratch.cleanup);cls.binaries=[]
        for mode in ['-O0','-O3']:
            b=Path(cls.scratch.name)/(mode+('.exe' if os.name=='nt' else ''));compile_host(cls.cc,b,mode);cls.binaries.append(b)
    def case(self,name):
        for b in self.binaries:
            r=subprocess.run([str(b),name],capture_output=True,text=True,timeout=30);self.assertEqual(r.returncode,0,r.stderr)
    def test_projection_negative_floor_known_height_inverse_and_range(self):self.case('projection')
    def test_height_occlusion_and_projected_separation(self):self.case('relations')
    def test_stable_topological_order_support_overrides_cycles_and_errors(self):self.case('sort')
    def test_stairs_upper_edges_underpass_collision_and_camera(self):self.case('world')
    def test_static_cache_owned_copy_pal_ntsc_and_pause(self):self.case('timing')
    def test_20000_mixed_movement_steps_keep_sort_and_surfaces_valid(self):self.case('soak')
    def test_6000_pairs_against_independent_convex_hull_oracle(self):
        rng=random.Random(20261016);pairs=[];expected=[]
        for i in range(6000):
            boxes=[]
            for _ in range(2):
                x,y=rng.randint(-2000,2000),rng.randint(-2000,2000);z=rng.randint(0,1200)
                boxes.append((x,y,z,x+rng.randint(16,1000),y+rng.randint(16,1000),z+rng.randint(16,800)))
            a,b=boxes;pairs.append(a+b);overlap=projected_overlap(hull(a),hull(b))
            before=a[3]<=b[0] or a[4]<=b[1] or a[5]<=b[2];after=b[3]<=a[0] or b[4]<=a[1] or b[5]<=a[2]
            expected.append(0 if not overlap else 1 if before and not after else 2 if after and not before else 3)
        self.assertGreater(expected.count(1),100);self.assertGreater(expected.count(3),100)
        data=''.join(' '.join(map(str,p))+'\n' for p in pairs)
        for binary in self.binaries:
            r=subprocess.run([str(binary),'pairs'],input=data,capture_output=True,text=True,timeout=30);self.assertEqual(r.returncode,0,r.stderr)
            self.assertEqual(list(map(int,r.stdout.split())),expected)

class IsometricAssetsTests(unittest.TestCase):
    def test_native_palettes_sheets_music_and_standalone_copy(self):
        from test_localization import pixels
        art=load('iso_asset_art',LAB/'tools/iso_art.py');assets=art.assets();self.assertEqual(assets,art.assets())
        for name,w,h,count,_,_ in SPECS:
            ww,hh,rows=pixels(assets[name+'.png']);self.assertEqual((ww,hh),(w*count,h));self.assertLess(max(v for row in rows for v in row),16)
            for f in range(count):self.assertTrue(any(v for row in rows for v in row[f*w:(f+1)*w]))
        self.assertEqual(pixels(assets['ground.png'])[:2],(512,256))
        for pal in [art.WORLD,art.PROPS,art.HERO,art.UI]:self.assertEqual(len(pal),16);self.assertTrue(all(0<=c<=7 for rgb in pal for c in rgb))
        font=art.font_words();self.assertEqual(len(font),768)
        self.assertTrue(all((v>>shift)&15 in (1,15) for v in font for shift in range(0,32,4)))
        audio=load('iso_asset_audio',LAB/'tools/iso_audio.py');self.assertEqual(audio.assets(),audio.assets());self.assertTrue(audio.assets()['garden.vgm'].startswith(b'Vgm '))
        prepare=load('iso_asset_prepare',LAB/'tools/prepare.py')
        with tempfile.TemporaryDirectory(prefix='isometric copy ') as scratch:
            first=prepare.prepare(Path(scratch)/'first')
            with self.assertRaises(ValueError):prepare.prepare(first)
            r=subprocess.run([sys.executable,str(first/'tools/prepare.py'),str(Path(scratch)/'second')],capture_output=True,text=True,timeout=30);self.assertEqual(r.returncode,0,r.stderr)
            self.assertEqual((first/'res/ground.png').read_bytes(),(Path(scratch)/'second/res/ground.png').read_bytes())
        with self.assertRaises(ValueError):prepare.prepare(LAB/'new output')

@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class IsometricContainerTests(unittest.TestCase):
    def test_roms_target_geometry_ordering_and_compiled_resources(self):
        from test_streaming import execute
        prepare=load('iso_container_prepare',LAB/'tools/prepare.py');art=load('iso_container_art',LAB/'tools/iso_art.py')
        out=ROOT/'build/isometric-integration';out.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='isometric container ',dir=ROOT/'build') as scratch:
            project=prepare.prepare(Path(scratch)/'standalone isometric lab')
            cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None);self.assertIsNotNone(cc)
            host=Path(scratch)/'host';compile_host(cc,host,'-O3');digest=int(subprocess.check_output([str(host),'digest'],text=True))
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                r=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240);self.assertEqual(r.returncode,0,(r.stdout+r.stderr)[-5000:]);return r.stdout+r.stderr
            hashes={};geometry=None;tiles=None;total=0
            for kind in ['rom','pal','ntsc','probe']:
                header=project/'src/boot/rom_head.c';s=(LAB/'src/boot/rom_head.c').read_text()
                if kind in ('pal','ntsc'):s=s.replace('.region="JUE"','.region="'+('E' if kind=='pal' else 'U')+'"')
                header.write_text(s)
                if kind=='probe':shutil.copyfile(FIXTURE/'probe.c',project/'src/main.c');shutil.copyfile(FIXTURE/'checks.h',project/'inc/checks.h')
                for mode in (['release','debug'] if kind in ('rom','probe') else ['release']):
                    run('make','clean');log=run('make',mode);(out/f'{kind}-{mode}.log').write_text(log);self.assertNotIn('warning:',log)
                    rom=(project/'out/rom.bin').read_bytes();self.assertEqual(len(rom),131072);hashes[kind+'-'+mode]=hashlib.sha256(rom).hexdigest()
                    if kind=='probe':
                        code,values=execute(rom,run('m68k-elf-nm','--defined-only','out/rom.out'));self.assertEqual(code,0,f'Target contract failed at {code}')
                        geometry=values[:4];self.assertEqual(geometry,[22,436,12,14]);self.assertEqual(values[4]<<16|values[5],digest)
                        tiles=values[6];self.assertLess(tiles+320+96+16+1,1440);self.assertEqual(values[7:9],[64,32]);cursor=9;total=0
                        for name,w,h,frames,pieces,instances in SPECS:
                            self.assertEqual(values[cursor:cursor+5],[w,h,w*h//64,pieces,frames]);cursor+=5
                            palette=art.HERO if name in ('hero','marker') else art.PROPS
                            self.assertEqual(values[cursor:cursor+16],[(b<<9)|(g<<5)|(r<<1) for r,g,b in palette]);cursor+=16;total+=w*h//64*instances
                        words=art.font_words()
                        self.assertEqual(values[cursor:cursor+1536],[half for v in words for half in (v>>16,v&65535)]);cursor+=1536
                        layout=[values[cursor+i:cursor+i+4] for i in range(0,60,4)];cursor+=60
                        sizes=[(32,24,12),(16,40,35),(32,48,36),(32,48,36),(32,64,55),(32,48,40),(32,32,24)]
                        peaks=[]
                        for y in range(256):
                            widths=[]
                            for x,yy,z,prop_kind in layout:
                                w,h,anchor=sizes[prop_kind];top=64+(x+yy-2*z)//32-anchor
                                if top<=y<top+h:widths.append(w)
                            # Conservatively add hero + debug foot marker on every line.
                            peaks.append((len(widths)+2,sum(widths)+24+16))
                        peak_pieces=max(p[0] for p in peaks);peak_pixels=max(p[1] for p in peaks)
                        self.assertLessEqual(peak_pieces,20);self.assertLessEqual(peak_pixels,320)
                        self.assertEqual(cursor,len(values));self.assertEqual(total,278)
            (out/'isometric-report.json').write_text(json.dumps({'passed':True,'hashes':hashes,'geometry':geometry,'trace_digest':digest,
                'budgets':{'ground_tiles':tiles,'max_instance_tiles':total,'sprite_reservation':320,'logical_sprites':17,'sort_items':16,'palettes':4,'hud_ram':480,'conservative_scanline_pieces':peak_pieces,'conservative_scanline_pixels':peak_pixels},
                'execution':'Base M68000 geometry, ordering, movement and cached trace; no VDP/audio timing emulation'},indent=2)+'\n')

if __name__=='__main__':unittest.main()
