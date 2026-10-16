#include "view.h"
#include <assert.h>
u16 panel_tile=100,big_font_tile=120,hud_tile=200,health_tile=300,label_tile=400,banner_tile=500,selected_panel_tile=600;
static u16 maps[3][28][40],uploads;
void VDP_setTileMapDataRowEx(VDPPlane plane,const u16 *tiles,u16 base,u16 row,u16 x,u16 length,u16 method)
{
    (void)base;(void)method;assert(row<28 && x+length<=40);
    memcpy(&maps[plane][row][x],tiles,length*sizeof(u16));uploads++;
}
void VDP_setTileMapXY(VDPPlane p,u16 tile,u16 x,u16 y){assert(x<40 && y<28);maps[p][y][x]=tile;}
void VDP_clearPlane(VDPPlane p,bool wait){(void)wait;memset(maps[p],0,sizeof maps[p]);}
void VDP_fillTileMapRect(VDPPlane p,u16 tile,u16 x,u16 y,u16 w,u16 h)
{for(u16 j=0;j<h;j++)for(u16 i=0;i<w;i++)maps[p][y+j][x+i]=tile;}
void sprite_text(const char *text,s16 x,s16 y){(void)text;(void)x;(void)y;}
int main(void)
{
    Game game={.state=PLAY,.stage=1,.lives=3,.meter=2,.speed=2,.weapon=0};
    ui_changed(&game);ui_frame(&game);
    assert(uploads==4);assert((maps[BG_A][0][0]&0x7ff)==hud_tile);
    assert((maps[BG_A][0][33]&0x7ff)==TILE_FONT_INDEX+'S'-32);
    assert((maps[BG_A][0][39]&0x7ff)==TILE_FONT_INDEX+'1'-32);
    /* The upgrade rail stays on the final two screen rows, not at the top. */
    assert((maps[WINDOW][26][8]&0x7ff)==hud_tile+14);
    assert((maps[WINDOW][27][8]&0x7ff)==hud_tile+46);
    assert((maps[BG_A][1][0]&0x7ff)==hud_tile+32);
    /* Only the pending purchase has a full-cell background. Owned Speed must
     * not gain the selection color; unused cells have no level numbers. */
    assert((maps[WINDOW][26][7]&0x7ff)==panel_tile);
    assert((maps[WINDOW][26][15]&0x7ff)==selected_panel_tile);
    assert((maps[WINDOW][26][0]&0x7ff)==hud_tile+2);
    assert((maps[WINDOW][26][23]&0x7ff)==panel_tile);
    ui_frame(&game);assert(uploads==4);
    game.meter=4;ui_frame(&game);
    assert((maps[WINDOW][26][15]&0x7ff)==panel_tile);
    assert((maps[WINDOW][26][31]&0x7ff)==selected_panel_tile);
    game.meter=0;ui_frame(&game);
    assert((maps[WINDOW][26][31]&0x7ff)==panel_tile);
    game.speed=3;game.meter=1;ui_frame(&game);
    assert((maps[WINDOW][26][0]&0x7ff)==hud_tile+22);
    assert((maps[WINDOW][26][7]&0x7ff)==selected_panel_tile);
    game.state=BOSS;game.boss.hp=BOSS_HP;game.boss.age=90;
    ui_changed(&game);ui_frame(&game);
    assert((maps[BG_A][1][17]&0x7ff)==health_tile);
    game.boss.age=135;ui_frame(&game);
    assert((maps[BG_A][1][17]&0x7ff)==health_tile+8);
    assert((maps[BG_A][1][26]&0x7ff)==health_tile);
    game.boss.age=180;ui_frame(&game);
    assert((maps[BG_A][1][34]&0x7ff)==health_tile+8);
    game.boss.hp=0;ui_frame(&game);
    assert((maps[BG_A][1][17]&0x7ff)==health_tile);
    assert((maps[BG_A][1][16]&0x7ff)==health_tile+9);
    assert((maps[BG_A][1][35]&0x7ff)==health_tile+10);
    game.state=LOADOUT;ui_changed(&game);ui_frame(&game);
    assert(!(maps[BG_A][9][20]&0x8000));
    game.state=CREDITS;ui_changed(&game);ui_frame(&game);
    const u16 credit_rows[]={7,10,12,15,17,20,26};
    for(u16 i=0;i<7;i++)
    {
        u16 y=credit_rows[i];assert(maps[BG_A][y][0]==0 && maps[BG_A][y][1]==0);
        assert(maps[BG_A][y][38]==0 && maps[BG_A][y][39]==0);
        assert(maps[BG_A][y][2]!=0 && maps[BG_A][y][37]!=0);
    }
    puts("hud");return 0;
}
