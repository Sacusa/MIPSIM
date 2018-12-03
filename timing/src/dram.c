#include "dram.h"

void dram_load_block(uint32_t address, uint32_t *block)
{
    uint32_t address_mask = 0xffffffff << (LOG2_WORD_SIZE + LOG2_BLOCK_SIZE);

    for (uint8_t index = 0; index < BLOCK_SIZE; ++index)
    {
        block[index] = mem_read_32((address & address_mask) + (index << LOG2_WORD_SIZE));
    }
}
