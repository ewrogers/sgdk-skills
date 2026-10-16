"""Gameplay animation contracts and actual SGDK compiled pose resources."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
LAB=ROOT/'skills/sgdk-animation/assets/animation-lab'
FIXTURE=ROOT/'tests/fixtures/animation'
SPECS=[('hero',24,24,17),('dummy',24,32,2),('arc',32,16,2),('attack_box',32,16,1),('body_box',16,24,1),('target_box',16,32,1)]

def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec)
    sys.path.insert(0,str(path.parent))
    try:spec.loader.exec_module(module)
    finally:sys.path.pop(0)
    return module

def compile_host(cc,binary,mode):
    command=[cc,'-DAL_HOST','-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'inc'),'-I',str(FIXTURE),
        str(LAB/'src/action.c'),str(FIXTURE/'host.c'),'-o',str(binary)]
    if os.name!='nt':command+=['-fsanitize=undefined','-fno-sanitize-recover=all']
    result=subprocess.run(command,capture_output=True,text=True,timeout=60)
    if result.returncode:raise RuntimeError(result.stdout+result.stderr)

class AnimationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cls.cc:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
            raise unittest.SkipTest('C17 compiler required')
        cls.scratch=tempfile.TemporaryDirectory(prefix='animation contracts ');cls.addClassCleanup(cls.scratch.cleanup)
        cls.binaries=[]
        for mode in ['-O0','-O3']:
            binary=Path(cls.scratch.name)/(mode+('.exe' if os.name=='nt' else ''))
            compile_host(cls.cc,binary,mode);cls.binaries.append(binary)

    def case(self,kind):
        for binary in self.binaries:
            result=subprocess.run([str(binary),kind],capture_output=True,text=True,timeout=30)
            self.assertEqual(result.returncode,0,result.stderr)

    def test_exact_holds_frame_entry_events_full_last_hold_and_explicit_restart(self):self.case('clock')
    def test_active_windows_one_hit_mirroring_input_buffer_expiry_and_recovery_cancel(self):self.case('actions')
    def test_hurt_cancels_future_strikes_and_death_completes_once_before_revive(self):self.case('interrupts')
    def test_pal_ntsc_progress_pause_edges_and_slow_mode_short_press_retention(self):self.case('timing')
    def test_100000_mixed_frames_keep_clocks_poses_boxes_and_batches_bounded(self):self.case('soak')

class AnimationArtTests(unittest.TestCase):
    def test_native_indexed_pose_sheets_reproduce_and_copy_as_standalone(self):
        from test_localization import pixels
        art=load('animation_art',LAB/'tools/animation_art.py');generated=art.assets();self.assertEqual(generated,art.assets())
        for name,w,h,count in SPECS:
            width,height,rows=pixels(generated[name+'.png']);self.assertEqual((width,height),(w*count,h))
            self.assertLess(max(v for row in rows for v in row),16)
            for frame in range(count):self.assertTrue(any(v for row in rows for v in row[frame*w:(frame+1)*w]))
        for palette in [art.HERO,art.DUMMY,art.WIRE,art.UI]:
            self.assertEqual(len(palette),16);self.assertTrue(all(0<=c<=7 for rgb in palette for c in rgb))
        prepare=load('animation_prepare',LAB/'tools/prepare.py')
        with tempfile.TemporaryDirectory(prefix='animation art ') as scratch:
            first=prepare.prepare(Path(scratch)/'first')
            with self.assertRaises(ValueError):prepare.prepare(first)
            result=subprocess.run([sys.executable,str(first/'tools/prepare.py'),str(Path(scratch)/'second')],capture_output=True,text=True,timeout=30)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertEqual((first/'res/hero.png').read_bytes(),(Path(scratch)/'second/res/hero.png').read_bytes())
        with self.assertRaises(ValueError):prepare.prepare(LAB/'src/new output')

@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class AnimationContainerTests(unittest.TestCase):
    def test_release_debug_target_logic_trace_and_all_compiled_pose_pixels(self):
        from test_streaming import execute
        from test_localization import pixels
        prepare=load('animation_container_prepare',LAB/'tools/prepare.py');art=load('animation_probe_art',LAB/'tools/animation_art.py')
        output=ROOT/'build/animation-integration';output.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='animation container ',dir=ROOT/'build') as scratch:
            project=prepare.prepare(Path(scratch)/'standalone animation lab')
            cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None);self.assertIsNotNone(cc)
            binary=Path(scratch)/'host';compile_host(cc,binary,'-O3')
            digest=int(subprocess.check_output([str(binary),'digest'],text=True))
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                result=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                self.assertEqual(result.returncode,0,(result.stdout+result.stderr)[-5000:]);return result.stdout+result.stderr
            hashes={};geometry=None
            for kind in ['rom','probe']:
                if kind=='probe':
                    shutil.copyfile(FIXTURE/'probe.c',project/'src/main.c');shutil.copyfile(FIXTURE/'checks.h',project/'inc/checks.h')
                for mode in ['release','debug']:
                    run('make','clean');log=run('make',mode);(output/f'{kind}-{mode}.log').write_text(log)
                    self.assertNotIn('warning:',log)
                    rom=(project/'out/rom.bin').read_bytes();self.assertEqual(len(rom),131072)
                    hashes[kind+'-'+mode]=hashlib.sha256(rom).hexdigest()
                    if kind=='probe':
                        code,values=execute(rom,run('m68k-elf-nm','--defined-only','out/rom.out'))
                        self.assertEqual(code,0,f'Target animation contract failed at {code}')
                        geometry=values[:4];self.assertEqual(geometry,[8,28,36,6])
                        self.assertEqual(values[4]<<16|values[5],digest);cursor=6
                        for d,(name,w,h,count) in enumerate(SPECS):
                            self.assertEqual(values[cursor:cursor+4],[w,h,count,w*h//64]);cursor+=4
                            palette=art.HERO if d==0 else art.DUMMY if d==1 else art.WIRE
                            colors=[(b<<9)|(g<<5)|(r<<1) for r,g,b in palette]
                            self.assertEqual(values[cursor:cursor+16],colors);cursor+=16
                            width,height,rows=pixels((project/'res'/(name+'.png')).read_bytes())
                            for frame in range(count):
                                tiles=values[cursor];cursor+=1;self.assertEqual(tiles,w*h//64)
                                data=b''.join(v.to_bytes(2,'big') for v in values[cursor:cursor+tiles*16]);cursor+=tiles*16
                                expected=bytearray()
                                for tx in range(w//8):
                                    for ty in range(h//8):
                                        for y in range(8):
                                            row=rows[ty*8+y];x=frame*w+tx*8
                                            expected.extend((row[x+i]<<4)|row[x+i+1] for i in range(0,8,2))
                                self.assertEqual(data,expected,(name,frame))
                        self.assertEqual(cursor,len(values))
            (output/'animation-report.json').write_text(json.dumps({'passed':True,'hashes':hashes,'geometry':geometry,
                'trace_digest':digest,'budgets':{'sprite_instances':6,'reserved_sprite_tiles':72,'worst_frame_tiles':51,
                    'hud_ram_bytes':2240,'dma_allowance_bytes':4096},
                'execution':'Base M68000 action clock, combat contracts, host/target trace and compiled pixels; no VDP/audio timing emulation'},indent=2)+'\n')

if __name__=='__main__':unittest.main()
