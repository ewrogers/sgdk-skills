#include "game.h"
#include <string.h>

static const char keyboard[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.-";
static void enter(Game *g,Screen screen)
{
    g->screen=screen;g->selection=0;g->screen_age=0;g->dirty=true;g->ui_dirty=true;
}

uint16_t menu_size(Screen screen)
{
    switch(screen){case TITLE:return 3;case SAVES:case WORLD:case OPTIONS:return 4;
        case MENU:return 7;case EQUIPMENT:return 5;case INVENTORY:return 8;case SPELLS:return 4;default:return 1;}
}

void game_init(Game *g)
{
    *g=(Game){0};g->screen=INTRO;g->dirty=true;g->ui_dirty=true;
    g->level=12;g->text_speed=1;g->music_volume=70;g->music_enabled=true;g->options_back=TITLE;
    field_enter(g);
}

static void move_selection(Game *g,uint16_t keys,uint16_t count,uint16_t columns)
{
    uint16_t old=g->selection;
    if(keys&UP)g->selection=(g->selection+count-columns)%count;
    else if(keys&DOWN)g->selection=(g->selection+columns)%count;
    else if(keys&LEFT)g->selection=(g->selection+count-1)%count;
    else if(keys&RIGHT)g->selection=(g->selection+1)%count;
    if(old!=g->selection){g->events|=CUE_MOVE;g->ui_dirty=true;}
}

static void naming(Game *g,uint16_t pressed,uint16_t keys)
{
    if(g->keyboard>=64)
    {
        if(keys&LEFT)g->keyboard=64+(g->keyboard-64+2)%3;
        else if(keys&RIGHT)g->keyboard=64+(g->keyboard-64+1)%3;
        else if(keys&UP)g->keyboard=56+(g->keyboard-64)*3;
        else if(keys&DOWN)g->keyboard=(g->keyboard-64)*3;
    }
    else if(keys&UP)g->keyboard=g->keyboard<8?64+g->keyboard/3:g->keyboard-8;
    else if(keys&DOWN)g->keyboard=g->keyboard>=56?64+(g->keyboard-56)/3:g->keyboard+8;
    else if(keys&LEFT)g->keyboard=(g->keyboard/8)*8+(g->keyboard+7)%8;
    else if(keys&RIGHT)g->keyboard=(g->keyboard/8)*8+(g->keyboard+1)%8;
    if(g->keyboard>66)g->keyboard=66;
    if(keys)g->ui_dirty=true;
    if(pressed&BACK)
    {
        if(g->name_length)g->name[--g->name_length]=0;
        else enter(g,TITLE);
        g->events|=CUE_BACK;g->ui_dirty=true;
    }
    if((pressed&START)||((pressed&CONFIRM)&&g->keyboard==66))
    {
        if(!g->name_length){g->events|=CUE_DENIED;return;}
        g->zone=0;enter(g,WORLD);g->selection=0;g->events|=CUE_ACCEPT;
    }
    else if(pressed&CONFIRM)
    {
        if(g->keyboard<64&&g->name_length<8)
        {g->name[g->name_length++]=keyboard[g->keyboard];g->name[g->name_length]=0;g->events|=CUE_NAME;}
        else if(g->keyboard==64&&g->name_length<8){g->name[g->name_length++]=' ';g->name[g->name_length]=0;}
        else if(g->keyboard==65&&g->name_length)g->name[--g->name_length]=0;
        else g->events|=CUE_DENIED;
        g->ui_dirty=true;
    }
}

void game_step(Game *g,uint16_t held,uint16_t pressed,uint16_t navigation)
{
    g->events=0;g->ticks++;g->screen_age++;
    if(g->screen==INTRO)
    {if((pressed&(CONFIRM|START|BACK))||g->screen_age>=960)enter(g,TITLE);return;}
    if(g->detail)
    {if(pressed&(BACK|CONFIRM|START)){g->detail=false;g->ui_dirty=true;g->events|=CUE_BACK;}return;}
    if(g->screen==NAMING){naming(g,pressed,navigation);return;}
    if(g->screen==FIELD)
    {
        field_step(g,held);
        if(pressed&(BACK|START)){enter(g,MENU);g->events|=CUE_ACCEPT;}
        else if(pressed&MAP){enter(g,WORLD);g->selection=g->zone;g->events|=CUE_ACCEPT;}
        else if(pressed&CONFIRM)
        {
            for(uint8_t n=0;n<2;n++)
            {
                int16_t dx=g->hero.x-g->npc[n].x,dy=g->hero.y-g->npc[n].y;
                if(dx*dx+dy*dy<=38*38)
                {g->speaker=n;g->dialogue_page=0;enter(g,DIALOGUE);g->events|=CUE_TALK;break;}
            }
        }
        return;
    }
    if(g->screen==DIALOGUE)
    {
        if(pressed&BACK){enter(g,FIELD);g->events|=CUE_BACK;}
        else if(pressed&(CONFIRM|START))
        {if(g->screen_age<300){g->screen_age=300;g->ui_dirty=true;}
            else if(++g->dialogue_page>=2)enter(g,FIELD);else {g->ui_dirty=true;g->screen_age=0;}
            g->events|=CUE_TALK;
        }
        return;
    }
    if(g->screen==OPTIONS)
    {
        if(navigation&(UP|DOWN))move_selection(g,navigation,4,1);
        if(navigation&(LEFT|RIGHT))
        {
            if(g->selection==0)g->text_speed=(g->text_speed+((navigation&LEFT)?2:1))%3;
            else if(g->selection==1)g->music_volume=(navigation&LEFT)?(g->music_volume?g->music_volume-10:100):(g->music_volume>=100?0:g->music_volume+10);
            else if(g->selection==2)g->music_enabled=!g->music_enabled;
            g->ui_dirty=true;g->events|=CUE_MOVE;
        }
        if(pressed&CONFIRM)
        {
            if(g->selection==3)enter(g,g->options_back);
            else {g->music_enabled=g->selection==2?!g->music_enabled:g->music_enabled;g->ui_dirty=true;}
            g->events|=CUE_ACCEPT;
        }
        if(pressed&BACK){enter(g,g->options_back);g->events|=CUE_BACK;}
        return;
    }
    if(g->screen==STATUS||g->screen==JOURNAL)
    {if(pressed&(BACK|CONFIRM|START)){enter(g,MENU);g->events|=CUE_BACK;}return;}
    if(g->screen==EQUIPMENT||g->screen==INVENTORY||g->screen==SPELLS)
    {
        move_selection(g,navigation,menu_size(g->screen),g->screen==INVENTORY?2:1);
        if(pressed&BACK){enter(g,MENU);g->events|=CUE_BACK;}
        else if(pressed&CONFIRM){g->detail=true;g->events|=CUE_ACCEPT;g->ui_dirty=true;}
        return;
    }
    move_selection(g,navigation,menu_size(g->screen),(g->screen==SAVES)?2:1);
    if(pressed&BACK)
    {
        if(g->screen==MENU)enter(g,FIELD);
        else if(g->screen==WORLD){enter(g,FIELD);}
        else enter(g,TITLE);
        g->events|=CUE_BACK;return;
    }
    if(!(pressed&(CONFIRM|START)))return;
    g->events|=CUE_ACCEPT;
    if(g->screen==TITLE)
    {
        if(g->selection==0){enter(g,NAMING);g->name_length=0;g->name[0]=0;g->keyboard=0;}
        else if(g->selection==1)enter(g,SAVES);
        else {g->options_back=TITLE;enter(g,OPTIONS);}
    }
    else if(g->screen==SAVES)
    {
        static const char *names[]={"Arlen","Mira","Rowan","Selene"};
        g->zone=g->selection;g->slot=g->selection;g->level=6+g->slot*3;strcpy(g->name,names[g->slot]);g->name_length=(uint8_t)strlen(g->name);
        field_enter(g);enter(g,FIELD);g->events|=CUE_TRAVEL;
    }
    else if(g->screen==WORLD)
    {g->zone=g->selection;field_enter(g);enter(g,FIELD);g->events|=CUE_TRAVEL;}
    else if(g->screen==MENU)
    {
        static const Screen destinations[]={STATUS,EQUIPMENT,INVENTORY,SPELLS,JOURNAL,WORLD,OPTIONS};
        uint16_t choice=g->selection;
        if(choice==6)g->options_back=MENU;
        enter(g,destinations[choice]);if(choice==5)g->selection=g->zone;
    }
}
