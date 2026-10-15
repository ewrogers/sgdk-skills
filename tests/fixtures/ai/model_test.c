#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "world.h"

static void disable_enemies(Game *game)
{
    for (u16 i=0;i<ENEMY_COUNT;++i) enemy_change(&game->enemies[i],STUNNED,60000);
}
static void test_world(void)
{
    assert(world_solid(0,15,64,16,16) && world_solid(0,16,63,16,16));
    assert(!world_solid(0,80,80,16,16) && world_solid(0,81,80,16,16));
    assert(!world_line(0,(Point){80,80},(Point){112,80}));
    assert(!world_line(0,(Point){96,64},(Point){112,80})); /* Solid side at a diagonal corner. */
    assert(world_line(0,(Point){16,64},(Point){80,64}));
    for (u16 a=0;a<GRID_CELLS;++a) for (u16 b=0;b<GRID_CELLS;++b)
        assert(world_line(0,world_point(a),world_point(b))==world_line(0,world_point(b),world_point(a)));
    Point point={80,80}; assert(world_move(0,&point,2,0) && point.x==82);
    assert(world_move(0,&point,2,2) && point.x==82 && point.y==82);
    assert(world_cell((Point){0,0})==NO_CELL);
}
static void test_senses(void)
{
    Game game; game_init(&game); Enemy *enemy=&game.enemies[0];
    enemy->pos=(Point){112,64}; enemy->facing=0;
    game.players[0].pos=(Point){112,160};
    assert(enemy_visible(0,enemy,&game.players[0],false));
    enemy->facing=2; assert(!enemy_visible(0,enemy,&game.players[0],false));
    assert(enemy_visible(0,enemy,&game.players[0],true));
    enemy->kind=HUNTER; assert(enemy_visible(0,enemy,&game.players[0],false));
    game.players[0].pos.y=162; assert(!enemy_visible(0,enemy,&game.players[0],false));
    assert(enemy_visible(0,enemy,&game.players[0],true));
    game.players[0].pos=(Point){112,128}; game.players[1]=game.players[0]; game.players[1].active=true;
    enemy_change(enemy,PATROL,0); enemy_sense(&game,0); assert(enemy->target==0 && enemy->state==CHASE);
    game.players[1].pos.y=96; enemy_sense(&game,0); assert(enemy->target==0); /* Keep a visible target. */
    game.players[0].active=false; enemy_sense(&game,0); assert(enemy->target==1);
    enemy->pos=(Point){80,80}; game.players[1].pos=(Point){112,80};
    assert(!enemy_visible(0,enemy,&game.players[1],true));
    assert(enemy_cone(0,10,10) && !enemy_cone(0,11,10) && !enemy_cone(0,0,-1));
}
static void oracle(u16 level,u16 start,u16 distance[GRID_CELLS])
{
    u16 queue[GRID_CELLS],head=0,tail=1; queue[0]=start;
    for (u16 i=0;i<GRID_CELLS;++i) distance[i]=NO_CELL;
    distance[start]=0;
    while (head<tail) {
        u16 cell=queue[head++]; s16 x=cell%GRID_W,y=cell/GRID_W;
        for (s16 ny=y-1;ny<=y+1;++ny) for (s16 nx=x-1;nx<=x+1;++nx) {
            if ((nx==x)==(ny==y) || world_wall(level,nx,ny)) continue;
            u16 next=ny*GRID_W+nx;
            if (distance[next]!=NO_CELL) continue;
            distance[next]=distance[cell]+1; queue[tail++]=next;
        }
    }
}
static void test_paths(void)
{
    Game game;
    for (u16 level=0;level<LEVEL_COUNT;++level) for (u16 start=0;start<GRID_CELLS;++start) {
        if (world_wall(level,start%GRID_W,start/GRID_W)) continue;
        u16 distance[GRID_CELLS]; oracle(level,start,distance);
        for (u16 goal=0;goal<GRID_CELLS;++goal) {
            if (world_wall(level,goal%GRID_W,goal/GRID_W)) continue;
            game_init(&game); game_load_level(&game,level); Enemy *enemy=&game.enemies[0];
            enemy->pos=enemy->home=world_point(start); enemy->patrol=world_point(goal); enemy->outbound=true;
            nav_request(enemy);
            for (u16 frame=0;frame<GRID_CELLS+1 && !game.nav.completed && !game.nav.failed;++frame) {
                nav_frame(&game,1); assert(game.nav.expanded<=1 && game.nav.tail<=GRID_CELLS);
            }
            assert(game.nav.completed==1 && !game.nav.failed && !enemy->pending);
            assert(enemy->route_len==(distance[goal]<ROUTE_CAPACITY ? distance[goal] : ROUTE_CAPACITY));
            u16 before=start;
            for (u16 i=0;i<enemy->route_len;++i) {
                u16 cell=enemy->route[i];
                s16 dx=(cell%GRID_W)-(before%GRID_W),dy=(cell/GRID_W)-(before/GRID_W);
                assert((dx==0 && (dy==1 || dy==-1)) || (dy==0 && (dx==1 || dx==-1)));
                assert(distance[cell]==i+1); before=cell;
            }
            if (distance[goal]<=ROUTE_CAPACITY) assert(before==goal);
        }
    }
}
static void test_scheduler(void)
{
    Game game; game_init(&game);
    for (u16 i=0;i<ENEMY_COUNT;++i) { game.enemies[i].state=PATROL; nav_request(&game.enemies[i]); }
    for (u16 frame=0;frame<ENEMY_COUNT*(GRID_CELLS+1) && game.nav.completed<ENEMY_COUNT;++frame) {
        nav_frame(&game,1); assert(game.nav.expanded<=1);
    }
    assert(game.nav.completed==ENEMY_COUNT);
    game_init(&game); game.enemies[0].patrol=(Point){288,192}; nav_request(&game.enemies[0]);
    nav_frame(&game,0); assert(!game.nav.active && !game.nav.expanded);
    nav_frame(&game,1); assert(game.nav.active);
    enemy_change(&game.enemies[0],STUNNED,90); nav_frame(&game,16);
    assert(game.nav.cancelled==1 && !game.enemies[0].route_len && !game.enemies[0].pending);
    game_init(&game); nav_request(&game.enemies[0]); nav_frame(&game,1);
    game.enemies[0].pos.x+=1; nav_frame(&game,16); assert(game.nav.cancelled==1);
}
static void test_states(void)
{
    Game game; game_init(&game); Enemy *enemy=&game.enemies[0]; enemy->kind=HUNTER;
    enemy->pos=enemy->home=(Point){32,80}; game.players[0].pos=(Point){48,80}; game.players[0].invulnerable=0;
    enemy_sense(&game,0); assert(enemy->state==WINDUP && enemy->timer==24);
    game.players[0].pos=(Point){144,64};
    for (u16 i=0;i<24;++i) enemy_tick(&game,0);
    assert(enemy->state==RECOVER && !game.players[0].hits);
    enemy_change(enemy,PATROL,0); game.players[0].pos=(Point){48,80}; enemy_sense(&game,0);
    for (u16 i=0;i<24;++i) enemy_tick(&game,0);
    assert(game.players[0].hits==1 && game.players[0].invulnerable==90);
    player_hit(&game,0); assert(game.players[0].hits==1);
    enemy->home=(Point){16,80}; enemy_change(enemy,CHASE,0); enemy->memory=1; enemy_tick(&game,0);
    assert(enemy->state==RETURN);
    enemy_change(enemy,STUNNED,1); enemy_tick(&game,0); assert(enemy->state==RETURN);
}
static void test_shots(void)
{
    static const s16 dx[]={24,17,0,-17,-24,-17,0,17},dy[]={0,17,24,17,0,-17,-24,-17};
    Game game; game_init(&game); game.players[0].active=false;
    for (u16 direction=0;direction<8;++direction) {
        game.shots[0]=(Shot){144*8,64*8,90,direction,true}; shots_tick(&game);
        assert(game.shots[0].x==144*8+dx[direction] && game.shots[0].y==64*8+dy[direction]);
    }
    game_init(&game); for (u16 i=0;i<SHOT_COUNT;++i) assert(shot_fire(&game,(Point){144,64},0));
    assert(!shot_fire(&game,(Point){144,64},0) && !shot_fire(&game,(Point){144,64},8));
    for (u16 i=0;i<90;++i) shots_tick(&game);
    for (u16 i=0;i<SHOT_COUNT;++i) assert(!game.shots[i].active);
    game_init(&game); game.players[0].pos=(Point){48,64}; game.players[0].invulnerable=0;
    game.shots[0]=(Shot){44*8,64*8,90,0,true}; shots_tick(&game);
    assert(!game.shots[0].active && game.players[0].hits==1);
    game.shots[0]=(Shot){88*8,80*8,90,0,true}; shots_tick(&game); assert(!game.shots[0].active);
    game_init(&game); game.players[0].pos=(Point){48,112}; game.players[0].invulnerable=0;
    game.shots[0]=(Shot){44*8,106*8,90,0,true}; shots_tick(&game);
    assert(game.shots[0].active && !game.players[0].hits); /* Artwork edge, outside the inset body. */
    game.shots[0]=(Shot){44*8,107*8,90,0,true}; shots_tick(&game);
    assert(!game.shots[0].active && game.players[0].hits==1);
}
static void test_input_pause_and_clock(void)
{
    Game game; game_init(&game); Input input={0}; u16 previous[2]={0},held[2]={START,START};
    game_input(&input,previous,held); game_frame(&game,&input,1);
    assert(game.paused && game.players[1].active && (game.events&EVENT_JOIN));
    Game frozen=game; frozen.events=frozen.sensors=frozen.nav.expanded=0;
    for (u16 i=0;i<120;++i) { game_input(&input,previous,held); game_frame(&game,&input,1); assert(!memcmp(&frozen,&game,sizeof game)); }
    held[0]=held[1]=0; game_input(&input,previous,held); game_frame(&game,&input,1);
    held[0]=START; game_input(&input,previous,held); game_frame(&game,&input,1); assert(!game.paused);
    for (u16 hz=50;hz<=60;hz+=10) {
        game_init(&game); disable_enemies(&game); game.players[0].pos=(Point){80,192}; u16 fraction=0; input=(Input){{RIGHT,0},{0}};
        for (u16 frame=0;frame<hz*5;++frame) {
            u16 ticks=game_clock(&fraction,hz); game_frame(&game,&input,ticks);
            assert(game.sensors<=ticks && game.nav.expanded<=NAV_BUDGET);
        }
        assert(game.tick==300 && game.enemies[0].timer==59700 && game.players[0].pos.x==178);
    }
    assert(!game_clock(NULL,60)); u16 fraction=0; assert(!game_clock(&fraction,55));
}
static void test_stun_recovery_and_long_run(void)
{
    Game game; game_init(&game);
    game.players[0].pos=(Point){48,80}; game.enemies[0].pos=(Point){66,80};
    Input input={{0},{A,0}}; game_frame(&game,&input,1);
    assert(game.enemies[0].state==STUNNED && (game.events&EVENT_STUN));
    input=(Input){0}; game.players[0].invulnerable=60000;
    for (u16 tick=0;tick<4000;++tick) {
        input.held[0]=0;
        game_frame(&game,&input,1);
        assert(game.nav.expanded<=NAV_BUDGET && game.nav.tail<=GRID_CELLS && game.sensors==1);
        for (u16 i=0;i<ENEMY_COUNT;++i) {
            assert(!world_actor_solid(0,game.enemies[i].pos));
            assert(game.enemies[i].route_len<=ROUTE_CAPACITY);
        }
    }
    assert(game.nav.completed>6);
    input.pressed[0]=B; game_frame(&game,&input,1); Game reset; game_init(&reset);
    assert(!memcmp(&game,&reset,sizeof game));
}

