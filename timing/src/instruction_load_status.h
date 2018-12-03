#ifndef _INSTRUCTION_LOAD_STATUS_
#define _INSTRUCTION_LOAD_STATUS_

#include <stdint.h>
#include <stdlib.h>

typedef struct Instruction_Load_Status
{
    uint8_t valid;         /* is this entry valid? */
    uint32_t address;      /* address requested */
    uint8_t current_state; /* current state in the load process */
    uint8_t stall;         /* number of cycles currently stalled for */
} Instruction_Load_Status;

#endif
