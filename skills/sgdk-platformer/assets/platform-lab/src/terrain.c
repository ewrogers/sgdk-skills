#include "game.h"
#include "layout.h"

Tile game_original_tile(s16 x,s16 y)
{
    if (x < 0 || x >= WORLD_W || y < 0) return SOLID;
    if (y >= WORLD_H) return EMPTY;
    char value = level_rows[y][x];
    return value == '#' ? SOLID : value == '-' ? ONEWAY : value == '/' ? RISE :
           value == '\\' ? FALL : value == '*' ? BREAKABLE : value == '?' ? COVER : EMPTY;
}

Tile game_tile(const Game *game,s16 x,s16 y)
{
    Tile tile = game_original_tile(x,y);
    if (tile == BREAKABLE)
    {
        u16 index = y * WORLD_W + x;
        if (game->broken[index >> 3] & (1 << (index & 7))) return EMPTY;
    }
    return tile;
}

static s32 surface(Tile tile,s16 column,s16 row,s32 foot)
{
    s32 local = foot - column * CELL * UNIT;
    s32 top = row * CELL * UNIT;
    return tile == RISE ? top + CELL * UNIT - local :
           tile == FALL ? top + local : top;
}

bool game_blocked(const Game *game,const Body *body,s32 x,s32 y)
{
    s32 right = x + body->width * UNIT - 1;
    s32 bottom = y + body->height * UNIT - 1;
    if (x < 0 || right >= WORLD_W * CELL * UNIT || y < 0) return true;
    s16 left_cell = x / (CELL * UNIT),right_cell = right / (CELL * UNIT);
    s16 top_cell = y / (CELL * UNIT),bottom_cell = bottom / (CELL * UNIT);
    s32 foot = x + body->width * UNIT / 2;
    for (s16 row = top_cell; row <= bottom_cell && row < WORLD_H; row++)
        for (s16 column = left_cell; column <= right_cell; column++)
        {
            Tile tile = game_tile(game,column,row);
            if (tile == SOLID || tile == BREAKABLE) return true;
            /* Slopes are floor heightfields sampled at the foot center. */
            if ((tile == RISE || tile == FALL) && foot / (CELL * UNIT) == column &&
                bottom >= surface(tile,column,row,foot)) return true;
        }
    return false;
}

bool game_supported(const Game *game,s32 x,s32 feet)
{
    if (x < 0 || feet < 0 || x >= WORLD_W * CELL * UNIT) return false;
    s16 column = x / (CELL * UNIT),row = feet / (CELL * UNIT);
    Tile tile = game_tile(game,column,row);
    return tile == SOLID || tile == BREAKABLE || tile == ONEWAY ||
           ((tile == RISE || tile == FALL) && feet >= surface(tile,column,row,x));
}

static bool break_at(Game *game,const Body *body,s32 x,s32 y)
{
    if (x < 0 || y < 0 || x + body->width * UNIT > WORLD_W * CELL * UNIT) return false;
    bool changed = false;
    for (s16 row = y / (CELL * UNIT); row <= (y + body->height * UNIT - 1) / (CELL * UNIT) && row < WORLD_H; row++)
        for (s16 col = x / (CELL * UNIT); col <= (x + body->width * UNIT - 1) / (CELL * UNIT); col++)
            if (game_tile(game,col,row) == BREAKABLE)
            {
                u16 index = row * WORLD_W + col;
                game->broken[index >> 3] |= 1 << (index & 7);
                changed = true;
            }
    if (changed) game->terrain_revision++;
    return changed;
}

static s32 substep(s32 value)
{
    return value > UNIT ? UNIT : value < -UNIT ? -UNIT : value;
}

