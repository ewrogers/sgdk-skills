"""Sound authoring assets, quality transformations and a bounded audio director."""
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import wave
from test_puzzle import load
ROOT=Path(__file__).resolve().parents[1];SKILL=ROOT/'skills/sgdk-audio';LAB=SKILL/'assets/sound-lab';FIXTURE=ROOT/'tests/fixtures/audio-workshop'

def compile_host(cc,path,mode):
    args=[cc,'-DAD_HOST','-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'inc'),'-I',str(FIXTURE),str(LAB/'src/director.c'),str(FIXTURE/'host.c'),'-o',str(path)]
    if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
    r=subprocess.run(args,capture_output=True,text=True,timeout=60)
    if r.returncode:raise RuntimeError(r.stdout+r.stderr)

class AudioDirectorTests(unittest.TestCase):
    def test_pending_commands_latest_scene_jingle_restore_pause_effect_budgets_and_20000_frames(self):
        cc=next((p for n in ['clang','cc','gcc']if(p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
            self.skipTest('C17 compiler required')
        with tempfile.TemporaryDirectory(prefix='audio director ')as scratch:
            traces=[]
            for mode in ['-O0','-O3']:
                binary=Path(scratch)/(mode+('.exe'if os.name=='nt'else''));compile_host(cc,binary,mode)
                r=subprocess.run([str(binary)],capture_output=True,text=True,timeout=30);self.assertEqual(r.returncode,0,r.stderr);traces.append(r.stdout)
            self.assertEqual(traces[0],traces[1])

class AudioAuthoringTests(unittest.TestCase):
    def test_vgm_command_boundaries_instrument_layers_and_one_shot_release(self):
        audio=load('workshop_authoring',LAB/'tools/desk_audio.py')
        for name in audio.TRACKS:
            data=audio.score(name);self.assertEqual(data,audio.score(name));self.assertEqual(int.from_bytes(data[4:8],'little')+4,len(data))
            cursor=256;samples=0;boundaries=set();writes=[];psg=[];streams=0
            while True:
                boundaries.add(cursor);op=data[cursor]
                if op==0x66:break
                if op in (0x52,0x53):writes.append((op,data[cursor+1],data[cursor+2]));cursor+=3
                elif op==0x50:psg.append(data[cursor+1]);cursor+=2
                elif op==0x62:samples+=735;cursor+=1
                elif op==0x67:cursor+=7+int.from_bytes(data[cursor+3:cursor+7],'little')
                elif op in (0x90,0x91):cursor+=5
                elif op==0x92:cursor+=6
                elif op==0x93:streams+=1;cursor+=11
                elif op==0x94:cursor+=2
                else:self.fail(f'{name}: unsupported fixture opcode {op:x}')
            self.assertEqual(cursor,len(data)-1);self.assertEqual(samples,int.from_bytes(data[0x18:0x1c],'little'))
            loop=int.from_bytes(data[0x1c:0x20],'little')
            if name=='fanfare':self.assertEqual(loop,0)
            else:self.assertIn(loop+0x1c,boundaries);self.assertEqual(int.from_bytes(data[0x20:0x24],'little'),samples)
            notes=[w for w in writes if 0xa0<=w[1]<=0xa6]
            self.assertEqual(bool(notes),name not in ('psg','drums'))
            self.assertEqual(bool(streams),name in ('journey','danger','drums'))
            self.assertEqual(psg[-4:],[0x9f,0xbf,0xdf,0xff]);self.assertTrue(all(not(v&0xf0)for _,r,v in writes[-6:]if r==0x28))
    def test_sample_pairs_have_same_pitch_length_headroom_and_different_edges(self):
        audio=load('workshop_samples',LAB/'tools/desk_audio.py');pcm=load('workshop_pcm',SKILL/'scripts/pcm_tool.py')
        for kind in range(4):
            clean=pcm.inspect(audio.sound(kind,True));raw=pcm.inspect(audio.sound(kind,False))
            self.assertEqual(clean['frames'],raw['frames']);self.assertEqual(clean['rate_hz'],13300)
            self.assertEqual(clean['start_peak'],0);self.assertEqual(clean['end_peak'],0);self.assertGreater(raw['start_peak'],1000)
            self.assertEqual(clean['clipped_samples'],0);self.assertLess(clean['peak_dbfs'],-6)
    def test_dc_gain_ramps_preserve_rate_length_source_and_refuse_invalid_formats(self):
        pcm=load('workshop_pcm_quality',SKILL/'scripts/pcm_tool.py');buffer=io.BytesIO()
        with wave.open(buffer,'wb')as wav:wav.setparams((1,2,22050,0,'NONE','not compressed'));wav.writeframes(struct.pack('<'+'h'*1000,*([12000,2000]*500)))
        source=buffer.getvalue();prepared=pcm.prepare(source,gain=50,fade_ms=3,remove_dc=True)
        self.assertEqual(buffer.getvalue(),source);report=pcm.inspect(prepared)
        self.assertEqual((report['rate_hz'],report['frames']),(22050,1000));self.assertEqual(report['start_peak'],0);self.assertEqual(report['end_peak'],0)
        self.assertLess(abs(report['dc_mean']),10);self.assertLessEqual(max(abs(v)for v in pcm.read(prepared)[3]),2500)
        with self.assertRaises(ValueError):pcm.prepare(source,gain=-1)
        with self.assertRaises(ValueError):pcm.prepare(source,fade_ms=1001)
        self.assertRaises((ValueError,wave.Error,EOFError),pcm.inspect,b'bad')
        with tempfile.TemporaryDirectory(prefix='audio quality ')as scratch:
            original=Path(scratch)/'source.wav';out=Path(scratch)/'prepared.wav';original.write_bytes(source)
            command=[sys.executable,str(SKILL/'scripts/pcm_tool.py'),'prepare',str(original),str(out),'--remove-dc']
            r=subprocess.run(command,capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr);before=out.read_bytes()
            r=subprocess.run(command,capture_output=True,text=True);self.assertNotEqual(r.returncode,0);self.assertEqual(out.read_bytes(),before);self.assertEqual(original.read_bytes(),source)
    def test_unsigned_stereo_inspection_and_explicit_preparation_format_boundary(self):
        pcm=load('workshop_pcm_stereo',SKILL/'scripts/pcm_tool.py');buffer=io.BytesIO()
        with wave.open(buffer,'wb')as wav:wav.setparams((2,1,13300,0,'NONE','not compressed'));wav.writeframes(bytes([0,255,128,128]))
        source=buffer.getvalue();report=pcm.inspect(source)
        self.assertEqual((report['channels'],report['frames'],report['clipped_samples']),(2,2,2))
        self.assertEqual((report['start_peak'],report['end_peak']),(32768,0))
        with self.assertRaises(ValueError):pcm.prepare(source)
    def test_native_art_and_independent_fresh_export(self):
        from test_localization import pixels
        art=load('workshop_art',LAB/'tools/desk_art.py');assets=art.assets();self.assertEqual(assets,art.assets());w,h,rows=pixels(assets['desk.png']);self.assertEqual((w,h),(320,224));self.assertLess(max(v for row in rows for v in row),16)
        self.assertEqual(len(art.font_words()),768);self.assertTrue(all(0<=c<=7 for rgb in art.PALETTE for c in rgb))
        prepare=load('workshop_prepare',LAB/'tools/prepare.py')
        with tempfile.TemporaryDirectory(prefix='sound desk ')as scratch:
            first=prepare.prepare(Path(scratch)/'first copy');second=Path(scratch)/'second copy'
            r=subprocess.run([sys.executable,str(first/'tools/prepare.py'),str(second)],capture_output=True,text=True,timeout=30);self.assertEqual(r.returncode,0,r.stderr)
            for name in ['res/harbor.vgm','res/fanfare.vgm','res/coin.wav','res/coin_raw.wav','res/desk.png','inc/desk_font.h']:self.assertEqual((first/name).read_bytes(),(second/name).read_bytes())
            with self.assertRaises(ValueError):prepare.prepare(first)
        with self.assertRaises(ValueError):prepare.prepare(LAB/'do not create')

@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class AudioWorkshopContainerTests(unittest.TestCase):
    def test_real_resources_regions_director_on_base_m68000_and_both_build_modes(self):
        from test_streaming import execute
        prepare=load('workshop_target_prepare',LAB/'tools/prepare.py');art=load('workshop_target_art',LAB/'tools/desk_art.py')
        out=ROOT/'build/audio-workshop-integration';out.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='sound desk target ',dir=ROOT/'build')as scratch:
            project=prepare.prepare(Path(scratch)/'standalone sound desk')
            cc=next((p for n in ['clang','cc','gcc']if(p:=shutil.which(n))),None);self.assertIsNotNone(cc)
            host=Path(scratch)/'host';compile_host(cc,host,'-O3');trace=int(subprocess.check_output([str(host),'trace'],text=True))
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                r=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240);self.assertEqual(r.returncode,0,(r.stdout+r.stderr)[-5000:]);return r.stdout+r.stderr
            hashes={};geometry=[];header=(project/'src/boot/rom_head.c').read_text()
            for kind in ['rom','pal','ntsc','probe']:
                region=('E'if kind=='pal'else'U'if kind=='ntsc'else'JUE')
                (project/'src/boot/rom_head.c').write_text(header.replace('.region="JUE"','.region="'+region+'"'))
                if kind=='probe':shutil.copyfile(FIXTURE/'probe.c',project/'src/main.c');shutil.copyfile(FIXTURE/'checks.h',project/'inc/checks.h')
                for mode in (['release','debug']if kind in ('rom','probe')else['release']):
                    run('make','clean');log=run('make',mode);(out/f'{kind}-{mode}.log').write_text(log)
                    self.assertEqual([line for line in log.splitlines()if 'warning:'in line],[])
                    rom=(project/'out/rom.bin').read_bytes();self.assertEqual(len(rom),262144);self.assertEqual(rom[0x1f0:0x200].rstrip(b'\0 '),region.encode())
                    self.assertEqual(int.from_bytes(rom[0x1a4:0x1a8],'big'),len(rom)-1);hashes[kind+'-'+mode]=hashlib.sha256(rom).hexdigest()
                    if kind=='probe':
                        code,values=execute(rom,run('m68k-elf-nm','--defined-only','out/rom.out'));self.assertEqual(code,0,f'Target contract failed at {code}')
                        self.assertEqual(values[1]<<16|values[2],trace);geometry=[values[0],values[3]];self.assertEqual(values[4:6],[40,28])
                        self.assertEqual(values[6:22],[(b<<9)|(g<<5)|(r<<1)for r,g,b in art.PALETTE])
                        self.assertEqual(values[22:],[part for v in art.font_words()for part in(v>>16,v&65535)])
            (out/'audio-workshop-report.json').write_text(json.dumps({'passed':True,'hashes':hashes,'geometry':geometry,'trace_digest':trace,
                'budgets':{'window_map_bytes':4480,'hardware_sprites':0,'palette_banks':1,'effect_batch_capacity':4,'effect_voices':2,'music_pcm_channel':1,'extra_dma_bytes':2048},
                'execution':'Base M68000 director and compiled art; VDP/Z80/audio timing requires listening and hardware checks','physical_hardware':'not tested'},indent=2)+'\n')
if __name__=='__main__':unittest.main()
