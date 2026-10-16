"""Exact rational oracle, bounded sliding and real base-M68000 collision contracts."""
from fractions import Fraction
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
LAB=ROOT/'skills/sgdk-collision/assets/collision-lab'
FIXTURE=ROOT/'tests/fixtures/collision'

def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec)
    sys.path.insert(0,str(path.parent))
    try:spec.loader.exec_module(module)
    finally:sys.path.pop(0)
    return module

def compile_host(cc,binary,mode):
    args=[cc,'-DCC_HOST','-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'inc'),'-I',str(FIXTURE),
        str(LAB/'src/collision.c'),str(LAB/'src/demo.c'),str(FIXTURE/'host.c'),'-o',str(binary)]
    if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
    result=subprocess.run(args,capture_output=True,text=True,timeout=60)
    if result.returncode:raise RuntimeError(result.stdout+result.stderr)

def oracle(row):
    ax,ay,aw,ah,dx,dy,bx,by,bw,bh,vx,vy,point=row
    if point:aw=ah=0;vx=vy=0
    if ax<bx+bw and ax+aw>bx and ay<by+bh and ay+ah>by:return (2,0,0,0,None)
    entry=[];leave=[];normals=[]
    for p,size,lo,hi,d,axis in [(ax,aw,bx,bx+bw,dx-vx,0),(ay,ah,by,by+bh,dy-vy,1)]:
        if d==0:
            if p>=hi or p+size<=lo:return (0,0,0,0,None)
            continue
        ends=sorted([Fraction(lo-p-size,d),Fraction(hi-p,d)])
        entry.append(ends[0]);leave.append(ends[1]);normals.append((ends[0],axis,-1 if d>0 else 1))
    if not entry:return (0,0,0,0,None)
    t=max(entry)
    if t<0 or t>1 or t>=min(leave):return (0,0,0,0,None)
    ns=[0,0]
    for at,axis,normal in normals:
        if at==t:ns[axis]=normal
    return (1,int(t*4096),*ns,t)

class CollisionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cls.cc:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
            raise unittest.SkipTest('C17 compiler required')
        cls.scratch=tempfile.TemporaryDirectory(prefix='collision contracts ');cls.addClassCleanup(cls.scratch.cleanup)
        cls.binaries=[]
        for mode in ['-O0','-O3']:
            binary=Path(cls.scratch.name)/(mode+('.exe' if os.name=='nt' else ''))
            compile_host(cls.cc,binary,mode);cls.binaries.append(binary)

    def case(self,name):
        for binary in self.binaries:
            r=subprocess.run([str(binary),name],capture_output=True,text=True,timeout=30)
            self.assertEqual(r.returncode,0,r.stderr)

    def test_sweeps_relative_motion_edge_contact_corner_grazing_and_range(self):self.case('sweep')
    def test_segment_parallel_boundary_inside_and_zero_length(self):self.case('ray')
    def test_nearest_exact_time_slide_corner_ties_and_initial_overlap(self):self.case('slide')
    def test_invalid_geometry_motion_pointer_and_candidate_capacity_fail_explicitly(self):self.case('invalid')
    def test_pal_ntsc_pause_and_long_slide_route(self):self.case('demo')

    def test_12000_queries_against_independent_fraction_oracle(self):
        rng=random.Random(80000);rows=[]
        for i in range(12000):
            ax,ay,bx,by=[rng.randint(-16384,16384) for _ in range(4)]
            if i%3:bx=ax+rng.randint(-1800,1800);by=ay+rng.randint(-1800,1800)
            bx=max(-16384,min(16384,bx));by=max(-16384,min(16384,by))
            aw,ah,bw,bh=[rng.randint(1,4096) for _ in range(4)]
            dx,dy,vx,vy=[rng.randint(-4096,4096) for _ in range(4)]
            dx=max(-16384-ax,min(16384-ax,dx));dy=max(-16384-ay,min(16384-ay,dy))
            vx=max(-16384-bx,min(16384-bx,vx));vy=max(-16384-by,min(16384-by,vy))
            rows.append((ax,ay,aw,ah,dx,dy,bx,by,bw,bh,vx,vy,i%2))
        data=''.join(' '.join(map(str,row))+'\n' for row in rows)
        expected=[oracle(row) for row in rows]
        self.assertGreater(sum(e[0]==1 for e in expected),300)
        self.assertGreater(sum(e[0]==2 for e in expected),300)
        for binary in self.binaries:
            r=subprocess.run([str(binary),'query'],input=data,capture_output=True,text=True,timeout=30)
            self.assertEqual(r.returncode,0,r.stderr)
            lines=r.stdout.splitlines();self.assertEqual(len(lines),len(rows))
            for row,line,want in zip(rows,lines,expected):
                got=list(map(int,line.split()));self.assertEqual(got[:4],list(want[:4]),row)
                if got[0]==1:self.assertEqual(Fraction(got[4],got[5]),want[4],row)

