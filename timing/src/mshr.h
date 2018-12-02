#ifndef _MSHR_H_
#define _MSHR_H_

#include <stdint.h>
#include <stdlib.h>

typedef struct MSHR {
    uint8_t valid;
    uint8_t done;
    uint32_t address;  // address of the cache block that triggered the miss
} MSHR;

#endif