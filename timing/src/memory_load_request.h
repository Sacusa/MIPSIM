#ifndef _MEMORY_LOAD_REQUEST_
#define _MEMORY_LOAD_REQUEST_

#include <stdint.h>
#include <stdlib.h>

typedef struct Memory_Load_Request
{
    uint8_t valid;         /* is this entry valid? */
    uint32_t address;      /* address requested */
    uint8_t current_state; /* current state in the load process */
    uint8_t stall;         /* number of cycles currently stalled for */
} Memory_Load_Request;

#endif
