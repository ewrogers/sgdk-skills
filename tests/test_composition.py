"""Cross-skill courier evaluation: new game, actual installed helpers, both targets."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from test_streaming import load

ROOT=Path(__file__).resolve().parents[1]
FIXTURE=ROOT/'tests/fixtures/composition'
USED=('sgdk-project','sgdk-planning','sgdk-assets','sgdk-input','sgdk-gameplay',
      'sgdk-graphics','sgdk-ui','sgdk-audio','sgdk-scenes','sgdk-testing','sgdk-release')

def host_trace(directory,mode):
    cc=next((p for n in ('clang','cc','gcc')if(p:=shutil.which(n))),None)
    if not cc:
        if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
        raise unittest.SkipTest('C17 compiler required')
    binary=Path(directory)/('courier'+mode+('.exe'if os.name=='nt'else''))
    command=[cc,'-DCOURIER_HOST','-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(FIXTURE),
             str(FIXTURE/'model.c'),str(FIXTURE/'host.c'),'-o',str(binary)]
    if os.name!='nt':command+=['-fsanitize=undefined','-fno-sanitize-recover=all']
    r=subprocess.run(command,capture_output=True,text=True,timeout=60)
    if r.returncode:raise RuntimeError(r.stderr)
    r=subprocess.run([str(binary)],capture_output=True,text=True,timeout=30)
    if r.returncode:raise RuntimeError(r.stderr)
    return int(r.stdout.strip())

class CompositionContracts(unittest.TestCase):
    def test_gate_delivery_held_input_pause_and_equal_region_time(self):
        with tempfile.TemporaryDirectory(prefix='courier contracts ')as scratch:
            self.assertEqual(host_trace(scratch,'-O0'),host_trace(scratch,'-O3'))

    def test_assets_have_native_dimensions_and_transparent_actor_only(self):
        from test_localization import pixels
        art=load('composition_art',FIXTURE/'prepare.py')
        assets=art.assets();self.assertEqual(assets,art.assets())
        for name,size in [('court.png',(40,8)),('courier.png',(16,16))]:
            w,h,rows=pixels(assets[name]);self.assertEqual((w,h),size)
            values={v for row in rows for v in row};self.assertLess(max(values),16)
            self.assertEqual(0 in values,name=='courier.png')

@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class CompositionContainer(unittest.TestCase):
    def test_installed_project_assets_regions_target_contracts_and_release(self):
        from test_streaming import execute
        out=ROOT/'build/composition-integration';out.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='courier composed ',dir=ROOT/'build')as scratch:
            scratch=Path(scratch);installed=scratch/'.agents/skills'
            # Actual CLI copying/discovery is tested separately, with telemetry off.
            # Here an identical project-local installation is isolated from the repo.
            for name in USED:shutil.copytree(ROOT/'skills'/name,installed/name,ignore=shutil.ignore_patterns('__pycache__'))
            helper=installed/'sgdk-project/scripts/sgdk.py';release=installed/'sgdk-release/scripts/release.py'
            project=scratch/'new courier game';trace=host_trace(scratch,'-O3')
            def run(command,name,timeout=240):
                with (out/(name+'.log')).open('w')as log:
                    r=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,timeout=timeout)
                self.assertEqual(r.returncode,0,(out/(name+'.log')).read_text()[-3500:])
                return (out/(name+'.log')).read_text()
            base=[sys.executable,str(helper)]
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            run(base+['init',str(project)],'init')
            prepare=load('composition_prepare',FIXTURE/'prepare.py');prepare.populate(project)
            budget=json.loads(run([sys.executable,str(installed/'sgdk-planning/scripts/budget_check.py'),
                                   str(project/'budget.json'),'--json'],'budget'))
            self.assertEqual(budget['status'],'incomplete')
            self.assertFalse(any(c['over_limit']for c in budget['checks']))
            header_path=project/'src/boot/rom_head.c';header=header_path.read_text()
            # Set a genuine project identity and exact expected padded 128-KiB range.
            import re
            header=re.sub(r'"SAMPLE PROGRAM\s*"','"COURIER COMPOSITION CHECK"',header)
            header=re.sub(r'0x000FFFFF', '0x0001FFFF',header,flags=re.I)
            self.assertRegex(header,r'"JUE\s*"')
            hashes={};regions={};checks={}
            for region in ('U','E'):
                header_path.write_text(re.sub(r'"JUE\s*"','"'+region.ljust(16)+'"',header))
                run(base+['--project',str(project),'build','clean'],region+'-clean')
                for mode in ('release','debug'):
                    # Each mode has a separate output tree, but rescomp shares intermediates.
                    log=run(base+['--project',str(project),'build',mode],region+'-'+mode)
                    self.assertEqual([line for line in log.splitlines()if 'warning:'in line],[])
                    data=(project/'out/rom.bin').read_bytes()
                    hashes[region+'-'+mode]=hashlib.sha256(data).hexdigest()
                    rom=project/(region+'-'+mode+'.bin');rom.write_bytes(data)
                    output=run([sys.executable,str(release),'inspect',str(rom),'--json'],region+'-'+mode+'-inspect')
                    details=json.loads(output);self.assertTrue(details['valid']);self.assertEqual(details['findings'],[])
                    self.assertEqual(details['header']['region'],region)
                    self.assertEqual(data[0x100:0x110],b'SEGA MEGA DRIVE ')
                    self.assertEqual(data[0x1f0:0x200],region.ljust(16).encode())
                    regions[region+'-'+mode]=details
            # Exercise the installed release helper's real artifact handoff.
            shutil.copyfile(ROOT/'LICENSE',project/'LICENSE.txt')
            run([sys.executable,str(release),'package','--rom',str(project/'U-release.bin'),'--output',str(project/'release.zip'),
                 '--name','Courier Check','--version','0.1.0','--readme',str(project/'README.txt'),
                 '--license',str(project/'LICENSE.txt')],'package')
            hashes['package']=hashlib.sha256((project/'release.zip').read_bytes()).hexdigest()
            shutil.copyfile(FIXTURE/'probe.c',project/'src/main.c')
            shutil.copyfile(FIXTURE/'checks.h',project/'inc/checks.h')
            for mode in ('release','debug'):
                run(base+['--project',str(project),'build','clean'],'probe-'+mode+'-clean')
                run(base+['--project',str(project),'build',mode],'probe-'+mode)
                folder=project/'out'
                symbols=run(base+['--project',str(project),'run','m68k-elf-nm','--defined-only','/workspace/out/rom.out'],'symbols-'+mode)
                data=(folder/'rom.bin').read_bytes();code,values=execute(data,symbols)
                self.assertEqual(code,0,f'M68000 contract failed at line {code}')
                self.assertEqual(values,[trace>>16,trace&65535]);checks[mode]=values
                hashes['probe-'+mode]=hashlib.sha256(data).hexdigest()
            report={'passed':True,'hashes':hashes,'trace_digest':trace,'target_checks':checks,
                    'scope':'New courier fixture composed from installed guidance; not a blind agent evaluation',
                    'skills':list(USED),'budget':budget,'physical_hardware':'not tested'}
            (out/'composition-report.json').write_text(json.dumps(report,indent=2)+'\n')
