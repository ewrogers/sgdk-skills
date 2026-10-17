"""Installed streaming/audio composition with rejected queues and deferred playback."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from test_streaming import load

ROOT=Path(__file__).resolve().parents[1]
FIXTURE=ROOT/'tests/fixtures/stream-audio'
PREPARE=load('stream_audio_prepare',FIXTURE/'prepare.py')

def installed_copy(directory):
    skills=Path(directory)/'.agents/skills'
    for name in PREPARE.SKILLS:shutil.copytree(ROOT/'skills'/name,skills/name,ignore=shutil.ignore_patterns('__pycache__'))
    return skills

def compile_host(project,directory,mode):
    cc=next((p for n in ('clang','cc','gcc')if(p:=shutil.which(n))),None)
    if not cc:
        if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
        raise unittest.SkipTest('C17 compiler required')
    binary=Path(directory)/('route'+mode+('.exe'if os.name=='nt'else''))
    args=[cc,'-DAD_HOST','-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(project/'inc'),'-I',str(FIXTURE)]
    args += [str(project/'src'/(name+'.c'))for name in ('stream_cache','stream_world','director','route','view')]
    args += [str(FIXTURE/'host.c'),'-o',str(binary)]
    if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
    r=subprocess.run(args,capture_output=True,text=True,timeout=60)
    if r.returncode:raise RuntimeError(r.stderr)
    r=subprocess.run([str(binary)],capture_output=True,text=True,timeout=60)
    if r.returncode:raise RuntimeError(r.stderr)
    return [int(n)for n in r.stdout.split()]

def host_project(directory):
    skills=installed_copy(directory);project=Path(directory)/'host project'
    (project/'src').mkdir(parents=True);(project/'src/main.c').write_text('/* host placeholder */')
    PREPARE.populate(project,skills)
    return project

class StreamAudioContracts(unittest.TestCase):
    def test_bounds_visibility_reversal_warp_rejected_black_pause_jingle_and_dead_driver(self):
        with tempfile.TemporaryDirectory(prefix='route composition ')as scratch:
            project=host_project(scratch)
            self.assertEqual(compile_host(project,scratch,'-O0'),compile_host(project,scratch,'-O3'))

@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class StreamAudioContainer(unittest.TestCase):
    def test_installed_modules_real_resources_regions_and_m68000(self):
        from test_streaming import execute
        out=ROOT/'build/stream-audio-integration';out.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='stream audio composed ',dir=ROOT/'build')as scratch:
            scratch=Path(scratch);skills=installed_copy(scratch);helper=skills/'sgdk-project/scripts/sgdk.py'
            project=scratch/'route project';base=[sys.executable,str(helper)]
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            def run(arguments,name,timeout=240):
                with(out/(name+'.log')).open('w')as log:r=subprocess.run(arguments,stdout=log,stderr=subprocess.STDOUT,timeout=timeout)
                text=(out/(name+'.log')).read_text();self.assertEqual(r.returncode,0,text[-3500:]);return text
            run(base+['init',str(project)],'init');PREPARE.populate(project,skills)
            trace=compile_host(project,scratch,'-O3');hashes={};geometry=[]
            header_path=project/'src/boot/rom_head.c';header=header_path.read_text()
            header=re.sub(r'"SAMPLE PROGRAM\s*"','"'+('STREAM AND SOUND CHECK'.ljust(48))+'"',header)
            for kind in ('U','E','probe'):
                region=kind if kind!='probe'else'U'
                source=re.sub(r'"JUE\s*"','"'+region.ljust(16)+'"',header)
                if kind=='probe':
                    shutil.copyfile(FIXTURE/'probe.c',project/'src/main.c');shutil.copyfile(FIXTURE/'checks.h',project/'inc/checks.h')
                for mode in ('release','debug'):
                    header_path.write_text(source)
                    run(base+['--project',str(project),'build','clean'],kind+'-'+mode+'-clean')
                    log=run(base+['--project',str(project),'build',mode],kind+'-'+mode)
                    size=(project/'out/rom.bin').stat().st_size
                    header_path.write_text(re.sub('0x000FFFFF',f'0x{size-1:08X}',source,flags=re.I))
                    log+=run(base+['--project',str(project),'build',mode],kind+'-'+mode+'-header')
                    self.assertEqual([s for s in log.splitlines()if 'warning:'in s],[])
                    data=(project/'out/rom.bin').read_bytes();hashes[kind+'-'+mode]=hashlib.sha256(data).hexdigest()
                    release=load('route_release',skills/'sgdk-release/scripts/release.py');report=release.inspect_rom(data)
                    self.assertTrue(report['valid']);self.assertEqual(report['findings'],[])
                    self.assertEqual(report['header']['region'],region)
                    if kind=='probe':
                        symbols=run(base+['--project',str(project),'run','m68k-elf-nm','--defined-only','/workspace/out/rom.out'],'symbols-'+mode)
                        code,values=execute(data,symbols,instruction_budget=200000000)
                        self.assertEqual(code,0,f'M68000 contract failed at line {code}')
                        self.assertEqual(values[:4],[trace[0]>>16,trace[0]&65535,trace[1]>>16,trace[1]&65535]);geometry=values[4:]
            (out/'stream-audio-report.json').write_text(json.dumps({'passed':True,'hashes':hashes,'trace':trace,'target_layout':geometry,
                'skills':list(PREPARE.SKILLS),'limits':{'tile_upload_bytes':256,'slow_tile_bytes':64,'frame_queue_bytes':1536,
                'hardware_sprites':1,'palette_banks':4,'cache_slots':18},
                'evidence':'Installed cache/director with new coordinator; host full-view and M68000 sampled-view contracts; not VDP/Z80 emulation',
                'physical_hardware':'not tested'},indent=2)+'\n')
