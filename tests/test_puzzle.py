"""Atomic grid rules, independent solution search and compiled SGDK resources."""
import collections
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import random
import shutil
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
LAB=ROOT/'skills/sgdk-puzzle/assets/puzzle-lab';FIXTURE=ROOT/'tests/fixtures/puzzle'
def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);sys.path.insert(0,str(path.parent))
    try:spec.loader.exec_module(m)
    finally:sys.path.pop(0)
    return m

def compile_host(cc,path,mode):
    args=[cc,'-DPZ_HOST','-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'inc'),'-I',str(FIXTURE),str(LAB/'src/puzzle.c'),str(LAB/'src/blocks.c'),str(FIXTURE/'host.c'),'-o',str(path)]
    if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
    r=subprocess.run(args,capture_output=True,text=True,timeout=60)
    if r.returncode:raise RuntimeError(r.stdout+r.stderr)

# This oracle uses sets of occupied cells, not C's indexed crate array/history.
def start(board):return board.index('@'),frozenset(i for i,c in enumerate(board)if c=='$'),frozenset()
def solved(board,state):return all(i in state[1] for i,c in enumerate(board)if c=='g')
def transition(board,state,direction):
    p,crates,bridges=state
    if board[p]=='E' and solved(board,state):return 0,state
    dx,dy={'L':(-1,0),'R':(1,0),'U':(0,-1),'D':(0,1)}[direction]
    def neighbor(cell):
        x,y=cell%12+dx,cell//12+dy
        return y*12+x if 0<=x<12 and 0<=y<9 else None
    gate=any(board[i]in'oD' for i in crates|{p})
    def walkable(cell,push=False):
        return cell is not None and board[cell]!='#' and (board[cell]!='D'or gate) and (board[cell]!='~'or cell in bridges or push)
    n=neighbor(p)
    if not walkable(n):return 0,state
    result=1;new=set(crates);filled=set(bridges)
    if n in crates:
        dest=neighbor(n)
        if not walkable(dest,True)or dest in crates:return 0,state
        new.remove(n);result=2
        if board[dest]=='~'and dest not in bridges:filled.add(dest);result=3
        else:new.add(dest)
    result_state=n,frozenset(new),frozenset(filled)
    return (4 if board[n]=='E'and solved(board,result_state)else result),result_state

def solution(board):
    initial=start(board);queue=collections.deque([(initial,'')]);seen={initial}
    while queue:
        state,path=queue.popleft()
        if board[state[0]]=='E'and solved(board,state):return path
        for direction in 'LRUD':
            event,n=transition(board,state,direction)
            if event and n not in seen:seen.add(n);queue.append((n,path+direction))
        if len(seen)>100000:raise AssertionError('Lab solver bound exceeded')
    raise AssertionError('Unsolvable room')

class PuzzleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cc=next((p for n in ['clang','cc','gcc']if(p:=shutil.which(n))),None)
        if not cls.cc:
            if os.environ.get('SGDK_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
            raise unittest.SkipTest('C17 compiler required')
        cls.scratch=tempfile.TemporaryDirectory(prefix='puzzle contracts ');cls.addClassCleanup(cls.scratch.cleanup);cls.binaries=[]
        for mode in ['-O0','-O3']:
            path=Path(cls.scratch.name)/(mode+('.exe'if os.name=='nt'else''));compile_host(cls.cc,path,mode);cls.binaries.append(path)
    def test_atomic_moves_gates_bridges_history_and_20000_step_soak(self):
        traces=[]
        for binary in self.binaries:
            r=subprocess.run([str(binary)],capture_output=True,text=True,timeout=60);self.assertEqual(r.returncode,0,r.stderr);traces.append(r.stdout)
        self.assertEqual(traces[0],traces[1])
    def test_independent_bfs_finds_solution_for_every_authored_room(self):
        for room in range(3):
            board=subprocess.check_output([str(self.binaries[1]),'room',str(room)],text=True).strip();self.assertEqual(len(board),108)
            sequence=solution(board)
            for binary in self.binaries:
                lines=subprocess.check_output([str(binary),'replay',str(room),sequence],text=True).splitlines()
                self.assertEqual(int(lines[-1].split()[0]),4)
                self.assertTrue(all(int(line.split()[0])for line in lines))
    def test_random_replays_match_independent_oracle_including_undo_and_restart(self):
        rng=random.Random(514)
        for room in range(3):
            board=subprocess.check_output([str(self.binaries[1]),'room',str(room)],text=True).strip()
            commands=''.join(rng.choice('LLRRUUDDZX')for _ in range(2400));expected=[];state=start(board);history=[];moves=0
            pits=[i for i,c in enumerate(board)if c=='~']
            for command in commands:
                if command=='X':state=start(board);history=[];moves=0;event=7
                elif command=='Z':
                    event=6 if history else 0
                    if history:state,moves=history.pop()
                else:
                    event,next_state=transition(board,state,command)
                    if event:history.append((state,moves));history=history[-64:];state=next_state;moves+=1
                expected.append((event,state[0],sum(1<<pits.index(i)for i in state[2]),moves,sorted(state[1])))
            for binary in self.binaries:
                lines=subprocess.check_output([str(binary),'replay',str(room),commands],text=True).splitlines()
                self.assertEqual(len(lines),len(expected))
                for line,want in zip(lines,expected):
                    parts=list(map(int,line.split()));self.assertEqual(tuple(parts[:4]),want[:4]);self.assertEqual(sorted(i for i in parts[4:]if i!=255),want[4])

    def test_block_rotation_matches_coordinate_oracle_and_row_compaction(self):
        expected=[]
        for piece,base in enumerate([0x00f0,0x0066,0x0072,0x0036,0x0063,0x0071,0x0074]):
            coords={(i%4,i//4)for i in range(16)if base&(1<<i)}
            for rotation in range(4):
                expected.append(sum(1<<(y*4+x)for x,y in coords))
                if piece!=1:coords={(3-y,x)for x,y in coords}
        rng=random.Random(781)
        for binary in self.binaries:
            self.assertEqual(list(map(int,subprocess.check_output([str(binary),'shapes'],text=True).split())),expected)
            for case in range(30):
                rows=[[rng.randrange(8)for _ in range(10)]for _ in range(18)]
                for row in rows:
                    if rng.randrange(3)==0:row[:]=[rng.randrange(1,8)for _ in range(10)]
                survivors=[row for row in rows if not all(row)];cleared=18-len(survivors)
                wanted=[[0]*10 for _ in range(cleared)]+survivors
                lines=subprocess.check_output([str(binary),'rows',''.join(str(v)for row in rows for v in row)],text=True).splitlines()
                self.assertEqual(int(lines[0]),cleared)
                self.assertEqual(lines[1],''.join(str(v)for row in wanted for v in row))

class PuzzleAssetsTests(unittest.TestCase):
    def test_native_palettes_font_and_standalone_generation(self):
        from test_localization import pixels
        art=load('puzzle_asset_art',LAB/'tools/puzzle_art.py');assets=art.assets();self.assertEqual(assets,art.assets())
        for name,shape in {'garden':(320,224),'atlas':(192,16),'hero':(128,24),'crate':(32,16)}.items():
            w,h,rows=pixels(assets[name+'.png']);self.assertEqual((w,h),shape);self.assertLess(max(v for row in rows for v in row),16)
        for pal in [art.WORLD,art.ACTOR,art.UI]:self.assertEqual(len(pal),16);self.assertTrue(all(0<=v<=7 for rgb in pal for v in rgb))
        self.assertEqual(len(art.font_words()),768)
        prepare=load('puzzle_asset_prepare',LAB/'tools/prepare.py')
        with tempfile.TemporaryDirectory(prefix='puzzle copy ')as scratch:
            first=prepare.prepare(Path(scratch)/'first copy')
            with self.assertRaises(ValueError):prepare.prepare(first)
            second=Path(scratch)/'second copy';r=subprocess.run([sys.executable,str(first/'tools/prepare.py'),str(second)],capture_output=True,text=True,timeout=30);self.assertEqual(r.returncode,0,r.stderr)
            for name in ['res/garden.png','res/atlas.png','res/hero.png','inc/font.h','res/garden.vgm','res/push.wav']:self.assertEqual((first/name).read_bytes(),(second/name).read_bytes())
            blocks=prepare.prepare(Path(scratch)/'blocks copy',lab='blocks')
            roundtrip=Path(scratch)/'garden from blocks'
            r=subprocess.run([sys.executable,str(blocks/'tools/prepare.py'),str(roundtrip)],capture_output=True,text=True,timeout=30);self.assertEqual(r.returncode,0,r.stderr)
            self.assertEqual((roundtrip/'src/main.c').read_bytes(),(first/'src/main.c').read_bytes())
            self.assertEqual((roundtrip/'src/boot/rom_head.c').read_bytes(),(first/'src/boot/rom_head.c').read_bytes())
        with self.assertRaises(ValueError):prepare.prepare(LAB/'new output')

@unittest.skipUnless(os.environ.get('SGDK_SKILLS_INTEGRATION')=='1','Docker integration is opt-in')
class PuzzleContainerTests(unittest.TestCase):
    def test_roms_base_m68000_contracts_and_compiled_resources(self):
        from test_streaming import execute
        prepare=load('puzzle_container_prepare',LAB/'tools/prepare.py');art=load('puzzle_container_art',LAB/'tools/puzzle_art.py');art.assets()
        out=ROOT/'build/puzzle-integration';out.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='puzzle target ',dir=ROOT/'build')as scratch:
            project=prepare.prepare(Path(scratch)/'standalone puzzle lab')
            cc=next((p for n in ['clang','cc','gcc']if(p:=shutil.which(n))),None);self.assertIsNotNone(cc)
            host=Path(scratch)/'host';compile_host(cc,host,'-O3');trace=int(subprocess.check_output([str(host),'trace'],text=True))
            base=['docker','run','--rm']
            if os.environ.get('SGDK_PLATFORM'):base+=['--platform',os.environ['SGDK_PLATFORM']]
            if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
            base+=['--mount',f'type=bind,source={project},target=/workspace',os.environ.get('SGDK_IMAGE','ewrogers/sgdk:latest')]
            def run(*args):
                r=subprocess.run(base+list(args),capture_output=True,text=True,timeout=240);self.assertEqual(r.returncode,0,(r.stdout+r.stderr)[-5000:]);return r.stdout+r.stderr
            hashes={};geometry=None;tiles=None
            for kind in ['rom','pal','ntsc','blocks','blocks-pal','blocks-ntsc','probe']:
                header=(LAB/'variants/rom_head.c').read_text()
                if kind.endswith('pal')or kind.endswith('ntsc'):header=header.replace('.region="JUE"','.region="'+('E'if kind.endswith('pal')else'U')+'"')
                if kind.startswith('blocks'):
                    shutil.copyfile(LAB/'variants/blocks_main.c',project/'src/main.c')
                    header=header.replace('CLOCKWORK GARDEN PUZZLE','FALLING LANTERNS BLOCK LAB').replace('0000063','0000064')
                (project/'src/boot/rom_head.c').write_text(header)
                if kind=='probe':shutil.copyfile(FIXTURE/'probe.c',project/'src/main.c');shutil.copyfile(FIXTURE/'checks.h',project/'inc/checks.h');shutil.copyfile(FIXTURE/'blocks_checks.h',project/'inc/blocks_checks.h')
                for mode in (['release','debug']if kind in ('rom','blocks','probe')else['release']):
                    run('make','clean');log=run('make',mode);(out/f'{kind}-{mode}.log').write_text(log)
                    self.assertEqual([line for line in log.splitlines()if 'warning:'in line],[])
                    rom=(project/'out/rom.bin').read_bytes();self.assertEqual(len(rom),131072)
                    if kind!='probe':self.assertEqual(int.from_bytes(rom[0x1a4:0x1a8],'big'),len(rom)-1)
                    self.assertEqual(rom[0x1f0:0x1f3].rstrip(b'\0 '),{'pal':b'E','ntsc':b'U','blocks-pal':b'E','blocks-ntsc':b'U'}.get(kind,b'JUE'))
                    hashes[kind+'-'+mode]=hashlib.sha256(rom).hexdigest()
                    if kind=='probe':
                        code,values=execute(rom,run('m68k-elf-nm','--defined-only','out/rom.out'));self.assertEqual(code,0,f'Target contract failed at {code}')
                        geometry=values[:2];self.assertEqual(values[2]<<16|values[3],trace)
                        tiles=values[4:6];self.assertEqual(values[6:8],[24,2]);self.assertLess(sum(tiles)+49+96,1024)
                        at=8
                        for w,h,count,pal in [(16,24,8,art.ACTOR),(16,16,2,art.WORLD)]:
                            self.assertEqual(values[at:at+6],[w,h,1,w*h//64,1,count]);at+=6
                            self.assertEqual(values[at:at+16],[(b<<9)|(g<<5)|(r<<1)for r,g,b in pal]);at+=16
                        self.assertEqual(values[at:at+1536],[half for v in art.font_words()for half in(v>>16,v&65535)]);at+=1536;self.assertEqual(values[at:],[])
            (out/'puzzle-report.json').write_text(json.dumps({'passed':True,'hashes':hashes,'geometry':geometry,'trace_digest':trace,
                'budgets':{'garden_atlas_tiles':tiles,'sprite_reservation':48,'hardware_sprites':5,'palettes':3,'board_window_ram_bytes':3104,'board_window_dma_bytes_per_change':3104,'undo_moves':64},
                'execution':'Base M68000 rules and compiled resources; no VDP/audio/peripheral protocol emulation','physical_hardware':'not tested'},indent=2)+'\n')
if __name__=='__main__':unittest.main()
