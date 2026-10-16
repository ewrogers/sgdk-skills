#ifndef LANGUAGE_LAB_LOC_H
#define LANGUAGE_LAB_LOC_H
#ifdef __m68k__
#include <types.h>
#else
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#endif

enum { LOC_NAME_MAX=8, LOC_TEXT_MAX=256, LOC_WIDTH_MAX=36, LOC_LINES_MAX=9,
       LOC_NAME_TOKEN=252, LOC_COUNT_TOKEN=253, LOC_NEWLINE=254 };
typedef struct { u8 id; const char *tag; u8 separator; } LocLanguage;
typedef struct { u8 width, rows, pages; } LocBox;
typedef struct { u8 cells[LOC_LINES_MAX][LOC_WIDTH_MAX]; u8 lines; } LocLayout;

/* Output buffers/layouts remain unchanged on failure. */
bool loc_number(u8 language, u16 number, u8 *out, u16 capacity);
bool loc_expand(u8 language, u16 message, const u8 *name, u16 count, u8 *out, u16 capacity);
bool loc_layout(const u8 *text, LocBox box, LocLayout *out);
u8 loc_language_index(u8 stable_id);
bool loc_pref_encode(u8 language_id, u8 out[6]);
bool loc_pref_decode(const u8 in[6], u8 *language_id);

#endif
