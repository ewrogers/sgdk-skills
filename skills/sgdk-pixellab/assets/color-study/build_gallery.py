#!/usr/bin/env python3
"""Build a local same-asset comparison gallery; no network or paid calls."""
import html
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parent
esc=html.escape
rows=json.loads((ROOT/'assets.json').read_text());notes=json.loads((ROOT/'reviews.json').read_text())
parts=['<!doctype html><meta charset="utf-8"><title>PixelLab — keeping the art alive</title><style>body{background:#10151d;color:#e6edf6;font:16px/1.5 system-ui;max-width:1500px;margin:auto;padding:32px}a{color:#84c8ff}section{border-top:1px solid #354151;padding:24px 0}.images{display:flex;flex-wrap:wrap;gap:24px}figure{margin:0;flex:1;min-width:260px}img{image-rendering:pixelated;max-width:100%;background:repeating-conic-gradient(#303640 0% 25%,#252a32 0% 50%) 50%/16px 16px}figcaption{color:#aab8cb}pre{white-space:pre-wrap;font-size:13px}summary{cursor:pointer;color:#84c8ff}</style><h1>Keeping the art alive</h1><p>Same source, same native dimensions. Original → one-line export → optional two-line background export. No redesign is passed off as a color conversion. Inspect at native size as well as integer zoom.</p><p>The second run used 76 of the additional 100 generations. Four active palette lines are planned per showcase scene: background 0/1, actors and items 2, UI 3. A different scene may load a different set during a safe transition.</p><p><a href="requests.json">Exact requests and receipts</a> / <a href="scene-palettes.json">Scene palette plans</a> / <a href="README.md">Findings and reproduction</a></p>']
for row in rows:
 n=row['id'];report=json.loads((ROOT/'reports'/f'{n}.json').read_text());w=row['width'];h=row['height']
 parts.append(f'<section id="{esc(n)}"><h2>{esc(n)} · {w}×{h}</h2><p>{esc(notes[n])}</p><div class="images">')
 variants=[('raw/'+n+'.png','Original returned asset'),('final/'+n+'.png','Same asset — one palette line')]
 if row.get('two_lines'):variants.append(('final/'+row['two_lines'],'Same asset — two planned background lines'))
 for path,label in variants:
  scale=4 if w<=96 else 2
  parts.append(f'<figure><img src="{esc(path)}" width="{w*scale}" height="{h*scale}" alt="{esc(n+" "+label)}"><figcaption>{esc(label)}</figcaption></figure>')
 parts.append(f'</div><p>{report["changed_opaque_rgb_pixels"]:,} opaque RGB pixels changed in the one-line export. Alpha policy, palette words and bounds are in <a href="reports/{esc(n)}.json">the report</a>.</p>')
 old=ROOT/'raw'/f'{n}-old-export.png'
 if old.exists():parts.append(f'<details><summary>Historical first-pass export (same source)</summary><img src="raw/{esc(old.name)}" width="{w*(4 if w<=96 else 2)}"><p>This is the older preparation, not the preferred result.</p></details>')
 candidates=ROOT/'raw'/f'{n}-candidates.png'
 if candidates.exists():parts.append(f'<details><summary>All 64 returned candidates, row order 0–63</summary><img src="raw/{esc(candidates.name)}" width="640"></details>')
 parts.append('</section>')
(ROOT/'gallery.html').write_text(''.join(parts),encoding='utf-8')
print(f'{len(rows)} same-asset comparisons')
