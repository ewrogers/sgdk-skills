"""Instruction proxy for identical sparse-hit projectile workloads, not VDP cycles."""
from pathlib import Path
import shutil

PROBE = '''#include "game.h"
void reference_game_projectiles(Game *game);
static Game game;
int __attribute__((noipa)) main(void) {
 game_start(&game); game.invulnerable=0;
 for (u16 i=0;i<ENEMY_COUNT;i++) game.enemies[i]=(Enemy){.x=(30+i*30)*UNIT,.y=40*UNIT,.hp=255,.active=true};
 for (u16 i=0;i<SHOT_COUNT;i++) game.shots[i]=(Bullet){.x=(12+i*18)*UNIT,.y=120*UNIT,.vy=-6*UNIT,.active=true};
 for (u16 i=0;i<BULLET_COUNT;i++) game.bullets[i]=(Bullet){.x=(8+i*7)*UNIT,.y=80*UNIT,.vy=2*UNIT,.active=true};
 CALL(&game); return game.lives==3?0:1; }
'''


def profile(project, run):
    from unicorn import Uc, UC_ARCH_M68K, UC_MODE_BIG_ENDIAN, UC_HOOK_CODE
    from unicorn.m68k_const import (UC_CPU_M68K_M68000, UC_M68K_REG_A7,
                                   UC_M68K_REG_SR, UC_M68K_REG_PC, UC_M68K_REG_D0)
    fixture = Path(__file__).parent/'fixtures/shmup/collision_reference.c'
    shutil.copyfile(fixture, project/'src/collision_reference.c')
    results = {}
    for function in ['reference_game_projectiles', 'game_projectiles']:
        (project/'src/main.c').write_text(PROBE.replace('CALL', function))
        # SGDK shares object paths between debug and release. Do not measure
        # a mixed build left by the preceding debug target probe.
        run('make', 'clean')
        run('make', 'release')
        symbols = run('m68k-elf-nm', 'out/rom.out')
        entry = next(int(line.split()[0], 16) for line in symbols.splitlines()
                     if len(line.split()) == 3 and line.split()[2] == 'main')
        machine = Uc(UC_ARCH_M68K, UC_MODE_BIG_ENDIAN)
        machine.ctl_set_cpu_model(UC_CPU_M68K_M68000)
        machine.mem_map(0, 0x400000)
        machine.mem_map(0xE0FF0000, 0x10000)
        machine.mem_write(0, (project/'out/rom.bin').read_bytes())
        machine.mem_write(0xE0FFFFE0, (0x300000).to_bytes(4, 'big'))
        machine.reg_write(UC_M68K_REG_SR, 0x2700)
        machine.reg_write(UC_M68K_REG_A7, 0xE0FFFFE0)
        count = [0]
        def record(*args): count[0] += 1
        machine.hook_add(UC_HOOK_CODE, record)
        machine.emu_start(entry, 0x300000, count=2_000_000)
        if machine.reg_read(UC_M68K_REG_PC) != 0x300000 or machine.reg_read(UC_M68K_REG_D0) != 0:
            raise AssertionError('Collision workload failed on M68000')
        results[function] = count[0]
    return results
