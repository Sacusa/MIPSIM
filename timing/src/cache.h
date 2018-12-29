#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>
#include <stdlib.h>
#include "block.h"
#include "mshr.h"

#define WORD_SIZE 4
#define LOG2_WORD_SIZE 2

/* cache types */
#define IS_L1I_CACHE 0
#define IS_L1D_CACHE 1

/* cache access return code */
#define CACHE_DATA_AVAILABLE 0
#define CACHE_DATA_UNAVAILABLE 1

/* cache stall stages */
#define STALLED_ON_DRAM 0
#define STALLED_ON_DRAM_REQUEST 1
#define STALLED_ON_DRAM_RESPONSE 2
#define STALLED_ON_L2C_HIT 3
#define NOT_STALLED 4

/* L1I cache information */
#define L1I_NUM_SETS 64
#define L1I_NUM_WAYS 4
#define L1I_NUM_MSHRS 0
#define L1I_LOG2_NUM_SETS 6

/* L1D cache information */
#define L1D_NUM_SETS 256
#define L1D_NUM_WAYS 8
#define L1D_NUM_MSHRS 0
#define L1D_LOG2_NUM_SETS 8

/* L2C cache information */
#define L2C_NUM_SETS 512
#define L2C_NUM_WAYS 16
#define L2C_NUM_MSHRS 16
#define L2C_LOG2_NUM_SETS 9
#define L2C_HIT_LATENCY 15

/* transitionary latencies */
#define L2C_TO_DRAM_LATENCY 5
#define DRAM_TO_L2C_LATENCY 5

typedef struct Cache
{
    uint16_t NUM_SET, NUM_WAY, NUM_MSHR;
    Block **block;
    MSHR *mshr;
} Cache;

/* initialize cache and set all values to 0 in all the blocks */
void cache_init(Cache *cache, uint16_t num_set, uint16_t num_way, uint16_t num_mshr);

/* deallocate memory for the cache */
void cache_destroy(Cache *cache);

/* returns the way number for the set and tag value */
uint16_t cache_get_way(Cache *cache, uint16_t set, uint32_t tag);

/* insert data into the specified set and way and update LRU state */
void cache_insert_data(Cache *cache, uint16_t set, uint16_t way, uint32_t tag,
                       uint32_t data[BLOCK_SIZE]);

/* find victim for replacement */
uint16_t cache_find_victim(Cache *cache, uint16_t set);

/* make the specified set and way the MRU block and increase LRU values of others */
void cache_update_lru_state(Cache *cache, uint16_t set, uint16_t way);

/* returns the index of a free MSHR entry; if none is free, returns NUM_MSHR */
uint16_t get_free_mshr_index(Cache *cache);

/* allocates a new mshr and returns mshr index; if no MSHR is available, returns NUM_MSHR */
uint16_t allocate_mshr(Cache *cache, uint32_t address);

/* sets the done bit in the MSHR corresponding to the given address */
void mark_mshr_done(Cache *cache, uint32_t address);

/* invalidates the MSHR corresponding to the given address */
void invalidate_mshr(Cache *cache, uint32_t address);

#endif
