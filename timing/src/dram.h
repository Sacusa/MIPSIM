#ifndef _DRAM_H_
#define _DRAM_H_

#include <stdint.h>
#include <stdlib.h>
#include "block.h"
#include "cache.h"

#define DRAM_ACCESS_LATENCY 50

/* loads a BLOCK_SIZE sized chunk starting at address 'address' into 'block' */
void dram_load_block(uint32_t address, uint32_t *block);

#endif