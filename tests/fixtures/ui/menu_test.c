#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "menu.h"

static void tap(Menu *m, uint16_t key)
{
    menu_step(m, 0, 17);
    menu_step(m, key, 17);
}

static void start(Menu *m)
{
    menu_init(m);
    menu_step(m, MENU_ACCEPT, 17);
    assert(m->screen == MENU_PLAY && m->remaining_ms == 12000);
}

static void navigation(void)
{
    Menu m;
    menu_init(&m);
    tap(&m, MENU_UP); assert(m.selected == 2);
    tap(&m, MENU_DOWN); assert(m.selected == 0);
    tap(&m, MENU_UP | MENU_DOWN); assert(m.selected == 0);
    tap(&m, MENU_DOWN); assert(m.selected == 1);
    for (int i = 0; i < 17; i++) menu_step(&m, MENU_DOWN, 17);
    assert(m.selected == 1);
    menu_step(&m, MENU_DOWN, 17); assert(m.selected == 2);
    for (int i = 0; i < 5; i++) menu_step(&m, MENU_DOWN, 17);
    assert(m.selected == 2);
    menu_step(&m, MENU_DOWN, 17); assert(m.selected == 0);
    menu_step(&m, MENU_UP, 17); assert(m.selected == 2);
    menu_step(&m, 0, 17); menu_step(&m, MENU_DOWN, 17); assert(m.selected == 0);
}

static void options(void)
{
    Menu m;
    menu_init(&m);
    tap(&m, MENU_DOWN); tap(&m, MENU_ACCEPT);
    assert(m.screen == MENU_OPTIONS && m.difficulty == 1 && m.custom_font);
    tap(&m, MENU_LEFT); assert(m.difficulty == 0);
    tap(&m, MENU_LEFT); assert(m.difficulty == 2);
    tap(&m, MENU_RIGHT); assert(m.difficulty == 0);
    tap(&m, MENU_DOWN); tap(&m, MENU_ACCEPT); assert(!m.custom_font);
    tap(&m, MENU_BACK); assert(m.screen == MENU_TITLE);
    tap(&m, MENU_DOWN); tap(&m, MENU_ACCEPT);
    assert(m.difficulty == 0 && !m.custom_font);
    tap(&m, MENU_DOWN); tap(&m, MENU_DOWN); tap(&m, MENU_ACCEPT);
    assert(m.screen == MENU_TITLE);
    tap(&m, MENU_UP); tap(&m, MENU_ACCEPT); assert(m.screen == MENU_CREDITS);
    tap(&m, MENU_BACK); assert(m.screen == MENU_TITLE);
}

static void edges(void)
{
    Menu m;
    start(&m);
    for (int i = 0; i < 60; i++) menu_step(&m, MENU_ACCEPT, 17);
    assert(m.score == 0); /* Starting press cannot also award points. */
    tap(&m, MENU_ACCEPT); assert(m.score == 100);
    for (int i = 0; i < 10; i++) menu_step(&m, MENU_ACCEPT, 17);
    assert(m.score == 100);
    tap(&m, MENU_BACK); assert(m.score == 0);
    m.score = 999950; tap(&m, MENU_ACCEPT); assert(m.score == 999999);
    tap(&m, MENU_ACCEPT); assert(m.score == 999999);
    tap(&m, MENU_BACK | MENU_ACCEPT); assert(m.score == 0);
}

static void transitions(void)
{
    Menu m;
    uint16_t time;
    start(&m); tap(&m, MENU_ACCEPT);
    tap(&m, MENU_START); time = m.remaining_ms;
    assert(m.screen == MENU_PAUSE);
    for (int i = 0; i < 1000; i++) menu_step(&m, MENU_START, 20);
    assert(m.screen == MENU_PAUSE && m.remaining_ms == time && m.score == 100);
    tap(&m, MENU_BACK); assert(m.screen == MENU_PLAY && m.remaining_ms == time);
    tap(&m, MENU_START); tap(&m, MENU_DOWN); tap(&m, MENU_ACCEPT);
    assert(m.screen == MENU_PLAY && m.score == 0 && m.remaining_ms == 12000);
    tap(&m, MENU_START); tap(&m, MENU_UP); tap(&m, MENU_ACCEPT);
    assert(m.screen == MENU_TITLE);
    tap(&m, MENU_ACCEPT); m.remaining_ms = 17;
    menu_step(&m, 0, 17); assert(m.screen == MENU_RESULTS && m.remaining_ms == 0);
    tap(&m, MENU_DOWN); tap(&m, MENU_START); assert(m.screen == MENU_TITLE);
    tap(&m, MENU_ACCEPT); m.remaining_ms = 1;
    menu_step(&m, 0, 20); assert(m.screen == MENU_RESULTS);
    tap(&m, MENU_ACCEPT); assert(m.screen == MENU_PLAY && m.remaining_ms == 12000);
}

