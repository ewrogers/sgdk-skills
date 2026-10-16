"""Software contracts and actual SGDK builds; no physical-device emulation."""
import binascii
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
LAB=ROOT/'skills/sgdk-hardware/assets/hardware-lab'
FIXTURE=ROOT/'tests/fixtures/hardware'

def load_prepare():
    spec=importlib.util.spec_from_file_location('hardware_prepare',LAB/'tools/prepare.py')
    module=importlib.util.module_from_spec(spec);sys.path.insert(0,str(LAB/'tools'))
    try:spec.loader.exec_module(module)
    finally:sys.path.pop(0)
    return module

def compile_host(cc,path,mode):
    args=[cc,'-DHL_HOST','-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'inc'),'-I',str(FIXTURE),str(LAB/'src/hardware.c'),str(FIXTURE/'host.c'),'-o',str(path)]
    if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
    r=subprocess.run(args,capture_output=True,text=True,timeout=60)
    if r.returncode:raise RuntimeError(r.stdout+r.stderr)

class HardwareTests(unittest.TestCase):
    def test_clock_fields_scanline_upload_and_crc_contracts(self):
        cc=next((p for n in ['clang','cc','gcc'] if(p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':self.fail('C17 compiler required')
            self.skipTest('C17 compiler required')
        body=b'HWL\x01\x12\x34\x56\x78\x55\xaa'
        expected=(body+binascii.crc_hqx(body,0xffff).to_bytes(2,'big')).hex()
        with tempfile.TemporaryDirectory(prefix='hardware contracts ') as scratch:
            for mode in ['-O0','-O3']:
                binary=Path(scratch)/('contracts'+('.exe' if os.name=='nt' else ''));compile_host(cc,binary,mode)
                r=subprocess.run([str(binary)],capture_output=True,text=True,timeout=30);self.assertEqual(r.returncode,0,r.stderr)
                self.assertEqual(subprocess.check_output([str(binary),'record'],text=True).strip(),expected)

    def test_standalone_copy_native_font_seamless_pcm_and_no_overwrite(self):
        prepare=load_prepare()
        self.assertEqual(len(prepare.font_words()),768)
        self.assertTrue(all((word>>shift)&15 in (1,15) for word in prepare.font_words() for shift in range(0,32,4)))
        with tempfile.TemporaryDirectory(prefix='hardware copy ') as scratch:
            project=prepare.prepare(Path(scratch)/'first copy')
            with self.assertRaises(ValueError):prepare.prepare(project)
            r=subprocess.run([sys.executable,str(project/'tools/prepare.py'),str(Path(scratch)/'second copy')],capture_output=True,text=True,timeout=30)
            self.assertEqual(r.returncode,0,r.stderr)
            for name in ['res/garden.vgm','res/reference.wav','inc/font.h']:
                self.assertEqual((project/name).read_bytes(),(Path(scratch)/'second copy'/name).read_bytes())
            import wave
            with wave.open(str(project/'res/reference.wav')) as wav:
                self.assertEqual((wav.getnchannels(),wav.getsampwidth(),wav.getframerate(),wav.getnframes()),(1,2,13300,256))
                import struct
                samples=struct.unpack('<256h',wav.readframes(256))
                self.assertEqual(sum(samples),0);self.assertEqual(samples[0],0)
                self.assertGreater(max(samples),7000)
                interior=max(abs(b-a) for a,b in zip(samples,samples[1:]))
                self.assertLessEqual(abs(samples[0]-samples[-1]),interior)
                self.assertLess(interior,1200)
        with self.assertRaises(ValueError):prepare.prepare(LAB/'new output')

@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class HardwareContainerTests(unittest.TestCase):
    def test_actual_rom_modes_regions_and_base_m68000_contracts(self):
        from test_streaming import execute
        prepare=load_prepare();out=ROOT/'build/hardware-integration';out.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='hardware target ',dir=ROOT/'build') as scratch:
            project=prepare.prepare(Path(scratch)/'standalone hardware lab')
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                r=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                self.assertEqual(r.returncode,0,(r.stdout+r.stderr)[-5000:]);return r.stdout+r.stderr
            hashes={};geometry=None
            for kind in ['rom','pal','ntsc','probe']:
                header=(LAB/'src/boot/rom_head.c').read_text()
                if kind in ('pal','ntsc'):header=header.replace('.region="JUE"','.region="'+('E' if kind=='pal' else 'U')+'"')
                (project/'src/boot/rom_head.c').write_text(header)
                if kind=='probe':
                    shutil.copyfile(FIXTURE/'probe.c',project/'src/main.c');shutil.copyfile(FIXTURE/'checks.h',project/'inc/checks.h')
                for mode in (['release','debug'] if kind in ('rom','probe') else ['release']):
                    run('make','clean');log=run('make',mode);(out/f'{kind}-{mode}.log').write_text(log);self.assertEqual([line for line in log.splitlines() if 'warning:' in line],[])
                    rom=(project/'out/rom.bin').read_bytes();self.assertEqual(len(rom),131072)
                    self.assertEqual(rom[0x1b0:0x1b4],b'RA\xf8\x20')
                    self.assertEqual(int.from_bytes(rom[0x1b4:0x1b8],'big'),0x200001)
                    self.assertEqual(int.from_bytes(rom[0x1b8:0x1bc],'big'),0x20ffff)
                    self.assertEqual(rom[0x1f0:0x1f3].rstrip(b'\0 '),{'pal':b'E','ntsc':b'U'}.get(kind,b'JUE'))
                    hashes[kind+'-'+mode]=hashlib.sha256(rom).hexdigest()
                    if kind=='probe':
                        code,values=execute(rom,run('m68k-elf-nm','--defined-only','out/rom.out'));self.assertEqual(code,0,f'Target contract failed at {code}')
                        geometry=values[:3];self.assertEqual(geometry[0],16);self.assertGreater(geometry[1],256);self.assertEqual(geometry[2],256)
                        body=b'HWL\x01\x12\x34\x56\x78\x55\xaa';record=body+binascii.crc_hqx(body,0xffff).to_bytes(2,'big')
                        self.assertEqual(values[3:15],list(record))
                        self.assertEqual(values[15:],[half for word in prepare.font_words() for half in (word>>16,word&65535)])
            (out/'hardware-report.json').write_text(json.dumps({'passed':True,'hashes':hashes,'geometry':geometry,
                'execution':'Base M68000 helper contracts; boot, VDP, Z80, I/O and SRAM devices not emulated',
                'physical_hardware':'not tested'},indent=2)+'\n')

if __name__=='__main__':unittest.main()
