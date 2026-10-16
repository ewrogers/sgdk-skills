#ifndef ASTERFALL_GAME_H
#define ASTERFALL_GAME_H
#ifdef SGDK_GCC
#include <types.h>
typedef s16 int16_t;
typedef u16 uint16_t;
typedef u8 uint8_t;
typedef u32 uint32_t;
#else
#include <stdint.h>
#include <stdbool.h>
#endif

typedef enum {INTRO,TITLE,NAMING,SAVES,OPTIONS,WORLD,FIELD,MENU,STATUS,EQUIPMENT,INVENTORY,SPELLS,JOURNAL,DIALOGUE} Screen;
enum {UP=1,DOWN=2,LEFT=4,RIGHT=8,CONFIRM=16,BACK=32,MAP=64,START=128};
enum {CUE_MOVE=1,CUE_ACCEPT=2,CUE_BACK=4,CUE_DENIED=8,CUE_TALK=16,CUE_TRAVEL=32,CUE_NAME=64};
typedef struct {int16_t x,y;uint8_t direction,frame;bool moving;} Actor;
typedef struct {
    Screen screen,options_back;
    uint16_t selection,keyboard,events;
    uint32_t ticks,screen_age;
    uint8_t zone,slot,speaker,dialogue_page,text_speed,music_volume,level;
    char name[9];uint8_t name_length;
    Actor hero,npc[2];
    bool dirty,ui_dirty,music_enabled,detail;
} Game;

void game_init(Game *game);
void game_step(Game *game,uint16_t held,uint16_t pressed,uint16_t navigation);
bool field_walkable(uint8_t zone,int16_t x,int16_t y);
void field_enter(Game *game);
void field_step(Game *game,uint16_t held);
const char *zone_name(uint8_t zone);
const char *zone_subtitle(uint8_t zone);
uint16_t menu_size(Screen screen);
#endif
