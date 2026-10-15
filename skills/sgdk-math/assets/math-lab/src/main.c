#include <genesis.h>
#include "resources.h"
#include "demo.h"

_Static_assert(KEY_A == BUTTON_A && KEY_B == BUTTON_B && KEY_START == BUTTON_START,
               "Demo input masks must match the cached SGDK snapshot");
_Static_assert(KEY_UP == BUTTON_UP && KEY_DOWN == BUTTON_DOWN &&
               KEY_LEFT == BUTTON_LEFT && KEY_RIGHT == BUTTON_RIGHT,
               "Demo directions must match SGDK joypad masks");

static Demo demo;
static s16 sine[360];
static u16 perspective[256];
static Sprite *stars[STAR_COUNT], *orbits[ORBIT_COUNT];

static bool create_sprites(void)
{
    for (u16 i = 0; i < STAR_COUNT; i++)
    {
        stars[i] = SPR_addSprite(&star_image, 0, 0, TILE_ATTR(PAL1, FALSE, FALSE, FALSE));
        if (!stars[i]) return false;
        SPR_setAutoAnimation(stars[i], FALSE);
        SPR_setVisibility(stars[i], VISIBLE); /* The model keeps the whole sprite onscreen. */
    }
    for (u16 i = 0; i < ORBIT_COUNT; i++)
    {
        orbits[i] = SPR_addSprite(&orb_image, 0, 0, TILE_ATTR(PAL2, FALSE, FALSE, FALSE));
        if (!orbits[i]) return false;
        SPR_setAutoAnimation(orbits[i], FALSE);
        SPR_setVisibility(orbits[i], VISIBLE);
    }
    return true;
}

static void write_line(char line[40], u16 row)
{
    u16 length = strlen(line);
    while (length < 38) line[length++] = ' ';
    line[38] = 0;
    /* RowEx copies its temporary tile data into SGDK's DMA queue buffer. */
    VDP_drawTextEx(BG_A, line, TILE_ATTR(PAL0, TRUE, FALSE, FALSE), 1, row, DMA_QUEUE);
}

static void draw_metrics(u16 hz, u16 fps)
{
    char line[40];
    static bool was_visible = true;
    static const char *names[] = {"ORBIT", "FIGURE EIGHT", "SPIRAL"};
    if (!demo.metrics)
    {
        if (was_visible)
        {
            line[0] = 0;
            write_line(line, 1);
            write_line(line, 2);
        }
        was_visible = false;
        return;
    }
    was_visible = true;
    sprintf(line, "MATH LAB  %s  %s", names[demo.pattern], demo.paused ? "PAUSED" : "      ");
    write_line(line, 1);
    sprintf(line, "%uHZ %uFPS SPEED %u/8 LOAD %u/100", hz, fps, demo.speed, SYS_getCPULoad());
    write_line(line, 2);
}

int main(bool hard_reset)
{
    (void)hard_reset;
    SYS_disableInts();
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    VDP_setBackgroundColor(0);
    PAL_setColor(0, RGB24_TO_VDPCOLOR(0x040408));
    PAL_setPalette(PAL1, star_image.palette->data, CPU);
    PAL_setPalette(PAL2, orb_image.palette->data, CPU);
    VDP_setTextPalette(PAL0);
    PAL_setColor(15, RGB24_TO_VDPCOLOR(0x92DBFF));
    SPR_initEx(160);
    if (!create_sprites())
    {
        VDP_drawText("NOT ENOUGH SPRITE MEMORY", 5, 12);
        SYS_enableInts();
        while (true) SYS_doVBlankProcess();
    }
    /* The SDK supplies the trig semantics. Cache once rather than repeatedly
     * normalizing angles for every object during the frame. */
    for (u16 i = 0; i < 360; i++) sine[i] = F16_sin(FIX16(i));
    perspective[0] = 0;
    for (u16 i = 1; i < 256; i++) perspective[i] = 16384 / i;
    demo_init(&demo, sine, perspective);
    VDP_drawText("S G D K    M A T H", 11, 13);
    VDP_drawText("LEFT/RIGHT PATTERN   UP/DOWN SPEED", 4, 25);
    VDP_drawText("A METRICS   B RESET   START PAUSE", 4, 26);
    SYS_enableInts();

    u16 frame = 0;
    u16 hz = SYS_isPAL() ? 50 : 60;
    while (true)
    {
        u16 fps = SYS_getFPS(); /* Exactly one call per rendered frame. */
        /* JOY state is sampled by SGDK's frame processing and cached here. */
        u16 held = JOY_readJoypad(JOY_1);
        demo_frame(&demo, held, hz);
        for (u16 i = 0; i < STAR_COUNT; i++)
        {
            SPR_setPosition(stars[i], demo.stars[i].screen.x - 4, demo.stars[i].screen.y - 4);
            SPR_setFrame(stars[i], demo.stars[i].shade);
        }
        u16 animation_phase = (demo.ticks / 12) & 3;
        for (u16 i = 0; i < ORBIT_COUNT; i++)
        {
            SPR_setPosition(orbits[i], demo.orbit[i].x - 8, demo.orbit[i].y - 8);
            SPR_setFrame(orbits[i], (i / 4 + animation_phase) & 3);
        }
        if (!(frame++ % 10)) draw_metrics(hz, fps);
        SPR_update();
        SYS_doVBlankProcess();
    }
}
