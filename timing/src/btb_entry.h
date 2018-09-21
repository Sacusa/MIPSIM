#ifndef _BTB_ENTRY_H_
#define _BTB_ENTRY_H_

#include <stdint.h>

typedef struct BTB_Entry {
    uint32_t address;
    uint32_t branch_target;
    uint8_t valid;
    uint8_t is_unconditional;
} BTB_Entry;

#endif
