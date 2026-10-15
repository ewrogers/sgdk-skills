#include "menu.h"

static uint16_t navigation(MenuInput *input, uint16_t held, uint16_t elapsed)
{
    uint16_t direction = held & (MENU_UP | MENU_DOWN | MENU_LEFT | MENU_RIGHT);
    if ((direction & (MENU_UP | MENU_DOWN)) == (MENU_UP | MENU_DOWN))
        direction &= (uint16_t)~(MENU_UP | MENU_DOWN);
    if ((direction & (MENU_LEFT | MENU_RIGHT)) == (MENU_LEFT | MENU_RIGHT))
        direction &= (uint16_t)~(MENU_LEFT | MENU_RIGHT);
    if (direction != input->direction) {
        input->direction = direction;
        input->repeat_ms = 0;
        input->repeating = false;
        return direction;
    }
    if (!direction) return 0;
    input->repeat_ms += elapsed;
    if (input->repeat_ms >= (input->repeating ? 100 : 300)) {
        input->repeat_ms -= input->repeating ? 100 : 300;
        input->repeating = true;
        return direction;
    }
    return 0;
}

static void enter(Menu *menu, MenuScreen screen)
{
    menu->screen = screen;
    menu->selected = 0;
    menu->input.direction = 0;
    menu->input.repeat_ms = 0;
    menu->input.repeating = false;
}

static void play(Menu *menu)
{
    enter(menu, MENU_PLAY);
    menu->score = 0;
    menu->remaining_ms = 12000;
    menu->scroll = 0;
    menu->marker_x = 20;
    menu->marker_y = 15;
}

void menu_init(Menu *menu)
{
    *menu = (Menu){0};
    menu->screen = MENU_TITLE;
    menu->difficulty = 1;
    menu->custom_font = true;
}

void menu_step(Menu *menu, uint16_t held, uint16_t elapsed_ms)
{
    uint16_t pressed = held & (uint16_t)~menu->input.previous;
    uint16_t nav;
    /* The adapter supplies one display frame, not unbounded catch-up time. */
    if (elapsed_ms > 20) elapsed_ms = 20;
    nav = navigation(&menu->input, held, elapsed_ms);
    menu->input.previous = held;
    if (menu->screen == MENU_PLAY) {
        if (pressed & MENU_START) { enter(menu, MENU_PAUSE); return; }
        if ((nav & MENU_LEFT) && menu->marker_x > 2) menu->marker_x--;
        if ((nav & MENU_RIGHT) && menu->marker_x < 37) menu->marker_x++;
        if ((nav & MENU_UP) && menu->marker_y > 7) menu->marker_y--;
        if ((nav & MENU_DOWN) && menu->marker_y < 23) menu->marker_y++;
        if (pressed & MENU_BACK) menu->score = 0;
        else if (pressed & MENU_ACCEPT) {
            uint32_t award = (uint32_t)(menu->difficulty + 1) * 50;
            menu->score = menu->score > 999999 - award ? 999999 : menu->score + award;
        }
        menu->scroll = (uint16_t)((menu->scroll + 1) & 511);
        if (elapsed_ms >= menu->remaining_ms) {
            menu->remaining_ms = 0;
            enter(menu, MENU_RESULTS);
        } else menu->remaining_ms -= elapsed_ms;
        return;
    }
    if (menu->screen == MENU_CREDITS) {
        if (pressed & (MENU_BACK | MENU_ACCEPT | MENU_START)) enter(menu, MENU_TITLE);
        return;
    }
    {
        uint16_t count = menu->screen == MENU_RESULTS ? 2 : 3;
        if (nav & MENU_UP) menu->selected = (uint16_t)((menu->selected + count - 1) % count);
        if (nav & MENU_DOWN) menu->selected = (uint16_t)((menu->selected + 1) % count);
    }
    if (menu->screen == MENU_OPTIONS) {
        if ((pressed & MENU_BACK) || (menu->selected == 2 && (pressed & MENU_ACCEPT)))
            enter(menu, MENU_TITLE);
        else if ((nav & (MENU_LEFT | MENU_RIGHT)) || (pressed & MENU_ACCEPT)) {
            if (menu->selected == 0)
                menu->difficulty = (uint16_t)((menu->difficulty + ((nav & MENU_LEFT) ? 2 : 1)) % 3);
            if (menu->selected == 1) menu->custom_font = !menu->custom_font;
        }
    } else if (menu->screen == MENU_PAUSE) {
        if ((pressed & (MENU_BACK | MENU_START)) || (menu->selected == 0 && (pressed & MENU_ACCEPT)))
            enter(menu, MENU_PLAY);
        else if (pressed & MENU_ACCEPT) {
            if (menu->selected == 1) play(menu);
            else enter(menu, MENU_TITLE);
        }
    } else if (pressed & (MENU_ACCEPT | MENU_START)) {
        if (menu->screen == MENU_RESULTS) {
            if (menu->selected == 0) play(menu);
            else enter(menu, MENU_TITLE);
        } else if (menu->selected == 0) play(menu);
        else enter(menu, menu->selected == 1 ? MENU_OPTIONS : MENU_CREDITS);
    }
}

bool menu_field(char *out, size_t capacity, const char *text, size_t width)
{
    size_t i;
    bool ended = false;
    if (!out || !text || width >= capacity) return false;
    for (i = 0; i < width; i++) {
        unsigned char c = ended ? 0 : (unsigned char)text[i];
        if (!c) ended = true;
        out[i] = ended ? ' ' : (c >= 32 && c <= 126 ? (char)c : '?');
    }
    out[width] = '\0';
    return true;
}

bool menu_number(char *out, size_t capacity, uint32_t value, size_t width)
{
    size_t i;
    if (!out || !width || width >= capacity) return false;
    for (i = width; i > 0; i--) {
        out[i - 1] = (char)('0' + value % 10);
        value /= 10;
    }
    out[width] = '\0';
    if (value) for (i = 0; i < width; i++) out[i] = '#';
    return true;
}
