#ifndef GENESIS_H
#define GENESIS_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#define TRUE true
#define FALSE false
void *MEM_alloc(u16 bytes);
void MEM_free(void *ptr);
void MEM_pack(void);
u16 MEM_getFree(void);
u16 MEM_getLargestFreeBlock(void);
u16 MEM_getAllocated(void);
#endif
