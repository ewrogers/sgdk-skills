"""Exercise bounded cosmetic simulation, render admission and the actual SGDK assets."""
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

ROOT=Path(__file__).resolve().parents[1]
LAB=ROOT/'skills/sgdk-effects/assets/effects-lab'
FIXTURE=ROOT/'tests/fixtures/effects'


def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec)
    sys.path.insert(0,str(path.parent))
    try:spec.loader.exec_module(module)
    finally:sys.path.pop(0)
    return module


def execute_profile(rom,symbols):
    """Count base-M68000 instructions between explicit workload markers."""
    from unicorn import Uc,UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,UC_HOOK_MEM_WRITE,UC_HOOK_CODE
    from unicorn.m68k_const import UC_CPU_M68K_M68000,UC_M68K_REG_A7,UC_M68K_REG_D0,UC_M68K_REG_PC,UC_M68K_REG_SR
    entries=[int(line.split()[0],16) for line in symbols.splitlines()
             if len(line.split())==3 and line.split()[2]=='main']
    if len(entries)!=1:raise AssertionError('Expected one direct-entry main')
    machine=Uc(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN);machine.ctl_set_cpu_model(UC_CPU_M68K_M68000)
    for address,size in [(0,0x400000),(0xC00000,0x1000),(0xE0FF0000,0x10000)]:machine.mem_map(address,size)
    machine.mem_write(0,rom);machine.mem_write(0xE0FFFFE0,(0x300000).to_bytes(4,'big'))
    machine.reg_write(UC_M68K_REG_SR,0x2700);machine.reg_write(UC_M68K_REG_A7,0xE0FFFFE0)
    values=[];work={};counter=[0];start=[0]
    def instruction(*args):counter[0]+=1
    def write(machine,access,address,size,value,data):
        if address!=0xC00000:return
        if size!=2:raise AssertionError('Expected word output')
        value&=65535
        if value in (0xEA01,0xEA03):start[0]=counter[0]
        elif value in (0xEA02,0xEA04):work['optimized' if value==0xEA02 else 'reference']=counter[0]-start[0]
        else:values.append(value)
    machine.hook_add(UC_HOOK_CODE,instruction);machine.hook_add(UC_HOOK_MEM_WRITE,write)
    machine.emu_start(entries[0],0x300000,count=2000000)
    if machine.reg_read(UC_M68K_REG_PC)!=0x300000:raise AssertionError('Probe instruction budget exceeded')
    return machine.reg_read(UC_M68K_REG_D0),values,work


class EffectsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler=next((p for name in ['clang','cc','gcc'] if (p:=shutil.which(name))),None)
        if not cls.compiler:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
            raise unittest.SkipTest('C17 compiler required')
        cls.scratch=tempfile.TemporaryDirectory(prefix='effects standalone ');cls.addClassCleanup(cls.scratch.cleanup)
        cls.project=load('effects_prepare',LAB/'tools/prepare.py').prepare(Path(cls.scratch.name)/'independent project')
        cls.binaries=[]
        for optimize in ['-O0','-O3']:
            binary=Path(cls.scratch.name)/(optimize+('.exe' if os.name=='nt' else ''))
            command=[cls.compiler,'-std=c17',optimize,'-Wall','-Wextra','-Werror','-I',str(cls.project/'inc'),
                     '-I',str(FIXTURE),str(cls.project/'src/effects.c'),str(FIXTURE/'host.c'),str(FIXTURE/'select_reference.c'),'-o',str(binary)]
            if os.name!='nt':command+=['-fsanitize=undefined','-fno-sanitize-recover=all']
            result=subprocess.run(command,capture_output=True,text=True,timeout=60)
            if result.returncode:raise RuntimeError(result.stdout+result.stderr)
            cls.binaries.append(binary)

    def case(self,name):
        for binary in self.binaries:
            result=subprocess.run([str(binary),name],capture_output=True,text=True,timeout=40)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)

    def test_full_pool_priority_replacement_rejection_validation_and_reuse(self):self.case('pool')
    def test_motion_lifetime_world_trails_pause_edges_and_reduced_shake(self):self.case('motion')
    def test_visible_admission_frame_and_scanline_caps_camera_edges(self):self.case('render')
    def test_equal_pal_ntsc_elapsed_time_and_clock_wrap(self):self.case('clocks')
    def test_equal_priority_fairness_and_render_budget_cannot_mutate_simulation(self):self.case('fairness')
    def test_every_renderable_row_rejects_a_ninth_rectangle_without_nibble_carry(self):self.case('scanlines')
    def test_5000_selections_match_pre_optimization_reference(self):self.case('reference')
    def test_100000_mixed_frames_against_scanline_oracle_and_bounds(self):self.case('soak')

    def test_frame_upload_failure_cleanup_shared_tiles_sat_termination_and_hud_isolation(self):
        binary=Path(self.scratch.name)/('adapter.exe' if os.name=='nt' else 'adapter')
        result=subprocess.run([self.compiler,'-std=c17','-O2','-Wall','-Wextra','-Werror',
            '-I',str(FIXTURE/'stubs'),'-I',str(self.project/'inc'),str(self.project/'src/effects.c'),
            str(self.project/'src/fx_view.c'),str(FIXTURE/'adapter.c'),'-o',str(binary)],capture_output=True,text=True,timeout=60)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        result=subprocess.run([str(binary)],capture_output=True,text=True,timeout=30)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)


