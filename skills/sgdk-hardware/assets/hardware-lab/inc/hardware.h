#ifndef HARDWARE_LAB_H
#define HARDWARE_LAB_H
#ifdef HL_HOST
#include <stdint.h>
#include <stdbool.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;
#else
#include <genesis.h>
#endif
#define HL_RECORD_BYTES 12
#define HL_FIELD_WIDTH 36
#define HL_UPLOAD_START 0x8000
#define HL_UPLOAD_END 0x8800
typedef struct {u32 last,elapsed,missed;u16 phase,max_gap;} HlClock;
void hl_clock_init(HlClock *clock,u32 now);
/* One sample per rendered loop. Unsigned VInt subtraction wraps naturally. */
u16 hl_clock_sample(HlClock *clock,u32 now,u16 hz);
bool hl_row_fits(u16 pieces,u16 width,u16 screen_width);
bool hl_upload_fits(u16 address,u16 words);
/* Complete padded replacement: never an empty intermediate row. */
bool hl_format_field(char *out,const char *label,const char *value);
void hl_encode(u8 *record,u32 sequence);
bool hl_decode(const u8 *record,u32 *sequence);
#endif
