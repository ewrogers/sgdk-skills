#ifndef HARDWARE_LAB_UI_H
#define HARDWARE_LAB_UI_H
#include <genesis.h>
/* Call once per entry with display disabled; all page art remains resident. */
void hl_ui_load(void);
void hl_ui_begin(u16 page);
void hl_ui_text(const char *value,u16 x,u16 y,u16 pal);
void hl_ui_hex(u32 value,u16 digits,u16 x,u16 y,u16 pal);
void hl_ui_number(u32 value,u16 x,u16 y);
void hl_ui_field(const char *value,u16 x,u16 y,u16 width,u16 pal);
void hl_ui_card(u16 y,u16 height);
void hl_ui_art(u16 offset,u16 width,u16 height,u16 x,u16 y);
void hl_ui_solid(u16 color,u16 x,u16 y,u16 width,u16 height);
void hl_ui_meter(u16 head,u16 x,u16 y);
void hl_ui_bar(u16 active,u16 cells,u16 x,u16 y);
void hl_ui_arrow(u16 direction,u16 x,u16 y,u16 pal);
/* Decorative playback activity, not an audio amplitude measurement. */
void hl_ui_amp(u16 phase,bool active,u16 x,u16 y);
#endif
