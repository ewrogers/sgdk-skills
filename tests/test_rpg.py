"""RPG operations are executable C contracts, not assertions about documentation."""
from pathlib import Path
import os,shutil,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
LAB=ROOT/'skills/sgdk-rpg/assets/adventure-lab'
class RPGTests(unittest.TestCase):
    def test_party_battle_checkpoint_and_game_flow(self):
        cc=shutil.which('cc')or shutil.which('clang')or shutil.which('gcc')
        if not cc:self.skipTest('Host C compiler unavailable')
        with tempfile.TemporaryDirectory(prefix='RPG contracts ')as temp:
            executable=Path(temp)/('probe.exe'if os.name=='nt'else'probe')
            cmd=[cc,'-std=c17','-O2','-Wall','-Wextra','-Werror','-I',str(LAB/'inc')]
            cmd += [str(LAB/'src'/name)for name in ['party.c','battle.c','checkpoint.c','game.c','clock.c','layout.c']]
            cmd += [str(ROOT/'tests/fixtures/rpg/probe.c'),'-o',str(executable)]
            result=subprocess.run(cmd,capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            result=subprocess.run([str(executable)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
    def test_sram_journal_interrupted_writes_wrap_and_corrupt_latest(self):
        cc=shutil.which('cc')or shutil.which('clang')or shutil.which('gcc')
        if not cc:self.skipTest('Host C compiler unavailable')
        with tempfile.TemporaryDirectory(prefix='RPG SRAM ')as temp:
            executable=Path(temp)/('save.exe'if os.name=='nt'else'save')
            fixture=ROOT/'tests/fixtures/rpg'
            cmd=[cc,'-std=c17','-O2','-Wall','-Wextra','-Werror','-I',str(fixture/'save-stubs'),'-I',str(LAB/'inc')]
            cmd += [str(LAB/'src'/name)for name in ['party.c','checkpoint.c','save.c']]
            cmd += [str(fixture/'save_probe.c'),'-o',str(executable)]
            result=subprocess.run(cmd,capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            result=subprocess.run([str(executable)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)



def preparation():
    import importlib.util,sys
    sys.path.insert(0,str(LAB/'tools'))
    spec=importlib.util.spec_from_file_location('adventure_prepare',LAB/'tools/prepare.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module


class RPGArtTests(unittest.TestCase):
    def test_native_art_palette_roles_padding_and_reproduction(self):
        import json
        module=preparation()
        with tempfile.TemporaryDirectory(prefix='RPG native art ')as temp:
            first=Path(temp)/'first';second=Path(temp)/'second'
            report=module.prepare(first);self.assertEqual(report['title']['size'],[256,144])
            self.assertEqual(report['harbor']['size'],[320,176]);self.assertEqual(report['battle']['size'],[320,128])
            self.assertEqual(len(report['sprite_palette']),16)
            for name in ['mara','ivo','neri','crab','moth','warden']:
                w,h,source=module.read_png(LAB/'raw'/f'{name}.png');ww,hh,prepared=module.read_png(first/'res'/f'{name}.png')
                self.assertEqual((w,h),(ww,hh));self.assertEqual([p[3]>=128 for p in source],[p[3]>0 for p in prepared])
            w,h,idle=module.read_png(first/'res/field_idle.png');self.assertEqual((w,h),(160,40))
            # Padding stays transparent. The down coat is deliberately shortened,
            # while its generated head silhouette remains intact at native size.
            self.assertTrue(all(p[3]==0 for p in idle[:w*4]))
            module.prepare(second)
            self.assertEqual({p.name:p.read_bytes()for p in(first/'res').iterdir()},
                             {p.name:p.read_bytes()for p in(second/'res').iterdir()})
            with self.assertRaises(ValueError):module.prepare(first)
        ledger=json.loads((LAB/'requests.json').read_text());self.assertEqual(ledger['generations_spent'],14)


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class RPGContainerTests(unittest.TestCase):
    def test_release_debug_and_base_68000_rules_and_linked_budgets(self):
        import hashlib,json,re,struct
        from test_math import execute_sdk
        output=ROOT/'build/rpg-integration';output.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='RPG SDK ',dir=ROOT/'build')as temp:
            project=Path(temp)/'standalone adventure';preparation().prepare(project)
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                result=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                log=result.stdout+result.stderr
                self.assertEqual(result.returncode,0,log[-6000:]);return log
            hashes={};budgets=None
            for mode in ['release','debug']:
                run('make','clean');log=run('make',mode);(output/(mode+'.log')).write_text(log)
                self.assertNotIn('warning:',log)
                rom=bytearray((project/'out/rom.bin').read_bytes());self.assertEqual(rom[0x100:0x104],b'SEGA')
                struct.pack_into('>I',rom,0x1a4,len(rom)-1)
                self.assertEqual(rom[0x1b0:0x1b2],b'RA');self.assertLessEqual(len(rom),0x200000)
                hashes[mode]=hashlib.sha256(rom).hexdigest()
            for file in(project/'src').glob('*.c'):
                if file.name not in ['party.c','battle.c','checkpoint.c','game.c','clock.c','layout.c']:file.unlink()
            shutil.copyfile(ROOT/'tests/fixtures/rpg/probe.c',project/'src/main.c')
            for mode in ['release','debug']:
                run('make','clean');log=run('make',mode);(output/('probe-'+mode+'.log')).write_text(log)
                self.assertNotIn('warning:',log)
                rom=(project/'out/rom.bin').read_bytes();code,values=execute_sdk(rom,run('m68k-elf-nm','out/rom.out'))
                self.assertEqual(code,0,f'Base M68000 RPG check failed at line {code}')
                self.assertEqual(len(values),7)
                measured=dict(zip(['campaign_bytes','battle_bytes','game_bytes','reserved_tile_end',
                                   'hardware_sprites','scanline_sprites','scanline_pixels'],values))
                if budgets is not None:self.assertEqual(measured,budgets)
                budgets=measured;hashes['probe-'+mode]=hashlib.sha256(rom).hexdigest()
            (output/'rpg-report.json').write_text(json.dumps(dict(passed=True,hashes=hashes,budgets=budgets),indent=2)+'\n')


if __name__=='__main__':unittest.main()
