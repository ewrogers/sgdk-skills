#include "game.h"

void game_enemies_init(Game *game)
{
    const s16 xs[ENEMY_COUNT]={224,272,544,864,1024};
    for (u16 i=0;i<ENEMY_COUNT;i++)
    {
        Enemy *enemy=&game->enemies[i];
        *enemy=(Enemy){.kind=(EnemyKind)i,.facing=1,.home_x=xs[i],
            .home_y=i==FLYER?168:242,.active=true,.guarded=i==ARMOR};
        enemy->body=(Body){.x=xs[i]*UNIT,.y=enemy->home_y*UNIT,
            .width=12,.height=14,.support=-1};
    }
}

void game_enemies_step(Game *game)
{
    for (u16 i=0;i<ENEMY_COUNT;i++)
    {
        Enemy *enemy=&game->enemies[i];
        Body *body=&enemy->body;
        if (!enemy->active)
        {
            if (enemy->retired && --enemy->retired==0)
            {
                body->x=enemy->home_x*UNIT; body->y=enemy->home_y*UNIT;
                body->vx=body->vy=0; body->ground=false; body->support=-1;
                enemy->active=true;
            }
            continue;
        }
        enemy->clock++;
        enemy->guarded=enemy->kind==ARMOR && enemy->clock%150<90;
        if (enemy->kind==FLYER)
        {
            if (body->x<832*UNIT) enemy->facing=1;
            if (body->x>944*UNIT) enemy->facing=-1;
            body->x+=enemy->facing*UNIT;
            u16 phase=(enemy->clock/4)&31;
            body->y=(enemy->home_y+(phase<16?(s16)phase:31-(s16)phase)-8)*UNIT;
            continue;
        }
        if (body->wall) enemy->facing=-enemy->facing;
        if (enemy->kind==PATROL && body->ground)
        {
            s32 probe=body->x+(enemy->facing>0?body->width+2:-2)*UNIT;
            if (!game_supported(game,probe,body->y+(body->height+2)*UNIT))
                enemy->facing=-enemy->facing;
        }
        if (enemy->kind==ARMOR)
        {
            if (body->x<1000*UNIT) enemy->facing=1;
            if (body->x>1048*UNIT) enemy->facing=-1;
            body->vx=enemy->facing*UNIT/2;
        }
        else if (enemy->kind==HOPPER)
        {
            if (body->ground && enemy->clock%60==0)
            {
                enemy->facing=game->player.body.x<body->x?-1:1;
                body->vy=-UNIT*9/2; body->vx=enemy->facing*UNIT;
            }
            else if (body->ground) body->vx=0;
        }
        else body->vx=enemy->facing*UNIT;
        body->vy+=GRAVITY;
        if (body->vy>MAX_FALL) body->vy=MAX_FALL;
        game_move(game,body,body->vx,body->vy,true,false,0);
        if (body->y>(WORLD_H*CELL+32)*UNIT)
        { enemy->active=false; enemy->retired=90; }
    }
}

void game_contacts(Game *game,s32 previous_feet,u16 held)
{
    Player *player=&game->player;
    for (u16 i=0;i<ENEMY_COUNT;i++)
    {
        Enemy *enemy=&game->enemies[i];
        if (!enemy->active || !game_overlap(&player->body,&enemy->body)) continue;
        bool stomp=player->body.vy>0 && previous_feet<=enemy->body.y+2*UNIT;
        if (player->dashing || (stomp && !enemy->guarded))
        {
            enemy->active=false; enemy->retired=0;
            game->score+=2; game->events|=EVENT_STOMP;
            if (stomp)
            {
                player->body.y=enemy->body.y-player->body.height*UNIT;
                player->body.vy=(held&A)?JUMP_SPEED:-4*UNIT;
                player->body.ground=false; player->variable_jump=false;
                player->air_jump=player->boots; player->air_dash=true;
            }
        }
        else
        {
            game_hurt(game,player->body.x<enemy->body.x?-1:1);
            if (game->teleport) break;
        }
    }
}
