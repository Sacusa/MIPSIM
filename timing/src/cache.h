#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>
#include <stdlib.h>
#include "block.h"

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
