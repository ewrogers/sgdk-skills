#!/usr/bin/env python3
"""Compile a bounded ASCII dialogue graph to C tokens; never parse markup per frame."""
import argparse
import json
from pathlib import Path
import re
import sys

OPS = {'glyph': 0, 'newline': 1, 'page': 2, 'pause': 3, 'speed': 4, 'finish': 5}
COLORS = {'white': 0, 'gold': 1, 'cyan': 2}
FONTS = {'normal': 0, 'soft': 1}
IDENTIFIER = re.compile(r'[a-z][a-z0-9_]*\Z')


def integer(value, minimum, maximum, context):
    if type(value) is not int or not minimum <= value <= maximum:
        raise ValueError(f'{context}: expected an integer from {minimum} to {maximum}')
    return value


def text(value, limit, context, empty=False):
    if not isinstance(value, str) or len(value) > limit or (not value.strip() and not empty):
        raise ValueError(f'{context}: expected {"0" if empty else "1"}..{limit} characters')
    if any(not 32 <= ord(c) <= 126 for c in value):
        raise ValueError(f'{context}: use printable ASCII; provide a glyph mapping before adding other languages')
    return value


def identifier(value, context):
    if not isinstance(value, str) or not IDENTIFIER.fullmatch(value):
        raise ValueError(f'{context}: use a lowercase identifier with letters, digits and underscores')
    return value


def exact_keys(value, allowed, context):
    if not isinstance(value, dict) or set(value) - set(allowed):
        raise ValueError(f'{context}: unknown fields or expected an object')


def markup(source):
    if not isinstance(source, str) or len(source) > 8192:
        raise ValueError('text: expected a string of at most 8192 characters')
    events = []
    color = font = 0
    stack = []
    offset = 0
    while offset < len(source):
        c = source[offset]
        if source.startswith('[[', offset):
            events.append(('glyph', ord('['), font * 3 + color)); offset += 2; continue
        if c != '[':
            if c == '\n': events.append(('newline', 0, 0))
            elif 32 <= ord(c) <= 126: events.append(('glyph', ord(c), font * 3 + color))
            else: raise ValueError(f'text offset {offset}: unsupported character {ord(c)}')
            offset += 1; continue
        end = source.find(']', offset)
        if end == -1: raise ValueError(f'text offset {offset}: missing ]')
        tag = source[offset + 1:end]
        if tag in ['br', 'page']:
            events.append(('newline' if tag == 'br' else 'page', 0, 0))
        elif tag.startswith('/'):
            name = tag[1:]
            if not stack or stack[-1][0] != name:
                raise ValueError(f'text offset {offset}: mismatched closing tag [{tag}]')
            _, color, font = stack.pop()
        elif '=' in tag:
            name, value = tag.split('=', 1)
            if name in ['color', 'font']:
                options = COLORS if name == 'color' else FONTS
                if value not in options: raise ValueError(f'text offset {offset}: unknown {name} {value}')
                stack.append((name, color, font))
                if len(stack) > 8: raise ValueError('text: style nesting exceeds eight levels')
                if name == 'color': color = options[value]
                else: font = options[value]
            elif name in ['pause', 'speed'] and re.fullmatch(r'[0-9]+', value):
                duration = integer(int(value), 1 if name == 'pause' else 0,
                                   5000 if name == 'pause' else 1000, name)
                events.append((name, duration, 0))
            else: raise ValueError(f'text offset {offset}: unknown tag [{tag}]')
        else: raise ValueError(f'text offset {offset}: unknown tag [{tag}]')
        offset = end + 1
    if stack: raise ValueError(f'text: unclosed [{stack[-1][0]}]')
    return events