static void test_level_flow(void)
{
    Game game; game_init(&game); Input idle={0};
    assert(game.tries==3 && game.level==0 && game.phase==PLAYING);
    game.players[1].active=true;
    for (u16 level=0;level<LEVEL_COUNT;++level) {
        assert(game.level==level && game.tries==3 && game.players[1].active);
        u16 distance[GRID_CELLS]; oracle(level,world_cell(game.players[0].pos),distance);
        assert(distance[world_cell(world_exit(level))]!=NO_CELL);
        for (u16 i=0;i<ENEMY_COUNT;++i) {
            assert(!world_solid(level,game.enemies[i].home.x,game.enemies[i].home.y,16,16));
            assert(!world_solid(level,game.enemies[i].patrol.x,game.enemies[i].patrol.y,16,16));
        }
        disable_enemies(&game); game.players[0].pos=world_exit(level);
        game_frame(&game,&idle,1); assert(game.phase==AREA_CLEAR && (game.events&EVENT_CLEAR));
        for (u16 tick=0;tick<60;++tick) {
            game_frame(&game,&idle,1); assert(!(game.events&EVENT_CLEAR));
        }
    }
    assert(game.phase==COMPLETE);
    idle.pressed[0]=START; game_frame(&game,&idle,1);
    assert(game.phase==PLAYING && game.level==0 && game.tries==3);
    idle=(Input){0};
    game_load_level(&game,1);
    for (u16 attempt=0;attempt<3;++attempt) {
        game.players[0].pos=(Point){144,64};
        game.players[0].invulnerable=0; player_hit(&game,0); player_hit(&game,0);
        assert(game.tries==2-attempt && game.phase==CAUGHT);
        for (u16 tick=0;tick<60;++tick) game_frame(&game,&idle,1);
        assert(game.level==1);
        if (attempt<2) assert(game.phase==PLAYING && game.players[0].pos.x==288 && !game.nav.active);
    }
    assert(game.phase==GAME_OVER && game.tries==0);
    u32 frozen=game.tick; for (u16 i=0;i<120;++i) game_frame(&game,&idle,1);
    assert(game.phase==GAME_OVER && game.tick==frozen);
    idle.pressed[0]=START; game_frame(&game,&idle,1);
    assert(game.phase==PLAYING && game.level==0 && game.tries==3);
}