static void timing(void)
{
    for (unsigned hz = 50; hz <= 60; hz += 10) {
        Menu m;
        unsigned fraction = 0;
        start(&m);
        for (unsigned i = 0; i < hz * 12; i++) {
            fraction += 1000;
            menu_step(&m, 0, (uint16_t)(fraction / hz));
            fraction %= hz;
            if (i < hz * 12 - 1) assert(m.screen == MENU_PLAY);
        }
        assert(m.screen == MENU_RESULTS && m.remaining_ms == 0);
        menu_init(&m);
        fraction = 0;
        menu_step(&m, MENU_DOWN, 0);
        {
            unsigned elapsed = 0;
            for (unsigned i = 0; i < hz * 2; i++) {
                unsigned dt, moves;
                fraction += 1000;
                dt = fraction / hz;
                fraction %= hz;
                elapsed += dt;
                menu_step(&m, MENU_DOWN, (uint16_t)dt);
                moves = 1 + (elapsed >= 300 ? (elapsed - 300) / 100 + 1 : 0);
                assert(m.selected == moves % 3);
            }
        }
    }
}

static void movement(void)
{
    Menu m;
    start(&m); m.remaining_ms = 65535;
    for (int i = 0; i < 1000; i++) menu_step(&m, MENU_LEFT | MENU_UP, 20);
    assert(m.marker_x == 2 && m.marker_y == 7 && m.scroll < 512);
    for (int i = 0; i < 1000; i++) menu_step(&m, MENU_RIGHT | MENU_DOWN, 20);
    assert(m.marker_x == 37 && m.marker_y == 23 && m.scroll < 512);
    menu_step(&m, MENU_LEFT | MENU_RIGHT, 20); assert(m.marker_x == 37);
}

static void formatting(void)
{
    char out[13];
    memset(out, '!', sizeof(out));
    assert(menu_field(out + 1, 11, "LONGER TITLE", 6));
    assert(strcmp(out + 1, "LONGER") == 0 && out[0] == '!' && out[8] == '!');
    assert(menu_field(out + 1, 11, "GO", 6));
    assert(strcmp(out + 1, "GO    ") == 0);
    assert(menu_field(out, sizeof(out), "A\n\xFF", 4));
    assert(strcmp(out, "A?? ") == 0);
    assert(menu_field(out, sizeof(out), "", 0) && out[0] == 0);
    memset(out, '!', sizeof(out));
    assert(!menu_field(out, 6, "X", 6) && out[0] == '!');
    assert(!menu_field(out, 0, "X", 0));
    assert(!menu_field(NULL, 7, "X", 6));
    assert(!menu_field(out, sizeof(out), NULL, 6));
    assert(menu_number(out, sizeof(out), 0, 6) && strcmp(out, "000000") == 0);
    assert(menu_number(out, sizeof(out), 999999, 6) && strcmp(out, "999999") == 0);
    assert(menu_number(out, sizeof(out), UINT32_MAX, 10) && strcmp(out, "4294967295") == 0);
    assert(menu_number(out, sizeof(out), 1000000, 6) && strcmp(out, "######") == 0);
    memset(out, '!', sizeof(out));
    assert(!menu_number(out, sizeof(out), 5, 0) && out[0] == '!');
    assert(!menu_number(out, 6, 5, 6) && out[0] == '!');
    assert(!menu_number(NULL, 7, 5, 6));
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (!strcmp(argv[1], "navigation")) navigation();
    else if (!strcmp(argv[1], "options")) options();
    else if (!strcmp(argv[1], "edges")) edges();
    else if (!strcmp(argv[1], "transitions")) transitions();
    else if (!strcmp(argv[1], "timing")) timing();
    else if (!strcmp(argv[1], "movement")) movement();
    else if (!strcmp(argv[1], "formatting")) formatting();
    else assert(0);
    puts(argv[1]);
    return 0;
}
