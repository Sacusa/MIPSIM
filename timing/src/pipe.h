/*
 * ECE 18-447, Spring 2013
 *
 * MIPS pipeline timing simulator
 *
 * Chris Fallin, 2012
 */

#ifndef _PIPE_H_
#define _PIPE_H_

#include "block.h"
#include "btb_entry.h"
#include "cache.h"
#include "dram.h"
#include "gshare.h"
#include "instruction_load_status.h"
#include "shell.h"

#define BTB_SIZE 1024

/* Pipeline ops (instances of this structure) are high-level representations of
 * the instructions that actually flow through the pipeline. This struct does
 * not correspond 1-to-1 with the control signals that would actually pass
 * through the pieline. Rather, it carries the orginal instruction, operand
 * information and values as they are collected, and destination information.
 */
typedef struct Pipe_Op
{
    /* PC of this instruction */
    uint32_t pc;
    /* raw instruction */
    uint32_t instruction;
    /* decoded opcode and subopcode fields */
    int opcode, subop;

    /* immediate value, if any, for ALU immediates */
    uint32_t imm16, se_imm16;
    /* shift amount */
    int shamt;

    /* register source values */
    int reg_src1, reg_src2;                  /* 0 -- 31 if this inst has register source(s), or
                                                -1 otherwise */
    uint32_t reg_src1_value, reg_src2_value; /* values of operands from source
                                                regs */

    /* memory access information */
    int is_mem;         /* is this a load/store? */
    uint32_t mem_addr;  /* address if applicable */
    int mem_write;      /* is this a write to memory? */
    uint32_t mem_value; /* value loaded from memory or to be written to memory */

    /* register destination information */
    int reg_dst;             /* 0 -- 31 if this inst has a destination register, -1
                                otherwise */
    uint32_t reg_dst_value;  /* value to write into dest reg. */
    int reg_dst_value_ready; /* destination value produced yet? */

    /* branch information */
    int is_branch;                  /* is this a branch? */
    uint32_t branch_dest;           /* branch destination (if taken) */
    int branch_cond;                /* is this a conditional branch? */
    int branch_taken;               /* branch taken? (set as soon as resolved: in decode
                                       for unconditional, execute for conditional) */
    int is_link;                    /* jump-and-link or branch-and-link inst? */
    int link_reg;                   /* register to place link into? */
    int predicted_is_branch;        /* predictor: is this a branch? */
    uint32_t predicted_branch_dest; /* predictor: branch destination (if taken) */
    int predicted_branch_taken;     /* predictor: branch taken? */

} Pipe_Op;

/* The pipe state represents the current state of the pipeline. It holds a
 * pointer to the op that is currently at the input of each stage. As stages
 * execute, they remove the op from their input (set the pointer to NULL) and
 * place an op at their output. If the pointer that represents a stage's output
 * is not null when that stage executes, then this represents a pipeline stall,
 * and the stage must not overwrite its output (otherwise an instruction would
 * be lost).
 */

typedef struct Pipe_State
{
    /* pipe op currently at the input of the given stage (NULL for none) */
    Pipe_Op *decode_op, *execute_op, *mem_op, *wb_op;

    /* register file state */
    uint32_t REGS[32];
    uint32_t HI, LO;

    /* program counter in fetch stage */
    uint32_t PC;

    /* information for PC update (branch recovery). Branches should use this
     * mechanism to redirect the fetch stage, and flush the ops that came after
     * the branch as necessary. */
    int branch_recover;   /* set to '1' to load a new PC */
    uint32_t branch_dest; /* next fetch will be from this PC */
    int branch_flush;     /* how many stages to flush during recover?
                             (1 = fetch, 2 = fetch/decode, ...) */

    /* multiplier stall info */
    int multiplier_stall; /* number of remaining cycles until HI/LO are ready */

    /* place other information here as necessary */

    /* caches */
    Cache l1i_cache, l1d_cache, l2_cache;

    /* cache state info */
    uint8_t dcache_state; /* current state of dcache (defined in cache.h) */
    uint8_t mem_stall;    /* number of cycles memory is stalled on D-Cache miss */
    uint8_t is_mem_stalled;

    /* branch predictor info */
    Gshare gshare_predictor;
    BTB_Entry BTB[BTB_SIZE];

    /* load/store queues */
    uint8_t instruction_queue_size;
    Instruction_Load_Status *instruction_queue;

} Pipe_State;

/* global variable -- pipeline state */
extern Pipe_State pipe;

/* called during simulator startup */
void pipe_init();

/* this function calls the others */
void pipe_cycle();

/* helper: pipe stages can call this to schedule a branch recovery */
/* flushes 'flush' stages (1 = execute only, 2 = fetch/decode, ...) and then
 * sets the fetch PC to the given destination. */
void pipe_recover(int flush, uint32_t dest);

/* each of these functions implements one stage of the pipeline */
void pipe_stage_fetch();
void pipe_stage_decode();
void pipe_stage_execute();
void pipe_stage_mem();
void pipe_stage_wb();

/* called when RUN_BIT is set to 0; frees all structures */
void pipe_stop();

/* accesses the memory (L1I -> L2C -> DRAM) and returns the next instruction. */
uint8_t i_cache_load(uint32_t *data);

/* accesses dcache and returns the requested data.
 * On a miss, sets is_mem_stalled and returns 0. */
uint32_t d_cache_load(uint32_t mem_addr);

/* write the given data into corresponding cache block */
void d_cache_store(uint32_t mem_addr, uint32_t data);

/* performs writeback for the given block if it is dirty */
void writeback_if_dirty(uint16_t set, uint16_t way);

/* initializes all branch prediction info */
void init_branch_pred();

/* returns BTB index for the given PC */
uint32_t get_btb_index(uint32_t PC);

/* performs branch prediction, updates prediction info in op, and returns the next PC value */
uint32_t predict_next_PC(Pipe_Op *op);

#endif
