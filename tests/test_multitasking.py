"""Worker ownership and actual SDK decoder output; no cycle claims."""
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
LAB=ROOT/'skills/sgdk-multitasking/assets/task-lab'
FIXTURE=ROOT/'tests/fixtures/multitasking'

def module(path,name):
    spec=importlib.util.spec_from_file_location(name,path)
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def weather(variant):
    def noise(x,y,seed):
        v=(x*374761393+y*668265263+seed*1274126177)&0xffffffff
        v=((v^(v>>13))*1274126177)&0xffffffff
        return (v^(v>>16))>>24
    def field(x,y,spacing,seed):
        gx,fx=divmod(x,spacing);gy,fy=divmod(y,spacing)
        a=noise(gx,gy,seed)*(spacing-fx)+noise(gx+1,gy,seed)*fx
        b=noise(gx,gy+1,seed)*(spacing-fx)+noise(gx+1,gy+1,seed)*fx
        return (a*(spacing-fy)+b*fy)//(spacing*spacing)
    rows=[]
    for y in range(128):
        row=[]
        for x in range(128):
            terrain=(field(x,y,32,15)+field(x,y,16,21))//2
            c=4 if terrain>123 else 3 if terrain>110 else 2 if terrain>85 else 1
            if 119<=terrain<=123:c=5
            rain=max([0]+[255-((x-(24+i*37+(9 if variant else -3)))**2*2+(y-(30+(i*43)%83+(-8 if variant else 0)))**2*3)//8 for i in range(3)])
            rain+=field(x,y,16,45+variant*3)//3-42
            if rain>45:c=6+sum(rain>n for n in [90,135,180,215,240])
            elif x%32==0 or y%32==0:c=3
            row.append(c)
        rows.append(row)
    return module(LAB/'tools/prepare.py','mt_prepare').tiles(rows)

class MultitaskingTests(unittest.TestCase):
    def test_cancel_acknowledgement_buffer_loans_stack_guards_and_weather(self):
        cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':self.fail('C17 compiler required')
            self.skipTest('C17 compiler required')
        with tempfile.TemporaryDirectory(prefix='task contracts ') as scratch:
            for mode in ['-O0','-O3']:
                binary=Path(scratch)/('host'+mode+('.exe' if os.name=='nt' else ''))
                cmd=[cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'inc'),str(LAB/'src/work.c'),str(FIXTURE/'host.c'),'-o',str(binary)]
                if os.name!='nt':cmd+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                r=subprocess.run(cmd,capture_output=True,text=True,timeout=60);self.assertEqual(r.returncode,0,r.stderr)
                r=subprocess.run([str(binary)],capture_output=True,timeout=60);self.assertEqual(r.returncode,0,r.stderr)
                for variant in range(2):
                    r=subprocess.run([str(binary),str(variant)],capture_output=True,timeout=60)
                    self.assertEqual(r.returncode,0,r.stderr);self.assertEqual(r.stdout,weather(variant))
                    self.assertTrue(set(b>>4 for b in r.stdout).issuperset({3,4,5,6,7,8,9,10,11}))

    def test_standalone_preparation_and_native_palette(self):
        m=module(LAB/'tools/prepare.py','mt_prepare')
        for v in range(2):self.assertEqual(len(m.tiles(m.pixels(v))),8192)
        self.assertTrue(all(0<=c<=7 for rgb in m.COLORS for c in rgb))
        with tempfile.TemporaryDirectory(prefix='task standalone ') as scratch:
            first=m.prepare(Path(scratch)/'first')
            with self.assertRaises(ValueError):m.prepare(first)
            r=subprocess.run([sys.executable,str(first/'tools/prepare.py'),str(Path(scratch)/'second')],capture_output=True,text=True,timeout=30)
            self.assertEqual(r.returncode,0,r.stderr)
            self.assertEqual((first/'src/catalog.c').read_bytes(),(Path(scratch)/'second/src/catalog.c').read_bytes())
            for v in range(2):self.assertEqual((first/f'res/atlas{v}.bin').read_bytes(),m.tiles(m.pixels(v)))
        with self.assertRaises(ValueError):m.prepare(LAB/'src/forbidden')

@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class MultitaskingContainerTests(unittest.TestCase):
    def test_actual_sdk_decoder_and_weather_on_base_m68000(self):
        from test_streaming import execute
        m=module(LAB/'tools/prepare.py','mt_prepare')
        output=ROOT/'build/multitasking-integration';output.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='task container ',dir=ROOT/'build') as scratch:
            project=m.prepare(Path(scratch)/'standalone lab')
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                r=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                self.assertEqual(r.returncode,0,(r.stdout+r.stderr)[-5000:]);return r.stdout+r.stderr
            hashes={};ram={};results={}
            for mode,region in [('release','U'),('release','E'),('debug','U')]:
                (project/'src/boot/rom_head.c').write_text(m.header(region));run('make','clean');log=run('make',mode);key=mode+'-'+region
                (output/(key+'.log')).write_text(log);self.assertNotIn('warning:',log)
                rom=(project/'out/rom.bin').read_bytes();hashes[key]=hashlib.sha256(rom).hexdigest()
                self.assertEqual(rom[0x1f0:0x1f1],region.encode())
                shutil.copyfile(project/'out/rom.bin',output/('task-'+key+'.bin'))
                sizes=run('m68k-elf-size','-A','out/rom.out')
                ram[key]=sum(int(s.split()[1]) for s in sizes.splitlines() if s.split() and s.split()[0] in ('.data','.bss'))
            shutil.copyfile(FIXTURE/'probe.c',project/'src/main.c')
            for mode in ['release','debug']:
                run('make','clean');log=run('make',mode);self.assertNotIn('warning:',log)
                (output/('probe-'+mode+'.log')).write_text(log)
                rom=(project/'out/rom.bin').read_bytes();hashes['probe-'+mode]=hashlib.sha256(rom).hexdigest()
                shutil.copyfile(project/'out/rom.out',output/('probe-'+mode+'.out'))
                (output/('probe-'+mode+'.sym')).write_text(run('m68k-elf-nm','out/rom.out'))
                (output/('probe-'+mode+'.bin')).write_bytes(rom)
                code,got=execute(rom,run('m68k-elf-nm','out/rom.out'),instruction_budget=150000000)
                self.assertEqual(code,0,f'Worker probe failed at {code}');self.assertEqual(len(got),8195)
                decoded=b''.join(v.to_bytes(2,'big') for v in got[1:4097])
                generated=b''.join(v.to_bytes(2,'big') for v in got[4098:8194])
                self.assertEqual(decoded,m.tiles(m.pixels(1)));self.assertEqual(generated,weather(1))
                self.assertEqual(got[0],8);self.assertEqual(got[4097],128)
                results[mode]={'decode_passes':got[0],'weather_rows':got[4097],
                    'mailbox_bytes':got[-1],'output_sha256':hashlib.sha256(decoded+generated).hexdigest()}
            report={'passed':True,'hashes':hashes,'static_ram_bytes':ram,'results':results,
                'execution':'Actual SDK APLIB and pure worker steps on base M68000; task scheduling/VDP/timing require emulator or hardware'}
            (output/'multitasking-report.json').write_text(json.dumps(report,indent=2)+'\n')

if __name__=='__main__':unittest.main()
