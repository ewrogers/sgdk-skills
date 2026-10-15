#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "view.h"
#include "resources.h"
#include "world.h"

static Sprite sprites[80]; static u16 count;
static char cells[28][41]; static u16 colors[64];
static u16 planes[2][28][40],text_priority;
static const u16 source_colors[16]={0}; static const Palette source_palette={source_colors};
const SpriteDefinition hero_image={4,8,&source_palette},pulse_image={64,4,&source_palette},
    shot_image={1,3,&source_palette},marker_image={1,1,&source_palette},status_image={4,3,&source_palette},
    tries_image={4,2,&source_palette},exit_image={4,2,&source_palette},bomb_image={4,2,&source_palette},
    target_image={4,2,&source_palette},blast_image={4,2,&source_palette},zap_image={4,2,&source_palette};
void VDP_setEnable(bool enabled) { (void)enabled; }
void VDP_setScreenWidth320(void) {}
void VDP_setScreenHeight224(void) {}
void VDP_setPlaneSize(u16 w,u16 h,bool setup) { assert(w==64 && h==32 && setup); }
void VDP_setTextPlane(VDPPlane plane) { assert(plane==BG_A); }
void VDP_setTextPalette(u16 palette) { assert(palette<4); }
void VDP_setTextPriority(u16 priority) { assert(priority); text_priority=priority; }
void VDP_setBackgroundColor(u8 value) { assert(value<64); }
void VDP_loadTileData(const u32 *data,u16 index,u16 n,TransferMethod method)
{ assert(data && index>=TILE_USER_INDEX && n && method==DMA); }
void VDP_fillTileMapRect(VDPPlane plane,u16 attr,u16 x,u16 y,u16 w,u16 h)
{
    assert((plane==BG_A || plane==BG_B) && x+w<=40 && y+h<=28);
    for (u16 row=y;row<y+h;++row) for (u16 col=x;col<x+w;++col) planes[plane][row][col]=attr;
}
void VDP_clearTextArea(u16 x,u16 y,u16 w,u16 h)
{
    assert(x+w<=40 && y+h<=28);
    for (u16 row=y;row<y+h;++row) {
        memset(cells[row]+x,' ',w);
        for (u16 col=x;col<x+w;++col) planes[BG_A][row][col]=0;
    }
}
void VDP_drawText(const char *text,u16 x,u16 y)
{
    assert(y<28); for (u16 i=0;text[i];++i) {
        assert(x+i<40); cells[y][x+i]=text[i]; planes[BG_A][y][x+i]=(text_priority<<15);
    }
}
void PAL_setColors(u16 index,const u16 *values,u16 n,TransferMethod method)
{ assert(index+n<=64 && method==CPU); for (u16 i=0;i<n;++i) colors[index+i]=values[i]; }
void SPR_initEx(u16 tiles) { assert(tiles==320); }
bool SPR_isInitialized(void) { return true; }
Sprite *SPR_addSprite(const SpriteDefinition *definition,s16 x,s16 y,u16 attr)
{
    assert(count<80); Sprite *sprite=&sprites[count++];
    assert(!(attr&0x8000)); /* Foreground panels must cover actors. */
    *sprite=(Sprite){.x=x,.y=y,.palette=(attr>>13)&3,.definition=definition}; return sprite;
}
void SPR_setAutoAnimation(Sprite *sprite,bool enabled) { sprite->automatic=enabled; }
void SPR_setVisibility(Sprite *sprite,SpriteVisibility value) { sprite->visible=value==VISIBLE; }
void SPR_setPosition(Sprite *sprite,s16 x,s16 y) { sprite->x=x; sprite->y=y; }
void SPR_setAnimAndFrame(Sprite *sprite,s16 animation,s16 frame)
{
    assert(animation>=0 && animation<3 && frame>=0 && frame<sprite->definition->frames);
    sprite->animation=animation; sprite->frame=frame;
}
void SPR_setHFlip(Sprite *sprite,bool flip) { sprite->flip=flip; }
void SPR_setVFlip(Sprite *sprite,bool flip) { sprite->vflip=flip; }
void SPR_setPalette(Sprite *sprite,u16 palette) { assert(palette<4); sprite->palette=palette; }
void SPR_setPriority(Sprite *sprite,bool value) { sprite->priority=value; }
void SPR_update(void) { for (u16 i=0;i<count;++i) assert(!sprites[i].automatic); }
bool SYS_doVBlankProcess(void) { return true; }
static Sprite *find(const SpriteDefinition *definition,s16 x,s16 y)
{
    for (u16 i=0;i<count;++i) if (sprites[i].definition==definition && sprites[i].x==x && sprites[i].y==y) return &sprites[i];
    assert(false); return NULL;
}
static void check_panel(void)
{
    for (u16 y=12;y<19;++y) for (u16 x=6;x<34;++x) {
        u16 tile=planes[BG_B][y][x]; assert(tile&0x8000);
        assert((tile&0x7FF)==TILE_USER_INDEX+(y==12 || y==18 || x==6 || x==33 ? 3 : 2));
    }
    assert(planes[BG_A][14][11]&0x8000); /* Letters stay above the panel. */
}
int main(void)
{
    memset(cells,' ',sizeof cells); for (u16 y=0;y<28;++y) cells[y][40]=0;
    Game game; game_init(&game); assert(view_init()); view_draw(&game);
    assert(count==68 && strstr(cells[1],"AREA") && strstr(cells[3],"TRIES"));
    for (u16 x=1;x<39;++x) assert((planes[BG_B][26][x]&0x7FF)==TILE_USER_INDEX+1);
    for (u16 y=8;y<26;++y) {
        assert((planes[BG_B][y][1]&0x7FF)==TILE_USER_INDEX+1);
        assert((planes[BG_B][y][38]&0x7FF)==TILE_USER_INDEX+1);
    }
    for (u16 p=0;p<4;++p) assert(colors[p*16+15]==RGB24_TO_VDPCOLOR(0xEEEEEE));
    assert(colors[8]!=colors[24] && colors[24]!=colors[40] && colors[40]!=colors[56]);
    assert((planes[BG_A][6][8]&0x7FF)==TILE_USER_INDEX+9);
    game.players[0].cooldown=STUN_COOLDOWN; view_draw(&game);
    assert((planes[BG_A][6][8]&0x7FF)==TILE_USER_INDEX+2);
    game.players[0].cooldown=STUN_COOLDOWN/2; view_draw(&game);
    for (u16 x=8;x<18;++x) assert((planes[BG_A][6][x]&0x7FF)==TILE_USER_INDEX+(x<13 ? 8 : 2));
    game.players[0].cooldown=0; view_draw(&game);
    for (u16 x=8;x<18;++x) assert((planes[BG_A][6][x]&0x7FF)==TILE_USER_INDEX+9);
    Enemy *enemy=&game.enemies[0]; enemy->facing=3; enemy_change(enemy,WINDUP,24); game.tick=6;
    view_draw(&game); Sprite *body=find(&hero_image,enemy->pos.x,enemy->pos.y);
    assert(body->visible && body->flip); /* Alerting no longer blinks the body. */
    Sprite *icon=find(&status_image,enemy->pos.x,enemy->pos.y-14); assert(icon->visible && !icon->frame);
    enemy_change(enemy,STUNNED,90); view_draw(&game); assert(body->visible && icon->frame>=1);
    enemy->timer=60; view_draw(&game); assert(!body->visible && icon->visible);
    game.tick=12; view_draw(&game); assert(body->visible);
    game.phase=CAUGHT; view_draw(&game); assert(strstr(cells[14],"CAUGHT!")); check_panel();
    game_load_level(&game,1); game.tries=1; view_draw(&game);
    assert(!strstr(cells[14],"CAUGHT!") && strstr(cells[1],"2"));
    for (u16 y=12;y<19;++y) for (u16 x=6;x<34;++x) assert(!(planes[BG_B][y][x]&0x8000));
    Sprite *door=find(&exit_image,world_exit(1).x,world_exit(1).y); assert(door->visible);
    u16 filled=0; for (u16 i=0;i<count;++i) if (sprites[i].definition==&tries_image && !sprites[i].frame) ++filled;
    assert(filled==1);
    game.phase=AREA_CLEAR; view_draw(&game); assert(strstr(cells[14],"AREA CLEAR")); check_panel();
    game.phase=GAME_OVER; view_draw(&game); assert(strstr(cells[14],"GAME OVER") && strstr(cells[16],"NEW RUN")); check_panel();
    for (u16 frame=0;frame<600;++frame) view_draw(&game);
    check_panel();
    game_init(&game); view_draw(&game);
    assert(!strstr(cells[14],"GAME OVER"));
    for (u16 y=12;y<19;++y) for (u16 x=6;x<34;++x) assert(!(planes[BG_B][y][x]&0x8000));
    game_new_run(&game);
    static const char *const names[]={"BOMBARDIER","SHOCKER","SENTRY"};
    for (u16 kind=0;kind<3;++kind) {
        game.phase=INTRO_BOMBARDIER+kind; game.phase_ticks=0; view_draw(&game);
        assert(strstr(cells[12],names[kind]) && strstr(cells[23],"START PLAY"));
        Sprite *portrait=find(&hero_image,152,112);
        assert(portrait->visible && portrait->priority && portrait->palette==kind+1);
        u16 visible=0;
        for (u16 i=0;i<count;++i) if (sprites[i].definition==&hero_image && sprites[i].visible) ++visible;
        assert(visible==1 && (planes[BG_B][17][6]&0x8000));
    }
    game_load_level(&game,0); view_draw(&game);
    assert(!strstr(cells[12],"SENTRY"));
    for (u16 i=0;i<count;++i) if (sprites[i].definition==&hero_image) assert(!sprites[i].priority);
    game.bombs[0]=(Bomb){.from={112,64},.target={112,112},.timer=15,.phase=BOMB_FLYING};
    view_draw(&game); assert(find(&target_image,112,112)->visible);
    assert(find(&bomb_image,112,60)->visible==false); /* A high arc cannot cover the HUD. */
    game.bombs[0].phase=BOMB_EXPLODING; game.bombs[0].timer=BOMB_BLAST; view_draw(&game);
    assert(find(&blast_image,112,112)->visible && !find(&target_image,112,112)->visible);
    enemy=&game.enemies[2]; enemy->pos=(Point){112,128}; enemy->facing=0;
    enemy_change(enemy,ZAP,TASER_TICKS); view_draw(&game); assert(find(&zap_image,112,144)->visible);
    enemy_change(enemy,RECOVER,TASER_RECOVERY); view_draw(&game); assert(!find(&zap_image,112,144)->visible);
    enemy->pos=(Point){114,112}; enemy->facing=1; enemy_change(enemy,ZAP,TASER_TICKS);
    view_draw(&game); assert(find(&zap_image,130,112)->visible);
    enemy->pos.x=116; view_draw(&game); assert(!find(&zap_image,132,112)->visible);
    puts("view"); return 0;
}
