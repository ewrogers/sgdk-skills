"""Compression ownership, reproducible art and real SDK decoder contracts."""
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
LAB=ROOT/'skills/sgdk-compression/assets/compression-lab'
FIXTURE=ROOT/'tests/fixtures/compression'

def load():
    spec=importlib.util.spec_from_file_location('compression_prepare',LAB/'tools/prepare.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module

class CompressionTests(unittest.TestCase):
    def test_loader_owns_pending_sources_retries_without_redecode_and_never_publishes_early(self):
        cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':self.fail('C17 compiler required')
            self.skipTest('C17 compiler required')
        with tempfile.TemporaryDirectory(prefix='compression contracts ') as scratch:
            for mode in ['-O0','-O3']:
                binary=Path(scratch)/('host'+mode+('.exe' if os.name=='nt' else ''))
                command=[cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'inc'),str(LAB/'src/loader.c'),str(FIXTURE/'host.c'),'-o',str(binary)]
                if os.name!='nt':command+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                result=subprocess.run(command,capture_output=True,text=True,timeout=60);self.assertEqual(result.returncode,0,result.stderr)
                result=subprocess.run([str(binary)],capture_output=True,text=True,timeout=60);self.assertEqual(result.returncode,0,result.stderr)

    def test_native_art_entropy_control_chunks_and_standalone_preparation(self):
        m=load();data=[m.tiles(m.pixels(i)) for i in range(2)]
        self.assertEqual([len(d) for d in data],[4096,4096]);self.assertNotEqual(*data)
        self.assertEqual(data,[m.tiles(m.pixels(i)) for i in range(2)])
        self.assertGreater(len(set(data[1])),240)
        for sample in range(2):
            decoded=[[0]*128 for _ in range(64)]
            for tile in range(128):
                for y in range(8):
                    for pair in range(4):
                        byte=data[sample][tile*32+y*4+pair]
                        x=(tile%16)*8+pair*2;row=(tile//16)*8+y
                        decoded[row][x]=byte>>4;decoded[row][x+1]=byte&15
            self.assertEqual(decoded,m.pixels(sample))
        self.assertTrue(all(len(rgb)==3 and all(0<=v<=7 for v in rgb) for rgb in m.COLORS))
        from test_localization import pixels
        for i in range(2):
            w,h,rows=pixels(m.png(m.pixels(i)));self.assertEqual((w,h),(128,64));self.assertEqual([list(row) for row in rows],m.pixels(i))
        with tempfile.TemporaryDirectory(prefix='compression standalone ') as scratch:
            first=m.prepare(Path(scratch)/'first')
            with self.assertRaises(ValueError):m.prepare(first)
            result=subprocess.run([sys.executable,str(first/'tools/prepare.py'),str(Path(scratch)/'second')],capture_output=True,text=True,timeout=30)
            self.assertEqual(result.returncode,0,result.stderr)
            for sample in range(2):
                joined=b''.join((first/f'res/chunk{sample}_{part}.bin').read_bytes() for part in range(8))
                self.assertEqual(joined,data[sample]);self.assertEqual((first/f'res/picture{sample}.bin').read_bytes(),joined)
            self.assertEqual((first/'src/catalog.c').read_bytes(),(Path(scratch)/'second/src/catalog.c').read_bytes())
        with self.assertRaises(ValueError):m.prepare(LAB/'src/forbidden')

@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class CompressionContainerTests(unittest.TestCase):
    def test_real_codecs_all_bytes_guards_and_capacity_on_base_m68000(self):
        from test_streaming import execute
        output=ROOT/'build/compression-integration';output.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='compression container ',dir=ROOT/'build') as scratch:
            project=load().prepare(Path(scratch)/'standalone lab')
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                result=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                self.assertEqual(result.returncode,0,(result.stdout+result.stderr)[-5000:]);return result.stdout+result.stderr
            hashes={};values=None;static_ram={}
            for mode,flags in [('release','-DCL_REGION_U'),('release','-DCL_REGION_E'),('debug','-DCL_REGION_U')]:
                # ROM header region flags are generated by the standard SDK header source.
                region='E' if flags.endswith('_E') else 'U'
                (project/'src/boot/rom_head.c').write_text(load().header(region))
                run('make','clean');log=run('make',mode);key=mode+'-'+region
                (output/(key+'.log')).write_text(log);self.assertNotIn('warning:',log)
                rom=(project/'out/rom.bin').read_bytes();hashes[key]=hashlib.sha256(rom).hexdigest()
                self.assertEqual(rom[0x1F0:0x1F1],region.encode())
                self.assertEqual(int.from_bytes(rom[0x1A4:0x1A8],'big')+1,len(rom))
                shutil.copyfile(project/'out/rom.bin',output/('compression-'+key+'.bin'))
                sections=run('m68k-elf-size','-A','out/rom.out')
                static_ram[key]=sum(int(line.split()[1]) for line in sections.splitlines()
                                    if line.split() and line.split()[0] in ('.data','.bss'))
            shutil.copyfile(FIXTURE/'probe.c',project/'src/main.c')
            for mode in ['release','debug']:
                run('make','clean');log=run('make',mode);(output/('probe-'+mode+'.log')).write_text(log)
                self.assertNotIn('warning:',log);rom=(project/'out/rom.bin').read_bytes()
                hashes['probe-'+mode]=hashlib.sha256(rom).hexdigest()
                code,got=execute(rom,run('m68k-elf-nm','out/rom.out'),instruction_budget=50000000)
                self.assertEqual(code,0,f'Actual SDK codec contract failed at {code}')
                self.assertEqual(len(got),23)
                if values is not None:self.assertEqual(values,got)
                values=got
            self.assertEqual(values[0],4096);self.assertEqual(values[11],4096)
            self.assertLess(values[1],4096);self.assertLess(values[2],4096)
            self.assertGreaterEqual(values[12],4096) # forced APLIB entropy can expand
            report={'passed':True,'hashes':hashes,'stored_bytes':{'mosaic':values[:3],'mosaic_chunks':values[3:11],
                'entropy':values[11:14],'entropy_chunks':values[14:22]},'loader_bytes':values[22],'static_ram_bytes':static_ram,
                'execution':'Actual SDK RAW/APLIB/LZ4W on base M68000; byte equality and guards; no VDP/timing emulation'}
            (output/'compression-report.json').write_text(json.dumps(report,indent=2)+'\n')

if __name__=='__main__':unittest.main()