def layout(source, columns):
    events = markup(source)
    output = []
    column = row = 0
    content = False

    def next_line():
        nonlocal column, row, content
        if row == 3:
            if not content: raise ValueError('text: an empty page cannot advance')
            output.append(('page', 0, 0)); row = 0; content = False
        else:
            output.append(('newline', 0, 0)); row += 1
        column = 0

    for i, (op, value, style) in enumerate(events):
        if op == 'glyph':
            if value == 32:
                length = 0
                for other, glyph, _ in events[i + 1:]:
                    if other in ['newline', 'page'] or (other == 'glyph' and glyph == 32): break
                    if other == 'glyph': length += 1
                if not column or not length: continue
                if column + 1 + length > columns: next_line(); continue
            if column == columns: next_line()
            output.append((op, value, style)); column += 1; content = True
        elif op == 'newline': next_line()
        elif op == 'page':
            if not content: raise ValueError('text: empty explicit page')
            output.append((op, value, style)); column = row = 0; content = False
        else: output.append((op, value, style))
    if not content: raise ValueError('text: missing text or empty trailing page')
    output.append(('finish', 0, 0))
    if len(output) > 2048: raise ValueError('text: exceeds 2048 tokens per node')
    actions = 0
    for op, _, _ in output:
        actions += 1
        if actions > 256: raise ValueError('text: exceeds 256 actions per page; reduce tags or add a page')
        if op == 'page': actions = 0
    return output


def compile_program(document):
    exact_keys(document, ['flags', 'speakers', 'stories', 'nodes'], 'program')
    flags = document.get('flags', [])
    if not isinstance(flags, list) or len(flags) > 16: raise ValueError('flags: expected at most sixteen names')
    flag_ids = {identifier(name, 'flag'): 1 << i for i, name in enumerate(flags)}
    if len(flag_ids) != len(flags): raise ValueError('flags: duplicate name')
    def mask(names):
        if not isinstance(names, list) or any(not isinstance(name, str) or name not in flag_ids for name in names):
            raise ValueError('choice: unknown flag or expected a list')
        if len(names) != len(set(names)): raise ValueError('choice: duplicate flag')
        return sum(flag_ids[name] for name in names)
    raw_speakers = document.get('speakers')
    if not isinstance(raw_speakers, list) or not 1 <= len(raw_speakers) <= 16:
        raise ValueError('speakers: expected 1..16 entries')
    speakers = {}
    for speaker in raw_speakers:
        exact_keys(speaker, ['id', 'name', 'portrait'], 'speaker')
        sid = identifier(speaker.get('id'), 'speaker id')
        if sid in speakers: raise ValueError('speaker: duplicate id')
        portrait = speaker.get('portrait')
        if portrait is not None: integer(portrait, 0, 1, 'portrait')
        speakers[sid] = {'name': text(speaker.get('name'), 12, 'speaker name', empty=True),
                         'portrait': 255 if portrait is None else portrait}
    raw_nodes = document.get('nodes')
    if not isinstance(raw_nodes, list) or not 1 <= len(raw_nodes) <= 64: raise ValueError('nodes: expected 1..64 entries')
    ids = {}
    for i, node in enumerate(raw_nodes):
        exact_keys(node, ['id', 'speaker', 'pane', 'split', 'nameplate', 'text', 'next', 'choices'], 'node')
        nid = identifier(node.get('id'), 'node id')
        if nid in ids: raise ValueError('node: duplicate id')
        ids[nid] = i
    def target(value, end=False):
        if end and value is None: return 65535
        if not isinstance(value, str) or value not in ids: raise ValueError(f'unknown target {value!r}')
        return ids[value]
    nodes = []
    for raw in raw_nodes:
        nid = raw['id']
        try:
            if raw.get('speaker') not in speakers: raise ValueError('unknown speaker')
            speaker = speakers[raw['speaker']]
            if raw.get('pane', 'bottom') not in ['top', 'bottom']: raise ValueError('pane must be top or bottom')
            for field in ['split', 'nameplate']:
                if field in raw and type(raw[field]) is not bool: raise ValueError(f'{field}: expected a boolean')
            choices = raw.get('choices', [])
            if not isinstance(choices, list) or len(choices) > 3: raise ValueError('choices: expected at most three entries')
            compiled_choices = []
            for choice in choices:
                exact_keys(choice, ['label', 'target', 'require', 'forbid', 'set'], 'choice')
                require, forbid = mask(choice.get('require', [])), mask(choice.get('forbid', []))
                if require & forbid: raise ValueError('choice both requires and forbids a flag')
                compiled_choices.append({'label': text(choice.get('label'), 26, 'choice label'),
                    'target': target(choice.get('target')), 'require': require, 'forbid': forbid,
                    'set': mask(choice.get('set', []))})
            nodes.append({'id': nid, 'tokens': layout(raw.get('text'), 34 if speaker['portrait'] == 255 else 29),
                'speaker': speaker['name'], 'portrait': speaker['portrait'],
                'pane': int(raw.get('pane', 'bottom') == 'bottom'),
                'flags': int(raw.get('split', False)) | (2 if raw.get('nameplate', True) and speaker['name'] else 0),
                'next': target(raw.get('next'), end=True), 'choices': compiled_choices})
        except (ValueError, TypeError) as error: raise ValueError(f'node {nid}: {error}') from error
    stories = document.get('stories')
    if not isinstance(stories, list) or not 1 <= len(stories) <= 8: raise ValueError('stories: expected 1..8 entries')
    compiled_stories = []
    for story in stories:
        exact_keys(story, ['title', 'start'], 'story')
        compiled_stories.append({'title': text(story.get('title'), 26, 'story title'),
                                 'start': target(story.get('start'))})
    return {'nodes': nodes, 'stories': compiled_stories, 'flag_mask': sum(flag_ids.values())}


