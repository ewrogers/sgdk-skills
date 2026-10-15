#ifndef TRANSITION_CHECKPOINT_H
#define TRANSITION_CHECKPOINT_H
#include "lab.h"
#define CHECKPOINT_SLOT_BYTES 48
#define CHECKPOINT_BYTES 96
typedef struct { Run run; bool music, effects; } Checkpoint;
typedef struct {
    bool (*read)(u16 offset, u8 *value);
    bool (*write)(u16 offset, u8 value);
} CheckpointIo;
bool checkpoint_load(const CheckpointIo *io, Checkpoint *out);
bool checkpoint_store(const CheckpointIo *io, const Checkpoint *value);
/* Public for byte-format tests and inspection; never dump a C struct to SRAM. */
u16 checkpoint_crc(const u8 *data, u16 size);
bool checkpoint_decode(const u8 data[CHECKPOINT_SLOT_BYTES], Checkpoint *out, u32 *sequence);
bool checkpoint_encode(const Checkpoint *value, u32 sequence, u8 data[CHECKPOINT_SLOT_BYTES]);
#endif
