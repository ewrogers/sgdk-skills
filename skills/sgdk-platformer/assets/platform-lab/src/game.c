#include "game.h"

static const s16 checkpoint_x[] = {32,448,704,976};
static const s16 checkpoint_y[] = {236,188,236,236};

static s32 approach(s32 value,s32 target,s32 amount)
{
    if (value < target) return value + amount > target ? target : value + amount;
    return value - amount < target ? target : value - amount;
}

void game_respawn(Game *game)
{
    Player *p = &game->player;
    p->body = (Body){.x=checkpoint_x[game->checkpoint]*UNIT,
        .y=checkpoint_y[game->checkpoint]*UNIT,.width=12,.height=20,.ground=true,.support=-1};
    p->facing = 1; p->invulnerable = 90; p->hearts = 3;
    p->buffer = p->coyote = p->drop = p->dash = p->lock = 0;
    p->charge = 0; p->air_jump = p->boots; p->air_dash = true;
    p->variable_jump = p->dashing = false;
    game->camera_x = checkpoint_x[game->checkpoint] - 144;
    if (game->camera_x < 0) game->camera_x = 0;
    if (game->camera_x > 960) game->camera_x = 960;
    game->camera_y = 64;
    game->teleport = true;
    game_enemies_init(game);
}

void game_init(Game *game)
{
    *game = (Game){0};
    game->platform = (Platform){.x=760*UNIT,.previous_x=760*UNIT,.y=208*UNIT,.direction=1};
    const Pickup pickups[PICKUP_COUNT] = {
        {544,224,0,false},{704,224,1,false},{880,224,2,false},
        {178,138,3,false},{338,146,3,false},{644,100,3,false},
        {1080,220,4,false},{1112,220,4,false},{1144,220,4,false},{1208,224,5,false}
    };
    for (u16 i=0;i<PICKUP_COUNT;i++) game->pickups[i]=pickups[i];
    game_respawn(game);
}

void game_hurt(Game *game,s16 direction)
{
    Player *p=&game->player;
    if (p->invulnerable || game->complete) return;
    game->events |= EVENT_HURT;
    if (--p->hearts == 0) { game->deaths++; game_respawn(game); return; }
    p->invulnerable=90; p->lock=12; p->dash=0; p->charge=0; p->dashing=false;
    p->body.vx=direction*UNIT*5/2; p->body.vy=-3*UNIT;
    p->body.ground=false; p->variable_jump=false;
}

static void platform_step(Game *game)
{
    Platform *platform=&game->platform;
    platform->previous_x=platform->x;
    platform->x += platform->direction*UNIT;
    if (platform->x >= 824*UNIT) { platform->x=824*UNIT; platform->direction=-1; }
    if (platform->x <= 760*UNIT) { platform->x=760*UNIT; platform->direction=1; }
    Body *body=&game->player.body;
    if (body->ground && body->support==0 && body->x < platform->previous_x+32*UNIT &&
        body->x+body->width*UNIT > platform->previous_x)
    {
        /* One-way horizontal platform: blocked carry releases the rider;
         * it never pushes them through a wall or crushes them. */
        s32 old=body->x;
        game_move(game,body,platform->x-platform->previous_x,0,false,false,0);
        if (body->x-old == platform->x-platform->previous_x)
        { body->ground=true; body->support=0; }
        else body->support=-1;
    }
}

static void jump(Player *p,Game *game,u16 held)
{
    if (!p->buffer) return;
    bool drop=(held&DOWN) && p->body.ground &&
        (p->body.support==0 || game_tile(game,(p->body.x+p->body.width*UNIT/2)/(CELL*UNIT),
                                  (p->body.y+p->body.height*UNIT+UNIT)/(CELL*UNIT))==ONEWAY);
    if (drop)
    { p->drop=14; p->buffer=0; p->coyote=0; p->body.y+=2*UNIT; p->body.ground=false; return; }
    if (p->body.ground || p->coyote)
        p->body.vy=JUMP_SPEED;
    else if (p->body.wall)
    {
        p->body.vy=-UNIT*25/4;
        p->body.vx=-p->body.wall*UNIT*13/4;
        p->facing=-p->body.wall; p->lock=8;
    }
    else if (p->boots && p->air_jump)
    { p->body.vy=-6*UNIT; p->air_jump=false; }
    else return;
    p->body.ground=false; p->body.support=-1; p->buffer=0; p->coyote=0;
    p->variable_jump=true; game->events|=EVENT_JUMP;
}

