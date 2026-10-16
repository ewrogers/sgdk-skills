#!/usr/bin/env python3
"""Create a local original/prepared art gallery beside a prepared project."""
import argparse
import html
import json
from pathlib import Path
import shutil
from prepare import ROOT


def gallery(project):
    originals=project/'catalog-source';originals.mkdir(exist_ok=True)
    ledger=json.loads((ROOT/'requests.json').read_text());prompts={}
    for row in ledger['requests']:
        if row['result'].get('isError'):continue
        prompts[row['id']]=row['args'].get('description','')
    report=json.loads((project/'asset-report.json').read_text())
    parts=['<!doctype html><meta charset="utf-8"><title>Asterfall art catalog</title><style>body{background:#101723;color:#ece7d4;font:16px/1.5 system-ui;margin:32px}a{color:#8bcdd2}.images{display:flex;flex-wrap:wrap;gap:24px;overflow:auto}figure{margin:0}img{image-rendering:pixelated;background:repeating-conic-gradient(#8d9398 0% 25%,#c6cace 0% 50%) 50%/16px 16px}section{padding:24px 0;border-top:1px solid #425160}pre{white-space:pre-wrap}</style><h1>Asterfall: The Lantern Road</h1><p>A playable fantasy JRPG art catalog. 55 of 200 allowed PixelLab generations. Original illustrations, characters, portraits, icons and font; native cleanup, glyph remapping and map pins are recorded separately.</p><p>Explore screens share two background lines, a character/portrait line and UI. Menus reuse the background palette space for inventory art. No smooth downscaling or forced global game palette. Colors here are authoring previews, not a calibrated analog display.</p>']
    for source in sorted((ROOT/'raw').glob('*.png')):
        target=project/'res'/source.name
        if not target.exists():target=project/'res'/(source.stem.replace('-','_')+'.png')
        if source.stem=='font':target=project/'res/font_ivory.png'
        if not target.exists():continue
        shutil.copyfile(source,originals/source.name)
        parts.append(f'<section><h2>{html.escape(source.stem)}</h2><p>{html.escape(prompts.get(source.stem,""))}</p><div class="images">')
        for name,path in [('Original','catalog-source/'+source.name),('Prepared','res/'+target.name)]:
            parts.append(f'<figure><img src="{path}" style="zoom:2" alt="{name}"><figcaption>{name}</figcaption></figure>')
        parts.append('</div></section>')
    for name in ['hero','guide','sage']:
        parts.append(f'<section><h2>{name}: four directions, four walk frames</h2><img src="res/{name}_walk.png" style="zoom:2"><p>Original rotations and all source frames remain in raw/{name}.zip. All three characters and portraits use one shared palette to avoid per-frame color flicker.</p></section>')
    parts.append('<section><h2>Native finishing</h2><p>Removed invented lettering from the intro sky strip; preserved the raw source. Recolored generated font masks, remapped actual atlas dimensions, added arrow/bracket glyphs, and created an integer-enlarged title variant. Authored two map markers and recolored the healing tonic liquid from blue to the requested emerald. These are explicit edits, not additional generations.</p></section>')
    (project/'catalog.html').write_text(''.join(parts),encoding='utf-8')
    return project/'catalog.html'


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('project',type=Path);args=parser.parse_args();print(gallery(args.project))