static bool one_way(const Game *game,const Body *body,s32 old_y,s32 new_y,u8 drop,s32 *landing)
{
    if (drop) return false;
    s32 old_feet = old_y + body->height * UNIT;
    s32 new_feet = new_y + body->height * UNIT;
    s16 row = new_feet / (CELL * UNIT);
    bool hit = false;
    if (row >= 0 && row < WORLD_H && old_feet <= row * CELL * UNIT)
        for (s16 col = body->x / (CELL * UNIT); col <= (body->x + body->width * UNIT - 1) / (CELL * UNIT); col++)
            if (game_tile(game,col,row) == ONEWAY)
            { *landing = row * CELL * UNIT - body->height * UNIT; hit = true; }
    const Platform *platform = &game->platform;
    if (body->x < platform->x + 32 * UNIT && body->x + body->width * UNIT > platform->x &&
        old_feet <= platform->y && new_feet >= platform->y &&
        (!hit || platform->y - body->height * UNIT < *landing))
    {
        *landing = platform->y - body->height * UNIT;
        hit = true;
    }
    return hit;
}

void game_move(Game *game,Body *body,s32 dx,s32 dy,bool step_up,bool break_blocks,u8 drop)
{
    /* The supported velocity domain is at most eight pixels on either axis. */
    if (dx > 8 * UNIT) dx = 8 * UNIT;
    if (dx < -8 * UNIT) dx = -8 * UNIT;
    if (dy > MAX_FALL) dy = MAX_FALL;
    if (dy < -MAX_FALL) dy = -MAX_FALL;
    bool was_ground = body->ground;
    body->wall = 0;
    while (dx)
    {
        s32 step = substep(dx),next = body->x + step;
        if (break_blocks) break_at(game,body,next,body->y);
        if (game_blocked(game,body,next,body->y))
        {
            bool climbed = false;
            if (was_ground && step_up)
                for (u8 rise = 1; rise <= 8; rise++)
                    if (!game_blocked(game,body,next,body->y - rise * UNIT))
                    { body->y -= rise * UNIT; climbed = true; break; }
            if (!climbed) { body->wall = step > 0 ? 1 : -1; body->vx = 0; break; }
        }
        body->x = next;
        dx -= step;
    }
    body->ground = false;
    body->support = -1;
    while (dy)
    {
        s32 step = substep(dy),next = body->y + step,landing = 0;
        if (step > 0 && one_way(game,body,body->y,next,drop,&landing))
        {
            body->y = landing; body->vy = 0; body->ground = true;
            if (landing + body->height * UNIT == game->platform.y &&
                body->x < game->platform.x + 32 * UNIT &&
                body->x + body->width * UNIT > game->platform.x) body->support = 0;
            break;
        }
        if (game_blocked(game,body,body->x,next))
        { body->vy = 0; body->ground = step > 0; break; }
        body->y = next; dy -= step;
    }
    /* Stay attached when walking down a slope, not only when falling onto it. */
    if (was_ground && step_up && body->vy >= 0 && !drop && !body->ground)
        for (u8 down = 0; down <= 8; down++)
        {
            s32 next = body->y + down * UNIT,landing = 0;
            if (one_way(game,body,body->y,next,drop,&landing))
            {
                body->y = landing; body->vy = 0; body->ground = true;
                if (landing + body->height * UNIT == game->platform.y &&
                body->x < game->platform.x + 32 * UNIT &&
                body->x + body->width * UNIT > game->platform.x) body->support = 0;
                break;
            }
            if (game_blocked(game,body,body->x,next + UNIT))
            { body->y = next; body->vy = 0; body->ground = true; break; }
        }
    if (!body->ground && !body->wall)
    {
        if (game_blocked(game,body,body->x + UNIT,body->y)) body->wall = 1;
        else if (game_blocked(game,body,body->x - UNIT,body->y)) body->wall = -1;
    }
}

bool game_overlap(const Body *a,const Body *b)
{
    return a->x < b->x + b->width * UNIT && a->x + a->width * UNIT > b->x &&
           a->y < b->y + b->height * UNIT && a->y + a->height * UNIT > b->y;
}
