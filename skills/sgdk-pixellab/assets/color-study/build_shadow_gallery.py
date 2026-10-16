#!/usr/bin/env python3
"""Build the normal/S/H comparison gallery from recorded native exports."""
import html,json
from pathlib import Path
ROOT=Path(__file__).resolve().parent
parts=['<!doctype html><meta charset="utf-8"><title>Same palettes, extra shades</title><style>body{background:#10151d;color:#e6edf6;font:16px/1.5 system-ui;margin:32px}a{color:#84c8ff}.images{display:flex;flex-wrap:wrap;gap:24px;overflow:auto}figure{margin:0}img{image-rendering:pixelated}section{border-top:1px solid #354151;padding:24px 0}</style><h1>Same palettes, extra shades</h1><p>The same source and the same two background palette lines. Both variants choose colors against ares’s documented RGB lookup; the lighting version can additionally choose shadow or highlight per pixel. These previews model that emulator, not calibrated analog hardware.</p><p>The ROM uses actual VDP shadow/highlight with PAL3 operator-mask sprites. No HBlank palette writes, new generations, added dithering or rescaling. Full-background masks can cost up to 32 hardware sprite pieces, 256 scanline pixels and 512 tiles (16 KiB). Actors/items are drawn ahead of the mask and remain normal.</p><p><a href="gallery.html">Main art study</a> / <a href="SHADOW-HIGHLIGHT.md">How it works and costs</a></p>']
for theme in ['modern','fantasy','sci-fi','colonial']:
 report=json.loads((ROOT/'shadow-study'/theme/'report.json').read_text())
 parts.append(f'<section><h2>{html.escape(theme.title())}</h2><div class="images">')
 for label,source in [('Original source',f'raw/{theme}-background.png'),('Normal brightness',f'shadow-study/{theme}/normal-preview.png'),('Shadow + highlight',f'shadow-study/{theme}/lit-preview.png')]:
  parts.append(f'<figure><img src="{source}" width="512" height="256" alt="{html.escape(label)}"><figcaption>{label}</figcaption></figure>')
 parts.append(f'</div><p>Displayed RGB shades: {report["normal"]["displayed_rgb_colors"]} → {report["lit"]["displayed_rgb_colors"]}. Model squared-RGB error decreases {report["error_reduction_percent"]}%. This metric is not an artistic score.</p><p><a href="shadow-study/{theme}/report.json">Pixel and palette assignment report</a></p></section>')
(ROOT/'shadow-gallery.html').write_text(''.join(parts),encoding='utf-8')
print('Built four-theme lighting gallery')
