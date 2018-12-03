#ifndef _BLOCK_H_
#define _BLOCK_H_

#include <stdint.h>

#define BLOCK_SIZE 8
#define LOG2_BLOCK_SIZE 3

typedef struct Block
{
    uint8_t valid, dirty;
    uint32_t tag;

    uint8_t lru;

    uint32_t data[BLOCK_SIZE];
} Block;

#endif
