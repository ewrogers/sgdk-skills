#!/usr/bin/env python3
"""Validate UTF-8 translations, bound substituted layout, emit native glyph data."""
import argparse
import json
import math
from pathlib import Path
import re
import string
import struct
import sys
import unicodedata
import zlib
from loc_glyphs import rows

NAME, COUNT, NEWLINE = 252, 253, 254


def bounded(value, low, high):
    if type(value) is not int or not low <= value <= high:
        raise ValueError(f'Expected an integer in {low}..{high}')
    return value


def normalized(text):
    if not isinstance(text, str): raise ValueError('Text must be a string')
    return unicodedata.normalize('NFC', text)


def pieces(text):
    result = []
    for literal, field, spec, conversion in string.Formatter().parse(text):
        result.append(literal)
        if field is not None:
            if field not in ('name', 'count') or spec or conversion:
                raise ValueError('Only {name} and {count} placeholders are supported')
            result.append(NAME if field == 'name' else COUNT)
    return result


def wrap(text, width):
    lines = ['']
    for paragraph_index, paragraph in enumerate(text.split('\n')):
        if paragraph_index: lines.append('')
        for word in paragraph.split(' '):
            if not word: continue
            if len(word) > width: raise ValueError(f'Word is wider than {width} cells: {word!r}')
            current = lines[-1]
            if current and len(current) + 1 + len(word) > width:
                lines.append(word)
            else: lines[-1] = current + (' ' if current else '') + word
    return lines


def number(value, separator):
    digits = str(value)
    return digits[:-3] + separator + digits[-3:] if separator and len(digits) > 3 else digits


