"""A playable PixelLab catalog: state transitions, native art and real SDK ROMs."""
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
LAB=ROOT/'skills/sgdk-pixellab/assets/asterfall'


def preparation():
    sys.path.insert(0,str(LAB/'tools'))
    spec=importlib.util.spec_from_file_location('asterfall_prepare',LAB/'tools/prepare.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    return module


class AsterfallTests(unittest.TestCase):
    def test_playable_flow_and_collision_with_host_c(self):
        cc=shutil.which('cc')or shutil.which('clang')or shutil.which('gcc')
        if not cc:self.skipTest('Host C compiler unavailable')
        with tempfile.TemporaryDirectory(prefix='asterfall logic ')as temp:
            executable=Path(temp)/('probe.exe'if os.name=='nt'else'probe')
            result=subprocess.run([cc,'-std=c17','-Wall','-Wextra','-Werror','-I',str(LAB/'inc'),str(LAB/'src/game.c'),str(LAB/'src/field.c'),str(ROOT/'tests/fixtures/asterfall/game_probe.c'),'-o',str(executable)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            result=subprocess.run([str(executable)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)

    def test_native_preparation_resource_roles_and_generation_ceiling(self):
        module=preparation()
        ledger=json.loads((LAB/'requests.json').read_text());self.assertLessEqual(ledger['generations_spent'],200)
        with tempfile.TemporaryDirectory(prefix='asterfall art ')as temp:
            project=Path(temp)/'new project';report=module.prepare(project)
            self.assertEqual(report['font']['source_size'],[64,80]);self.assertEqual(len(report['font']['glyphs']),80)
            self.assertEqual(len(report['actor_palette']),16)
            # Every generated glyph keeps its exact alpha silhouette, despite
            # the service returning a different atlas orientation than documented.
            fw,fh,source=module.read_png(LAB/'raw/font.png')
            w,h,prepared=module.read_png(project/'res/font_ivory.png')
            ink=tuple(bytes.fromhex(module.UI[15]))
            for glyph,char in enumerate(module.GLYPHS):
                for y in range(8):
                    for x in range(8):
                        self.assertEqual(source[((glyph//8)*8+y)*fw+(glyph%8)*8+x][3]>=128,
                            prepared[y*w+(ord(char)-32)*8+x][:3]==ink)

            for name in ['town','forest','coast','dungeon']:
                self.assertEqual(report[name]['native_size'],[320,192])
                self.assertEqual([b['slot']for b in report[name]['banks']],[0,1])
            for name in ['hero','guide','sage']:
                self.assertEqual(report[name+'_walk']['native_size'],[192,192])
                self.assertEqual(report[name+'_idle']['native_size'],[192,48])
            self.assertEqual(report['intro_sky_cleanup']['region'],[55,18,266,30])
            self.assertGreater(report['tonic_cleanup']['changed_pixels'],0)
            _,_,tonic=module.read_png(project/'res/icon_potion.png')
            self.assertGreater(sum(p[3]>0 and p[1]>p[0] and p[1]>p[2] for p in tonic),3)
            with self.assertRaises(ValueError):module.prepare(project)


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class AsterfallContainerTests(unittest.TestCase):
    def test_real_catalog_rom_and_logic_probe_in_both_modes(self):
        import hashlib
        from test_math import execute_sdk
        output=ROOT/'build/asterfall-integration';output.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='asterfall SDK ',dir=ROOT/'build')as temp:
            project=Path(temp)/'project';preparation().prepare(project)
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                result=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                self.assertEqual(result.returncode,0,(result.stdout+result.stderr)[-4000:]);return result.stdout+result.stderr
            hashes={}
            for mode in ['release','debug']:
                run('make','clean');log=run('make',mode);(output/f'{mode}.log').write_text(log)
                self.assertNotIn('warning:',log)
                rom=(project/'out/rom.bin').read_bytes();self.assertEqual(rom[0x100:0x104],b'SEGA')
                hashes[mode]=hashlib.sha256(rom).hexdigest()
            # Keep the full resource set linked into the regular ROM above; the
            # separate direct-entry probe executes gameplay logic without VDP/audio.
            for file in(project/'src').glob('*.c'):
                if file.name not in ['game.c','field.c']:file.unlink()
            shutil.copyfile(ROOT/'tests/fixtures/asterfall/game_probe.c',project/'src/main.c')
            for mode in ['release','debug']:
                run('make','clean');run('make',mode)
                rom=(project/'out/rom.bin').read_bytes();code,_=execute_sdk(rom,run('m68k-elf-nm','out/rom.out'))
                self.assertEqual(code,0,f'Playable flow probe failed at line {code}')
                hashes['probe-'+mode]=hashlib.sha256(rom).hexdigest()
            (output/'asterfall-report.json').write_text(json.dumps(dict(passed=True,hashes=hashes),indent=2)+'\n')


if __name__=='__main__':unittest.main()
