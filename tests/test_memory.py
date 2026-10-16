"""Memory ownership tests, ELF accounting and actual target allocator contracts."""
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
LAB=ROOT/'skills/sgdk-memory/assets/memory-lab'
FIXTURE=ROOT/'tests/fixtures/memory'

def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module);return module

class MemoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cls.cc:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
            raise unittest.SkipTest('C17 compiler required')
        cls.scratch=tempfile.TemporaryDirectory(prefix='memory contracts ');cls.addClassCleanup(cls.scratch.cleanup)
        cls.binaries=[]
        for mode in ['-O0','-O3']:
            binary=Path(cls.scratch.name)/(mode+('.exe' if os.name=='nt' else ''))
            command=[cls.cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(FIXTURE/'stubs'),'-I',str(FIXTURE),
                '-I',str(LAB/'inc'),str(LAB/'src/memory_lab.c'),str(FIXTURE/'host.c'),'-o',str(binary)]
            if os.name!='nt':command+=['-fsanitize=undefined','-fno-sanitize-recover=all']
            result=subprocess.run(command,capture_output=True,text=True,timeout=60)
            if result.returncode:raise RuntimeError(result.stdout+result.stderr)
            cls.binaries.append(binary)

    def case(self,kind):
        for binary in self.binaries:
            result=subprocess.run([str(binary),kind],capture_output=True,text=True,timeout=60)
            self.assertEqual(result.returncode,0,result.stderr)

    def test_holes_block_large_requests_packing_preserves_live_bytes_and_cleanup_restores_capacity(self):self.case('fragments')
    def test_delayed_dma_keeps_staging_and_room_alive_until_completion(self):self.case('lifetimes')
    def test_each_allocation_failure_rolls_back_without_losing_owned_resources(self):self.case('failures')
    def test_500_heap_and_room_cycles_restore_free_and_largest_block(self):self.case('soak')

class AccountingTests(unittest.TestCase):
    def setUp(self):
        self.module=load('memory_account',LAB/'tools/account.py')
        self.sections='.data 76 3774808064\n.bss 1570 3774808140\n'
        self.symbols='e0ff066e B _bend\n'

    def test_static_heap_and_stack_account_for_exactly_64k_without_double_counting(self):
        r=self.module.account(self.sections,self.symbols,2560)
        self.assertEqual(r['static_ram_bytes'],1646)
        self.assertEqual(sum(r[k] for k in ['static_ram_bytes','heap_alignment_bytes','heap_capacity_bytes',
            'heap_terminator_bytes','stack_reserved_bytes']),65536)
        self.assertEqual(r['heap_capacity_bytes'],61328)
        self.assertIn('65,536',self.module.table(r))
        self.assertEqual(self.module.account('.data 2 16711680\n.bss 3 16711682\n','00ff0005 B _bend\n',2560)['heap_alignment_bytes'],1)

    def test_reject_missing_symbols_overlapping_sections_and_excess_static_ram(self):
        for sections,symbols,stack in [('',self.symbols,2560),(self.sections,'',2560),
            (self.sections,self.symbols,1),(self.sections,self.symbols,65536),
            (self.sections+'.data 1 0\n',self.symbols,2560),
            ('.data 100 3774808064\n.bss 100 3774808065\n',self.symbols,2560),
            (self.sections,'e0fff800 B _bend\n',2560),
            (self.sections,'00001000 B _bend\n',2560)]:
            with self.subTest(sections=sections,symbols=symbols,stack=stack):
                with self.assertRaises(ValueError):self.module.account(sections,symbols,stack)

    def test_prepare_copies_standalone_project_and_refuses_overwrite_or_source_output(self):
        module=load('memory_prepare',LAB/'tools/prepare.py')
        with tempfile.TemporaryDirectory(prefix='memory standalone ') as scratch:
            first=module.prepare(Path(scratch)/'first')
            with self.assertRaises(ValueError):module.prepare(first)
            result=subprocess.run([sys.executable,str(first/'tools/prepare.py'),str(Path(scratch)/'second')],capture_output=True,text=True,timeout=30)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertEqual((first/'src/memory_lab.c').read_bytes(),(Path(scratch)/'second/src/memory_lab.c').read_bytes())
        with self.assertRaises(ValueError):module.prepare(LAB/'src/new output')

@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class MemoryContainerTests(unittest.TestCase):
    def test_actual_sgdk_allocator_on_base_m68000_and_both_rom_build_modes(self):
        from test_streaming import execute
        import hashlib
        output=ROOT/'build/memory-integration';output.mkdir(parents=True,exist_ok=True)
        prepare=load('memory_container_prepare',LAB/'tools/prepare.py')
        with tempfile.TemporaryDirectory(prefix='memory container ',dir=ROOT/'build') as scratch:
            project=prepare.prepare(Path(scratch)/'standalone memory lab')
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                result=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                self.assertEqual(result.returncode,0,(result.stdout+result.stderr)[-4000:]);return result.stdout+result.stderr
            hashes={};geometry=None;accounting={}
            for kind in ['rom','probe']:
                if kind=='probe':
                    shutil.copyfile(FIXTURE/'probe.c',project/'src/main.c');shutil.copyfile(FIXTURE/'checks.h',project/'inc/checks.h')
                for mode in ['release','debug']:
                    run('make','clean');log=run('make',mode);(output/f'{kind}-{mode}.log').write_text(log)
                    self.assertNotIn('warning:',log)
                    rom=(project/'out/rom.bin').read_bytes();self.assertEqual(len(rom),131072)
                    hashes[kind+'-'+mode]=hashlib.sha256(rom).hexdigest()
                    symbols=run('m68k-elf-nm','--defined-only','out/rom.out')
                    if kind=='rom':
                        module=load('memory_target_account',LAB/'tools/account.py')
                        accounting[mode]=module.account(run('m68k-elf-size','-A','out/rom.out'),symbols,2560)
                        self.assertEqual(module.inspect(project/'out/rom.out',os.environ.get('SGDK_PLATFORM')),accounting[mode])
                    else:
                        code,values=execute(rom,symbols)
                        self.assertEqual(code,0,f'Actual allocator contract failed at {code}')
                        geometry=values;self.assertEqual(len(values),6)
                        self.assertEqual(values[1],2560);self.assertEqual(values[3:], [0,1024,128])
            (output/'memory-report.json').write_text(json.dumps({'passed':True,'hashes':hashes,'geometry':geometry,
                'accounting':accounting,'execution':'Actual SGDK allocator and Lab lifetimes on base M68000; no VDP timing or stack-peak emulation'},indent=2)+'\n')

if __name__=='__main__':unittest.main()
