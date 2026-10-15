#!/usr/bin/env python3
"""Optionally regenerate the lab's robot announcements using eSpeak NG.

The committed WAVs build on every host; speech synthesis is not a build step.
"""
import argparse
from array import array
from pathlib import Path
import math
import shutil
import subprocess
import sys
import tempfile
import wave

PHRASES = {'voice_bomb':'Bomb', 'voice_power':'Power up', 'voice_shield':'Shield',
           'voice_boss':'Boss incoming', 'voice_over':'Game. Over!', 'voice_twin':'Twin', 'voice_spread':'Three way shot'}


def generate(output):
    if any((output/(name+'.wav')).exists() for name in PHRASES):
        raise ValueError('Choose a new output folder; voice sources already exist')
    synth = shutil.which('espeak-ng')
    if not synth:
        raise ValueError('Optional regeneration needs espeak-ng; the committed WAVs need no speech tool to build')
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='shooter robot voices ') as scratch:
        for name, phrase in PHRASES.items():
            source = Path(scratch)/(name+'.wav')
            subprocess.run([synth, '-v', 'en-us+croak', '-s', '155', '-p', '30',
                            '-w', str(source), phrase], check=True)
            with wave.open(str(source)) as wav:
                if (wav.getnchannels(), wav.getsampwidth()) != (1, 2):
                    raise ValueError('Unexpected speech output format')
                rate=wav.getframerate()
                samples = array('h', wav.readframes(wav.getnframes()))
            if sys.byteorder != 'little': samples.byteswap()
            # Offline deterministic rate conversion and a light ring-modulated
            # robot texture. No speech synthesis or DSP runs in the ROM.
            original=samples
            samples=array('h')
            for i in range(len(original)*13300//rate):
                position=i*rate/13300; at=int(position); fraction=position-at
                value=original[at]*(1-fraction)+original[min(at+1,len(original)-1)]*fraction
                value*=.8+.2*math.sin(2*math.pi*37*i/13300)
                samples.append(round(value/128)*128)
            # Retain a short edge margin and fade to avoid sample-boundary clicks.
            start = next((i for i, value in enumerate(samples) if abs(value)>48), 0)
            end = next((i for i in range(len(samples)-1, -1, -1) if abs(samples[i])>48), len(samples)-1)
            samples = samples[max(0,start-100):min(len(samples),end+101)]
            peak = max(abs(value) for value in samples) or 1
            for i, value in enumerate(samples):
                edge = min(i, len(samples)-1-i, 80)/80
                samples[i] = round(value*24000/peak*edge)
            if sys.byteorder != 'little': samples.byteswap()
            with wave.open(str(output/(name+'.wav')), 'wb') as wav:
                wav.setparams((1,2,13300,0,'NONE','not compressed'))
                wav.writeframes(samples.tobytes())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    try: generate(args.output)
    except ValueError as error: parser.error(str(error))
    print(f'Created {len(PHRASES)} short robot announcements')


if __name__ == '__main__': main()
