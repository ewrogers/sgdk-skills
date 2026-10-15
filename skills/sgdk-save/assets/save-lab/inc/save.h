#ifndef SAVE_LAB_SAVE_H
#define SAVE_LAB_SAVE_H

#ifdef __m68k__
#include <types.h>
#else
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#endif

#define SAVE_SLOT_BYTES 32
#define SAVE_BYTES (2 * SAVE_SLOT_BYTES)
#define SAVE_PASSWORD_LENGTH 12
#define SAVE_MAX_SCORE 999999UL

typedef struct {
    u32 score;
    u8 level;
    u8 lives;
    bool music;
} SaveState;

/* Offsets are logical SRAM bytes, not addresses on the 68000 bus. */
typedef struct {
    void *context;
    u32 capacity;
    bool (*read)(void *context, u32 offset, u8 *value);
    bool (*write)(void *context, u32 offset, u8 value);
} SaveIo;

typedef enum {
    SAVE_OK, SAVE_EMPTY, SAVE_CORRUPT, SAVE_UNSUPPORTED,
    SAVE_CONFLICT, SAVE_IO_ERROR, SAVE_BAD_STATE
} SaveResult;

void save_defaults(SaveState *state);
bool save_state_valid(const SaveState *state);
u16 save_crc16(const u8 *bytes, u16 length);
/* On failure, load and password decoding leave the caller's state untouched. */
SaveResult save_load(const SaveIo *io, SaveState *state, u8 *slot);
SaveResult save_store(const SaveIo *io, const SaveState *state);
/* Erasing is an explicit, destructive operation; never call on load failure. */
SaveResult save_erase(const SaveIo *io);
bool save_password_encode(const SaveState *state, char output[SAVE_PASSWORD_LENGTH + 1]);
bool save_password_decode(const char *text, SaveState *state);
extern const char save_password_alphabet[33];

#endif
