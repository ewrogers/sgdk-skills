"""Native asset contracts, palette preparation, PNG failures and SDK resources."""
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

ROOT=Path(__file__).resolve().parents[1]
SKILL=ROOT/'skills/sgdk-pixellab'
SCRIPTS=SKILL/'scripts'
sys.path.insert(0,str(SCRIPTS))
# Other labs have modules named pixel_io; this helper deliberately has its own name.
from png_io import chunk, indexed_png, read_png, SIGNATURE
import art_pipeline as art
sys.path.pop(0)

PALETTE=['000000','242449','49496d','926d49','db926d','ffdb92','ffffff','6d2449',
         'b6496d','db9292','246d6d','49b6b6','92dbdb','242424','494949','929292']


def plan(w=8,h=8,**kwargs):
    return art.contract(dict(width=w,height=h,kind='sprite',palette=PALETTE,**kwargs))


def rgba_png(w,h,pixels,filter_type=0):
    raw=bytearray(); previous=bytes(w*4)
    for y in range(h):
        row=bytes(c for p in pixels[y*w:(y+1)*w] for c in p)
        raw.append(filter_type)
        for x,value in enumerate(row):
            a=row[x-4] if x>=4 else 0; b=previous[x]; c=previous[x-4] if x>=4 else 0
            p=a+b-c
            paeth=min([(abs(p-a),0,a),(abs(p-b),1,b),(abs(p-c),2,c)])[2]
            prediction=(0,a,b,(a+b)//2,paeth)[filter_type]
            raw.append((value-prediction)%256)
        previous=row
    return SIGNATURE+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b'')


