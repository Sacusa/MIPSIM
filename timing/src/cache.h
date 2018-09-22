#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>
#include <stdlib.h>
#include "block.h"

#define WORD_SIZE 4
#define LOG2_WORD_SIZE 2

#define L1I_NUM_SETS 64
#define L1I_NUM_WAYS 4
#define L1I_LOG2_NUM_SETS 6
#define L1I_MISS_STALL_CYCLE_COUNT 49

#define L1D_NUM_SETS 256
#define L1D_NUM_WAYS 8
#define L1D_LOG2_NUM_SETS 8
#define L1D_MISS_STALL_CYCLE_COUNT 49

typedef struct Cache {
    uint16_t NUM_SET, NUM_WAY;
    Block **block;
} Cache;

/* initialize cache and set all values to 0 in all the blocks */
void cache_init(Cache *cache, uint16_t num_set, uint16_t num_way);

/* deallocate memory for the cache */
void cache_destroy(Cache *cache);

/* returns the way number for the set and tag value */
uint16_t cache_get_way(Cache *cache, uint16_t set, uint32_t tag);

/* insert data into the specified set and way and update LRU state */
void cache_insert_data(Cache *cache, uint16_t set, uint16_t way, uint32_t tag, \
                       uint32_t data[BLOCK_SIZE]);

/* find victim for replacement */
uint16_t cache_find_victim(Cache *cache, uint16_t set);

/* make the specified set and way the MRU block and increase LRU values of others */
void cache_update_lru_state(Cache *cache, uint16_t set, uint16_t way);

#endif
