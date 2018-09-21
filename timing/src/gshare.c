#include "gshare.h"

void init_gshare(Gshare *gshare)
{
    gshare->GHR = 0;
    
    for (int i = 0; i < PHT_SIZE; ++i) {
        gshare->PHT[i] = 0;
    }
}

uint32_t get_pht_index(Gshare *gshare, uint32_t PC)
{
    return ((gshare->GHR) ^ ((PC >> 2) & (PHT_SIZE - 1)));
}

void update_gshare(Gshare *gshare, uint32_t PC, uint8_t is_taken)
{
    uint32_t pht_index = get_pht_index(gshare, PC);

    if (is_taken) {
        gshare->PHT[pht_index] += (gshare->PHT[pht_index] == 3 ? 0 : 1);
    }
    else {
        gshare->PHT[pht_index] -= (gshare->PHT[pht_index] == 0 ? 0 : 1);
    }

    gshare->GHR = (gshare->GHR << 1) + is_taken;
}