class EffectsAssetTests(unittest.TestCase):
    def test_native_dimensions_indexed_art_reproduction_and_no_overwrite(self):
        from test_localization import pixels
        prepare=load('effects_art_prepare',LAB/'tools/prepare.py')
        with tempfile.TemporaryDirectory(prefix='effects art ') as scratch:
            first=prepare.prepare(Path(scratch)/'one');second=prepare.prepare(Path(scratch)/'two')
            for name,height in [('fx.png',8),('lantern.png',16)]:
                data=(first/'res'/name).read_bytes();self.assertEqual(data,(second/'res'/name).read_bytes())
                w,h,rows=pixels(data);self.assertEqual((w,h),(64,height))
                colors={v for row in rows for v in row};self.assertIn(0,colors);self.assertLess(max(colors),16)
                for frame in range(8 if height==8 else 4):
                    width=8 if height==8 else 16
                    self.assertTrue(any(v for row in rows for v in row[frame*width:(frame+1)*width]))
            with self.assertRaises(ValueError):prepare.prepare(first)
            result=subprocess.run([sys.executable,str(first/'tools/prepare.py'),str(Path(scratch)/'copied again')],
                                  capture_output=True,text=True,timeout=30)
            self.assertEqual(result.returncode,0,result.stderr)
            with self.assertRaises(ValueError):prepare.prepare(LAB/'src/forbidden output')


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class EffectsContainerTests(unittest.TestCase):
    def test_real_resources_and_m68000_rules_in_both_build_modes(self):
        from test_localization import pixels
        prepare=load('effects_container_prepare',LAB/'tools/prepare.py')
        output=ROOT/'build/effects-integration';output.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='effects container ',dir=ROOT/'build') as scratch:
            project=prepare.prepare(Path(scratch)/'independent effects lab')
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                result=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                self.assertEqual(result.returncode,0,(result.stdout+result.stderr)[-5000:]);return result.stdout+result.stderr
            # Compile the exact same bounded simulation as host code for the trace oracle.
            cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
            self.assertIsNotNone(cc)
            binary=Path(scratch)/'trace'
            result=subprocess.run([cc,'-std=c17','-O2','-I',str(project/'inc'),'-I',str(FIXTURE),
                str(project/'src/effects.c'),str(FIXTURE/'host.c'),str(FIXTURE/'select_reference.c'),'-o',str(binary)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            expected_digest=int(subprocess.check_output([str(binary),'digest'],text=True))
            hashes={};geometry=None
            for kind in ['rom','probe']:
                if kind=='probe':
                    shutil.copyfile(FIXTURE/'probe.c',project/'src/main.c');shutil.copyfile(FIXTURE/'checks.h',project/'inc/checks.h');shutil.copyfile(FIXTURE/'select_reference.c',project/'src/select_reference.c')
                for mode in ['release','debug']:
                    run('make','clean');log=run('make',mode);(output/f'{kind}-{mode}.log').write_text(log)
                    self.assertNotIn('warning:',log)
                    rom=(project/'out/rom.bin').read_bytes();self.assertEqual(len(rom),131072)
                    self.assertEqual(rom[0x100:0x104],b'SEGA');self.assertEqual(struct.unpack_from('>I',rom,0x1A4)[0],len(rom)-1)
                    hashes[kind+'-'+mode]=hashlib.sha256(rom).hexdigest()
                    if kind=='probe':
                        code,raw,work=execute_profile(rom,run('m68k-elf-nm','out/rom.out'))
                        self.assertLess(work['optimized'],work['reference']*0.85)
                        self.assertEqual(code&65535,0,f'Target contract failed at {code&65535}')
                        values=[v&65535 for v in raw];geometry=values[:4]
                        self.assertEqual(geometry[:3],[1670,634,16])
                        self.assertEqual((values[4]<<16)|values[5],expected_digest)
                        cursor=6
                        for name,w,h,frames in [('fx.png',8,8,8),('lantern.png',16,16,4)]:
                            self.assertEqual(values[cursor:cursor+4],[w,h,frames,w*h//64]);cursor+=4
                            art=load('effects_resource_art',project/'tools/fx_art.py')
                            palette=[((b>>5)<<9)|((g>>5)<<5)|((r>>5)<<1) for r,g,b in art.PALETTE]
                            self.assertEqual(values[cursor:cursor+16],palette);cursor+=16
                            _,_,rows=pixels((project/'res'/name).read_bytes())
                            for frame in range(frames):
                                self.assertEqual(values[cursor],w*h//64);cursor+=1
                                expected=[]
                                # A single hardware sprite's tiles run down columns first.
                                for tx in range(w//8):
                                    for ty in range(h//8):
                                        for y in range(8):
                                            row=rows[ty*8+y][frame*w+tx*8:frame*w+tx*8+8];word=0
                                            for nibble in row:word=(word<<4)|nibble
                                            expected.extend([word>>16,word&65535])
                                self.assertEqual(values[cursor:cursor+len(expected)],expected);cursor+=len(expected)
                        self.assertEqual(cursor,len(values))
            (output/'effects-report.json').write_text(json.dumps({'passed':True,'hashes':hashes,'geometry':geometry,
                'trace_digest':expected_digest,'selection_instructions':work,'budgets':{'pool':96,'weather':48,'effect_sprites':64,'effects_per_line':8,
                'cached_tiles':24,'background_tiles':9},'execution':'Base M68000 rules/resources; no VDP timing emulation'},indent=2)+'\n')


if __name__=='__main__':unittest.main()
