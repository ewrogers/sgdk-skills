#include "view.h"
#include "resources.h"
void menu_draw(const Game *g);
void dialogue_draw(const Game *g,bool setup);

static void title(const Game *g)
{
    ui_large("ASTERFALL",11,3);ui_text("THE LANTERN ROAD",12,6,true);
    ui_panel(10,17,20,10);
    const char *items[]={"Start","Continue","Options"};
    for(u16 i=0;i<3;i++)
    {
        ui_text(items[i],15,19+i*2,g->selection==i);
    }
    ui_text(">",13,19+g->selection*2,true);
}

static void naming(const Game *g)
{
    ui_text("A NEW ROADKEEPER",12,2,true);ui_panel(3,4,34,5);
    ui_text("Your name",5,5,true);ui_text(g->name,17,6,false);
    for(u16 i=g->name_length;i<8;i++)ui_text("_",17+i,6,true);
    ui_text("Choose letters with the D-pad",6,10,false);
    static const char keys[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.-";
    for(u16 i=0;i<64;i++)
    {
        const u16 x=8+(i%8)*3,y=12+(i/8);
        if(g->keyboard==i)ui_text(">",x-1,y,true);
        char s[2]={keys[i],0};ui_text(s,x,y,g->keyboard==i);
    }
    if(g->keyboard==64)ui_text(">",3,21,true);
    else if(g->keyboard==65)ui_text(">",11,21,true);
    else if(g->keyboard==66)ui_text(">",27,21,true);
    ui_text("Space",5,21,g->keyboard==64);ui_text("Erase",13,21,g->keyboard==65);
    ui_text("Done",29,21,g->keyboard==66);
    ui_text("B Erase     Start Begin",8,25,true);
}

static void saves(const Game *g)
{
    ui_text("CHOOSE A MEMORY",12,2,true);
    static const char *names[]={"Arlen","Mira","Rowan","Selene"};
    for(u16 i=0;i<4;i++)
    {
        u16 x=1+(i%2)*20,y=5+(i/2)*10;
        ui_panel(x,y,18,9);ui_text(g->selection==i?"> MEMORY":"  Memory",x+2,y+1,true);
        ui_number(i+1,x+11,y+1,true);ui_text(names[i],x+2,y+3,false);
        ui_text("Level",x+2,y+4,false);ui_number(6+i*3,x+9,y+4,true);
        ui_text(i==0?"Willowmere":i==1?"Lantern Crypt":i==2?"Mossbell Wood":"Tideglass Quay",x+2,y+6,false);
    }
    ui_text("A Continue    B Title",9,26,true);
}

static void world(const Game *g)
{
    ui_panel(0,0,40,3);ui_text("THE LANTERN ROAD",12,1,true);
    static const u16 pins[4][2]={{11,12},{31,7},{19,6},{32,16}};
    for(u16 i=0;i<4;i++)
    {
        ui_image(i==g->selection?&art_map_pin_selected:&art_map_pin,PAL3,pins[i][0],pins[i][1]);
    }
    ui_panel(0,23,40,5);ui_text(zone_name(g->selection),3,24,true);
    ui_text("D-pad Choose   A Travel   B Back",3,26,false);
}

static void intro(const Game *g)
{
    const u16 part=(g->screen_age/240)%4;
    static const char *story[4][3]={
        {"When the last lantern faded,","the roads between our homes","fell silent."},
        {"But beyond the old mountains,","one small light remained.","A promise waiting to be kept."},
        {"Take the lantern. Find the road.","There are still stories","worth carrying home."},
        {"ASTERFALL","The Lantern Road","Your journey begins."}};
    ui_fill(3,5,34,7,1);
    for(u16 row=0;row<3;row++)ui_text(story[part][row],5,6+row*2,false);
    ui_text("Start Skip",15,27,true);
}

void ui_draw(const Game *g)
{
    switch(g->screen)
    {
        case INTRO:intro(g);break;
        case TITLE:title(g);break;
        case NAMING:naming(g);break;
        case SAVES:saves(g);break;
        case WORLD:world(g);break;
        case FIELD:break;
        case DIALOGUE:dialogue_draw(g,true);break;
        default:menu_draw(g);break;
    }
}

void ui_tick(const Game *g)
{
    if(g->screen==DIALOGUE)dialogue_draw(g,false);
    else if(g->screen==TITLE)ui_text((g->ticks/30)%2?">":" ",13,19+g->selection*2,true);
}