class PixelLabPipelineTests(unittest.TestCase):
    def test_native_dimensions_frames_and_palette_contract_failures(self):
        for change in [dict(width=True),dict(height=0),dict(width=9),dict(frame_width=16),
                       dict(kind='image'),dict(dither='random'),dict(alpha_threshold=0),
                       dict(unexpected=True),dict(palette=['000000']),dict(palette=['#112233']*16)]:
            doc=dict(width=8,height=8,kind='sprite',palette=PALETTE);doc.update(change)
            with self.subTest(change=change),self.assertRaises(ValueError):art.contract(doc)
        with self.assertRaisesRegex(ValueError,'Wrong native dimensions'):
            art.inspect(16,16,[(0,0,0,0)]*256,plan())

    def test_transparent_and_opaque_black_keep_different_indices(self):
        doc=dict(width=8,height=8,kind='sprite',palette=['000000','000000']+PALETTE[2:])
        p=art.contract(doc); pixels=[(0,0,0,0),(0,0,0,255)]*32
        indices=art.quantize(8,8,pixels,p)
        self.assertEqual(indices,[0,1]*32)
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)/'black.png';path.write_bytes(indexed_png(8,8,indices,p['colors'],True))
            self.assertEqual(read_png(path),(8,8,pixels))

    def test_palette_mapping_matches_nearest_squared_distance_oracle(self):
        p=plan(32,32); pixels=[((i*47)%256,(i*113)%256,(i*193)%256,255) for i in range(1024)]
        result=art.quantize(32,32,pixels,p)
        for pixel,index in zip(pixels,result):
            distance=lambda rgb:sum((a-b)**2 for a,b in zip(pixel[:3],rgb))
            self.assertEqual(distance(p['colors'][index]),min(map(distance,p['colors'][1:])))
            self.assertNotEqual(index,0)
        report=art.inspect(32,32,[(*p['colors'][i],255) for i in result],p)
        self.assertEqual(report['off_palette_pixels'],0)
        self.assertLessEqual(report['opaque_colors'],15)

    def test_dithering_preserves_exact_colors_alpha_and_shared_frame_phase(self):
        colors=['000000','242424','494949']+['ffffff']*13
        doc=dict(width=16,height=8,frame_width=8,frame_height=8,kind='sprite',palette=colors,dither='bayer2')
        p=art.contract(doc); pixels=[(54,54,54,255)]*128
        indices=art.quantize(16,8,pixels,p)
        self.assertEqual({indices[i] for i in range(128)},{1,2})
        for y in range(8):self.assertEqual(indices[y*16:y*16+8],indices[y*16+8:y*16+16])
        for mode in ['none','bayer2','bayer4']:
            p['dither']=mode
            self.assertEqual(art.quantize(16,8,[(73,73,73,255)]*128,p),[2]*128)
            self.assertEqual(art.quantize(16,8,[(73,73,73,0)]*128,p),[0]*128)

    def test_partial_alpha_requires_a_decision_and_backgrounds_stay_opaque(self):
        pixels=[(255,255,255,127),(255,255,255,128)]*32
        with self.assertRaisesRegex(ValueError,'Partial alpha'):art.quantize(8,8,pixels,plan())
        p=plan(alpha_threshold=128); result=art.quantize(8,8,pixels,p)
        self.assertEqual(result,[0,6]*32)
        p['kind']='background'
        with self.assertRaisesRegex(ValueError,'transparency'):art.quantize(8,8,pixels,p)

    def test_review_hints_distinguish_bounds_edges_seams_and_stray_pixels(self):
        pixels=[(0,0,0,0)]*128
        for x,y in [(0,0),(2,2),(3,2),(10,3),(11,3)]:pixels[y*16+x]=(255,255,255,255)
        r=art.inspect(16,8,pixels,plan(16,8,frame_width=8))
        self.assertEqual(r['bounds'],[0,0,11,3]);self.assertEqual(r['isolated_pixels'],[[0,0]])
        self.assertEqual([f['edge_pixels'] for f in r['frames']],[1,0])
        self.assertEqual(r['seam_mismatched_rows'],1)
        self.assertEqual(r['unique_8x8_patterns'],2)

    def test_png_all_filters_and_indexed_transparency_round_trip(self):
        pixels=[((i*11)%256,(i*31)%256,(i*57)%256,(i*17)%256) for i in range(64)]
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/'image.png'
            for filter_type in range(5):
                p.write_bytes(rgba_png(8,8,pixels,filter_type));self.assertEqual(read_png(p),(8,8,pixels))
            colors=plan()['colors'];p.write_bytes(indexed_png(8,8,list(range(16))*4,colors,True))
            w,h,decoded=read_png(p)
            self.assertEqual((w,h),(8,8));self.assertEqual(decoded,[(*colors[i],0 if i==0 else 255) for i in list(range(16))*4])

    def test_png_rejects_crc_truncation_invalid_index_oversized_output_and_trailing_data(self):
        valid=indexed_png(8,8,[1]*64,plan()['colors'],True)
        crc=bytearray(valid);crc[-1]^=1
        huge=SIGNATURE+chunk(b'IHDR',struct.pack('>IIBBBBB',65535,65535,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(b''))+chunk(b'IEND',b'')
        wrong_index=SIGNATURE+chunk(b'IHDR',struct.pack('>IIBBBBB',1,1,8,3,0,0,0))+chunk(b'PLTE',b'\0\0\0')+chunk(b'IDAT',zlib.compress(b'\0\x02'))+chunk(b'IEND',b'')
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/'bad.png'
            for data in [valid[:-3],bytes(crc),valid+b'extra',huge,wrong_index]:
                p.write_bytes(data)
                with self.assertRaises(ValueError):read_png(p)

    def test_cli_prepare_validate_paths_with_spaces_and_no_overwrite(self):
        with tempfile.TemporaryDirectory(prefix='pixel lab ') as temp:
            root=Path(temp);src=root/'source art.png';cfg=root/'plan.json';out=root/'final art.png';report=root/'report.json'
            src.write_bytes(rgba_png(8,8,[(100,150,200,255)]*64))
            cfg.write_text(json.dumps(dict(width=8,height=8,kind='sprite',palette=PALETTE)))
            base=[sys.executable,str(SCRIPTS/'art_pipeline.py')]
            def run(*args):return subprocess.run(base+list(map(str,args)),capture_output=True,text=True)
            self.assertNotEqual(run('validate',src,cfg).returncode,0)
            original=src.read_bytes();self.assertEqual(run('prepare',src,cfg,'--output',out,'--report',report).returncode,0)
            self.assertEqual(run('validate',out,cfg).returncode,0)
            contents=out.read_bytes();self.assertEqual(src.read_bytes(),original)
            self.assertNotEqual(run('prepare',src,cfg,'--output',out).returncode,0)
            self.assertEqual(out.read_bytes(),contents)
            other=root/'must not exist.png'
            self.assertNotEqual(run('prepare',src,cfg,'--output',other,'--report',report).returncode,0)
            self.assertFalse(other.exists())


class PixelLabStudyTests(unittest.TestCase):
    def test_committed_native_assets_validate_and_reproduce_offline(self):
        study=SKILL/'assets/prompt-study'
        result=subprocess.run([sys.executable,str(study/'rebuild.py')],capture_output=True,text=True,timeout=30)
        self.assertEqual(result.returncode,0,result.stderr)
        for cfg in (study/'contracts').glob('*.json'):
            p=art.contract(json.loads(cfg.read_text()));w,h,pixels=read_png(study/'final'/f'{cfg.stem}.png')
            r=art.inspect(w,h,pixels,p)
            self.assertEqual(r['off_palette_pixels'],0);self.assertEqual(r['partial_alpha_pixels'],0)
        # The 32x16 chest is a literal native crop, not a scaled 32x32 image.
        _,_,whole=read_png(study/'raw/object-chest.png');w,h,cropped=read_png(study/'raw/chest-native-crop.png')
        self.assertEqual((w,h),(32,16));self.assertEqual(whole[8*32:24*32],cropped)
        # Keep the actual input plus all four generated motion frames.
        w,h,sheet=read_png(study/'final/robot-idle-sheet.png');self.assertEqual((w,h),(160,32))
        for frame in range(5):
            _,_,pixels=read_png(study/'final'/('robot-profile.png' if frame==0 else f'robot-idle-{frame}.png'))
            self.assertEqual([sheet[y*160+frame*32+x]for y in range(32)for x in range(32)],pixels)


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class PixelLabContainerTests(unittest.TestCase):
    def test_real_rescomp_palette_sprite_frames_tiles_and_both_build_modes(self):
        import hashlib,shutil
        from test_math import execute_sdk
        output=ROOT/'build/pixellab-integration';output.mkdir(parents=True,exist_ok=True)
        study=SKILL/'assets/prompt-study'
        with tempfile.TemporaryDirectory(prefix='pixel lab SDK ',dir=ROOT/'build') as temp:
            project=Path(temp)
            for d in ['src','inc','res']:(project/d).mkdir()
            (project/'Makefile').write_text('include $(GDK)/makefile.gen\n')
            shutil.copyfile(ROOT/'tests/fixtures/pixellab/probe.c',project/'src/main.c')
            (project/'src/boot').mkdir()
            shutil.copyfile(ROOT/'tests/fixtures/pixellab/rom_head.c',project/'src/boot/rom_head.c')
            for name in ['tile-eight','robot-idle-sheet','key-pro-selected','chest-native-crop','desert-dither-2x2','title-illustration']:
                shutil.copyfile(study/'final'/f'{name}.png',project/'res'/f'{name}.png')
            (project/'res/resources.res').write_text('TILESET native_tile "tile-eight.png" NONE NONE ROW\n'
                'SPRITE robot "robot-idle-sheet.png" 4 4 NONE 0 NONE\n'
                'SPRITE key "key-pro-selected.png" 2 2 NONE 0 NONE\n'
                'SPRITE chest "chest-native-crop.png" 4 2 NONE 0 NONE\n'
                'IMAGE background "desert-dither-2x2.png" NONE\n'
                'IMAGE title "title-illustration.png" NONE\n')
            p=plan();words=[((r>>5)<<1)|((g>>5)<<5)|((b>>5)<<9)for r,g,b in p['colors']]
            w,h,pixels=read_png(study/'final/tile-eight.png');indices=[p['colors'].index(px[:3])for px in pixels]
            rows=[sum(indices[y*8+x]<<(28-x*4)for x in range(8))for y in range(8)]
            (project/'inc/expected.h').write_text('static const u16 expected_palette[]={'+','.join(map(str,words))+'};\n'
                'static const u32 expected_tile[]={'+','.join(str(v)+'UL' for v in rows)+'};\n')
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                r=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                self.assertEqual(r.returncode,0,(r.stdout+r.stderr)[-4000:]);return r.stdout+r.stderr
            hashes={};budgets={}
            for mode in ['release','debug']:
                run('make','clean');log=run('make',mode);(output/f'{mode}.log').write_text(log)
                self.assertFalse('warning:' in log,'\n'.join(line for line in log.splitlines() if 'warning:' in line))
                rom=(project/'out/rom.bin').read_bytes();hashes[mode]=hashlib.sha256(rom).hexdigest()
                code,values=execute_sdk(rom,run('m68k-elf-nm','out/rom.out'))
                self.assertEqual(code,0,f'M68000 asset contract failed at line {code}')
                self.assertEqual(len(values),3)
                budgets[mode]={'resident_sprite_frame_tiles':values[0],'background_tiles':values[1],'title_tiles':values[2]}
            (output/'pixellab-report.json').write_text(json.dumps(dict(passed=True,hashes=hashes,budgets=budgets),indent=2)+'\n')


class PixelLabEvidenceTests(unittest.TestCase):
    def test_reports_match_raw_pixels_and_title_edit_stays_in_recorded_region(self):
        study=SKILL/'assets/prompt-study'
        for cfg in (study/'contracts').glob('*.json'):
            p=art.contract(json.loads(cfg.read_text()))
            w,h,pixels=read_png(study/'raw'/f'{cfg.stem}.png')
            report=json.loads((study/'reports'/f'{cfg.stem}.json').read_text())
            self.assertEqual(report['source'],art.inspect(w,h,pixels,p))
        _,_,original=read_png(study/'raw/title-illustration.png')
        _,_,final=read_png(study/'final/title-illustration.png')
        changed={(i%320,i//320)for i,(a,b)in enumerate(zip(original,final))if a!=b}
        recorded=json.loads((study/'reports/title-illustration.json').read_text())['exact_edit_pixels']
        self.assertTrue(changed)
        self.assertEqual(changed,{tuple(p)for p in recorded})
        self.assertTrue(all(288<=x<320 and 216<=y<224 for x,y in changed))
        source=read_png(study/'raw/robot-profile.png'); first=read_png(study/'raw/robot-idle-0.png')
        self.assertEqual(source[:2],first[:2])
        # Regression fixture: the service's advertised unchanged frame changed art.
        changed=[i for i,(a,b) in enumerate(zip(source[2],first[2])) if a!=b and (a[3] or b[3])]
        self.assertEqual(len(changed),14)
        # The actual original is retained separately and used by the prepared sheet.
        _,_,sheet=read_png(study/'final/robot-idle-sheet.png')
        original=read_png(study/'final/robot-profile.png')[2]
        self.assertEqual([sheet[y*160+x] for y in range(32) for x in range(32)],original)

    def test_gallery_reproduces_and_every_image_reference_is_local_and_present(self):
        from html.parser import HTMLParser
        study=SKILL/'assets/prompt-study'
        before=(study/'gallery.html').read_bytes()
        result=subprocess.run([sys.executable,str(study/'build_gallery.py')],capture_output=True,text=True,timeout=30)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertEqual(before,(study/'gallery.html').read_bytes())
        sources=[]
        class Images(HTMLParser):
            def handle_starttag(self,tag,attrs):
                if tag=='img':sources.append(dict(attrs)['src'])
        Images().feed(before.decode())
        self.assertGreater(len(sources),40)
        for src in sources:
            self.assertNotIn('://',src)
            self.assertTrue((study/src).is_file(),src)


if __name__=='__main__':unittest.main()
