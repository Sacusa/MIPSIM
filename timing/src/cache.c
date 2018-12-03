#include "cache.h"

void cache_init (Cache *cache, uint16_t num_set, uint16_t num_way, uint16_t num_mshr) {
    cache->NUM_SET = num_set;
    cache->NUM_WAY = num_way;
    cache->NUM_MSHR = num_mshr;

    if (num_mshr > 0) {
        /* allocate and initialize mshrs */
        cache->mshr = (MSHR*) malloc(num_mshr * sizeof(MSHR));

        for (uint16_t i = 0; i < num_mshr; ++i) {
            cache->mshr[i].valid = 0;
            cache->mshr[i].done = 0;
            cache->mshr[i].address = 0;
        }
    }

    cache->block = (Block**) malloc(num_set * sizeof(Block*));

    for (uint16_t set = 0; set < num_set; ++set) {
        cache->block[set] = (Block*) malloc(num_way * sizeof(Block));

        for (uint16_t way = 0; way < num_way; ++way) {
            cache->block[set][way].dirty = 0;
            cache->block[set][way].valid = 0;
            cache->block[set][way].tag = 0;
            cache->block[set][way].lru = num_way - 1;

            for (uint8_t offset = 0; offset < BLOCK_SIZE; ++offset) {
                cache->block[set][way].data[offset] = 0;
            }
        }
    }
}

void cache_destroy(Cache *cache) {
    for (uint16_t set = 0; set < cache->NUM_SET; ++set) {
        free(cache->block[set]);
    }
    free(cache->block);
    free(cache->mshr);
}

uint16_t cache_get_way(Cache *cache, uint16_t set, uint32_t tag) {
    uint16_t way = 0;
    for (way = 0; way < cache->NUM_WAY; ++way) {
        if (cache->block[set][way].valid && (cache->block[set][way].tag == tag)) {
            return way;
        }
    }

    return way;  // return NUM_WAY
}

void cache_insert_data(Cache *cache, uint16_t set, uint16_t way, uint32_t tag, \
                       uint32_t data[BLOCK_SIZE]) {
    cache->block[set][way].valid = 1;
    cache->block[set][way].dirty = 0;
    cache->block[set][way].tag = tag;
    cache_update_lru_state(cache, set, way);
    for (uint8_t offset = 0; offset < BLOCK_SIZE; ++offset) {
        cache->block[set][way].data[offset] = data[offset];
    }
}

uint16_t cache_find_victim(Cache *cache, uint16_t set) {
    uint16_t way = 0;

    // look for an invalid block
    for (way = 0; way < cache->NUM_WAY; ++way) {
        if (cache->block[set][way].valid == 0) {
            return way;
        }
    }

    // LRU victim
    uint16_t lru_val = cache->NUM_WAY - 1;
    for (way = 0; way < cache->NUM_WAY; ++way) {
        if (cache->block[set][way].lru == lru_val) {
            return way;
        }
    }

    // something went wrong!
    return cache->NUM_WAY;
}

void cache_update_lru_state(Cache *cache, uint16_t set, uint16_t way) {
    for (uint16_t i = 0; i < cache->NUM_WAY; ++i) {
        if (cache->block[set][i].lru < cache->block[set][way].lru) {
            cache->block[set][i].lru++;
        }
    }

    cache->block[set][way].lru = 0;
}

uint16_t get_free_mshr_index(Cache *cache) {
    uint16_t mshr_index;

    for (mshr_index = 0; mshr_index < cache->NUM_MSHR; ++mshr_index) {
        if (!cache->mshr[mshr_index].valid) {
            break;
        }
    }

    return mshr_index;
}

uint16_t allocate_mshr(Cache *cache, uint32_t address) {
    uint16_t mshr_index = get_free_mshr_index(cache);

    /* if empty MSHR found, fill it */
    if (mshr_index != cache->NUM_MSHR) {
        cache->mshr[mshr_index].valid = 1;
        cache->mshr[mshr_index].done = 0;
        cache->mshr[mshr_index].address = address;
    }

    return mshr_index;
}

void mark_mshr_done(Cache *cache, uint32_t address) {
    for (uint16_t mshr_index = 0; mshr_index < cache->NUM_MSHR; ++mshr_index) {
        if (cache->mshr[mshr_index].address == address) {
            cache->mshr[mshr_index].done = 1;
        }
    }
}

void invalidate_mshr(Cache *cache, uint32_t address) {
    for (uint16_t mshr_index = 0; mshr_index < cache->NUM_MSHR; ++mshr_index) {
        if (cache->mshr[mshr_index].address == address) {
            cache->mshr[mshr_index].valid = 0;
        }
    }
}
