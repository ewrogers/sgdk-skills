"""Validate authored languages, executable formatting and real linked glyph data."""
import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

ROOT=Path(__file__).resolve().parents[1]
LAB=ROOT/'skills/sgdk-localization/assets/language-lab'
FIXTURE=ROOT/'tests/fixtures/localization'


def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);result=importlib.util.module_from_spec(spec)
    sys.path.insert(0,str(path.parent))
    try:spec.loader.exec_module(result)
    finally:sys.path.pop(0)
    return result


def pixels(data):
    offset=8;payload=bytearray()
    while offset<len(data):
        size=struct.unpack_from('>I',data,offset)[0];kind=data[offset+4:offset+8];chunk=data[offset+8:offset+8+size]
        if zlib.crc32(kind+chunk)!=struct.unpack_from('>I',data,offset+8+size)[0]:raise ValueError('PNG checksum')
        if kind==b'IHDR':width,height,depth,mode,_,_,_=struct.unpack('>IIBBBBB',chunk)
        if kind==b'IDAT':payload.extend(chunk)
        offset+=size+12
    if (depth,mode)!=(8,3):raise ValueError('Expected indexed PNG')
    data=zlib.decompress(payload)
    if len(data)!=(width+1)*height:raise ValueError('Unexpected scanline count')
    if any(data[y*(width+1)]!=0 for y in range(height)):raise ValueError('Expected filter zero')
    return width,height,[data[y*(width+1)+1:(y+1)*(width+1)] for y in range(height)]


class CatalogTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.compiler=module('localization_catalog',LAB/'tools/catalog.py')

    def build(self,data):
        with tempfile.TemporaryDirectory(prefix='translation source ') as scratch:
            path=Path(scratch)/'catalog.json';path.write_text(json.dumps(data,ensure_ascii=False),encoding='utf-8')
            return self.compiler.build(path)

    def source(self):return json.loads((LAB/'res/catalog.json').read_text(encoding='utf-8'))

    def test_catalog_budgets_native_font_and_distinct_accented_lowercase(self):
        files,report=self.compiler.build(LAB/'res/catalog.json')
        self.assertEqual(report['font'],{'glyphs':164,'tiles':176,'bytes':5632,'base':128,'reserved_tiles':192})
        self.assertEqual(report['text_bytes'],2016);self.assertEqual(report['fallbacks'],[])
        width,height,image=pixels(files['res/font.png']);self.assertEqual((width,height),(128,88))
        def glyph(char):
            index=report['glyph_map'][char]
            return tuple(row[(index%16)*8:(index%16+1)*8] for row in image[(index//16)*8:(index//16+1)*8])
        for left,right in [('e','é'),('n','ñ'),('u','ü'),('c','ç'),('E','e'),('カ','ガ' if 'ガ' in report['glyph_map'] else 'キ')]:
            self.assertNotEqual(glyph(left),glyph(right))
        self.assertEqual(set(b for row in image for b in row),{0,15})

    def test_normalized_accents_and_reordered_stable_ids_emit_identical_files(self):
        original=self.source();expected,_=self.build(original)
        changed=copy.deepcopy(original);changed['languages'].reverse();changed['messages'].reverse()
        changed['messages'][0]['text']['fr']=changed['messages'][0]['text']['fr'].replace('é','e\u0301')
        self.assertEqual(self.build(changed)[0],expected)

    def test_missing_translation_requires_an_explicit_reported_fallback(self):
        data=self.source();del data['messages'][10]['text']['es']
        with self.assertRaisesRegex(ValueError,'Missing translation: es/item'):self.build(data)
        data['allow_fallback']=[['es','item']]
        files,report=self.build(data)
        self.assertEqual(report['fallbacks'],[['es','item']]);self.assertIn(b'text_2_10',files['src/catalog.c'])
        data['allow_fallback'].append(['fr','item'])
        with self.assertRaisesRegex(ValueError,'actual missing'):self.build(data)

    def test_bad_placeholders_unknown_glyphs_and_layout_or_font_overflow_are_rejected(self):
        cases=[]
        for text in ['Hello {actor}', 'Hello {name:08}', 'Hello {name!r}', 'Hello {name}', '🙂']:
            data=self.source();data['messages'][0]['text']['es']=text;cases.append(data)
        for field,value in [('font_tiles',175),('font_base',1400),('version',True)]:
            data=self.source();data[field]=value;cases.append(data)
        data=self.source();data['messages'][0]['text']['es']='A'*37;cases.append(data)
        data=self.source();data['messages'][0]['text']['es']='word '*15;cases.append(data)
        data=self.source();data['messages'][0]['id']=True;cases.append(data)
        data=self.source();data['languages'][1]['id']=0;cases.append(data)
        data=self.source();data['messages'][0]['text']['es']='tab\ttext';cases.append(data)
        for data in cases:
            with self.subTest(data=data['messages'][0]['text']['es']):
                with self.assertRaises(ValueError):self.build(data)

    def test_longest_substitutions_count_glyphs_and_nonbreaking_groups(self):
        _,report=self.compiler.build(LAB/'res/catalog.json')
        self.assertLessEqual(report['layouts']['fr/found_many']['lines'],2)
        self.assertEqual(self.compiler.wrap('12\u00a0345',6),['12\u00a0345'])
        with self.assertRaises(ValueError):self.compiler.wrap('12\u00a0345',5)
        self.assertEqual(self.compiler.wrap('A B C',3),['A B','C'])

    def test_standalone_cli_prepare_and_overwrite_protection(self):
        prepare=module('localization_prepare',LAB/'tools/prepare.py')
        with tempfile.TemporaryDirectory(prefix='language lab source ') as scratch:
            destination=Path(scratch)/'new project'
            project,report=prepare.prepare(destination)
            self.assertTrue((project/'src/catalog.c').is_file());self.assertEqual(report['font']['tiles'],176)
            before=(project/'res/font.png').read_bytes()
            with self.assertRaises(ValueError):prepare.prepare(project)
            self.assertEqual((project/'res/font.png').read_bytes(),before)
            output=Path(scratch)/'compiler output'
            result=subprocess.run([sys.executable,str(project/'tools/catalog.py'),str(project/'res/catalog.json'),
                                   '--output',str(output)],capture_output=True,text=True,timeout=20)
            self.assertEqual(result.returncode,0,result.stderr)
            result=subprocess.run([sys.executable,str(project/'tools/catalog.py'),str(project/'res/catalog.json'),
                                   '--output',str(output)],capture_output=True,text=True,timeout=20)
            self.assertEqual(result.returncode,2)


class LocalizationBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cls.compiler:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
            raise unittest.SkipTest('C17 compiler required')
        cls.scratch=tempfile.TemporaryDirectory(prefix='localization standalone ');cls.addClassCleanup(cls.scratch.cleanup)
        prepare=module('behavior_localization_prepare',LAB/'tools/prepare.py')
        cls.project,_=prepare.prepare(Path(cls.scratch.name)/'project with spaces')
        cls.binaries=[]
        for optimize in ['-O0','-O3']:
            binary=Path(cls.scratch.name)/(optimize+('.exe' if os.name=='nt' else ''))
            command=[cls.compiler,'-std=c17',optimize,'-Wall','-Wextra','-Werror','-I',str(cls.project/'inc'),
                     '-I',str(FIXTURE),*[str(cls.project/'src'/n) for n in ['loc.c','lab.c','catalog.c']],
                     str(FIXTURE/'host.c'),'-o',str(binary)]
            result=subprocess.run(command,capture_output=True,text=True,timeout=60)
            if result.returncode:raise RuntimeError(result.stdout+result.stderr)
            cls.binaries.append(binary)

    def case(self,name):
        for binary in self.binaries:
            result=subprocess.run([str(binary),name],capture_output=True,text=True,timeout=20)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
    def test_bounded_grouping_roundtrips_all_digit_lengths_and_preserves_short_buffers(self):self.case('numbers')
    def test_every_translation_max_expansion_paging_long_words_and_nonbreaking_spaces(self):self.case('layout')
    def test_preferences_validate_ids_versions_crc_and_preserve_invalid_outputs(self):self.case('preferences')
    def test_edges_language_switch_selection_retention_accent_names_and_page_exit(self):self.case('controller')

    def test_renderer_bounds_and_preference_interruption_at_every_write(self):
        binary=Path(self.scratch.name)/('adapter.exe' if os.name=='nt' else 'adapter')
        command=[self.compiler,'-std=c17','-O2','-Wall','-Wextra','-Werror','-I',str(FIXTURE/'stubs'),
                 '-I',str(self.project/'inc'),*[str(self.project/'src'/n) for n in ['loc.c','lab.c','catalog.c','view.c','prefs.c']],
                 str(FIXTURE/'adapter.c'),'-o',str(binary)]
        result=subprocess.run(command,capture_output=True,text=True,timeout=60)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        result=subprocess.run([str(binary)],capture_output=True,text=True,timeout=20)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)


@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class LocalizationContainerTests(unittest.TestCase):
    def test_real_font_resource_and_m68000_formatter_in_both_build_modes(self):
        from test_math import execute_sdk
        prepare=module('container_localization_prepare',LAB/'tools/prepare.py')
        output=ROOT/'build/localization-integration';output.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='localization container ',dir=ROOT/'build') as scratch:
            project,report=prepare.prepare(Path(scratch)/'independent language lab')
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                result=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240)
                self.assertEqual(result.returncode,0,(result.stdout+result.stderr)[-5000:]);return result.stdout+result.stderr
            hashes={};geometry=None
            for kind in ['rom','probe']:
                if kind=='probe':
                    shutil.copyfile(FIXTURE/'probe.c',project/'src/main.c');shutil.copyfile(FIXTURE/'checks.h',project/'inc/checks.h')
                for mode in ['release','debug']:
                    run('make','clean');log=run('make',mode);(output/f'{kind}-{mode}.log').write_text(log)
                    self.assertNotIn('warning:',log)
                    rom=(project/'out/rom.bin').read_bytes();self.assertEqual(len(rom),131072)
                    self.assertEqual(rom[0x100:0x104],b'SEGA');self.assertEqual(struct.unpack_from('>I',rom,0x1A4)[0],len(rom)-1)
                    hashes[kind+'-'+mode]=hashlib.sha256(rom).hexdigest()
                    if kind=='probe':
                        code,raw=execute_sdk(rom,run('m68k-elf-nm','out/rom.out'))
                        self.assertEqual(code&65535,0,f'Target contract failed at {code&65535}')
                        values=[v&65535 for v in raw];geometry=values[:4]
                        self.assertEqual(geometry[:2],[164,176])
                        cursor=4
                        catalog=json.loads((project/'res/catalog.json').read_text(encoding='utf-8'))
                        for language in catalog['languages']:
                            for message in catalog['messages']:
                                layout=report['layouts'][language['tag']+'/'+message['key']]
                                self.assertEqual(values[cursor:cursor+2],[layout['lines'],layout['max_cells']]);cursor+=2
                        width,height,image=pixels((project/'res/font.png').read_bytes());expected=[]
                        for tile in range(report['font']['tiles']):
                            for y in range(8):
                                row=image[(tile//16)*8+y][(tile%16)*8:(tile%16+1)*8]
                                word=0
                                for nibble in row:word=(word<<4)|nibble
                                expected.extend([word>>16,word&65535])
                        self.assertEqual(values[cursor:],expected)
            (output/'localization-report.json').write_text(json.dumps({'passed':True,'hashes':hashes,
                'font':report['font'],'geometry':geometry,'execution':'Host and base M68000 rules/resource data; no VDP/SRAM hardware emulation'},indent=2)+'\n')


if __name__=='__main__':unittest.main()
