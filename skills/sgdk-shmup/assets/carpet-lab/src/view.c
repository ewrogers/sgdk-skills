#include "view.h"
static State previous;
static bool drawn;
bool view_start(void) { return assets_start(); }
void view_error_screen(void)
{
    SYS_doVBlankProcess(); VDP_setEnable(FALSE);
    VDP_setWindowOff(); VDP_setScrollingMode(HSCROLL_PLANE,VSCROLL_PLANE);
    VDP_setHorizontalScroll(BG_A,0); VDP_setHorizontalScroll(BG_B,0);
    VDP_setVerticalScroll(BG_A,0); VDP_setVerticalScroll(BG_B,0);
    VDP_clearPlane(BG_A,TRUE); VDP_clearPlane(BG_B,TRUE); VDP_clearPlane(WINDOW,TRUE);
    sprites_begin(); sprites_end();
    VDP_setTextPlane(BG_A); VDP_setTextPalette(PAL3); VDP_setTextPriority(TRUE);
    VDP_drawText("CARPET LAB COULD NOT LOAD",5,8);
    VDP_drawText(assets_error(),3,11);
    VDP_drawText("RESET AFTER FIXING THE ASSETS",3,16);
    VDP_setEnable(TRUE);
}
bool view_frame(const Game *game)
{
    if (!assets_scene(game)) return false;
    bool changed=!drawn || previous!=game->state;
    if (changed) ui_changed(game);
    bool world=game->state==STAGECARD || game->state==PLAY || game->state==BOSS || game->state==CLEAR;
    if (world)
    {
        u16 scroll=(u16)game->ticks;
        s16 far_scroll[28],near_scroll[28];
        for (u16 row=0;row<28;row++)
        { far_scroll[row]=-(s16)((scroll/4)&255); near_scroll[row]=row<2?0:-(s16)((scroll/2)&255); }
        VDP_setHorizontalScrollTile(BG_B,0,far_scroll,28,DMA_QUEUE_COPY);
        VDP_setHorizontalScrollTile(BG_A,0,near_scroll,28,DMA_QUEUE_COPY);
    }
    sprites_begin(); ui_frame(game);
    if (world)
    {
        bool blink=game->state!=CLEAR && game->invulnerable && (game->ticks&4);
        if (!blink) sprite_draw(ART_HERO,game->x,game->y,(game->ticks/8)%5,FALSE);
        for (u16 i=0;i<game->familiars;i++)
        { s16 x,y; game_familiar(game,i,&x,&y); sprite_draw(ART_EFFECT,x,y,8+game->loadout,FALSE); }
        if (game->barrier)
            sprite_shield(game->x,game->y,game->barrier);
        /* Player/hazards precede cosmetic effects in hardware evaluation order. */
        for (u16 i=0;i<BULLET_COUNT;i++) if (game->bullets[i].active)
            sprite_draw(ART_EFFECT,game->bullets[i].x,game->bullets[i].y,12+game->bullets[i].kind%3,FALSE);
        if (game->boss.active && game->boss.age>=90)
            sprite_draw(ART_BOSS,game->boss.x,game->boss.y,(game->ticks/10)%5,TRUE);
        for (u16 i=0;i<ENEMY_COUNT;i++) if (game->enemies[i].active)
            sprite_draw((Art)(ART_TEAPOT+game->enemies[i].kind),game->enemies[i].x,game->enemies[i].y,0,FALSE);
        if (game->ribbon_active)
        {
            s16 start=game->x/UNIT+12,end=start+RIBBON_REACH;
            if (end>320) end=320;
            for (s16 x=start+16;x<end+16;x+=32)
                sprite_draw(ART_RIBBON,x*UNIT,game->y,0,(game->ticks&4)!=0);
            sprite_draw(ART_MOON,game->x+12*UNIT,game->y,0,FALSE);
        }
        for (u16 i=0;i<SHOT_COUNT;i++) if (game->shots[i].active)
        {
            const Shot *shot=&game->shots[i];
            sprite_draw((Art)(ART_SUN+shot->kind%3),shot->x,shot->y,0,shot->vx<0);

        }
        for (u16 i=0;i<GEM_COUNT;i++) if (game->gems[i].active)
            sprite_draw(ART_EFFECT,game->gems[i].x,game->gems[i].y,4+(game->ticks/6)%4,FALSE);
        if (game->state==CLEAR)
        {
            if (game->age<150)
                for (u16 i=0;i<8;i++) sprite_draw(ART_EFFECT,game->boss.x+((s16)i%3-1)*20*UNIT,
                    game->boss.y+((s16)i/3-1)*20*UNIT,((game->age+i*7)/6)%4,FALSE);
        }
        else for (u16 i=0;i<EFFECT_COUNT;i++) if (game->effects[i].age)
        {
            const Effect *effect=&game->effects[i];
            if (effect->kind==2) sprite_draw(ART_STORM,effect->x,effect->y,0,(effect->age&4)!=0);
            else sprite_draw(ART_EFFECT,effect->x,effect->y,(effect->age-1)/6,FALSE);
        }
    }
    else if (game->state==LOADOUT)
    {
        for (u16 i=0;i<3;i++)
        {
            s16 y=(80+i*40)*UNIT, travel=(game->age%48)*UNIT;
            sprite_draw(ART_HERO,168*UNIT,y,(game->ticks/8)%5,FALSE);
            for (u16 shot=0;shot<3;shot++)
            {
                s16 x=(210+shot*24)*UNIT+travel/2;
                s16 dy=i==0?((s16)shot-1)*7*UNIT:i==2?direction_y[(game->age/3+shot*3)&15]/4:0;
                sprite_draw((Art)(ART_SUN+i),x,y+dy,0,FALSE);
                if (i==1) sprite_draw(ART_MOON,x-8*UNIT,y,0,FALSE);
            }
        }
    }
    else if (game->state==HOWTO)
    {
        sprite_draw(ART_BUTTON,56*UNIT,68*UNIT,0,FALSE);
        sprite_draw(ART_BUTTON,56*UNIT,100*UNIT,1,FALSE);
        sprite_draw(ART_BUTTON,56*UNIT,132*UNIT,2,FALSE);
    }
    else if (game->state==CREDITS)
        sprite_draw(ART_HERO,(40+(game->age/2)%240)*UNIT,184*UNIT,(game->ticks/8)%5,FALSE);
    sprites_end();
    VDP_setEnable(TRUE); previous=game->state; drawn=true; return true;
}
