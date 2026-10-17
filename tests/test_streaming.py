"""Exercise cache ownership/failures, the real SGDK adapter and compiled assets."""
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
LAB=ROOT/'skills/sgdk-streaming/assets/streaming-lab'
FIXTURE=ROOT/'tests/fixtures/streaming'

def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec)
    sys.path.insert(0,str(path.parent))
    try:spec.loader.exec_module(module)
    finally:sys.path.pop(0)
    return module

def execute(rom,symbols,instruction_budget=20000000):
    from unicorn import Uc,UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,UC_HOOK_MEM_WRITE
    from unicorn.m68k_const import UC_CPU_M68K_M68000,UC_M68K_REG_A7,UC_M68K_REG_D0,UC_M68K_REG_PC,UC_M68K_REG_SR
    entries=[int(l.split()[0],16) for l in symbols.splitlines() if len(l.split())==3 and l.split()[2]=='main']
    if len(entries)!=1:raise AssertionError('Expected one direct-entry main')
    machine=Uc(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN);machine.ctl_set_cpu_model(UC_CPU_M68K_M68000)
    for address,size in [(0,0x400000),(0xC00000,0x1000),(0xE0FF0000,0x10000)]:machine.mem_map(address,size)
    machine.mem_write(0,rom);machine.mem_write(0xE0FFFFE0,(0x300000).to_bytes(4,'big'))
    machine.reg_write(UC_M68K_REG_SR,0x2700);machine.reg_write(UC_M68K_REG_A7,0xE0FFFFE0)
    values=[]
    def write(machine,access,address,size,value,data):
        if address==0xC00000:
            if size!=2:raise AssertionError('Expected word output')
            values.append(value&65535)
    machine.hook_add(UC_HOOK_MEM_WRITE,write)
    machine.emu_start(entries[0],0x300000,count=instruction_budget)
    if machine.reg_read(UC_M68K_REG_PC)!=0x300000:raise AssertionError('Probe instruction budget exceeded')
    return machine.reg_read(UC_M68K_REG_D0)&65535,values

class StreamingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cls.cc:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
            raise unittest.SkipTest('C17 compiler required')
        cls.scratch=tempfile.TemporaryDirectory(prefix='streaming standalone ');cls.addClassCleanup(cls.scratch.cleanup)
        cls.project=load('streaming_prepare',LAB/'tools/prepare.py').prepare(Path(cls.scratch.name)/'independent project')
        cls.binaries=[]
        for mode in ['-O0','-O3']:
            suffix='.exe' if os.name=='nt' else ''
            binary=Path(cls.scratch.name)/(mode+suffix)
            command=[cls.cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(cls.project/'inc'),'-I',str(FIXTURE),
                str(cls.project/'src/stream_cache.c'),str(cls.project/'src/stream_world.c'),str(FIXTURE/'host.c'),'-o',str(binary)]
            if os.name!='nt':command+=['-fsanitize=undefined','-fno-sanitize-recover=all']
            result=subprocess.run(command,capture_output=True,text=True,timeout=60)
            if result.returncode:raise RuntimeError(result.stdout+result.stderr)
            cls.binaries.append(binary)

    def case(self,name):
        for binary in self.binaries:
            result=subprocess.run([str(binary),name],capture_output=True,text=True,timeout=60)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)

    def test_partial_upload_acknowledgment_failures_lru_pins_exhaustion_and_wrap(self):self.case('core')
    def test_circular_columns_partial_pixels_lookahead_and_covered_teleports(self):self.case('world')
    def test_prefetched_forward_and_reverse_walks_never_wait_at_either_region(self):self.case('walk')
    def test_10000_mixed_requests_keep_ready_pages_intact_and_respect_chunk_budget(self):self.case('soak')

    def test_actual_rom_adapter_copies_temporary_far_views_and_checks_queue_and_frame_budgets(self):
        for mode in ['-O0','-O3']:
            binary=Path(self.scratch.name)/('adapter'+mode+('.exe' if os.name=='nt' else ''))
            command=[self.cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(self.project/'inc'),'-I',str(FIXTURE/'stubs'),
                str(self.project/'src/stream_cache.c'),str(self.project/'src/stream_io.c'),str(FIXTURE/'adapter.c'),'-o',str(binary)]
            if os.name!='nt':command+=['-fsanitize=undefined','-fno-sanitize-recover=all']
            result=subprocess.run(command,capture_output=True,text=True,timeout=60)
            self.assertEqual(result.returncode,0,result.stderr)
            result=subprocess.run([str(binary)],capture_output=True,text=True,timeout=30)
            self.assertEqual(result.returncode,0,result.stderr)

