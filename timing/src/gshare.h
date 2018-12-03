#ifndef _GSHARE_H_
#define _GSHARE_H_

#include <stdint.h>

#define PHT_SIZE 256

typedef struct Gshare
{
    uint8_t GHR;           /* Global History Register */
    uint8_t PHT[PHT_SIZE]; /* Pattern History Table */
} Gshare;

/* initializes all predictor structures to 0 */
void init_gshare(Gshare *gshare);

/* returns PHT index for the given PC */
uint32_t get_pht_index(Gshare *gshare, uint32_t PC);

/* update predictor for the given PC */
void update_gshare(Gshare *gshare, uint32_t PC, uint8_t is_taken);

#endif
