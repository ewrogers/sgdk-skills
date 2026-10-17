#!/usr/bin/env python3
"""Inspect PCM WAV quality, or apply explicit gain/DC removal/edge ramps to a new file."""
import argparse
import io
import json
import math
from pathlib import Path
import struct
import wave


def read(data):
    with wave.open(io.BytesIO(data)) as wav:
        channels,width,rate,count=wav.getnchannels(),wav.getsampwidth(),wav.getframerate(),wav.getnframes()
        if wav.getcomptype()!='NONE' or width not in (1,2) or channels not in (1,2):
            raise ValueError('Use uncompressed mono/stereo 8-bit or 16-bit PCM WAV')
        if count*channels*width>8*1024*1024:raise ValueError('This small SFX helper supports at most 8 MiB of PCM')
        raw=wav.readframes(count)
        if len(raw)!=count*channels*width:raise ValueError('Truncated WAV data')
    samples=[(v-128)*256 for v in raw]if width==1 else list(struct.unpack('<'+'h'*(len(raw)//2),raw))
    if not samples:raise ValueError('WAV has no samples')
    return channels,width,rate,samples


def inspect(data):
    channels,width,rate,samples=read(data);peak=max(abs(v)for v in samples)
    rms=math.sqrt(sum(v*v for v in samples)/len(samples))
    return {'channels':channels,'sample_bytes':width,'rate_hz':rate,'frames':len(samples)//channels,
            'seconds':len(samples)/(channels*rate),'peak_dbfs':round(20*math.log10(peak/32768),2)if peak else None,
            'rms_dbfs':round(20*math.log10(rms/32768),2)if rms else None,
            'dc_mean':sum(samples)//len(samples),'clipped_samples':sum(v in (-32768,32512 if width==1 else 32767)for v in samples),
            'start_peak':max(abs(v)for v in samples[:channels]),'end_peak':max(abs(v)for v in samples[-channels:])}


def prepare(data,gain=80,fade_ms=3,remove_dc=False):
    channels,width,rate,samples=read(data)
    if channels!=1 or width!=2:raise ValueError('Preparation expects mono 16-bit PCM; mix/resample in your audio editor first')
    if not 0<=gain<=400 or not 0<=fade_ms<=1000:raise ValueError('Gain must be 0..400 percent; fade 0..1000 ms')
    dc=sum(samples)//len(samples)if remove_dc else 0
    ramp=min(len(samples)//2,rate*fade_ms//1000);out=[]
    for i,v in enumerate(samples):
        v=(v-dc)*gain//100
        if ramp>1:v=v*min(i,len(samples)-1-i,ramp-1)//(ramp-1)
        out.append(max(-32768,min(32767,v)))
    buffer=io.BytesIO()
    with wave.open(buffer,'wb')as wav:
        wav.setparams((1,2,rate,0,'NONE','not compressed'));wav.writeframes(struct.pack('<'+'h'*len(out),*out))
    return buffer.getvalue()


def main():
    parser=argparse.ArgumentParser(description=__doc__);sub=parser.add_subparsers(dest='command',required=True)
    check=sub.add_parser('inspect');check.add_argument('input',type=Path)
    fix=sub.add_parser('prepare');fix.add_argument('input',type=Path);fix.add_argument('output',type=Path)
    fix.add_argument('--gain-percent',type=int,default=80);fix.add_argument('--fade-ms',type=int,default=3);fix.add_argument('--remove-dc',action='store_true')
    args=parser.parse_args()
    try:
        data=args.input.read_bytes()
        if args.command=='prepare':
            data=prepare(data,args.gain_percent,args.fade_ms,args.remove_dc)
            with args.output.open('xb')as target:target.write(data)
        print(json.dumps(inspect(data),indent=2))
    except (OSError,ValueError,wave.Error,struct.error,EOFError)as error:parser.exit(2,f'PCM tool: {error}\n')


if __name__=='__main__':main()
