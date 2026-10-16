#include "view.h"
#include "resources.h"

static void footer(const char *hint){ui_text(hint,3,26,true);}
static void heading(const char *name){ui_text(name,3,2,true);ui_text("The Lantern Road",21,2,false);}
static void portrait(const Game *g)
{
    ui_image(&art_portrait_hero,PAL2,3,5);ui_text(g->name,13,6,true);
    ui_text("Roadkeeper / Lv",13,8,false);ui_number(g->level,29,8,true);
    ui_text("HP",13,10,true);ui_bar(17,10,16,186,224,12);
    ui_text("186 / 224",18,11,false);
    ui_text("MP",13,13,true);ui_bar(17,13,16,38,64,5);
    ui_text("38 / 64",18,14,false);
}
static void status(const Game *g)
{
    heading("STATUS");portrait(g);
    ui_text("Strength   28",4,17,false);ui_text("Spirit    34",23,17,false);
    ui_text("Defense    21",4,19,false);ui_text("Agility   26",23,19,false);
    ui_text("Next level",4,22,true);ui_bar(17,22,17,640,1000,8);
    ui_text("640 / 1000 EXP",18,23,false);footer("A / B Return to menu");
}
static void equipment(const Game *g)
{
    heading("EQUIPMENT");
    const Image *icons[]={&art_icon_sword,&art_icon_shield,&art_icon_armor,&art_icon_boots,&art_icon_ring};
    const char *names[]={"Dawnsteel Sword","Tideglass Shield","Traveller's Vest","Wayfarer Boots","Lantern Ring"};
    const char *types[]={"Hand","Guard","Body","Feet","Charm"};
    for(u16 i=0;i<5;i++)
    {
        u16 y=6+i*3;ui_image(icons[i],PAL1,4,y);
        ui_text(types[i],8,y,true);ui_text(names[i],15,y,false);
        if(g->selection==i)ui_text(">",2,y,true);
    }
    ui_text("Attack 42    Defense 31",5,23,true);footer("A Inspect    B Return");
}
static void inventory(const Game *g)
{
    heading("SATCHEL");
    const Image *icons[]={&art_icon_potion,&art_icon_ether,&art_icon_herb,&art_icon_key,&art_icon_sword,&art_icon_ring,&art_icon_shield,&art_icon_boots};
    const char *names[]={"Green Tonic","Blue Ether","Moss Herb","Crypt Key","Spare Blade","Old Ring","Teal Guard","Soft Boots"};
    const u16 counts[]={5,3,12,1,1,2,1,1};
    for(u16 i=0;i<8;i++)
    {
        u16 x=2+(i%2)*19,y=6+(i/2)*4;
        if(g->selection==i)ui_text(">",x,y,true);
        ui_image(icons[i],PAL1,x+1,y);ui_text(names[i],x+4,y,false);
        ui_text("x",x+4,y+2,true);ui_number(counts[i],x+6,y+2,true);
    }
    ui_text("A well-packed road is a kind road.",3,23,false);footer("A Inspect    B Return");
}
static void spells(const Game *g)
{
    heading("ARTS AND SPELLS");
    const Image *icons[]={&art_spell_flame,&art_spell_frost,&art_spell_heal,&art_spell_bolt};
    const char *names[]={"Lantern Flame","Winter Shard","Gentle Mending","Stormlight"};
    const char *details[]={"A brave little fire.","A crystal of cold air.","A moment to recover.","Light across the clouds."};
    const u16 costs[]={4,6,3,9};
    for(u16 i=0;i<4;i++)
    {
        u16 y=6+i*4;ui_image(icons[i],PAL1,4,y);ui_text(names[i],8,y,g->selection==i);
        ui_text(details[i],8,y+1,false);ui_text("MP",31,y,true);ui_number(costs[i],34,y,true);
        if(g->selection==i)ui_text(">",2,y,true);
    }
    footer("A Inspect    B Return");
}
static void journal(void)
{
    heading("JOURNAL");
    ui_text("A light worth carrying",4,6,true);
    ui_text("Visit the four roads of Asterfall.",4,9,false);
    ui_text("Meet the people who keep them.",4,11,false);
    ui_text("Find the old lantern in the crypt.",4,13,false);
    ui_text("Then watch the tide at the quay.",4,15,false);
    ui_text("Chapter 1 / The Open Road",4,19,true);
    ui_bar(4,22,30,3,5,8);footer("A / B Return to menu");
}
static void options(const Game *g)
{
    heading("OPTIONS");
    const char *labels[]={"Text speed","Music volume","Music","Return"};
    const char *speed[]={"Swift","Gentle","Leisurely"};
    for(u16 i=0;i<4;i++)
    {
        u16 y=7+i*4;ui_text(labels[i],5,y,g->selection==i);
        if(g->selection==i)ui_text(">",3,y,true);
    }
    ui_text(speed[g->text_speed],25,7,false);ui_number(g->music_volume,25,11,false);
    ui_text(g->music_enabled?"On":"Off",25,15,false);
    footer("Left / Right Adjust   B Back");
}
static void main_menu(const Game *g)
{
    heading("ROADKEEPER");ui_panel(2,5,16,19);ui_panel(19,5,19,19);
    const char *labels[]={"Status","Equipment","Inventory","Spells","Journal","Travel map","Options"};
    for(u16 i=0;i<7;i++)
    {
        ui_text(labels[i],5,7+i*2,g->selection==i);
        if(g->selection==i)ui_text(">",3,7+i*2,true);
    }
    ui_image(&art_portrait_hero,PAL2,24,7);ui_text(g->name,24,16,true);
    ui_text("Lv",25,18,false);ui_number(g->level,28,18,true);ui_text("Gold  1,280",22,21,true);
    footer("A Choose    B Resume your journey");
}
static void detail(const Game *g)
{
    const char *heading_text=g->screen==EQUIPMENT?"ROADKEEPER'S GEAR":g->screen==INVENTORY?"FROM YOUR SATCHEL":"A LITTLE MAGIC";
    const char *equipment_text[]={"A blade for a brighter dawn.","Keep the storm at your back.","Patched for another journey.","Soft steps on a long road.","A promise set in gold."};
    const char *item_text[]={"Restore a little courage.","A sip of bottled starlight.","The forest shares its gifts.","A key to an older story.","A spare for uncertain roads.","Someone kept this for years.","A trusty guard against rain.","Made for wandering."};
    const char *spell_text[]={"A small fire, bravely carried.","Winter held in a single shard.","Kindness is its own magic.","Call a light beyond the clouds."};
    const char *text=g->screen==EQUIPMENT?equipment_text[g->selection]:g->screen==INVENTORY?item_text[g->selection]:spell_text[g->selection];
    ui_panel(2,7,36,15);ui_text(heading_text,5,10,true);
    ui_text(text,5,14,false);ui_text("Part of your roadkeeper's story.",5,16,false);
    ui_text("A / B Close",14,19,true);
}
void menu_draw(const Game *g)
{
    switch(g->screen)
    {
        case STATUS:status(g);break;case EQUIPMENT:equipment(g);break;
        case INVENTORY:inventory(g);break;case SPELLS:spells(g);break;
        case JOURNAL:journal();break;case OPTIONS:options(g);break;
        default:main_menu(g);break;
    }
    if(g->detail)detail(g);
}