def font_png(characters, tiles):
    width, height = 128, (tiles // 16) * 8
    pixels = bytearray(width * height)
    for index, character in enumerate(characters, 1):
        pattern = rows(character)
        if len(pattern) != 8 or any(not 0 <= row <= 255 for row in pattern):
            raise ValueError(f'Invalid authored glyph: {character!r}')
        for y, value in enumerate(pattern):
            for x in range(8):
                if value & (128 >> x): pixels[((index // 16) * 8 + y) * width + (index % 16) * 8 + x] = 15
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    scanlines = b''.join(b'\0' + pixels[y * width:(y + 1) * width] for y in range(height))
    palette = bytes([0,0,0] * 15 + [252,252,252])
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width,height,8,3,0,0,0)) +
            chunk(b'PLTE', palette) + chunk(b'IDAT', zlib.compress(scanlines,9)) + chunk(b'IEND', b''))


def build(path):
    data = json.loads(Path(path).read_text(encoding='utf-8'))
    required = {'version','font_base','font_tiles','name_chars','languages','messages'}
    if not isinstance(data, dict) or not required <= set(data) or set(data)-required-{'allow_fallback'}:
        raise ValueError('Unknown or missing catalog fields')
    if type(data['version']) is not int or data['version'] != 1: raise ValueError('Unsupported catalog version')
    base = bounded(data['font_base'],16,1439)
    budget = bounded(data['font_tiles'],16,256)
    if base + budget > 1440: raise ValueError('Font reservation overlaps the Lab SDK font boundary')
    names = normalized(data['name_chars'])
    if len(names) != 32 or len(set(names)) != 32 or any(c in '\r\n\t' for c in names):
        raise ValueError('The Lab keyboard requires 32 distinct glyphs')
    languages = data['languages']
    if not isinstance(languages,list) or not 1 <= len(languages) <= 8: raise ValueError('Expected 1..8 languages')
    tags, ids = set(), set()
    for language in languages:
        if not isinstance(language,dict) or set(language) != {'id','tag','separator'}: raise ValueError('Bad language fields')
        identity = bounded(language['id'],0,255)
        tag = language['tag']
        if not isinstance(tag,str) or not re.fullmatch('[a-z][a-z0-9-]{0,11}',tag): raise ValueError('Bad language tag')
        if identity in ids or tag in tags: raise ValueError('Duplicate language ID/tag')
        ids.add(identity); tags.add(tag)
        if language['separator'] not in ('',',','.','\u00a0'): raise ValueError('Unsupported grouping separator')
    languages = sorted(languages,key=lambda l:l['id'])
    if languages[0]['id'] != 0 or languages[0]['tag'] != 'en': raise ValueError('Default language must be stable ID 0 / en')
    messages = data['messages']
    if not isinstance(messages,list) or not 1 <= len(messages) <= 64: raise ValueError('Expected 1..64 messages')
    keys, message_ids = set(), set()
    for message in messages:
        if not isinstance(message,dict) or set(message) != {'id','key','box','text'}: raise ValueError('Bad message fields')
        identity = bounded(message['id'],0,len(messages)-1)
        key = message['key']
        if not isinstance(key,str) or not re.fullmatch('[a-z][a-z0-9_]{0,31}',key): raise ValueError('Invalid message key')
        if identity in message_ids or key in keys: raise ValueError('Duplicate message ID/key')
        message_ids.add(identity); keys.add(key)
        box = message['box']
        if not isinstance(box,list) or len(box)!=3: raise ValueError('Box needs width, rows, pages')
        width, height, pages = box
        bounded(width,1,36); bounded(height,1,9); bounded(pages,1,9)
        if height * pages > 9: raise ValueError('Box exceeds nine layout lines')
        if not isinstance(message['text'],dict) or set(message['text'])-tags or 'en' not in message['text']:
            raise ValueError('Unknown translation language or missing English default')
    messages = sorted(messages,key=lambda m:m['id'])
    allowed = data.get('allow_fallback',[])
    if not isinstance(allowed,list) or any(not isinstance(x,list) or len(x)!=2 for x in allowed):
        raise ValueError('allow_fallback needs [language, message] pairs')
    if any(not isinstance(a,str) or not isinstance(b,str) or a not in tags or b not in keys for a,b in allowed):
        raise ValueError('Unknown fallback language/message')
    allowed = set(map(tuple,allowed))
    fallbacks = []
    characters = set(chr(c) for c in range(32,127)) | set(names) | {'\u00a0'}
    parsed = {}; layouts = {}; locale_chars = {}
    for language in languages:
        tag = language['tag']; locale_chars[tag] = set(names) | set('0123456789') | set(language['separator'])
        for message in messages:
            key = message['key']
            text = message['text'].get(tag)
            if text is None:
                if (tag,key) not in allowed: raise ValueError(f'Missing translation: {tag}/{key}')
                text = message['text']['en']; fallbacks.append([tag,key])
            text = normalized(text)
            tokens = pieces(text)
            if {x for x in tokens if isinstance(x,int)} != {x for x in pieces(normalized(message['text']['en'])) if isinstance(x,int)}:
                raise ValueError(f'Placeholder mismatch: {tag}/{key}')
            literal = ''.join(x for x in tokens if isinstance(x,str))
            if any(ord(c)<32 and c!='\n' for c in literal): raise ValueError('Unsupported text control character')
            characters.update(literal.replace('\n','')); locale_chars[tag].update(literal.replace('\n',''))
            expanded = ''.join('W'*8 if x==NAME else number(65535,language['separator']) if x==COUNT else x for x in tokens)
            if len(expanded)>=256: raise ValueError(f'Expanded text too long: {tag}/{key}')
            width,height,pages = message['box']
            try: lines = wrap(expanded,width)
            except ValueError as error: raise ValueError(f'{tag}/{key}: {error}') from error
            if len(lines)>height*pages: raise ValueError(f'Text overflows {tag}/{key}: {len(lines)} lines, limit {height*pages}')
            layouts[f'{tag}/{key}'] = {'lines':len(lines),'pages':math.ceil(len(lines)/height),'max_cells':max(map(len,lines))}
            parsed[tag,key] = tokens
    if allowed != set(map(tuple,fallbacks)): raise ValueError('Fallback exceptions must name actual missing translations')
    characters = sorted(characters,key=ord)
    if len(characters)>251: raise ValueError('Catalog exceeds this codec\'s 251 glyph symbols; use wider IDs or font banks')
    tiles = math.ceil((len(characters)+1)/16)*16
    if tiles>budget: raise ValueError(f'Font needs {tiles} tiles / {tiles*32} bytes; reserved {budget} tiles')
    glyph = {c:i+1 for i,c in enumerate(characters)}
    header = ['#ifndef LANGUAGE_LAB_CATALOG_H','#define LANGUAGE_LAB_CATALOG_H','#include "loc.h"',
              f'#define LOC_LANGUAGE_COUNT {len(languages)}u',f'#define LOC_MESSAGE_COUNT {len(messages)}u',
              f'#define LOC_GLYPH_COUNT {len(characters)+1}u',f'#define LOC_FONT_TILES {tiles}u',
              f'#define LOC_FONT_BASE {base}u',f'#define LOC_FONT_BUDGET {budget}u',
              f'#define LOC_SPACE {glyph[" "]}u',f'#define LOC_NBSP {glyph[chr(160)]}u',
              'enum { '+', '.join('TXT_'+m['key'].upper()+'='+str(m['id']) for m in messages)+' };',
              'extern const LocLanguage loc_languages[LOC_LANGUAGE_COUNT];',
              'extern const LocBox loc_boxes[LOC_MESSAGE_COUNT];',
              'extern const u8 *const loc_catalog[LOC_LANGUAGE_COUNT][LOC_MESSAGE_COUNT];',
              'extern const u8 loc_name_chars[32];','#endif','']
    source = ['#include "catalog.h"','const LocLanguage loc_languages[LOC_LANGUAGE_COUNT] = {']
    source += [f'    {{{l["id"]},"{l["tag"]}",{glyph[l["separator"]] if l["separator"] else 0}}},' for l in languages]
    source += ['};','const LocBox loc_boxes[LOC_MESSAGE_COUNT] = {']
    source += ['    {'+','.join(map(str,m['box']))+'},' for m in messages]
    source += ['};','const u8 loc_name_chars[32] = {'+','.join(str(glyph[c]) for c in names)+'};']
    text_bytes = 0
    for li,language in enumerate(languages):
        for message in messages:
            codes=[]
            for token in parsed[language['tag'],message['key']]:
                codes.extend([token] if isinstance(token,int) else [NEWLINE if c=='\n' else glyph[c] for c in token])
            codes.append(0); text_bytes+=len(codes)
            source.append(f'static const u8 text_{li}_{message["id"]}[] = {{'+','.join(map(str,codes))+'};')
    source += ['const u8 *const loc_catalog[LOC_LANGUAGE_COUNT][LOC_MESSAGE_COUNT] = {']
    source += ['    {'+','.join(f'text_{li}_{m["id"]}' for m in messages)+'},' for li,_ in enumerate(languages)]
    source += ['};','']
    report = {'version':1,'font':{'glyphs':len(characters)+1,'tiles':tiles,'bytes':tiles*32,'base':base,'reserved_tiles':budget},
              'locale_glyphs':{k:len(v) for k,v in locale_chars.items()},'text_bytes':text_bytes,
              'fallbacks':fallbacks,'layouts':layouts,'glyph_map':glyph}
    return {'inc/catalog.h':'\n'.join(header).encode(), 'src/catalog.c':'\n'.join(source).encode(),
            'res/font.png':font_png(characters,tiles), 'res/resources.res':b'TILESET language_font "font.png" NONE NONE ROW\n',
            'catalog-report.json':(json.dumps(report,ensure_ascii=False,indent=2)+'\n').encode('utf-8')},report


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('catalog',type=Path); parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    try:
        if args.output.exists(): raise ValueError('Choose a new output directory')
        files,report=build(args.catalog)
        args.output.mkdir(parents=True)
        for relative,content in files.items():
            path=args.output/relative; path.parent.mkdir(parents=True,exist_ok=True); path.write_bytes(content)
        print(json.dumps({'font':report['font'],'text_bytes':report['text_bytes'],'fallbacks':report['fallbacks']}))
        return 0
    except (OSError,ValueError) as error:
        print(f'FAIL: {error}',file=sys.stderr);return 2


if __name__=='__main__':raise SystemExit(main())
