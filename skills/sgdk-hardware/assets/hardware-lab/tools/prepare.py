#!/usr/bin/env python3
"""Copy a fresh standalone Hardware Lab and generate its original resources."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import struct
import sys
import wave
from hardware_glyphs import rows
from hardware_audio import assets as music
from readiness_art import header as readiness_header
from lab_art import header as lab_header
ROOT=Path(__file__).resolve().parents[1]

def font_words():
    words=[]
    for code in range(32,128):
        for bits in rows(chr(code)):
            pixels=[1]+[15 if bits&(1<<(4-x)) else 1 for x in range(5)]+[1,1]
            words.append(sum(p<<(28-4*i) for i,p in enumerate(pixels)))
    return words

def build_identity(mode='release',region='auto',build_id=None):
    if mode not in ('release','debug') or region not in ('auto','ntsc','pal'):
        raise ValueError('Unknown build mode or region')
    digest=hashlib.sha256()
    generated={'font.h','readiness_art.h','lab_art.h','build_id.h','resources.h'}
    paths=[ROOT/'Makefile']
    for folder in ('src','inc','tools'):
        paths += [p for p in (ROOT/folder).rglob('*') if p.is_file() and
                  p.suffix in ('.c','.h','.py') and p.name not in generated]
    for path in sorted(paths):
        digest.update(path.relative_to(ROOT).as_posix().encode('utf-8')+b'\0'+path.read_bytes()+b'\0')
    source_hash=digest.hexdigest()
    label=build_id if build_id is not None else f'HL-{source_hash[:8].upper()}-{mode[0].upper()}{region[0].upper()}'
    if not re.fullmatch(r'[A-Z0-9][A-Z0-9._-]{0,19}',label):
        raise ValueError('Build ID must be 1-20 uppercase letters/digits, dot, underscore or dash')
    return {'build_id':label,'source_sha256':source_hash,'mode':mode,'region':region}


def prepare(output,mode='release',region='auto',build_id=None):
    output=Path(output).resolve()
    if output.exists():raise ValueError('Choose a new output directory')
    if output.is_relative_to(ROOT):raise ValueError('Output cannot be inside the source Lab')
    identity=build_identity(mode,region,build_id)
    output.mkdir(parents=True)
    for name in ('inc','src','tools'):shutil.copytree(ROOT/name,output/name,ignore=shutil.ignore_patterns('__pycache__'))
    for name in ('Makefile','README.md'):shutil.copyfile(ROOT/name,output/name)
    (output/'res').mkdir()
    for name,data in music().items():(output/'res'/name).write_bytes(data)
    # Seamless 256-sample reference: six complete cycles, no restart gap.
    with wave.open(str(output/'res/reference.wav'),'wb') as wav:
        wav.setparams((1,2,13300,0,'NONE','not compressed'))
        wav.writeframes(b''.join(struct.pack('<h',int(7500*math.sin(2*math.pi*6*i/256)))for i in range(256)))
    (output/'inc/font.h').write_text('/* Original 5x7 opaque font. */\nstatic const u32 font_tiles[768]={\n'+',\n'.join(f'0x{v:08x}'for v in font_words())+'\n};\n')
    (output/'inc/readiness_art.h').write_text(readiness_header())
    (output/'inc/lab_art.h').write_text(lab_header())
    (output/'inc/build_id.h').write_text('/* Preparation identity; full inputs in build-info.json. */\n#define HL_BUILD_ID "'+identity['build_id']+'"\n')
    (output/'build-info.json').write_text(json.dumps(identity,indent=2)+'\n')
    (output/'res/resources.res').write_text('XGM2 reference_music "garden.vgm"\nWAV reference_pcm "reference.wav" XGM2\n')
    return output

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path)
    p.add_argument('--build-id')
    try:
        args=p.parse_args();print(prepare(args.output,build_id=args.build_id))
    except (OSError,ValueError) as error:print(error,file=sys.stderr);raise SystemExit(2)
