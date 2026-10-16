#include "view.h"
static void center(const char *s, u16 y, bool gold)
{
    u16 n = 0;
    while (s[n])
        n++;
    ui_text(s, n < 40 ? (40 - n) / 2 : 0, y, gold);
}
static void footer(const char *s)
{
    ui_text(s, 2, 27, true);
}
static void title(const Game *g)
{
    ui_panel(7, 1, 26, 6);
    center("THE LAST BELL", 3, true);
    center("A Keeper's Adventure", 5, false);
    ui_panel(10, 18, 20, 9);
    static const char *const labels[] = {"New Voyage", "Continue", "Credits"};
    for (u16 i = 0; i < 3; i++)
    {
        ui_text(labels[i], 14, 20 + i * 2, g->selection == i);
        if (g->selection == i)
            ui_text(">", 12, 20 + i * 2, true);
    }
    if (g->notice)
    {
        ui_panel(0, 10, 40, 5);
        center(g->notice, 12, false);
    }
}
static void help(void)
{
    ui_panel(0, 0, 40, 28);
    center("KEEPERS OF THE LAST LIGHT", 2, true);
    ui_text("Mara  / Cutlass and tide magic", 3, 5, false);
    ui_text("Ivo   / Tools and lightning", 3, 7, false);
    ui_text("Neri  / Star charts and healing", 3, 9, false);
    ui_text("Tidecut soaks. Spark hits harder.", 3, 12, true);
    ui_text("Guard when the Orrery gathers light.", 3, 14, false);
    ui_text("A advances each battle message.", 3, 16, false);
    ui_text("D-pad Walk / choose   A Confirm", 3, 19, true);
    ui_text("B Party / back        C Depart", 3, 21, true);
    ui_text("Start pauses battle. Rest at the inn.", 3, 23, false);
    footer("A / Start  Begin the voyage");
}
static void camp(const Game *g)
{
    ui_panel(0, 0, 40, 3);
    ui_text("BELLWETHER HARBOR", 2, 1, true);
    ui_text("Lv", 28, 1, false);
    ui_number(party_level(g->campaign.xp), 31, 1, true);
    ui_panel(0, 24, 40, 4);
    ui_text(g->notice    ? g->notice
            : g->x < 105 ? "Workshop: A changes your equipment."
            : g->x > 225 ? "Lantern Inn: A rests and saves."
                         : "Expedition Bell: A chooses a route.",
            2, 25, false);
    footer("A Interact    B Party    C Depart");
}
static void route(const Game *g)
{
    ui_panel(0, 0, 40, 28);
    center("THE DROWNED OBSERVATORY", 2, true);
    static const char *const details[] = {"Two bell-shell crabs guard the road.",
                                          "Chart moths weaken their pursuers.",
                                          "The guardian still keeps its watch."};
    for (u16 i = 0; i < 3; i++)
    {
        u16 y = 5 + i * 6;
        ui_panel(2, y, 36, 5);
        ui_text(g->selection == i ? ">" : " ", 3, y + 1, true);
        ui_number(i + 1, 5, y + 1, true);
        ui_text(encounter_name(i), 8, y + 1, g->selection == i);
        ui_text(i > g->campaign.route ? "Restore the earlier bell first." : details[i], 4, y + 3,
                false);
    }
    if (g->notice)
        center(g->notice, 24, false);
    footer("A Depart / save    B Harbor");
}
static void party(const Game *g)
{
    ui_panel(0, 0, 40, 28);
    static const char *const tabs[] = {"Status", "Gear", "Pack"};
    for (u16 t = 0; t < 3; t++)
        ui_text(tabs[t], 4 + t * 12, 2, g->tab == t);
    for (u16 i = 0; i < 3; i++)
    {
        u16 y = 5 + i * 6;
        const Member *m = &g->campaign.party[i];
        Stats s = member_stats(&g->campaign, i);
        ui_text(species_name((Species)i), 4, y, g->member == i);
        if (g->member == i)
            ui_text(">", 2, y, true);
        if (g->tab == 0)
        {
            ui_text("HP", 4, y + 1, false);
            ui_number(m->hp, 7, y + 1, true);
            ui_text("/", 10, y + 1, false);
            ui_number(s.hp, 11, y + 1, false);
            ui_text("MP", 17, y + 1, false);
            ui_number(m->mp, 20, y + 1, true);
            ui_text("/", 23, y + 1, false);
            ui_number(s.mp, 24, y + 1, false);
            ui_bar(4, y + 2, 11, m->hp, s.hp, m->hp ? 11 : 9);
            ui_bar(17, y + 2, 9, m->mp, s.mp, 5);
            ui_text("ATK", 4, y + 3, false);
            ui_number(s.attack, 8, y + 3, true);
            ui_text("DEF", 12, y + 3, false);
            ui_number(s.defense, 16, y + 3, true);
            ui_text("SPD", 20, y + 3, false);
            ui_number(s.speed, 24, y + 3, true);
        }
        else if (g->tab == 1)
        {
            ui_text(weapon_name(i, m->weapon), 4, y + 1, false);
            ui_text(m->weapon ? "Heavy / ATK +5 DEF +2 SPD -2" : "Light / balanced keeper's kit", 4,
                    y + 3, false);
        }
        else
        {
            ui_text(item_name(i), 5, y + 1, g->selection == i);
            ui_text("x", 19, y + 1, false);
            ui_number(g->campaign.items[i], 21, y + 1, true);
            if (g->selection == i)
                ui_text(">", 3, y + 1, true);
            ui_text(i == 0   ? "Restore 50 HP"
                    : i == 1 ? "Restore 12 MP"
                             : "Revive with half HP",
                    5, y + 3, false);
        }
    }
    if (g->notice)
        ui_text(g->notice, 2, 24, false);
    else
    {
        ui_text("Level", 3, 24, false);
        ui_number(party_level(g->campaign.xp), 9, 24, true);
        ui_text("EXP", 14, 24, false);
        ui_number(g->campaign.xp, 18, 24, true);
        ui_text("Gold", 25, 24, false);
        ui_number(g->campaign.gold, 30, 24, true);
    }
    footer(g->tab == 0   ? "Left/Right Tab   Up/Down Keeper   B Back"
           : g->tab == 1 ? "A Swap gear   Up/Down Keeper   B Back"
                         : "A Use   C Keeper   Up/Down Item  B Back");
}
static void battle_status(const Game *g)
{
    const Battle *b = &g->battle;
    ui_fill(0, 0, 40, 1, 1);
    ui_text(encounter_name(b->encounter), 1, 0, true);
    ui_text("ROUND", 29, 0, false);
    ui_number(b->round, 35, 0, true);
    if (b->encounter == 2 && b->actors[3].hp)
    {
        ui_text("ORRERY", 2, 15, true);
        ui_bar(10, 15, 26, b->actors[3].hp, b->actors[3].stats.hp, b->actors[3].charging ? 7 : 9);
    }
    ui_panel(0, 17, 40, 8);
    for (u16 i = 0; i < 3; i++)
    {
        const Fighter *f = &b->actors[i];
        u16 x = 2 + i * 13;
        ui_text(species_name((Species)i), x, 18,
                b->outcome == LIVE &&
                    (g->battle_menu == RESOLVE ? b->event.source : battle_actor(b)) == i);
        ui_text("HP", x, 19, false);
        ui_number(f->hp, x + 3, 19, true);
        ui_text("/", x + 6, 19, false);
        ui_number(f->stats.hp, x + 7, 19, false);
        ui_bar(x, 20, 10, f->hp, f->stats.hp, f->hp ? 11 : 9);
        ui_text("MP", x, 21, false);
        ui_number(f->mp, x + 3, 21, true);
        ui_text("/", x + 6, 21, false);
        ui_number(f->stats.mp, x + 7, 21, false);
        ui_text(!f->hp        ? "FALLEN"
                : f->guarding ? "GUARD"
                : f->weakened ? "WEAK"
                : f->soaked   ? "SOAKED"
                              : "READY",
                x, 23, false);
    }
}
static void event_text(const Game *g)
{
    const Event *e = &g->battle.event;
    ui_text("A Next", 32, 26, true);
    ui_text(species_name(g->battle.actors[e->source].species), 2, 25, true);
    if (e->kind == CHARGE)
    {
        ui_text("is gathering light. Guard!", 2, 27, false);
        return;
    }
    ui_text(e->kind == HIT      ? "hits"
            : e->kind == BRACE  ? "guards"
            : e->kind == WEAKEN ? "weakens"
                                : "restores",
            20, 25, false);
    ui_text(species_name(g->battle.actors[e->target].species), 2, 27, false);
    if (e->amount)
    {
        ui_number(e->amount, 22, 27, true);
        ui_text(e->kind == RESTORE ? "MP" : "HP", 27, 27, false);
    }
    if (e->combo)
        ui_text("COMBO!", 31, 27, true);
    else if (e->critical)
        ui_text("CRIT!", 32, 27, true);
}
static void battle(const Game *g)
{
    battle_status(g);
    const Battle *b = &g->battle;
    if (g->paused)
    {
        ui_panel(7, 5, 26, 8);
        center("THE TIDE CAN WAIT", 7, true);
        center("Start  Resume battle", 10, false);
        return;
    }
    if (g->battle_menu == RESOLVE)
    {
        event_text(g);
        return;
    }
    uint8_t actor = battle_actor(b);
    if (actor >= 3)
    {
        ui_text(species_name(b->actors[actor].species), 2, 25, true);
        ui_text("is preparing an action...", 2, 27, false);
        return;
    }
    if (g->battle_menu == COMMAND)
    {
        static const char *const labels[] = {"Strike", "Arts", "Pack", "Guard"};
        for (u16 i = 0; i < 4; i++)
        {
            u16 x = 2 + i * 10;
            ui_text(labels[i], x, 25, g->selection == i);
            if (g->selection == i)
                ui_text(">", x - 1, 25, true);
        }
        footer("A Choose             Start Pause");
    }
    else if (g->battle_menu == ARTS)
    {
        for (u16 i = 0; i < 2; i++)
        {
            u16 x = 2 + i * 20;
            const ArtDef *a = art_definition(actor, i);
            ui_text(a->name, x, 25, g->selection == i);
            ui_number(a->cost, x + 14, 25, true);
            ui_text("MP", x + 16, 25, false);
        }
        footer(art_definition(actor, g->selection)->description);
    }
    else if (g->battle_menu == PACK)
    {
        for (u16 i = 0; i < 3; i++)
        {
            u16 x = 2 + i * 13;
            ui_text(item_name(i), x, 25, g->selection == i);
            ui_number(g->campaign.items[i], x + 10, 25, true);
        }
        footer("A Choose item   B Commands");
    }
    else
    {
        ui_text("Target:", 2, 25, true);
        ui_text(species_name(b->actors[g->pending.target].species), 11, 25, false);
        footer("D-pad Target   A Confirm   B Back");
    }
    if (g->notice)
    {
        ui_fill(0, 27, 40, 1, 1);
        footer(g->notice);
    }
}
static void result(const Game *g)
{
    ui_panel(4, 4, 32, 19);
    bool won = g->battle.outcome == WON;
    center(won ? "A BELL RINGS AGAIN" : "THE LIGHT IS NOT LOST", 6, true);
    center(won ? "The keepers gather their spoils." : "Return to the departure checkpoint.", 9,
           false);
    if (won)
    {
        ui_text("EXP gained", 7, 12, false);
        ui_number(g->battle.encounter == 0 ? 25 : g->battle.encounter == 1 ? 40 : 60, 26, 12, true);
        ui_text("Gold found", 7, 14, false);
        ui_number(30 + g->battle.encounter * 25, 26, 14, true);
        ui_text("Party level", 7, 16, false);
        ui_number(party_level(g->campaign.xp), 26, 16, true);
        center("Tonic +1     Ether +1", 18, false);
    }
    center(won && g->campaign.complete ? "A / Start  The last bell" : "A / Start  Return to harbor",
           21, true);
}
static void credits(void)
{
    ui_panel(3, 5, 34, 18);
    center("THE LAST BELL", 7, true);
    center("Three keepers. One more dawn.", 10, false);
    center("An original SGDK Adventure Lab", 13, false);
    center("Code / Music / Pixel finishing", 15, true);
    center("SGDK Skills", 17, false);
    center("New art created with PixelLab", 19, false);
    center("A / Start  Title", 21, true);
}
void ui_draw(const Game *g)
{
    switch (g->screen)
    {
    case TITLE:
        title(g);
        break;
    case HELP:
        help();
        break;
    case CAMP:
        camp(g);
        break;
    case ROUTE:
        route(g);
        break;
    case PARTY:
        party(g);
        break;
    case BATTLE:
        battle(g);
        break;
    case RESULT:
        result(g);
        break;
    case CREDITS:
        credits();
        break;
    }
}