class StreamingArtTests(unittest.TestCase):
    def test_native_indexed_pages_tile_order_palettes_reproduction_and_standalone_copy(self):
        from test_localization import pixels
        module=load('streaming_art',LAB/'tools/stream_art.py');prepare=load('streaming_art_prepare',LAB/'tools/prepare.py')
        assets=module.assets();self.assertEqual(assets,module.assets())
        self.assertEqual(len(assets['pages.bin']),24*512);self.assertEqual(len(assets['hero.bin']),256)
        self.assertEqual(len(set(assets['pages.bin'][i:i+512] for i in range(0,12288,512))),24)
        for z in range(4):
            w,h,rows=pixels(assets[f'area-{z}.png']);self.assertEqual((w,h),(192,32))
            self.assertLess(max(v for row in rows for v in row),16)
            for page in range(6):
                native=[v for row in rows for v in row[page*32:(page+1)*32]]
                self.assertEqual(module.tiles(native,32,32),assets['pages.bin'][(z*6+page)*512:(z*6+page+1)*512])
        for palette in module.COLORS+[module.HERO]:
            self.assertEqual(len(palette),16);self.assertTrue(all(0<=c<=7 for rgb in palette for c in rgb))
        self.assertNotEqual(module.tiles(module.hero(0),16,16),module.tiles(module.hero(0),16,16,True))
        with tempfile.TemporaryDirectory(prefix='stream art ') as scratch:
            first=prepare.prepare(Path(scratch)/'first')
            with self.assertRaises(ValueError):prepare.prepare(first)
            result=subprocess.run([sys.executable,str(first/'tools/prepare.py'),str(Path(scratch)/'second')],capture_output=True,text=True,timeout=30)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertEqual((first/'res/pages.bin').read_bytes(),(Path(scratch)/'second/res/pages.bin').read_bytes())
        with self.assertRaises(ValueError):prepare.prepare(LAB/'src/forbidden output')

@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class StreamingContainerTests(unittest.TestCase):
    def test_release_debug_resources_and_cache_rules_execute_as_base_m68000(self):
        output=ROOT/'build/streaming-integration';output.mkdir(parents=True,exist_ok=True)
        prepare=load('streaming_container_prepare',LAB/'tools/prepare.py')
        with tempfile.TemporaryDirectory(prefix='stream container ',dir=ROOT/'build') as scratch:
            project=prepare.prepare(Path(scratch)/'independent streaming lab')
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                result=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                self.assertEqual(result.returncode,0,(result.stdout+result.stderr)[-4000:]);return result.stdout+result.stderr
            hashes={};geometry=None
            for kind in ['rom','probe']:
                if kind=='probe':
                    shutil.copyfile(FIXTURE/'probe.c',project/'src/main.c');shutil.copyfile(FIXTURE/'checks.h',project/'inc/checks.h')
                for mode in ['release','debug']:
                    run('make','clean');log=run('make',mode);(output/f'{kind}-{mode}.log').write_text(log)
                    self.assertNotIn('warning:',log)
                    rom=(project/'out/rom.bin').read_bytes();self.assertEqual(len(rom),131072)
                    self.assertEqual(rom[0x100:0x104],b'SEGA');self.assertEqual(struct.unpack_from('>I',rom,0x1A4)[0],len(rom)-1)
                    hashes[kind+'-'+mode]=hashlib.sha256(rom).hexdigest()
                    if kind=='probe':
                        code,values=execute(rom,run('m68k-elf-nm','out/rom.out'))
                        self.assertEqual(code,0,f'Target contract failed at {code}')
                        geometry=values[:4];self.assertEqual(geometry,[492,8,18,8])
                        art=load('streaming_probe_art',LAB/'tools/stream_art.py')
                        colors=[(b<<9)|(g<<5)|(r<<1) for palette in art.COLORS+[art.HERO] for r,g,b in palette]
                        self.assertEqual(values[4:84],colors)
                        data=b''.join(v.to_bytes(2,'big') for v in values[84:])
                        self.assertEqual(data,(project/'res/pages.bin').read_bytes()+(project/'res/hero.bin').read_bytes())
            shutil.copyfile(LAB/'src/main.c',project/'src/main.c')
            run('make','clean');log=run('make','release','EXTRA_FLAGS=-DSC_AUTOPLAY')
            (output/'demo-release.log').write_text(log);self.assertNotIn('warning:',log)
            hashes['demo-release']=hashlib.sha256((project/'out/rom.bin').read_bytes()).hexdigest()
            (output/'streaming-report.json').write_text(json.dumps({'passed':True,'hashes':hashes,'geometry':geometry,
                'budgets':{'catalog_tiles':384,'cache_tiles':288,'resident_tiles':9,'pattern_bytes_per_frame':256,'frame_dma_allowance':1536},
                'execution':'Base M68000 cache contracts and asset bytes; no VDP timing or physical mapper emulation'},indent=2)+'\n')

if __name__=='__main__':unittest.main()
