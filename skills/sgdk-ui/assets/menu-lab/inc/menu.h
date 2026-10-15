#ifndef MENU_H
#define MENU_H

#ifdef __m68k__
#include <genesis.h>
typedef u16 uint16_t;
typedef s16 int16_t;
typedef u32 uint32_t;
#else
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#endif

enum { MENU_UP = 1, MENU_DOWN = 2, MENU_LEFT = 4, MENU_RIGHT = 8,
       MENU_ACCEPT = 16, MENU_BACK = 32, MENU_START = 64 };
typedef enum { MENU_TITLE, MENU_OPTIONS, MENU_CREDITS, MENU_PLAY,
               MENU_PAUSE, MENU_RESULTS } MenuScreen;

typedef struct {
    uint16_t previous, direction, repeat_ms;
    bool repeating;
} MenuInput;

typedef struct {
    MenuScreen screen;
    uint16_t selected, difficulty;
    bool custom_font;
    uint32_t score;
    uint16_t remaining_ms, scroll;
    int16_t marker_x, marker_y;
    MenuInput input;
} Menu;

void menu_init(Menu *menu);
void menu_step(Menu *menu, uint16_t held, uint16_t elapsed_ms);
/* Writes exactly width ASCII cells plus a terminator, or fails without writes. */
bool menu_field(char *out, size_t capacity, const char *text, size_t width);
bool menu_number(char *out, size_t capacity, uint32_t value, size_t width);

#endif
