#include "view.h"
static char rows[28][41];
static bool inset;
static u32 old_score=0xFFFFFFFFUL,old_status=0xFFFFFFFFUL;
static u16 old_health=65535;
static const char *stage_names[]={"THE SAFFRON SKY BAZAAR","GARDENS ABOVE THE MOON","STORMGLASS OBSERVATORY"};
static const char *boss_names[]={"BRASSWING","RIBBON ORACLE","LANTERN VIZIER"};
static const char *families[]={"SUNFIRE","MOONWEAVE","STORMGLASS"};
static bool in_world(const Game *g) { return g->state==STAGECARD || g->state==PLAY || g->state==BOSS || g->state==CLEAR; }
static void line(const char *text,u16 row,VDPPlane plane)
{
    char padded[41]; u16 count=0;
    while (*text && count<40) padded[count++]=*text++;
    while (count<40) padded[count++]=' ';
    padded[40]=0;
    if (!strcmp(padded,rows[row])) return;
    strcpy(rows[row],padded); u16 tiles[40];
    for (u16 i=0;i<40;i++) tiles[i]=TILE_ATTR_FULL(PAL3,plane==WINDOW,FALSE,FALSE,TILE_FONT_INDEX+(u8)padded[i]-32);
    VDP_setTileMapDataRowEx(plane,tiles+(inset?2:0),0,row,inset?2:0,inset?36:40,DMA_QUEUE_COPY);
}
static void centered(const char *value,u16 row)
{
    char padded[41]; u16 length=strlen(value),margin=length<40?(40-length)/2:0;
    for (u16 i=0;i<margin;i++) padded[i]=' ';
    strcpy(padded+margin,value); line(padded,row,BG_A);
}
static void help_line(const char *text,u16 row)
{
    char padded[41]="          ";
    u16 i=10; while (*text && i<40) padded[i++]=*text++;
    padded[i]=0; line(padded,row,BG_A);
}
static void large(const char *value,u16 row)
{
    const char *alphabet=" ACDEFHILMNOPRSTUVWY"; u16 length=strlen(value),x=(40-length*2)/2,stride=strlen(alphabet)*2;
    while (*value)
    {
        u16 glyph=0; char letter=*value++;
        while (alphabet[glyph] && alphabet[glyph]!=letter) glyph++;
        if (!alphabet[glyph]) glyph=0;
        for (u16 y=0;y<2;y++) for (u16 dx=0;dx<2;dx++)
            VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(PAL3,FALSE,FALSE,FALSE,big_font_tile+y*stride+glyph*2+dx),x+dx,row+y);
        x+=2;
    }
}
void ui_changed(const Game *game)
{
    memset(rows,0,sizeof rows); old_score=0xFFFFFFFFUL; old_status=0xFFFFFFFFUL; old_health=65535;
    inset=game->state!=TITLE;
    if (in_world(game)) return;
    VDP_clearPlane(BG_A,TRUE);
    if (game->state!=TITLE) VDP_fillTileMapRect(BG_A,TILE_ATTR_FULL(PAL3,FALSE,FALSE,FALSE,panel_tile),2,5,36,22);
    if (game->state==TITLE)
    { large("CARPET OF",1); large("THE CRESCENT",3); centered("PRESS A / START",25); centered("A STORYBOOK ARCADE ADVENTURE",27); }
    else if (game->state==HOWTO)
    {
        large("RIDE THE WIND",2);
        help_line("FIRE YOUR MAGIC",8);
        help_line("BUY SELECTED SPELL",12);
        help_line("SLOW DOWN / AIM",16);
        centered("D-PAD TO FLY / START TO PAUSE",19);
        centered("GEMS ADVANCE THE UPGRADE BAR",21);
        centered("A / START: CONTINUE",25);
    }
    else if (game->state==LOADOUT)
    { large("CHOOSE A PATH",2); centered("LEFT / RIGHT TO CHOOSE",6); centered("A / START: BEGIN THE JOURNEY",24); }
    else if (game->state==OVER)
    { large("THE WIND WAITS",2); centered("YOUR JOURNEY IS NOT OVER",10); centered("A / START: RETRY THIS STAGE",15); centered("YOUR CHOSEN PATH STAYS WITH YOU",19); }
    else if (game->state==CREDITS)
    {
        large("THE END",2); centered("THE SKIES ARE FREE AGAIN",7);
        centered("CARPET OF THE CRESCENT",10); centered("AN SGDK SKILLS ADVENTURE",12);
        centered("ART CREATED WITH PIXELLAB",15); centered("ORIGINAL CODE / MUSIC / EFFECTS",17);
        centered("THANK YOU FOR RIDING WITH US",20); centered("A / START: TITLE",26);
    }
}
/* Native 16px icons occupy two tile rows in the shared HUD atlas. */
static void icon(u16 *tiles,u16 x,u16 frame)
{
    u16 packed=frame==3?0:frame>=18?11+frame-18:frame>=13?6+frame-13:frame-3;
    for (u16 y=0;y<2;y++) for (u16 dx=0;dx<2;dx++)
        tiles[y*40+x+dx]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,hud_tile+y*32+packed*2+dx);
}
static void words(u16 *tiles,u16 x,u16 y,const char *text)
{
    while (*text && x<40) tiles[y*40+x++]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,TILE_FONT_INDEX+(u8)*text++-32);
}
static void label(u16 *tiles,u16 x,const char *text,bool selected)
{
    const char *alphabet=" ABDEFGIKLMNOPRSTVWY";
    u16 stride=strlen(alphabet);
    while (*text && x<40)
    {
        u16 glyph=0; while (alphabet[glyph] && alphabet[glyph]!=*text) glyph++;
        if (!alphabet[glyph]) glyph=0;
        for (u16 y=0;y<2;y++) tiles[y*40+x]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,label_tile+(selected?stride*2:0)+y*stride+glyph);
        text++; x++;
    }
}
void ui_frame(const Game *game)
{
    char text[64];
    if (game->state==LOADOUT)
    {
        const char *descriptions[]={"FLAMES / SPREAD / PHOENIX","CRESCENTS / RIBBONS / WISPS","SEEKING SPARKS / LIGHTNING"};
        for (u16 i=0;i<3;i++) { sprintf(text,"   %c %s",game->loadout==i?'>':' ',families[i]); line(text,9+i*5,BG_A); }
        centered(descriptions[game->loadout],22); return;
    }
    if (!in_world(game)) return;
    u32 status=game->lives|((u32)game->stage<<3)|((u32)game->loadout<<5)|((u32)game->meter<<7)|
        ((u32)game->weapon<<10)|((u32)game->familiars<<12)|((u32)game->barrier<<14)|((u32)game->speed<<16)|((u32)game->paused<<18);
    u16 shown_health=game->boss.hp;
    if (game->state==BOSS && game->boss.age<180)
        shown_health=game->boss.age<90?0:(u32)(game->boss.age-90)*BOSS_HP/90;
    if (old_score!=game->score || old_status!=status || old_health!=shown_health)
    {
        u16 top[80],bottom[80];
        for (u16 i=0;i<80;i++) top[i]=bottom[i]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,panel_tile);
        for (u16 i=0;i<game->lives && i<3;i++) icon(top,2*i,3);
        sprintf(text,"%08lu",(unsigned long)game->score); words(top,8,0,text);
        sprintf(text,"STAGE %u",game->stage); words(top,33,0,text);
        if (game->paused) words(top,17,0,"PAUSED");
        else if (game->state==BOSS)
        {
            words(top,17,0,boss_names[game->stage-1]);
            u16 pixels=(u32)shown_health*144/BOSS_HP;
            top[40+16]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,health_tile+9);
            for (u16 i=0;i<18;i++)
            {
                u16 fill=pixels>=8?8:pixels; pixels-=fill;
                top[40+17+i]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,health_tile+fill);
            }
            top[40+35]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,health_tile+10);
        }
        else words(top,17,1,families[game->loadout]);
        for (u16 i=0;i<5;i++)
        {
            u16 x=i*8; bool available=game_upgrade_available(game,i+1),selected=game->meter==i+1;
            if (selected) for (u16 y=0;y<2;y++) for (u16 dx=0;dx<8;dx++)
                bottom[y*40+x+dx]=TILE_ATTR_FULL(PAL3,TRUE,FALSE,FALSE,selected_panel_tile);
            icon(bottom,x,!available?18+i:selected?13+i:4+i);
            const char *short_labels[3][5]={{"SPEED","FORK","FLAME","ALLY","AEGIS"},{"SPEED","TWIN","RIBBON","WISP","VEIL"},{"SPEED","SPARK","STORM","ALLY","WARD"}};
            label(bottom,x+2,short_labels[game->loadout][i],selected);
        }
        for (u16 y=0;y<2;y++)
        {
            VDP_setTileMapDataRowEx(BG_A,top+y*40,0,y,0,40,DMA_QUEUE_COPY);
            VDP_setTileMapDataRowEx(WINDOW,bottom+y*40,0,26+y,0,40,DMA_QUEUE_COPY);
        }
        old_status=status; old_score=game->score; old_health=shown_health;
    }
    if (game->state==STAGECARD)
    { sprite_text(stage_names[game->stage-1],(320-(s16)strlen(stage_names[game->stage-1])*8)/2,104); sprite_text("A / START TO FLY",100,128); }
    if (game->state==BOSS && game->boss.age<180)
    { sprite_text("A GUARDIAN APPROACHES",76,88); sprite_text(boss_names[game->stage-1],(320-(s16)strlen(boss_names[game->stage-1])*8)/2,104); }
    if (game->state==CLEAR && game->age>=150)
    { sprite_text("STAGE CLEAR",116,96); sprite_text(game->stage==3?"A / START: THE END":"A / START: NEXT STAGE",80,120); }
}