static void test_entry(void)
{
    Game game; Input idle={0}; game_init(&game);
    for (u16 level=0;level<LEVEL_COUNT;++level) {
        game_load_level(&game,level); game.players[1].active=true;
        if (level) {
            Point exit=world_exit(level-1);
            assert(game.players[0].pos.x==exit.x && game.players[0].pos.y==exit.y);
        }
        for (u16 p=0;p<2;++p) {
            assert(world_entry_pad(level,game.players[p].pos)); game.players[p].invulnerable=0;
            assert(!world_solid(level,game.players[p].pos.x,game.players[p].pos.y,16,16));
            for (u16 i=0;i<ENEMY_COUNT;++i) {
                if (enemy_visible(level,&game.enemies[i],&game.players[p],false))
                    fprintf(stderr,"Entry exposed: level %u player %u enemy %u\n",level,p,i);
                assert(!enemy_visible(level,&game.enemies[i],&game.players[p],false));
            }
        }
        for (u16 frame=0;frame<1800;++frame) game_frame(&game,&idle,1);
        assert(game.phase==PLAYING && game.tries==3 && !game.players[0].hits);
        game.players[0].pos=(Point){144,64}; game.players[0].invulnerable=0;
        assert(!world_entry_pad(level,game.players[0].pos)); player_hit(&game,0);
        assert(game.phase==CAUGHT && game.tries==2);
        game_load_level(&game,level); game.players[0].invulnerable=0;
        player_hit(&game,0); /* The visual entry pad does not grant immunity. */
        assert(game.phase==CAUGHT && game.tries==1);
        game_init(&game);
    }
}
static void test_stun_cooldown(void)
{
    for (u16 hz=50;hz<=60;hz+=10) {
        Game game; game_init(&game); disable_enemies(&game);
        Input input={{0},{A,0}}; u16 fraction=0;
        game_frame(&game,&input,game_clock(&fraction,hz));
        assert(game.players[0].pulse && game.players[0].cooldown==179);
        u32 fired=game.tick;
        for (u16 frame=1;frame<hz*3;++frame) {
            u16 before=game.players[0].cooldown;
            game_frame(&game,&input,game_clock(&fraction,hz));
            assert(game.players[0].cooldown<before);
        }
        assert(!game.players[0].cooldown && !game.players[0].pulse && game.tick-fired==179);
        game_frame(&game,&input,1); assert(game.players[0].cooldown==179 && game.players[0].pulse==15);
        input=(Input){.pressed={START,0}}; game_frame(&game,&input,1);
        assert(game.paused); u16 frozen=game.players[0].cooldown;
        input=(Input){.pressed={A,0}};
        for (u16 frame=0;frame<300;++frame) game_frame(&game,&input,1);
        assert(game.players[0].cooldown==frozen);
    }
}