static void player_step(Game *game,u16 held,u16 pressed)
{
    Player *p=&game->player;
    Body *body=&p->body;
    if (p->invulnerable) p->invulnerable--;
    if (p->boost) p->boost--;
    if (p->drop) p->drop--;
    if (body->ground) { p->coyote=COYOTE_TICKS; p->air_jump=p->boots; p->air_dash=true; }
    else if (p->coyote) p->coyote--;
    if (pressed&A) p->buffer=BUFFER_TICKS;
    jump(p,game,held);
    if (p->buffer) p->buffer--;
    if (p->variable_jump && !(held&A) && body->vy<SHORT_JUMP) body->vy=SHORT_JUMP;
    if (body->vy>=0) p->variable_jump=false;
    if (p->battery && (held&B) && !p->dash)
    { if (p->charge<CHARGE_TICKS) p->charge++; }
    else
    {
        if (p->charge==CHARGE_TICKS && !(held&B) && p->air_dash)
        {
            p->dash=DASH_TICKS; p->air_dash=false;
            body->vy=0; p->variable_jump=false; game->events|=EVENT_DASH;
        }
        p->charge=0;
    }
    p->dashing=p->dash>0;
    if (p->dashing)
    {
        body->vx=p->facing*8*UNIT; body->vy=0;
        p->dash--;
    }
    else
    {
        s16 direction=((held&RIGHT)!=0)-((held&LEFT)!=0);
        if (direction) p->facing=direction;
        s32 speed=p->boost ? 7*UNIT/2 : RUN_SPEED;
        if (p->charge) speed/=2;
        if (p->lock) p->lock--;
        else body->vx=approach(body->vx,direction*speed,direction?RUN_ACCEL:BRAKE);
        body->vy+=GRAVITY;
        if (body->vy>MAX_FALL) body->vy=MAX_FALL;
        if (body->wall && !body->ground && body->vy>3*UNIT/2) body->vy=3*UNIT/2;
    }
    bool charging_dash=p->dashing;
    game_move(game,body,body->vx,body->vy,true,charging_dash,p->drop);
    if (body->wall && charging_dash) p->dash=0;
    /* A buffered jump fires immediately when a descent lands. */
    if (body->ground && p->buffer) jump(p,game,held);
    if (body->y>(WORLD_H*CELL+32)*UNIT)
    { game->deaths++; game->events|=EVENT_HURT; game_respawn(game); }
}

static void interactions(Game *game)
{
    Body *body=&game->player.body;
    if (!game->secret && body->x+body->width*UNIT>1056*UNIT && body->x<1168*UNIT &&
        body->y+body->height*UNIT>192*UNIT)
    { game->secret=true; game->terrain_revision++; game->events|=EVENT_SECRET; }
    for (u16 i=0;i<PICKUP_COUNT;i++)
    {
        Pickup *pickup=&game->pickups[i];
        if (pickup->collected || (pickup->kind==4 && !game->secret)) continue;
        Body box={.x=pickup->x*UNIT,.y=pickup->y*UNIT,.width=16,.height=16};
        if (!game_overlap(body,&box)) continue;
        pickup->collected=true; game->events|=EVENT_PICKUP;
        if (pickup->kind==0) { game->player.boots=true; game->player.air_jump=true; }
        else if (pickup->kind==1) game->player.battery=true;
        else if (pickup->kind==2) game->player.boost=360;
        else if (pickup->kind==5) { game->complete=true; game->events|=EVENT_CLEAR; }
        else game->score+=pickup->kind==4 ? 5 : 1;
    }
    for (u8 i=1;i<4;i++)
        if (i>game->checkpoint && body->x>=checkpoint_x[i]*UNIT)
        { game->checkpoint=i; game->events|=EVENT_CHECKPOINT; }
}

static void camera_step(Game *game)
{
    s32 x=game->player.body.x/UNIT-144+game->player.facing*32;
    s32 y=game->player.body.y/UNIT-112;
    if (x<0) x=0;
    if (x>960) x=960;
    if (y<0) y=0;
    if (y>64) y=64;
    game->camera_x=(s16)approach(game->camera_x,x,4);
    game->camera_y=(s16)approach(game->camera_y,y,2);
}

void game_step(Game *game,u16 held,u16 pressed)
{
    if (game->paused || game->complete) return;
    game->ticks++;
    platform_step(game);
    s32 old_feet=game->player.body.y+game->player.body.height*UNIT;
    player_step(game,held,pressed);
    if (game->teleport) return;
    game_enemies_step(game);
    game_contacts(game,old_feet,held);
    interactions(game);
    camera_step(game);
}

void game_frame(Game *game,u16 held,u16 hz)
{
    u16 pressed=held&~game->held;
    game->held=held; game->events=0; game->teleport=false;
    if (game->complete)
    {
        if (pressed&START) { game_init(game); game->held=held; }
        else if (hz==50 || hz==60)
        {
            game->clock+=60;
            while (game->clock>=hz)
            { game->clock-=hz; game->celebration=(game->celebration+1)%240; }
        }
        return;
    }
    if (pressed&START) game->paused=!game->paused;
    if (game->paused) return;
    if (pressed&C) { game_respawn(game); return; }
    if (hz!=50 && hz!=60) return;
    game->clock+=60;
    while (game->clock>=hz)
    {
        game->clock-=hz;
        game_step(game,held,pressed);
        pressed=0;
    }
}
