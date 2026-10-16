#ifndef FX_VIEW_H
#define FX_VIEW_H
#include "effects.h"
bool fx_view_init(void);
void fx_view_end(void);
void fx_view_draw(const FxLab *s,const FxRender *render);
void fx_view_hud(const FxLab *s,const FxRender *render,u16 hz,u16 fps);
#endif