static void test_doorways_and_routes(void)
{
    for (u16 level=0;level<LEVEL_COUNT;++level) {
        for (u16 player=0;player<2;++player) for (s16 offset=-2;offset<=2;++offset) {
            Point position=world_entry(level,player); position.x+=offset;
            if (world_actor_solid(level,position)) continue;
            s16 start_y=position.y,dy=level ? 2 : -2;
            for (u16 step=0;step<24;++step) {
                assert(world_move(level,&position,0,dy));
                assert(!world_actor_solid(level,position));
            }
            assert(position.y==start_y+24*dy);
        }
        /* A slightly off-center body can pass the lower two-cell gap too. */
        for (s16 offset=-2;offset<=2;++offset) {
            Point position={level==1 ? 208 : level==2 ? 64 : 80,176+offset};
            for (u16 step=0;step<16;++step) assert(world_move(level,&position,2,0));
        }
        Point start=world_entry(level,0),exit=world_exit(level);
        u16 distance[GRID_CELLS]; oracle(level,world_cell(exit),distance);
        s16 dx=exit.x-start.x,dy=exit.y-start.y;
        u16 direct=((dx<0 ? -dx : dx)+(dy<0 ? -dy : dy))/16;
        assert(distance[world_cell(start)]>=direct+8 && distance[world_cell(start)]!=NO_CELL);
        Point position=start;
        while (distance[world_cell(position)]) {
            u16 cell=world_cell(position),next=NO_CELL;
            static const s16 offsets[4][2]={{-1,0},{1,0},{0,-1},{0,1}};
            for (u16 d=0;d<4;++d) {
                s16 x=cell%GRID_W+offsets[d][0],y=cell/GRID_W+offsets[d][1];
                if (!world_wall(level,x,y) && distance[y*GRID_W+x]+1==distance[cell]) { next=y*GRID_W+x; break; }
            }
            assert(next!=NO_CELL); Point goal=world_point(next);
            for (u16 step=0;step<8;++step) {
                s16 mx=2*((goal.x>position.x)-(goal.x<position.x));
                s16 my=2*((goal.y>position.y)-(goal.y<position.y));
                assert(world_move(level,&position,mx,my) && !world_actor_solid(level,position));
            }
            assert(position.x==goal.x && position.y==goal.y);
        }
        assert(position.x==exit.x && position.y==exit.y);

        /* Searching only the outer two rows/columns must not reach the exit. */
        bool seen[GRID_CELLS]={0}; u16 queue[GRID_CELLS],head=0,tail=1;
        queue[0]=world_cell(start); seen[queue[0]]=true;
        while (head<tail) {
            u16 cell=queue[head++]; s16 x=cell%GRID_W,y=cell/GRID_W;
            for (s16 ny=y-1;ny<=y+1;++ny) for (s16 nx=x-1;nx<=x+1;++nx) {
                if ((nx==x)==(ny==y) || world_wall(level,nx,ny)) continue;
                if (nx>=2 && nx<GRID_W-2 && ny>=2 && ny<GRID_H-2) continue;
                u16 next=ny*GRID_W+nx;
                if (!seen[next]) { seen[next]=true; queue[tail++]=next; }
            }
        }
        assert(!seen[world_cell(exit)]);
    }
}

int main(int argc,char **argv)
{
    if (argc!=2) return 2;
    if (!strcmp(argv[1],"flow")) test_level_flow();
    else if (!strcmp(argv[1],"world")) test_world();
    else if (!strcmp(argv[1],"senses")) test_senses();
    else if (!strcmp(argv[1],"paths")) test_paths();
    else if (!strcmp(argv[1],"scheduler")) test_scheduler();
    else if (!strcmp(argv[1],"states")) test_states();
    else if (!strcmp(argv[1],"shots")) test_shots();
    else if (!strcmp(argv[1],"clock")) test_input_pause_and_clock();
    else if (!strcmp(argv[1],"run")) test_stun_recovery_and_long_run();
    else if (!strcmp(argv[1],"entry")) test_entry();
    else if (!strcmp(argv[1],"stun")) test_stun_cooldown();
    else if (!strcmp(argv[1],"doors")) test_doorways_and_routes();
    else return 2;
    puts(argv[1]); return 0;
}
