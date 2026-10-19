#!/usr/bin/env python3
"""Copy optional USB input sources into an existing project, without overwrite."""
import argparse
from pathlib import Path
import shutil


def install(project,headers='inc'):
    project=Path(project).resolve()
    if not project.is_dir():raise ValueError('select an existing project directory')
    if headers not in ('inc','src'):raise ValueError('headers must be inc or src')
    source=Path(__file__).resolve().parents[1]/'assets/usb-input'
    targets={project/'src/usb_input.c':source/'usb_input.c',
             project/headers/'usb_input.h':source/'usb_input.h',
             project/headers/'usb_input_ed.h':source/'usb_input_ed.h'}
    if any(p.exists() or p.is_symlink() for p in targets):raise ValueError('USB input files already exist; no files replaced')
    for p in targets:
        if p.parent.exists() and (p.parent.is_symlink() or not p.parent.is_dir()):raise ValueError('source/header directory must be a real directory')
    for p,s in targets.items():
        p.parent.mkdir(exist_ok=True)
        with p.open("xb") as output,s.open("rb") as input_file:shutil.copyfileobj(input_file,output)
    return list(targets)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('project',type=Path)
    parser.add_argument('--headers',choices=('inc','src'),default='inc');args=parser.parse_args()
    try:
        for path in install(args.project,args.headers):print(path)
    except (ValueError,OSError) as error:parser.exit(2,str(error)+'\n')
