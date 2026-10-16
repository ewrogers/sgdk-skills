#include "game.h"
bool battle_position(const Battle *b, uint16_t i, int16_t *x, int16_t *y)
{
    if (i >= 6)
        return false;
    static const int16_t party_x[] = {224, 264, 224}, party_y[] = {26, 56, 86};
    static const int16_t enemy_x[] = {44, 104, 148}, enemy_y[] = {48, 80, 42};
    if (i < 3)
    {
        *x = party_x[i];
        *y = party_y[i];
    }
    else if (b->encounter == 2)
    {
        *x = i == 3 ? 40 : 130;
        *y = i == 3 ? 48 : 34;
    }
    else
    {
        *x = enemy_x[i - 3];
        *y = enemy_y[i - 3];
    }
    return true;
}
