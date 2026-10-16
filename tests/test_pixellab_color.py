"""Source-derived palette proposals and banked background contracts."""
from collections import Counter
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
SCRIPTS=ROOT/'skills/sgdk-pixellab/scripts'
sys.path.insert(0,str(SCRIPTS))
import fit_palette as fitting
import background_banks as banks
from png_io import indexed_png,read_png
from art_pipeline import LEVELS,contract,quantize
sys.path.pop(0)


def line(*colors):
    result=list(colors)
    return result+[result[-1]]*(16-len(result))


class SourcePaletteTests(unittest.TestCase):
    def test_fitting_preserves_all_colors_when_the_native_source_already_fits(self):
        colors=[(0,0,0),(36,73,109),(255,219,73),(255,255,255),(0,146,73)]
        result=fitting.fit(Counter({c:100-i for i,c in enumerate(colors)}),15)
        self.assertEqual(set(result),set(colors))
        p=contract(dict(width=8,height=8,kind='sprite',palette=['000000']+[''.join(f'{v:02x}'for v in c)for c in result]+['ffffff']*(15-len(result))))
        pixels=[(*colors[i%5],255)for i in range(64)]
        indices=quantize(8,8,pixels,p)
        self.assertEqual([p['colors'][i]for i in indices],[p[:3]for p in pixels])
        self.assertNotIn(0,indices) # Opaque black remains opaque at another index.

    def test_budget_protected_colors_and_extremes_do_not_depend_on_frequency(self):
        histogram=Counter({(x,y,z):1+(x+y+z)%20 for x in LEVELS for y in LEVELS for z in LEVELS})
        locked=[(255,0,0),(0,255,255)]
        palette=fitting.fit(histogram,15,locked)
        self.assertEqual(len(palette),15);self.assertEqual(len(set(palette)),15)
        self.assertTrue(set(locked)<=set(palette))
        self.assertIn((0,0,0),palette);self.assertIn((255,255,255),palette)
        self.assertEqual(palette,fitting.fit(Counter(dict(reversed(list(histogram.items())))),15,locked))
        for count in [0,17,True]:
            with self.assertRaises(ValueError):fitting.fit(histogram,count)
        with self.assertRaises(ValueError):fitting.fit(histogram,1,locked)
        with self.assertRaises(ValueError):fitting.fit(histogram,15,[(17,34,51)])
        with self.assertRaises(ValueError):fitting.fit({},15)

    def test_shared_source_fit_cli_refuses_overwrites(self):
        with tempfile.TemporaryDirectory(prefix='palette source ')as temp:
            root=Path(temp);one=root/'one.png';two=root/'two.png';output=root/'palette.json'
            c=[(0,0,0),(255,219,73),(36,146,255)]
            one.write_bytes(indexed_png(8,8,[0,1]*32,c));two.write_bytes(indexed_png(8,8,[0,2]*32,c))
            cmd=[sys.executable,str(SCRIPTS/'fit_palette.py'),str(one),str(two),'--kind','sprite','--lock','ffffff','--output',str(output)]
            result=subprocess.run(cmd,capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            plan=json.loads(output.read_text());self.assertEqual(len(plan['palette']),16)
            self.assertIn('ffffff',plan['palette']);self.assertIn('ffdb49',plan['palette']);self.assertIn('2492ff',plan['palette'])
            original=output.read_bytes();self.assertNotEqual(subprocess.run(cmd,capture_output=True).returncode,0);self.assertEqual(output.read_bytes(),original)


class BankedBackgroundTests(unittest.TestCase):
    def setUp(self):
        self.document={'banks':[{'slot':0,'palette':line('000000','ff0000')},{'slot':1,'palette':line('000000','0000ff')}]}
        self.plan=banks.validate_banks(self.document)

    def test_independent_tiles_select_lines_and_pixels_stay_in_that_line(self):
        pixels=[(255,0,0,255)]*8+[(0,0,255,255)]*8
        pixels*=8
        colors,indices,report=banks.prepare(16,8,pixels,self.plan)
        self.assertEqual(report['tile_palette_slots'],[0,1]);self.assertEqual(report['rgb_squared_error'],0)
        for y in range(8):
            self.assertEqual(set(i//16 for i in indices[y*16:y*16+8]),{0})
            self.assertEqual(set(i//16 for i in indices[y*16+8:y*16+16]),{1})
        self.assertEqual([colors[i]for i in indices],[p[:3]for p in pixels])
        with tempfile.TemporaryDirectory()as temp:
            p=Path(temp)/'banked.png';p.write_bytes(indexed_png(16,8,indices,colors))
            self.assertEqual(read_png(p),(16,8,pixels))

    def test_tile_cost_matches_independent_oracle_even_with_mixed_colors(self):
        pixels=[((i*37)%256,(i*71)%256,(i*199)%256,255)for i in range(256)]
        _,indices,report=banks.prepare(16,16,pixels,self.plan)
        expected=[];total=0
        for ty in [0,8]:
            for tx in [0,8]:
                positions=[(ty+y)*16+tx+x for y in range(8)for x in range(8)]
                scores=[]
                for b in self.plan:
                    scores.append(sum(min(sum((p-v)**2 for p,v in zip(pixels[i][:3],color))for color in b['colors'])for i in positions))
                slot=min(range(2),key=lambda n:(scores[n],n));expected.append(slot);total+=scores[slot]
                self.assertEqual({indices[i]//16 for i in positions},{slot})
        self.assertEqual(report['tile_palette_slots'],expected);self.assertEqual(report['rgb_squared_error'],total)

    def test_rejects_duplicate_slots_inconsistent_backdrops_alpha_and_wrong_grid(self):
        for change in [lambda d:d['banks'][1].update(slot=0),lambda d:d['banks'][1].update(slot=4),lambda d:d['banks'][1]['palette'].__setitem__(0,'ffffff')]:
            document=json.loads(json.dumps(self.document));change(document)
            with self.assertRaises(ValueError):banks.validate_banks(document)
        with self.assertRaises(ValueError):banks.prepare(9,8,[(0,0,0,255)]*72,self.plan)
        with self.assertRaises(ValueError):banks.prepare(8,8,[(0,0,0,0)]*64,self.plan)

    def test_more_planned_lines_cannot_increase_the_assignment_error(self):
        pixels=[((i*41)%256,(i*13)%256,(i*19)%256,255)for i in range(128)]
        one=banks.prepare(16,8,pixels,self.plan[:1])[2]['rgb_squared_error']
        two=banks.prepare(16,8,pixels,self.plan)[2]['rgb_squared_error']
        self.assertLessEqual(two,one)


class ColorStudyTests(unittest.TestCase):
    def test_all_committed_exports_reproduce_without_generation_or_resizing(self):
        study=ROOT/'skills/sgdk-pixellab/assets/color-study'
        result=subprocess.run([sys.executable,str(study/'rebuild.py'),'--check'],capture_output=True,text=True,timeout=60)
        self.assertEqual(result.returncode,0,result.stderr)
        for row in json.loads((study/'assets.json').read_text()):
            raw=read_png(study/'raw'/f'{row["id"]}.png')
            final=read_png(study/'final'/f'{row["id"]}.png')
            self.assertEqual(raw[:2],final[:2])
            if row['theme']in ['fantasy','sci-fi','modern','colonial']and row['id'].endswith(('-actor','-object','-ui')):
                self.assertEqual([p[:3]for p in raw[2]if p[3]],[p[:3]for p in final[2]if p[3]])
                self.assertEqual([p[3]for p in raw[2]],[p[3]for p in final[2]])

    def test_gallery_reproduces_and_every_local_image_exists(self):
        from html.parser import HTMLParser
        study=ROOT/'skills/sgdk-pixellab/assets/color-study';before=(study/'gallery.html').read_bytes()
        result=subprocess.run([sys.executable,str(study/'build_gallery.py')],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr);self.assertEqual(before,(study/'gallery.html').read_bytes())
        sources=[]
        class Images(HTMLParser):
            def handle_starttag(self,tag,attrs):
                if tag=='img':sources.append(dict(attrs)['src'])
        Images().feed(before.decode())
        self.assertGreaterEqual(len(sources),70)
        for src in sources:
            self.assertNotIn('://',src);self.assertTrue((study/src).is_file(),src)

    def test_showcase_copies_to_a_new_project_and_keeps_four_palette_roles(self):
        import importlib.util
        study=ROOT/'skills/sgdk-pixellab/assets/color-study'
        spec=importlib.util.spec_from_file_location('color_rom',study/'build_rom.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        with tempfile.TemporaryDirectory(prefix='native color ')as temp:
            project=Path(temp)/'new project';module.make_project(project)
            resource=(project/'res/resources.res').read_text()
            self.assertEqual(len(resource.splitlines()),16)
            for theme in module.THEMES:
                bg=read_png(project/'res'/f'{theme}-background-two-lines.png')
                self.assertEqual(bg[:2],(256,128))
                plan=banks.validate_banks(json.loads((study/'plans'/f'{theme}-background-banks.json').read_text()))
                self.assertEqual([b['slot']for b in plan],[0,1])
            with self.assertRaises(ValueError):module.make_project(project)


if __name__=='__main__':unittest.main()
