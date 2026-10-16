"""Same-palette shadow/highlight conversion and real compiled mask encoding."""
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
STUDY=ROOT/'skills/sgdk-pixellab/assets/color-study'
sys.path.insert(0,str(STUDY.parents[1]/'scripts'))
import shadow_highlight as sh
from background_banks import validate_banks
from png_io import read_png
sys.path.pop(0)


def builder():
    spec=importlib.util.spec_from_file_location('shadow_builder',STUDY/'build_shadow_rom.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    return module


class ShadowHighlightTests(unittest.TestCase):
    def test_operator_indices_and_exact_additional_brightness(self):
        banks=validate_banks({'banks':[{'slot':0,'palette':['000000','ffffff']+['000000']*14}]})
        source=[(130,130,130,255)]*64
        normal=sh.prepare(8,8,source,banks,False)
        lit=sh.prepare(8,8,source,banks,True)
        self.assertGreater(normal[3]['rgb_squared_error'],0)
        self.assertEqual(lit[3]['rgb_squared_error'],0)
        self.assertEqual(set(lit[1]),{15}) # White base, shadow operator.
        self.assertEqual(set(lit[2]),{(130,130,130)})
        banks[0]['colors']=[(0,0,0)]*16
        lit=sh.prepare(8,8,source,banks,True)
        self.assertEqual(set(lit[1]),{14}) # Black base, highlight operator.
        normal=sh.prepare(8,8,[(0,0,0,255)]*64,banks,True)
        self.assertEqual(set(normal[1]),{0}) # Ties keep ordinary brightness.

    def test_each_tile_keeps_one_bank_and_best_per_pixel_mode_matches_oracle(self):
        banks=validate_banks({'banks':[{'slot':0,'palette':['000000','ff0000']*8},
                                      {'slot':1,'palette':['000000','0000ff']*8}]})
        source=[((i*19)%256,(i*31)%256,(i*67)%256,255)for i in range(128)]
        for lighting in [False,True]:
            indices,mask,preview,report=sh.prepare(16,8,source,banks,lighting)
            total=0
            for tx in [0,8]:
                positions=[y*16+tx+x for y in range(8)for x in range(8)]
                options=[]
                for bank in banks:
                    options.append(sum(min(sum((a-b)**2 for a,b in zip(source[i][:3],sh.displayed(c,m)))
                        for c in bank['colors']for m in ([0,1,2]if lighting else[1]))for i in positions))
                slot=min(range(2),key=lambda b:(options[b],b));total+=options[slot]
                self.assertEqual({indices[i]//16 for i in positions},{slot})
            self.assertEqual(total,report['rgb_squared_error'])
            self.assertTrue(set(mask)<={0,14,15})
            for i,index in enumerate(indices):
                mode={0:1,14:2,15:0}[mask[i]]
                self.assertEqual(preview[i],sh.displayed(banks[index//16]['colors'][index%16],mode))
        self.assertLessEqual(sh.prepare(16,8,source,banks,True)[3]['rgb_squared_error'],sh.prepare(16,8,source,banks,False)[3]['rgb_squared_error'])

    def test_invalid_alpha_grid_and_existing_destination_are_rejected(self):
        banks=validate_banks({'banks':[{'slot':0,'palette':['000000']*16}]})
        for w,h,pixels in [(9,8,[(0,0,0,255)]*72),(8,8,[(0,0,0,0)]*64)]:
            with self.assertRaises(ValueError):sh.prepare(w,h,pixels,banks,True)
        with tempfile.TemporaryDirectory()as temp:
            with self.assertRaises(ValueError):sh.write_comparison(Path('unused'),Path('unused'),Path(temp))

    def test_all_theme_exports_keep_palettes_and_never_raise_model_error(self):
        with tempfile.TemporaryDirectory(prefix='shadow native ')as temp:
            destination=Path(temp)/'demo';reports=builder().make_project(destination)
            self.assertEqual(len(reports),4)
            for theme,report in reports.items():
                self.assertLessEqual(report['lit']['rgb_squared_error'],report['normal']['rgb_squared_error'])
                for variant in ['normal','lit']:
                    self.assertEqual(read_png(destination/'res'/theme/f'{variant}.png')[:2],(256,128))
                self.assertEqual(sum(report['lit']['mode_pixels'].values()),256*128)
                for half in range(2):
                    self.assertEqual(read_png(destination/'res'/theme/f'mask-{half}.png')[:2],(128,128))
            with self.assertRaises(ValueError):builder().make_project(destination)


    def test_recorded_previews_and_gallery_reproduce_offline(self):
        result=subprocess.run([sys.executable,str(STUDY/'rebuild_shadow_study.py'),'--check'],capture_output=True,text=True,timeout=60)
        self.assertEqual(result.returncode,0,result.stderr)
        before=(STUDY/'shadow-gallery.html').read_bytes()
        result=subprocess.run([sys.executable,str(STUDY/'build_shadow_gallery.py')],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr);self.assertEqual(before,(STUDY/'shadow-gallery.html').read_bytes())
        from html.parser import HTMLParser
        sources=[]
        class Images(HTMLParser):
            def handle_starttag(self,tag,attrs):
                if tag=='img':sources.append(dict(attrs)['src'])
        Images().feed(before.decode());self.assertEqual(len(sources),12)
        for src in sources:
            self.assertNotIn('://',src);self.assertTrue((STUDY/src).is_file(),src)


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class ShadowHighlightContainerTests(unittest.TestCase):
    def test_normal_rom_and_executable_mask_probe_in_both_modes(self):
        from test_math import execute_sdk
        output=ROOT/'build/pixellab-shadow-integration';output.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='shadow SDK ',dir=ROOT/'build')as temp:
            project=Path(temp)/'project';builder().make_project(project)
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                r=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                self.assertEqual(r.returncode,0,(r.stdout+r.stderr)[-4000:]);return r.stdout+r.stderr
            hashes={}
            for mode in ['release','debug']:
                run('make','clean');log=run('make',mode);(output/f'{mode}.log').write_text(log)
                self.assertNotIn('warning:',log)
                rom=(project/'out/rom.bin').read_bytes();self.assertEqual(rom[0x100:0x104],b'SEGA')
                hashes[mode]=hashlib.sha256(rom).hexdigest()
            # Decode deterministic mask samples from the linked SpriteDefinition,
            # including ResComp piece trimming and column-major hardware tiles.
            samples=[]
            for theme in builder().THEMES:
                banks=validate_banks(json.loads((STUDY/'plans'/f'{theme}-background-banks.json').read_text()))
                w,h,pixels=read_png(STUDY/'raw'/f'{theme}-background.png');mask=sh.prepare(w,h,pixels,banks,True)[1]
                for half in range(2):
                    samples.append([mask[((i*43+i*i*7)%128)*256+half*128+(i*37)%128]for i in range(64)])
            (project/'inc').mkdir(exist_ok=True)
            (project/'inc/expected.h').write_text('static const u8 expected[8][64]={'+','.join('{'+','.join(map(str,row))+'}'for row in samples)+'};\n')
            shutil.copyfile(ROOT/'tests/fixtures/pixellab_shadow.c',project/'src/main.c')
            for mode in ['release','debug']:
                run('make','clean');run('make',mode)
                rom=(project/'out/rom.bin').read_bytes();code,_=execute_sdk(rom,run('m68k-elf-nm','out/rom.out'))
                self.assertEqual(code,0,f'Linked S/H mask contract failed at line {code}')
                hashes['probe-'+mode]=hashlib.sha256(rom).hexdigest()
            (output/'pixellab-shadow-report.json').write_text(json.dumps(dict(passed=True,hashes=hashes),indent=2)+'\n')


if __name__=='__main__':unittest.main()