class CollisionArtTests(unittest.TestCase):
    def test_reproducible_native_art_and_fresh_standalone_copy(self):
        from test_localization import pixels
        art=load('collision_art',LAB/'tools/collision_art.py');generated=art.assets();self.assertEqual(generated,art.assets())
        for name,w,h in [('actor',16,16),('shot',8,8),('wall',8,32),('dot',8,8),('cursor',8,8)]:
            width,height,rows=pixels(generated[name+'.png']);self.assertEqual((width,height),(w,h))
            self.assertTrue(any(v for row in rows for v in row));self.assertLess(max(v for row in rows for v in row),16)
        prepare=load('collision_prepare',LAB/'tools/prepare.py')
        with tempfile.TemporaryDirectory(prefix='collision copy ') as scratch:
            first=prepare.prepare(Path(scratch)/'first')
            with self.assertRaises(ValueError):prepare.prepare(first)
            r=subprocess.run([sys.executable,str(first/'tools/prepare.py'),str(Path(scratch)/'second')],capture_output=True,text=True,timeout=30)
            self.assertEqual(r.returncode,0,r.stderr)
            self.assertEqual((first/'res/actor.png').read_bytes(),(Path(scratch)/'second/res/actor.png').read_bytes())
        with self.assertRaises(ValueError):prepare.prepare(LAB/'new output')

@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class CollisionContainerTests(unittest.TestCase):
    def test_release_debug_pal_ntsc_and_executed_m68000_contracts(self):
        from test_streaming import execute
        from test_localization import pixels
        prepare=load('collision_container_prepare',LAB/'tools/prepare.py')
        art=load('collision_container_art',LAB/'tools/collision_art.py')
        output=ROOT/'build/collision-integration';output.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='collision container ',dir=ROOT/'build') as scratch:
            project=prepare.prepare(Path(scratch)/'standalone collision lab')
            cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None);self.assertIsNotNone(cc)
            binary=Path(scratch)/'host';compile_host(cc,binary,'-O3')
            digest=int(subprocess.check_output([str(binary),'digest'],text=True))
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                r=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                self.assertEqual(r.returncode,0,(r.stdout+r.stderr)[-5000:]);return r.stdout+r.stderr
            hashes={};geometry=None
            for kind in ['rom','pal','ntsc','probe']:
                header=project/'src/boot/rom_head.c';s=(LAB/'src/boot/rom_head.c').read_text()
                if kind in ('pal','ntsc'):s=s.replace('.region="JUE"','.region="'+('E' if kind=='pal' else 'U')+'"')
                header.write_text(s)
                if kind=='probe':
                    shutil.copyfile(FIXTURE/'probe.c',project/'src/main.c');shutil.copyfile(FIXTURE/'checks.h',project/'inc/checks.h')
                for mode in (['release','debug'] if kind in ('rom','probe') else ['release']):
                    run('make','clean');log=run('make',mode);(output/f'{kind}-{mode}.log').write_text(log)
                    self.assertNotIn('warning:',log)
                    rom=(project/'out/rom.bin').read_bytes();self.assertEqual(len(rom),131072)
                    hashes[kind+'-'+mode]=hashlib.sha256(rom).hexdigest()
                    if kind=='probe':
                        code,values=execute(rom,run('m68k-elf-nm','--defined-only','out/rom.out'))
                        self.assertEqual(code,0,f'Target collision contract failed at {code}')
                        geometry=values[:4];self.assertEqual(geometry,[8,18,20,82])
                        self.assertEqual(values[4]<<16|values[5],digest)
                        cursor=6;tiles_total=0
                        for name,w,h,instances in [('actor',16,16,1),('shot',8,8,2),('wall',8,32,2),('dot',8,8,16),('cursor',8,8,2)]:
                            self.assertEqual(values[cursor:cursor+3],[w,h,w*h//64]);cursor+=3
                            self.assertEqual(values[cursor:cursor+16],[(b<<9)|(g<<5)|(r<<1) for r,g,b in art.COLORS]);cursor+=16
                            tiles=values[cursor];cursor+=1;self.assertEqual(tiles,w*h//64);tiles_total+=tiles*instances
                            data=b''.join(v.to_bytes(2,'big') for v in values[cursor:cursor+tiles*16]);cursor+=tiles*16
                            _,_,rows=pixels((project/'res'/(name+'.png')).read_bytes());expected=bytearray()
                            for tx in range(w//8):
                                for ty in range(h//8):
                                    for y in range(8):
                                        row=rows[ty*8+y];x=tx*8
                                        expected.extend((row[x+i]<<4)|row[x+i+1] for i in range(0,8,2))
                            self.assertEqual(data,expected,name)
                        self.assertEqual(cursor,len(values));self.assertEqual(tiles_total,32)
            (output/'collision-report.json').write_text(json.dumps({'passed':True,'hashes':hashes,'geometry':geometry,
                'trace_digest':digest,'budgets':{'max_boxes':32,'slide_queries':3,'reserved_sprite_tiles':40,
                    'sprite_instances':23,'max_instance_tiles':32,'ray_scanline_pieces':19,'hud_ram_bytes':2240,'dma_allowance_bytes':4096},
                'execution':'Base M68000 collision contracts and 1000-query host/target trace; no VDP/audio timing emulation'},indent=2)+'\n')

if __name__=='__main__':unittest.main()
