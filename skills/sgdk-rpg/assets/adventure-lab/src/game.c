#include "game.h"
static void enter(Game *g, Screen screen)
{
    g->screen = screen;
    g->selection = 0;
    g->elapsed = 0;
    g->dirty = true;
    g->notice = 0;
}
const char *encounter_name(uint8_t e)
{
    return e == 0 ? "The Bell Causeway" : e == 1 ? "The Chart Vault" : "The Tidal Orrery";
}
void game_init(Game *g, bool saved)
{
    *g = (Game){0};
    campaign_new(&g->campaign);
    g->checkpoint = g->campaign;
    g->screen = TITLE;
    g->dirty = true;
    g->saved = saved;
    g->x = 160;
    g->y = 128;
}
void game_load(Game *g, const Campaign *c)
{
    g->campaign = *c;
    g->checkpoint = *c;
    g->saved = true;
    enter(g, CAMP);
    g->x = 160;
    g->y = 128;
}
static uint16_t choose(uint16_t value, uint16_t count, uint16_t keys)
{
    if (keys & (UP | LEFT))
        return (value + count - 1) % count;
    if (keys & (DOWN | RIGHT))
        return (value + 1) % count;
    return value;
}
static void next_battle_menu(Game *g)
{
    g->battle_menu = COMMAND;
    g->selection = 0;
    g->elapsed = 0;
    g->dirty = true;
}
static void delay_step(Game *g, uint16_t dt)
{
    uint32_t elapsed = (uint32_t)g->elapsed + dt;
    g->elapsed = elapsed > 1024 ? 1024 : (uint16_t)elapsed;
}
static void resolved(Game *g)
{
    g->battle_menu = RESOLVE;
    g->elapsed = 0;
    g->dirty = true;
    Event *e = &g->battle.event;
    g->events |= e->kind == HIT                                               ? SFX_HIT
                 : e->kind == HEAL || e->kind == REVIVE || e->kind == RESTORE ? SFX_HEAL
                                                                              : SFX_SPELL;
}
static bool target_first(Game *g)
{
    for (uint8_t i = 0; i < 6; i++)
        if (battle_target(&g->battle, g->pending, i))
        {
            g->pending.target = i;
            g->battle_menu = TARGET;
            return true;
        }
    return false;
}
static void battle_input(Game *g, uint16_t pressed, uint16_t nav, uint16_t dt)
{
    Battle *b = &g->battle;
    if (pressed & START)
    {
        g->paused = !g->paused;
        g->dirty = true;
        g->events |= SFX_ACCEPT;
        return;
    }
    if (g->paused)
        return;
    if (g->battle_menu == RESOLVE)
    {
        delay_step(g, dt);
        bool victory_ready = b->outcome == WON && g->elapsed >= 65;
        if (g->elapsed < 18 || (!(pressed & A) && !victory_ready))
            return;
        if (b->outcome != LIVE)
        {
            enter(g, RESULT);
            if (b->outcome == WON)
            {
                battle_claim(b, &g->campaign);
                g->events |= SFX_WIN;
            }
            else
                g->events |= SFX_LOSE;
        }
        else
            next_battle_menu(g);
        return;
    }
    uint8_t actor = battle_actor(b);
    if (actor >= 3)
    {
        delay_step(g, dt);
        if (g->elapsed >= 40)
        {
            battle_enemy(b);
            resolved(g);
        }
        return;
    }
    if (g->battle_menu == TARGET)
    {
        if (pressed & B)
        {
            g->battle_menu = g->return_menu;
            g->dirty = true;
            g->events |= SFX_BACK;
            return;
        }
        if (nav)
        {
            uint8_t target = g->pending.target;
            for (uint8_t n = 0; n < 6; n++)
            {
                target = (target + ((nav & (UP | LEFT)) ? 5 : 1)) % 6;
                if (battle_target(b, g->pending, target))
                    break;
            }
            g->pending.target = target;
            g->notice = 0;
            g->dirty = true;
            g->events |= SFX_MOVE;
        }
        if (pressed & A)
        {
            if (battle_commit(b, &g->campaign, g->pending))
                resolved(g);
            else
            {
                g->notice = "Not enough MP, stock or missing HP.";
                g->events |= SFX_DENIED;
                g->dirty = true;
            }
        }
        return;
    }
    uint16_t count = g->battle_menu == COMMAND ? 4 : g->battle_menu == ARTS ? 2 : 3;
    if (nav)
    {
        g->selection = choose(g->selection, count, nav);
        g->dirty = true;
        g->notice = 0;
        g->events |= SFX_MOVE;
    }
    if (pressed & B && g->battle_menu != COMMAND)
    {
        next_battle_menu(g);
        g->events |= SFX_BACK;
        return;
    }
    if (!(pressed & A))
        return;
    g->events |= SFX_ACCEPT;
    g->notice = 0;
    if (g->battle_menu == COMMAND)
    {
        if (g->selection == 1)
        {
            g->battle_menu = ARTS;
            g->selection = 0;
            g->dirty = true;
            return;
        }
        if (g->selection == 2)
        {
            g->battle_menu = PACK;
            g->selection = 0;
            g->dirty = true;
            return;
        }
        if (g->selection == 3)
        {
            if (battle_commit(b, &g->campaign, (Action){GUARD, 0, actor}))
                resolved(g);
            return;
        }
        g->pending = (Action){STRIKE, 0, 3};
    }
    else
        g->pending = (Action){g->battle_menu == ARTS ? ART : ITEM, (uint8_t)g->selection, 0};
    g->return_menu = g->battle_menu;
    if (!target_first(g))
    {
        g->notice = "No valid target for that action.";
        g->events |= SFX_DENIED;
    }
    g->dirty = true;
}
void game_step(Game *g, uint16_t held, uint16_t pressed, uint16_t nav, uint16_t dt)
{
    g->events = 0;
    if (g->screen != BATTLE || !g->paused)
        g->ticks += dt;
    if (g->screen == BATTLE)
    {
        battle_input(g, pressed, nav, dt);
        return;
    }
    if (g->screen == TITLE)
    {
        if (nav)
        {
            g->selection = choose(g->selection, 3, nav);
            g->dirty = true;
            g->events |= SFX_MOVE;
        }
        if (pressed & (A | START))
        {
            if (g->selection == 0)
            {
                bool saved = g->saved;
                game_init(g, saved);
                enter(g, HELP);
            }
            else if (g->selection == 1)
            {
                if (g->saved)
                    g->load_request = true;
                else
                {
                    g->notice = "No checkpoint yet. Begin a new voyage.";
                    g->dirty = true;
                    g->events |= SFX_DENIED;
                }
            }
            else
                enter(g, CREDITS);
            g->events |= SFX_ACCEPT;
        }
        return;
    }
    if (g->screen == HELP)
    {
        if (pressed & (A | START | B))
        {
            enter(g, CAMP);
            g->events |= SFX_ACCEPT;
        }
        return;
    }
    if (g->screen == CAMP)
    {
        uint8_t old_area = g->x < 105 ? 0 : g->x > 225 ? 2 : 1;
        g->moving = false;
        if (held & LEFT)
        {
            if (g->x > 44)
                g->x -= dt;
            g->direction = 3;
            g->moving = true;
        }
        else if (held & RIGHT)
        {
            if (g->x < 276)
                g->x += dt;
            g->direction = 1;
            g->moving = true;
        }
        if (held & UP)
        {
            if (g->y > 88)
                g->y -= dt;
            g->direction = 2;
            g->moving = true;
        }
        else if (held & DOWN)
        {
            if (g->y < 149)
                g->y += dt;
            g->direction = 0;
            g->moving = true;
        }
        if (g->x < 44)
            g->x = 44;
        if (g->x > 276)
            g->x = 276;
        if (g->y < 88)
            g->y = 88;
        if (g->y > 149)
            g->y = 149;
        if (old_area != (g->x < 105 ? 0 : g->x > 225 ? 2 : 1))
        {
            g->dirty = true;
            g->notice = 0;
        }
        g->walk_frame = g->moving ? (g->ticks / 8) % 4 : 0;
        if (pressed & (B | START))
        {
            enter(g, PARTY);
            g->tab = 0;
            g->member = 0;
            g->events |= SFX_ACCEPT;
        }
        else if (pressed & C)
        {
            enter(g, ROUTE);
            g->selection = g->campaign.route;
            g->events |= SFX_ACCEPT;
        }
        else if (pressed & A)
        {
            if (g->x < 105)
            {
                enter(g, PARTY);
                g->tab = 1;
                g->member = 0;
            }
            else if (g->x > 225)
            {
                campaign_rest(&g->campaign);
                g->checkpoint = g->campaign;
                g->save_request = true;
                g->notice = "Rested. Checkpoint saved at the inn.";
                g->events |= SFX_SAVE;
                g->dirty = true;
            }
            else
            {
                enter(g, ROUTE);
                g->selection = g->campaign.route;
            }
            g->events |= SFX_ACCEPT;
        }
        return;
    }
    if (g->screen == ROUTE)
    {
        if (nav)
        {
            g->selection = choose(g->selection, 3, nav);
            g->dirty = true;
            g->events |= SFX_MOVE;
        }
        if (pressed & B)
        {
            enter(g, CAMP);
            g->events |= SFX_BACK;
        }
        else if (pressed & (A | START))
        {
            if (g->selection > g->campaign.route)
            {
                g->notice = "Restore the earlier bell first.";
                g->dirty = true;
                g->events |= SFX_DENIED;
                return;
            }
            g->checkpoint = g->campaign;
            g->save_request = true;
            uint8_t encounter = (uint8_t)g->selection;
            battle_begin(&g->battle, &g->campaign, encounter, 0x5EA123UL + encounter);
            enter(g, BATTLE);
            next_battle_menu(g);
            g->paused = false;
            g->events |= SFX_ACCEPT;
        }
        return;
    }
    if (g->screen == PARTY)
    {
        if (nav & (LEFT | RIGHT))
        {
            g->tab = choose(g->tab, 3, nav);
            g->selection = 0;
            g->dirty = true;
            g->notice = 0;
            g->events |= SFX_MOVE;
        }
        if (nav & (UP | DOWN))
        {
            if (g->tab == 2)
                g->selection = choose(g->selection, 3, nav);
            else
                g->member = choose(g->member, 3, nav);
            g->dirty = true;
            g->notice = 0;
            g->events |= SFX_MOVE;
        }
        if (pressed & C)
        {
            g->member = (g->member + 1) % 3;
            g->dirty = true;
            g->events |= SFX_MOVE;
        }
        if (pressed & B)
        {
            enter(g, CAMP);
            g->events |= SFX_BACK;
        }
        else if (pressed & A)
        {
            if (g->tab == 1)
            {
                campaign_equip(&g->campaign, g->member, !g->campaign.party[g->member].weapon);
                g->notice = "Equipment changed. Stats recalculated.";
                g->events |= SFX_ACCEPT;
            }
            else if (g->tab == 2)
            {
                bool used = campaign_item(&g->campaign, g->selection, g->member);
                g->notice =
                    used ? "Used on the selected keeper." : "That item would have no effect.";
                g->events |= used ? SFX_HEAL : SFX_DENIED;
            }
            g->dirty = true;
        }
        return;
    }
    if (g->screen == RESULT && pressed & (A | START))
    {
        if (g->battle.outcome == LOST)
            g->campaign = g->checkpoint;
        bool finished = g->battle.outcome == WON && g->campaign.complete;
        enter(g, finished ? CREDITS : CAMP);
        g->events |= SFX_ACCEPT;
    }
    else if (g->screen == CREDITS && pressed & (A | START | B))
        enter(g, TITLE);
}
