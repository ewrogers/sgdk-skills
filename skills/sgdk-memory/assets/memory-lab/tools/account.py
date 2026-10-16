#!/usr/bin/env python3
"""Report static RAM and heap/stack reservations from an SGDK ELF, using Docker."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys


def account(sections,symbols,stack):
    if not 0<stack<65536 or stack&1:raise ValueError('Stack reservation must be positive, even and below 64 KiB')
    rows={}
    for line in sections.splitlines():
        parts=line.split()
        if len(parts)==3 and parts[0] in ('.data','.bss'):
            if parts[0] in rows:raise ValueError('Duplicate RAM section')
            rows[parts[0]]=(int(parts[1]),int(parts[2]))
    if set(rows)!={'.data','.bss'}:raise ValueError('Expected SGDK .data and .bss sections')
    ends=[]
    for line in symbols.splitlines():
        parts=line.split()
        if len(parts)==3 and parts[2]=='_bend':ends.append(int(parts[0],16))
    if len(ends)!=1:raise ValueError('Expected one _bend symbol')
    # Normalize the stock SGDK high address alias to its 24-bit bus address.
    base=0xFF0000;end=ends[0]&0xFFFFFF
    if not base<=end<0x1000000:raise ValueError('_bend is outside work RAM')
    static=end-base;size_sum=0;previous=base
    for name in ('.data','.bss'):
        size,address=rows[name];address &= 0xFFFFFF
        if size<0 or address<previous or address+size>end:raise ValueError('RAM sections overlap or exceed _bend')
        previous=address+size;size_sum+=size
    aligned=(end+1)&~1;heap_end=0x1000000-stack
    capacity=heap_end-aligned-2
    if capacity<0:raise ValueError('Static RAM overlaps heap/stack reservation')
    return {'ram_bytes':65536,'data_bytes':rows['.data'][0],'bss_bytes':rows['.bss'][0],
        'static_padding_bytes':static-size_sum,'static_ram_bytes':static,
        'heap_alignment_bytes':aligned-end,'heap_capacity_bytes':capacity,'heap_terminator_bytes':2,
        'stack_reserved_bytes':stack,'heap_start':aligned,'heap_end':heap_end,
        'note':'Heap capacity includes allocator headers. Stack reservation is not measured peak stack use.'}


def inspect(elf,platform=None):
    elf=Path(elf).resolve()
    if not elf.is_file():raise ValueError('ELF does not exist')
    base=['docker','run','--rm','-i']
    if platform:base+=['--platform',platform]
    if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
    base+=['--mount',f'type=bind,source={elf.parent},target=/workspace,readonly','ewrogers/sgdk:latest']
    def run(*command,input=None):
        result=subprocess.run(base+list(command),input=input,capture_output=True,text=True,timeout=60)
        if result.returncode:raise ValueError(result.stderr.strip() or 'Toolchain command failed')
        return result.stdout
    sections=run('m68k-elf-size','-A','/workspace/'+elf.name)
    symbols=run('m68k-elf-nm','--defined-only','/workspace/'+elf.name)
    header=run('m68k-elf-gcc','-E','-P','-x','c','-include','/opt/sgdk/inc/memory_base.h','-',input='STACK_SIZE\n')
    value=header.strip()
    if not re.fullmatch(r'0x[0-9a-fA-F]+|[0-9]+',value):raise ValueError('Cannot read selected SDK stack reservation')
    return account(sections,symbols,int(value,0))


def table(report):
    labels=[('Initialized data','data_bytes'),('Zeroed globals','bss_bytes'),('Static padding','static_padding_bytes'),
        ('Heap alignment','heap_alignment_bytes'),('Heap capacity','heap_capacity_bytes'),
        ('Heap end marker','heap_terminator_bytes'),('Stack reservation','stack_reserved_bytes'),('Total work RAM','ram_bytes')]
    lines=['+--------------------+--------------+','| RAM reservation    |        Bytes |','+--------------------+--------------+']
    lines += [f'| {label:<18} | {report[key]:>12,} |' for label,key in labels]
    return '\n'.join(lines+['+--------------------+--------------+',report['note']])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf',type=Path,required=True)
    parser.add_argument('--platform',choices=['linux/arm64','linux/amd64'])
    parser.add_argument('--json',action='store_true');args=parser.parse_args()
    try:
        report=inspect(args.elf,args.platform)
        print(json.dumps(report,indent=2) if args.json else table(report));return 0
    except (OSError,ValueError,subprocess.TimeoutExpired) as error:print(error,file=sys.stderr);return 2

if __name__=='__main__':raise SystemExit(main())
