"""Compose installed skill modules, not a completed Lab's main loop."""
import importlib.util
from pathlib import Path
import shutil
import sys

SKILLS=('sgdk-project','sgdk-streaming','sgdk-audio','sgdk-scenes','sgdk-graphics','sgdk-input','sgdk-testing','sgdk-release','sgdk-planning')
def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec)
    sys.path.insert(0,str(path.parent))
    try:spec.loader.exec_module(m)
    finally:sys.path.pop(0)
    return m
def populate(project,skills):
    project=Path(project);skills=Path(skills);fixture=Path(__file__).parent
    if not(project/'src/main.c').is_file() or (project/'inc/route.h').exists():raise ValueError('Use a fresh project initializer output')
    stream=skills/'sgdk-streaming/assets/streaming-lab';audio=skills/'sgdk-audio/assets/sound-lab'
    for folder in ('inc','res'):(project/folder).mkdir(exist_ok=True)
    for name in ('stream_cache','stream_world','stream_io'):
        shutil.copyfile(stream/'inc'/(name+'.h'),project/'inc'/(name+'.h'))
        shutil.copyfile(stream/'src'/(name+'.c'),project/'src'/(name+'.c'))
    shutil.copyfile(audio/'inc/director.h',project/'inc/director.h');shutil.copyfile(audio/'src/director.c',project/'src/director.c')
    for name in ('route.h','view.h','controls.h'):shutil.copyfile(fixture/name,project/'inc'/name)
    for name in ('route.c','view.c','main.c'):shutil.copyfile(fixture/name,project/'src'/name)
    art=load('installed_stream_art',stream/'tools/stream_art.py')
    for name,data in art.assets().items():
        if name in ('pages.bin','hero.bin','palettes.h'):(project/('inc'if name.endswith('.h')else'res')/name).write_bytes(data)
    music=load('installed_audio_authoring',audio/'tools/desk_audio.py')
    for name,track in [('theme0','harbor'),('theme1','danger'),('fanfare','fanfare')]:(project/'res'/(name+'.vgm')).write_bytes(music.score(track))
    (project/'res/coin.wav').write_bytes(music.sound(0,True));(project/'res/alarm.wav').write_bytes(music.sound(3,True))
    (project/'res/resources.res').write_text('BIN stream_hero "hero.bin" 4 4 0 NONE FALSE\nBIN stream_pages "pages.bin" 4 4 0 NONE TRUE\nXGM2 theme0 "theme0.vgm"\nXGM2 theme1 "theme1.vgm"\nXGM2 fanfare "fanfare.vgm"\nWAV coin "coin.wav" XGM2\nWAV alarm "alarm.wav" XGM2\n')
    (project/'Makefile').write_text('include $(GDK)/makefile.gen\n')
    (project/'README.txt').write_text('Stream + Sound composition check\nLeft/Right walk, A warp, B toggle 64/256-byte tile uploads, C fanfare, Start pause. Pause requests during transitions apply after the fade/start completes.\n')
    return project
