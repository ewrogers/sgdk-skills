/* Compile the unmodified submitted SDK adapter, including its actual mapping. */
#define main submitted_main
#include "submitted_main.c"
#undef main
#include "review_checks.h"
/* Keep a direct entry for the bare-M68000 runner despite release LTO. */
__attribute__((used, noipa, externally_visible)) int main(bool hardReset) {
    volatile u16 *output = (volatile u16 *)0xC00000;
    int failure;
    (void)hardReset;
    *output = 0xD15C;
    if (edges(BUTTON_C) != K_C || edges(BUTTON_B) != K_B ||
        edges(BUTTON_A) != K_A || edges(BUTTON_START) != K_START ||
        edges(BUTTON_LEFT | BUTTON_DOWN) != (K_LEFT | K_DOWN)) return __LINE__;
    failure = review_checks();
    if (failure) return failure;
    *output = (u16)(review_trace >> 16);
    *output = (u16)review_trace;
    *output = (u16)sizeof(Desk);
    return 0;
}
