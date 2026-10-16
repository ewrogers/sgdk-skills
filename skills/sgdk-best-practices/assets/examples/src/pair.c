#include "pair.h"

static Sprite *first;
static Sprite *second;

bool bp_pair_open(const SpriteDefinition *left, const SpriteDefinition *right)
{
    if (first || second || !left || !right) return false;
    Sprite *a = SPR_addSprite(left, 32, 32, TILE_ATTR(PAL0, false, false, false));
    if (!a) return false;
    Sprite *b = SPR_addSprite(right, 64, 32, TILE_ATTR(PAL0, false, false, false));
    if (!b)
    {
        SPR_releaseSprite(a);
        return false;
    }
    /* Publish only after acquiring both resources. */
    first = a;
    second = b;
    return true;
}

void bp_pair_close(void)
{
    if (second) { SPR_releaseSprite(second); second = NULL; }
    if (first) { SPR_releaseSprite(first); first = NULL; }
}

bool bp_pair_is_open(void)
{
    return first != NULL && second != NULL;
}
