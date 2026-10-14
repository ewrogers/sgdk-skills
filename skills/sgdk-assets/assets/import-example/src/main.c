#include <genesis.h>

#include "resources.h"

int main(bool hard_reset)
{
    (void) hard_reset;

    VDP_setScreenWidth320();
    SPR_init();

    VDP_drawImageEx(BG_A, &backdrop,
        TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX),
        0, 0, TRUE, TRUE);
    PAL_setPalette(PAL1, sprite_colors.data, DMA);

    Sprite* marble_sprite = SPR_addSprite(&marble, 152, 104,
        TILE_ATTR(PAL1, FALSE, FALSE, FALSE));

    if (marble_sprite == NULL)
    {
        VDP_drawText("Sprite allocation failed", 2, 2);
    }

    while (TRUE)
    {
        SPR_update();
        SYS_doVBlankProcess();
    }

    return 0;
}
