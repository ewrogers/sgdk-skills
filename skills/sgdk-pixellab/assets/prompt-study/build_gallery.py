#!/usr/bin/env python3
"""Build a standalone local comparison gallery using committed evidence only."""
import html
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parent

def build():
    requests=json.loads((ROOT/'requests.json').read_text())
    trials={t['id']:t for t in requests['trials']}
    reviews=json.loads((ROOT/'reviews.json').read_text())
    parts=[]
    for row in reviews:
        trial=trials.get(row['id'],{})
        args=trial.get('args',requests['exact_drawing'] if row['id']=='tile-eight' else {})
        prompt=args.get('description',args.get('action',args.get('lower_description','Exact pixel grid; see controls.')))
        controls=dict(args)
        for key in ['color_image_url','palette_image_url']:
            if controls.get(key,'').startswith('data:'):controls[key]='Palette PNG data URL; exact colors are in the contract.'
        text=lambda s:html.escape(str(s))
        report=ROOT/'reports'/(row['raw'].removesuffix('.png')+'.json')
        metrics=''
        if report.exists():
            s=json.loads(report.read_text())['source']
            metrics=f"{s['width']}×{s['height']} | {s['opaque_colors']} opaque RGB colors | {s['off_palette_pixels']} pixels outside usable palette"
        status='Selected for this role' if row['selected'] else 'Not selected for the original role'
        parts.append(f'''<article data-group="{text(row['group'])}"><h2>{text(row['id'])}</h2>
<p class="status">{text(row['group'])} — {status}</p><p class="metrics">{metrics}</p>
<div class="compare"><section><h3>Returned</h3><div class="image"><img src="raw/{text(row['raw'])}" alt="Raw {text(row['id'])}" loading="lazy"></div></section>
<section><h3>{'Would use' if row['selected'] else 'Preferred alternative / prepared evidence'}</h3><div class="image"><img src="final/{text(row['final'])}" alt="Prepared {text(row['id'])}" loading="lazy"></div></section></div>
<h3>Assessment</h3><p>{text(row['opinion'])}</p><h3>Additional work</h3><p>{text(row['work'])}</p>
<details><summary>Exact prompt and controls</summary><p>{text(prompt)}</p><pre>{text(json.dumps(controls,indent=2))}</pre></details></article>''')
    groups=sorted({r['group']for r in reviews})
    options='<option>All</option>'+''.join('<option>'+html.escape(g)+'</option>'for g in groups)
    document='''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>SGDK PixelLab prompt study</title>
<style>*{box-sizing:border-box}body{margin:0;background:#101419;color:#e2e8ef;font:16px/1.55 system-ui,sans-serif}header,main{max-width:1320px;margin:auto;padding:24px}h1{font-size:30px}h2{font-size:21px;color:#91caff}h3{font-size:15px;margin-bottom:8px}a{color:#91caff}article{border:1px solid #34404c;border-radius:12px;margin:24px 0;padding:24px;background:#181e25}.compare{display:grid;grid-template-columns:1fr 1fr;gap:20px}.image{overflow:auto;min-height:120px;padding:20px;background:repeating-conic-gradient(#222b35 0% 25%,#29333f 0% 50%) 0/16px 16px;border-radius:6px}img{image-rendering:pixelated;max-width:none}pre{white-space:pre-wrap;word-break:break-word;font-size:13px}.metrics,.status{color:#aab9c8}select,button{background:#253442;color:#fff;border:1px solid #5b7185;border-radius:5px;padding:8px;margin-right:12px}nav{position:sticky;top:0;background:#101419;padding:12px 0;z-index:1}details{margin-top:18px}summary{cursor:pointer;color:#91caff}@media(max-width:700px){.compare{grid-template-columns:1fr}header,main{padding:14px}}</style>
<header><h1>PixelLab → Mega Drive: prompt study</h1><p>Exact requests, original outputs, honest assessments and prepared native assets. 42.3 generations used within a 100-generation allowance. No image was resized into its working resolution.</p><p>The palette and multiple styles here are experiments, not recommendations for another game's appearance. “Selected” means suitable for the stated art role after the recorded work, not console-tested.</p><nav><label>Category <select id="filter">'''+options+'''</select></label><label>Preview <select id="zoom"><option value="1">Native 1×</option><option value="2" selected>2×</option><option value="4">4×</option><option value="8">8×</option></select></label></nav></header><main>'''+''.join(parts)+'''
<article><h2>All 64 Pro candidates</h2><p>Row-major order, indices 0–63. Key candidate 5 was selected; the wide chest batch was rejected for the requested role.</p><div class="compare"><div class="image"><img src="raw/key-pro-candidates.png" alt="64 key candidates"></div><div class="image"><img src="raw/chest-pro-candidates.png" alt="64 wide chest candidates"></div></div></article>
<article><h2>Native map from actual tile metadata</h2><p>The first painted path produced missing-shape warnings. Axis-aligned terrain surrounded by grass fixed them. This map uses returned corner rules and atlas bounding boxes, not Wang names as coordinates. It is a placement demonstration, not a collision map.</p><div class="image"><img src="final/map-native.png" alt="Native grass and stone map"></div></article>
</main><script>const images=[...document.querySelectorAll('img')];function resize(){for(const im of images){if(im.naturalWidth)im.style.width=(im.naturalWidth*Number(document.querySelector('#zoom').value))+'px';}}for(const im of images)im.addEventListener('load',resize);document.querySelector('#zoom').addEventListener('change',resize);document.querySelector('#filter').addEventListener('change',e=>{for(const a of document.querySelectorAll('article[data-group]'))a.hidden=e.target.value!=='All'&&a.dataset.group!==e.target.value;});resize();</script></html>'''
    return document

if __name__=='__main__':
    (ROOT/'gallery.html').write_text(build(),encoding='utf-8')
    print('Built local gallery with original requests and native prepared artwork')
