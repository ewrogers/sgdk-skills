#ifndef DL_PANEL_H
#define DL_PANEL_H
#include "dl_types.h"
/* Visible H40 tile coordinates. A valid rectangle is entirely on screen. */
bool panel_fits(u16 x, u16 y, u16 width, u16 height);
/* Nine pieces in row-major order; 255 means an invalid size/coordinate. */
u8 panel_piece(u16 x, u16 y, u16 width, u16 height);
#endif