def emit(program):
    quote = lambda value: json.dumps(value, ensure_ascii=True).replace("?", r"\?")
    header = ('/* Generated by compile_dialogue.py; edit the JSON source. */\n'
              '#ifndef DL_LAB_DATA_H\n#define DL_LAB_DATA_H\n#include "dialogue.h"\n'
              f'#define LAB_STORY_COUNT {len(program["stories"])}\n'
              'extern const DlgProgram lab_program;\nextern const u16 lab_starts[LAB_STORY_COUNT];\n'
              'extern const char *const lab_titles[LAB_STORY_COUNT];\n#endif\n')
    lines = ['/* Generated by compile_dialogue.py; edit the JSON source. */', '#include "dialogue_data.h"']
    for i, node in enumerate(program['nodes']):
        lines.append(f'static const DlgToken tokens_{node["id"]}[] = {{')
        lines.extend(f'    {{{value}, {OPS[op]}, {style}}},' for op, value, style in node['tokens'])
        lines.append('};')
    lines.append('static const DlgNode nodes[] = {')
    for i, node in enumerate(program['nodes']):
        choices = ', '.join('{'+f'{quote(c["label"])}, {c["target"]}, {c["require"]}, {c["forbid"]}, {c["set"]}' + '}' for c in node['choices']) or '{0}'
        lines.append('    {' + f'.tokens=tokens_{node["id"]}, .speaker={quote(node["speaker"])}, .choices={{{choices}}},\n'
                     f'     .token_count={len(node["tokens"])}, .next={node["next"]}, .pane={node["pane"]}, '
                     f'.portrait={node["portrait"]}, .flags={node["flags"]}, .choice_count={len(node["choices"])}' + '},')
    lines += ['};', f'const DlgProgram lab_program = {{nodes, {len(program["nodes"])}, {program["flag_mask"]}}};',
              'const u16 lab_starts[LAB_STORY_COUNT] = {' + ', '.join(str(s['start']) for s in program['stories']) + '};',
              'const char *const lab_titles[LAB_STORY_COUNT] = {' + ', '.join(quote(s['title']) for s in program['stories']) + '};']
    return {'inc/dialogue_data.h': header, 'src/dialogue_data.c': '\n'.join(lines) + '\n'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('--out', type=Path)
    parser.add_argument('--force', action='store_true', help='replace only the two generated C/header files')
    args = parser.parse_args()
    try:
        program = compile_program(json.loads(args.source.read_text(encoding='utf-8')))
        if args.out:
            files = emit(program)
            if not args.force and any((args.out / name).exists() for name in files):
                raise ValueError('generated files exist; use --force for deliberate regeneration')
            for name, content in files.items():
                path = args.out / name; path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(content, encoding='utf-8')
        print(f'Validated {len(program["nodes"])} nodes and {len(program["stories"])} stories')
        return 0
    except (OSError, ValueError, TypeError) as error:
        print(f'{args.source}: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
