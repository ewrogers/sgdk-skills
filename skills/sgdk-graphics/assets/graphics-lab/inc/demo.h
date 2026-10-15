#ifndef GRAPHICS_LAB_DEMO_H
#define GRAPHICS_LAB_DEMO_H
#include <genesis.h>

/* Example-owned scene controller, not an SGDK API. */
bool demo_init(const Image *far_image, const Image *near_image,
               const SpriteDefinition *actor);
void demo_step(u16 held);
void demo_end(void);
u16 demo_completions(void);
#endif
