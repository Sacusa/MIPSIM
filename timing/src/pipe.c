/*
 * ECE 18-447, Spring 2013
 *
 * MIPS pipeline timing simulator
 *
 * Chris Fallin, 2012
 */

#include "pipe.h"
#include "shell.h"
#include "mips.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

//#define DEBUG

/* debug */
void print_op(Pipe_Op *op)
{
    if (op)
        printf("OP (PC=%08x inst=%08x) src1=R%d (%08x) src2=R%d (%08x) dst=R%d valid %d (%08x) br=%d taken=%d dest=%08x mem=%d addr=%08x\n",
                op->pc, op->instruction, op->reg_src1, op->reg_src1_value, op->reg_src2, op->reg_src2_value, op->reg_dst, op->reg_dst_value_ready,
                op->reg_dst_value, op->is_branch, op->branch_taken, op->branch_dest, op->is_mem, op->mem_addr);
    else
        printf("(null)\n");
}

/* global pipeline state */
Pipe_State pipe;

void pipe_init()
{
    memset(&pipe, 0, sizeof(Pipe_State));
    pipe.PC = 0x00400000;

    // initialize caches
    cache_init(&pipe.l1i_cache, L1I_NUM_SETS, L1I_NUM_WAYS);
    cache_init(&pipe.l1d_cache, L1D_NUM_SETS, L1D_NUM_WAYS);

    // initialize fetch stall info
    pipe.fetch_stall = 0;
    pipe.is_fetch_stalled = 0;

    // initialize mem stall info
    pipe.mem_stall = 0;
    pipe.is_mem_stalled = 0;

    init_branch_pred();
}

void pipe_cycle()
{
#ifdef DEBUG
    printf("\n\n----\n\nPIPELINE:\n");
    printf("DCODE: "); print_op(pipe.decode_op);
    printf("EXEC : "); print_op(pipe.execute_op);
    printf("MEM  : "); print_op(pipe.mem_op);
    printf("WB   : "); print_op(pipe.wb_op);
    printf("\n");
#endif

    pipe_stage_wb();
    pipe_stage_mem();
    pipe_stage_execute();
    pipe_stage_decode();
    pipe_stage_fetch();

    /* handle branch recoveries */
    if (pipe.branch_recover) {
#ifdef DEBUG
        printf("branch recovery: new dest %08x flush %d stages\n", pipe.branch_dest, pipe.branch_flush);
#endif

        // "unstall" the fetch stage on misprediction
        if (pipe.PC != pipe.branch_dest) {
            pipe.fetch_stall = 0;
            pipe.is_fetch_stalled = 0;
        }

        pipe.PC = pipe.branch_dest;

        if (pipe.branch_flush >= 2) {
            if (pipe.decode_op) free(pipe.decode_op);
            pipe.decode_op = NULL;
        }

        if (pipe.branch_flush >= 3) {
            if (pipe.execute_op) free(pipe.execute_op);
            pipe.execute_op = NULL;
        }

        if (pipe.branch_flush >= 4) {
            if (pipe.mem_op) free(pipe.mem_op);
            pipe.mem_op = NULL;
        }

        if (pipe.branch_flush >= 5) {
            if (pipe.wb_op) free(pipe.wb_op);
            pipe.wb_op = NULL;
        }

        pipe.branch_recover = 0;
        pipe.branch_dest = 0;
        pipe.branch_flush = 0;

        stat_squash++;
    }
}

void pipe_recover(int flush, uint32_t dest)
{
    /* if there is already a recovery scheduled, it must have come from a later
     * stage (which executes older instructions), hence that recovery overrides
     * our recovery. Simply return in this case. */
    if (pipe.branch_recover) return;

    /* schedule the recovery. This will be done once all pipeline stages simulate the current cycle. */
    pipe.branch_recover = 1;
    pipe.branch_flush = flush;
    pipe.branch_dest = dest;
}

void pipe_stage_wb()
{
    /* if there is no instruction in this pipeline stage, we are done */
    if (!pipe.wb_op)
        return;

    /* grab the op out of our input slot */
    Pipe_Op *op = pipe.wb_op;
    pipe.wb_op = NULL;

    /* if this instruction writes a register, do so now */
    if (op->reg_dst != -1 && op->reg_dst != 0) {
        pipe.REGS[op->reg_dst] = op->reg_dst_value;
#ifdef DEBUG
        printf("R%d = %08x\n", op->reg_dst, op->reg_dst_value);
#endif
    }

    /* if this was a syscall, perform action */
    if (op->opcode == OP_SPECIAL && op->subop == SUBOP_SYSCALL) {
        if (op->reg_src1_value == 0xA) {
            pipe.PC = op->pc; /* fetch will do pc += 4, then we stop with correct PC */
            RUN_BIT = 0;
            pipe_stop();  /* close down pipe and free all structures */
        }
    }

    /* free the op */
    free(op);

    stat_inst_retire++;
}

void pipe_stage_mem()
{
    /* if a dcache miss is in progress, decrement cycles and return */
    if (pipe.mem_stall > 0) {
        pipe.mem_stall--;
        return;
    }

    /* if there is no instruction in this pipeline stage, we are done */
    if (!pipe.mem_op)
        return;

    /* grab the op out of our input slot */
    Pipe_Op *op = pipe.mem_op;

    uint32_t val = 0;

    /* access dcache */
    if (op->is_mem) {
        val = d_cache_load(op->mem_addr);
        if (pipe.is_mem_stalled) {
            return;
        }
    }

    switch (op->opcode) {
        case OP_LW:
        case OP_LH:
        case OP_LHU:
        case OP_LB:
        case OP_LBU:
            {
                /* extract needed value */
                op->reg_dst_value_ready = 1;
                if (op->opcode == OP_LW) {
                    op->reg_dst_value = val;
                }
                else if (op->opcode == OP_LH || op->opcode == OP_LHU) {
                    if (op->mem_addr & 2)
                        val = (val >> 16) & 0xFFFF;
                    else
                        val = val & 0xFFFF;

                    if (op->opcode == OP_LH)
                        val |= (val & 0x8000) ? 0xFFFF8000 : 0;

                    op->reg_dst_value = val;
                }
                else if (op->opcode == OP_LB || op->opcode == OP_LBU) {
                    switch (op->mem_addr & 3) {
                        case 0:
                            val = val & 0xFF;
                            break;
                        case 1:
                            val = (val >> 8) & 0xFF;
                            break;
                        case 2:
                            val = (val >> 16) & 0xFF;
                            break;
                        case 3:
                            val = (val >> 24) & 0xFF;
                            break;
                    }

                    if (op->opcode == OP_LB)
                        val |= (val & 0x80) ? 0xFFFFFF80 : 0;

                    op->reg_dst_value = val;
                }
            }
            break;

        case OP_SB:
            switch (op->mem_addr & 3) {
                case 0: val = (val & 0xFFFFFF00) | ((op->mem_value & 0xFF) << 0); break;
                case 1: val = (val & 0xFFFF00FF) | ((op->mem_value & 0xFF) << 8); break;
                case 2: val = (val & 0xFF00FFFF) | ((op->mem_value & 0xFF) << 16); break;
                case 3: val = (val & 0x00FFFFFF) | ((op->mem_value & 0xFF) << 24); break;
            }

            d_cache_store(op->mem_addr & ~3, val);
            break;

        case OP_SH:
#ifdef DEBUG
            printf("SH: addr %08x val %04x old word %08x\n", op->mem_addr, op->mem_value & 0xFFFF, val);
#endif
            if (op->mem_addr & 2)
                val = (val & 0x0000FFFF) | (op->mem_value) << 16;
            else
                val = (val & 0xFFFF0000) | (op->mem_value & 0xFFFF);
#ifdef DEBUG
            printf("new word %08x\n", val);
#endif

            d_cache_store(op->mem_addr & ~3, val);
            break;

        case OP_SW:
            val = op->mem_value;
            d_cache_store(op->mem_addr & ~3, val);
            break;
    }

    /* clear stage input and transfer to next stage */
    pipe.mem_op = NULL;
    pipe.wb_op = op;
}

void pipe_stage_execute()
{
    /* if a multiply/divide is in progress, decrement cycles until value is ready */
    if (pipe.multiplier_stall > 0)
        pipe.multiplier_stall--;

    /* if downstream stall, return (and leave any input we had) */
    if (pipe.mem_op != NULL)
        return;

    /* if no op to execute, return */
    if (pipe.execute_op == NULL)
        return;

    /* grab op and read sources */
    Pipe_Op *op = pipe.execute_op;

    /* read register values, and check for bypass; stall if necessary */
    int stall = 0;
    if (op->reg_src1 != -1) {
        if (op->reg_src1 == 0)
            op->reg_src1_value = 0;
        else if (pipe.mem_op && pipe.mem_op->reg_dst == op->reg_src1) {
            if (!pipe.mem_op->reg_dst_value_ready)
                stall = 1;
            else
                op->reg_src1_value = pipe.mem_op->reg_dst_value;
        }
        else if (pipe.wb_op && pipe.wb_op->reg_dst == op->reg_src1) {
            op->reg_src1_value = pipe.wb_op->reg_dst_value;
        }
        else
            op->reg_src1_value = pipe.REGS[op->reg_src1];
    }
    if (op->reg_src2 != -1) {
        if (op->reg_src2 == 0)
            op->reg_src2_value = 0;
        else if (pipe.mem_op && pipe.mem_op->reg_dst == op->reg_src2) {
            if (!pipe.mem_op->reg_dst_value_ready)
                stall = 1;
            else
                op->reg_src2_value = pipe.mem_op->reg_dst_value;
        }
        else if (pipe.wb_op && pipe.wb_op->reg_dst == op->reg_src2) {
            op->reg_src2_value = pipe.wb_op->reg_dst_value;
        }
        else
            op->reg_src2_value = pipe.REGS[op->reg_src2];
    }

    /* if bypassing requires a stall (e.g. use immediately after load),
     * return without clearing stage input */
    if (stall) 
        return;

    /* execute the op */
    switch (op->opcode) {
        case OP_SPECIAL:
            op->reg_dst_value_ready = 1;
            switch (op->subop) {
                case SUBOP_SLL:
                    op->reg_dst_value = op->reg_src2_value << op->shamt;
                    break;
                case SUBOP_SLLV:
                    op->reg_dst_value = op->reg_src2_value << op->reg_src1_value;
                    break;
                case SUBOP_SRL:
                    op->reg_dst_value = op->reg_src2_value >> op->shamt;
                    break;
                case SUBOP_SRLV:
                    op->reg_dst_value = op->reg_src2_value >> op->reg_src1_value;
                    break;
                case SUBOP_SRA:
                    op->reg_dst_value = (int32_t)op->reg_src2_value >> op->shamt;
                    break;
                case SUBOP_SRAV:
                    op->reg_dst_value = (int32_t)op->reg_src2_value >> op->reg_src1_value;
                    break;
                case SUBOP_JR:
                case SUBOP_JALR:
                    op->reg_dst_value = op->pc + 4;
                    op->branch_dest = op->reg_src1_value;
                    op->branch_taken = 1;
                    break;

                case SUBOP_MULT:
                    {
                        /* we set a result value right away; however, we will
                         * model a stall if the program tries to read the value
                         * before it's ready (or overwrite HI/LO). Also, if
                         * another multiply comes down the pipe later, it will
                         * update the values and re-set the stall cycle count
                         * for a new operation.
                         */
                        int64_t val = (int64_t)((int32_t)op->reg_src1_value) * (int64_t)((int32_t)op->reg_src2_value);
                        uint64_t uval = (uint64_t)val;
                        pipe.HI = (uval >> 32) & 0xFFFFFFFF;
                        pipe.LO = (uval >>  0) & 0xFFFFFFFF;

                        /* four-cycle multiplier latency */
                        pipe.multiplier_stall = 4;
                    }
                    break;
                case SUBOP_MULTU:
                    {
                        uint64_t val = (uint64_t)op->reg_src1_value * (uint64_t)op->reg_src2_value;
                        pipe.HI = (val >> 32) & 0xFFFFFFFF;
                        pipe.LO = (val >>  0) & 0xFFFFFFFF;

                        /* four-cycle multiplier latency */
                        pipe.multiplier_stall = 4;
                    }
                    break;

                case SUBOP_DIV:
                    if (op->reg_src2_value != 0) {

                        int32_t val1 = (int32_t)op->reg_src1_value;
                        int32_t val2 = (int32_t)op->reg_src2_value;
                        int32_t div, mod;

                        div = val1 / val2;
                        mod = val1 % val2;

                        pipe.LO = div;
                        pipe.HI = mod;
                    } else {
                        // really this would be a div-by-0 exception
                        pipe.HI = pipe.LO = 0;
                    }

                    /* 32-cycle divider latency */
                    pipe.multiplier_stall = 32;
                    break;

                case SUBOP_DIVU:
                    if (op->reg_src2_value != 0) {
                        pipe.HI = (uint32_t)op->reg_src1_value % (uint32_t)op->reg_src2_value;
                        pipe.LO = (uint32_t)op->reg_src1_value / (uint32_t)op->reg_src2_value;
                    } else {
                        /* really this would be a div-by-0 exception */
                        pipe.HI = pipe.LO = 0;
                    }

                    /* 32-cycle divider latency */
                    pipe.multiplier_stall = 32;
                    break;

                case SUBOP_MFHI:
                    /* stall until value is ready */
                    if (pipe.multiplier_stall > 0)
                        return;

                    op->reg_dst_value = pipe.HI;
                    break;
                case SUBOP_MTHI:
                    /* stall to respect WAW dependence */
                    if (pipe.multiplier_stall > 0)
                        return;

                    pipe.HI = op->reg_src1_value;
                    break;

                case SUBOP_MFLO:
                    /* stall until value is ready */
                    if (pipe.multiplier_stall > 0)
                        return;

                    op->reg_dst_value = pipe.LO;
                    break;
                case SUBOP_MTLO:
                    /* stall to respect WAW dependence */
                    if (pipe.multiplier_stall > 0)
                        return;

                    pipe.LO = op->reg_src1_value;
                    break;

                case SUBOP_ADD:
                case SUBOP_ADDU:
                    op->reg_dst_value = op->reg_src1_value + op->reg_src2_value;
                    break;
                case SUBOP_SUB:
                case SUBOP_SUBU:
                    op->reg_dst_value = op->reg_src1_value - op->reg_src2_value;
                    break;
                case SUBOP_AND:
                    op->reg_dst_value = op->reg_src1_value & op->reg_src2_value;
                    break;
                case SUBOP_OR:
                    op->reg_dst_value = op->reg_src1_value | op->reg_src2_value;
                    break;
                case SUBOP_NOR:
                    op->reg_dst_value = ~(op->reg_src1_value | op->reg_src2_value);
                    break;
                case SUBOP_XOR:
                    op->reg_dst_value = op->reg_src1_value ^ op->reg_src2_value;
                    break;
                case SUBOP_SLT:
                    op->reg_dst_value = ((int32_t)op->reg_src1_value <
                            (int32_t)op->reg_src2_value) ? 1 : 0;
                    break;
                case SUBOP_SLTU:
                    op->reg_dst_value = (op->reg_src1_value < op->reg_src2_value) ? 1 : 0;
                    break;
            }
            break;

        case OP_BRSPEC:
            switch (op->subop) {
                case BROP_BLTZ:
                case BROP_BLTZAL:
                    if ((int32_t)op->reg_src1_value < 0) op->branch_taken = 1;
                    break;

                case BROP_BGEZ:
                case BROP_BGEZAL:
                    if ((int32_t)op->reg_src1_value >= 0) op->branch_taken = 1;
                    break;
            }
            break;

        case OP_BEQ:
            if (op->reg_src1_value == op->reg_src2_value) op->branch_taken = 1;
            break;

        case OP_BNE:
            if (op->reg_src1_value != op->reg_src2_value) op->branch_taken = 1;
            break;

        case OP_BLEZ:
            if ((int32_t)op->reg_src1_value <= 0) op->branch_taken = 1;
            break;

        case OP_BGTZ:
            if ((int32_t)op->reg_src1_value > 0) op->branch_taken = 1;
            break;

        case OP_ADDI:
        case OP_ADDIU:
            op->reg_dst_value_ready = 1;
            op->reg_dst_value = op->reg_src1_value + op->se_imm16;
            break;
        case OP_SLTI:
            op->reg_dst_value_ready = 1;
            op->reg_dst_value = (int32_t)op->reg_src1_value < (int32_t)op->se_imm16 ? 1 : 0;
            break;
        case OP_SLTIU:
            op->reg_dst_value_ready = 1;
            op->reg_dst_value = (uint32_t)op->reg_src1_value < (uint32_t)op->se_imm16 ? 1 : 0;
            break;
        case OP_ANDI:
            op->reg_dst_value_ready = 1;
            op->reg_dst_value = op->reg_src1_value & op->imm16;
            break;
        case OP_ORI:
            op->reg_dst_value_ready = 1;
            op->reg_dst_value = op->reg_src1_value | op->imm16;
            break;
        case OP_XORI:
            op->reg_dst_value_ready = 1;
            op->reg_dst_value = op->reg_src1_value ^ op->imm16;
            break;
        case OP_LUI:
            op->reg_dst_value_ready = 1;
            op->reg_dst_value = op->imm16 << 16;
            break;

        case OP_LW:
        case OP_LH:
        case OP_LHU:
        case OP_LB:
        case OP_LBU:
            op->mem_addr = op->reg_src1_value + op->se_imm16;
            break;

        case OP_SW:
        case OP_SH:
        case OP_SB:
            op->mem_addr = op->reg_src1_value + op->se_imm16;
            op->mem_value = op->reg_src2_value;
            break;
    }

    /* update branch predictor and perform branch recovery */
    if (op->is_branch) {
        /* flush pipeline
         * cond 1: mispredicted direction
         * cond 2: if taken, mispredicted target
         * cond 3: BTB miss
         */
        if ((op->branch_taken != op->predicted_branch_taken) || \
            (op->branch_taken && (op->branch_dest != op->predicted_branch_dest)) || \
            (!op->predicted_is_branch)) {
            if (op->branch_taken) {
                pipe_recover(3, op->branch_dest);
            }
            else {
                pipe_recover(3, op->pc + 4);
            }
        }

        // update GHR and PHT for conditional branches
        if (op->branch_cond) {
            update_gshare(&pipe.gshare_predictor, op->pc, op->branch_taken);
        }

        // update BTB
        uint32_t btb_index = get_btb_index(op->pc);
        pipe.BTB[btb_index].address = op->pc;
        pipe.BTB[btb_index].branch_target = op->branch_dest;
        pipe.BTB[btb_index].valid = 1;
        pipe.BTB[btb_index].is_unconditional = !(op->branch_cond);
    }

    /* remove from upstream stage and place in downstream stage */
    pipe.execute_op = NULL;
    pipe.mem_op = op;
}

void pipe_stage_decode()
{
    /* if downstream stall, return (and leave any input we had) */
    if (pipe.execute_op != NULL)
        return;

    /* if no op to decode, return */
    if (pipe.decode_op == NULL)
        return;

    /* grab op and remove from stage input */
    Pipe_Op *op = pipe.decode_op;
    pipe.decode_op = NULL;

    /* set up info fields (source/dest regs, immediate, jump dest) as necessary */
    uint32_t opcode = (op->instruction >> 26) & 0x3F;
    uint32_t rs = (op->instruction >> 21) & 0x1F;
    uint32_t rt = (op->instruction >> 16) & 0x1F;
    uint32_t rd = (op->instruction >> 11) & 0x1F;
    uint32_t shamt = (op->instruction >> 6) & 0x1F;
    uint32_t funct1 = (op->instruction >> 0) & 0x1F;
    uint32_t funct2 = (op->instruction >> 0) & 0x3F;
    uint32_t imm16 = (op->instruction >> 0) & 0xFFFF;
    uint32_t se_imm16 = imm16 | ((imm16 & 0x8000) ? 0xFFFF8000 : 0);
    uint32_t targ = (op->instruction & ((1UL << 26) - 1)) << 2;

    op->opcode = opcode;
    op->imm16 = imm16;
    op->se_imm16 = se_imm16;
    op->shamt = shamt;

    switch (opcode) {
        case OP_SPECIAL:
            /* all "SPECIAL" insts are R-types that use the ALU and both source
             * regs. Set up source regs and immediate value. */
            op->reg_src1 = rs;
            op->reg_src2 = rt;
            op->reg_dst = rd;
            op->subop = funct2;
            if (funct2 == SUBOP_SYSCALL) {
                op->reg_src1 = 2; // v0
                op->reg_src2 = 3; // v1
            }
            if (funct2 == SUBOP_JR || funct2 == SUBOP_JALR) {
                op->is_branch = 1;
                op->branch_cond = 0;
            }

            break;

        case OP_BRSPEC:
            /* branches that have -and-link variants come here */
            op->is_branch = 1;
            op->reg_src1 = rs;
            op->reg_src2 = rt;
            op->is_branch = 1;
            op->branch_cond = 1; /* conditional branch */
            op->branch_dest = op->pc + 4 + (se_imm16 << 2);
            op->subop = rt;
            if (rt == BROP_BLTZAL || rt == BROP_BGEZAL) {
                /* link reg */
                op->reg_dst = 31;
                op->reg_dst_value = op->pc + 4;
                op->reg_dst_value_ready = 1;
            }
            break;

        case OP_JAL:
            op->reg_dst = 31;
            op->reg_dst_value = op->pc + 4;
            op->reg_dst_value_ready = 1;
            op->branch_taken = 1;
            /* fallthrough */
        case OP_J:
            op->is_branch = 1;
            op->branch_cond = 0;
            op->branch_taken = 1;
            op->branch_dest = (op->pc & 0xF0000000) | targ;
            break;

        case OP_BEQ:
        case OP_BNE:
        case OP_BLEZ:
        case OP_BGTZ:
            /* ordinary conditional branches (resolved after execute) */
            op->is_branch = 1;
            op->branch_cond = 1;
            op->branch_dest = op->pc + 4 + (se_imm16 << 2);
            op->reg_src1 = rs;
            op->reg_src2 = rt;
            break;

        case OP_ADDI:
        case OP_ADDIU:
        case OP_SLTI:
        case OP_SLTIU:
            /* I-type ALU ops with sign-extended immediates */
            op->reg_src1 = rs;
            op->reg_dst = rt;
            break;

        case OP_ANDI:
        case OP_ORI:
        case OP_XORI:
        case OP_LUI:
            /* I-type ALU ops with non-sign-extended immediates */
            op->reg_src1 = rs;
            op->reg_dst = rt;
            break;

        case OP_LW:
        case OP_LH:
        case OP_LHU:
        case OP_LB:
        case OP_LBU:
        case OP_SW:
        case OP_SH:
        case OP_SB:
            /* memory ops */
            op->is_mem = 1;
            op->reg_src1 = rs;
            if (opcode == OP_LW || opcode == OP_LH || opcode == OP_LHU || opcode == OP_LB || opcode == OP_LBU) {
                /* load */
                op->mem_write = 0;
                op->reg_dst = rt;
            }
            else {
                /* store */
                op->mem_write = 1;
                op->reg_src2 = rt;
            }
            break;
    }

    /* we will handle reg-read together with bypass in the execute stage */

    /* place op in downstream slot */
    pipe.execute_op = op;
}

void pipe_stage_fetch()
{
    /* if execution halted, increment PC and return */
    if (!RUN_BIT) {
        pipe.PC += 4;
        return;
    }

    /* if an icache miss is in progress, decrement cycles and return */
    if (pipe.fetch_stall > 0) {
        pipe.fetch_stall--;
        return;
    }
    
    /* if pipeline is stalled (our output slot is not empty), return */
    if (pipe.decode_op != NULL) {
        return;
    }
    
    uint32_t next_instruction = i_cache_load();

    /* return on cache miss */
    if (pipe.is_fetch_stalled) {
        return;
    }

    /* Allocate an op and send it down the pipeline. */
    Pipe_Op *op = malloc(sizeof(Pipe_Op));
    memset(op, 0, sizeof(Pipe_Op));
    op->reg_src1 = op->reg_src2 = op->reg_dst = -1;
    op->instruction = next_instruction;
    op->pc = pipe.PC;
    pipe.decode_op = op;

    /* update PC */
    pipe.PC = predict_next_PC(op);

    stat_inst_fetch++;
}

void pipe_stop()
{
    cache_destroy(&pipe.l1i_cache);
    cache_destroy(&pipe.l1d_cache);
}

uint32_t i_cache_load()
{
    uint32_t l1i_cache_tag = pipe.PC >> (LOG2_WORD_SIZE + LOG2_BLOCK_SIZE + L1I_LOG2_NUM_SETS);
    uint16_t l1i_cache_set = (pipe.PC >> (LOG2_WORD_SIZE + LOG2_BLOCK_SIZE)) & (L1I_NUM_SETS - 1);
    uint8_t l1i_cache_offset = (pipe.PC >> LOG2_WORD_SIZE) & (BLOCK_SIZE - 1);
    uint16_t l1i_cache_way = cache_get_way(&pipe.l1i_cache, l1i_cache_set, l1i_cache_tag);

    /* serve L1I cache miss */
    if (pipe.is_fetch_stalled) {
        pipe.is_fetch_stalled = 0;

        /* access main memory */
        uint32_t l1i_cache_data[BLOCK_SIZE];
        uint32_t address_mask = 0xffffffff << (LOG2_WORD_SIZE + LOG2_BLOCK_SIZE);
        for (uint8_t index = 0; index < BLOCK_SIZE; ++index) {
            l1i_cache_data[index] = mem_read_32((pipe.PC & address_mask) + \
                                                (index << LOG2_WORD_SIZE));
        }
        
        l1i_cache_way = cache_find_victim(&pipe.l1i_cache, l1i_cache_set);
        cache_insert_data(&pipe.l1i_cache, l1i_cache_set, l1i_cache_way, l1i_cache_tag, \
                          l1i_cache_data);
    }

    /* stall on L1I cache miss */
    if (l1i_cache_way == pipe.l1i_cache.NUM_WAY) {
        pipe.fetch_stall = L1I_MISS_STALL_CYCLE_COUNT;
        pipe.is_fetch_stalled = 1;
        return 0;
    }
    /* update LRU state on hit */
    else {
        cache_update_lru_state(&pipe.l1i_cache, l1i_cache_set, l1i_cache_way);
    }

    return pipe.l1i_cache.block[l1i_cache_set][l1i_cache_way].data[l1i_cache_offset];
}

uint32_t d_cache_load(uint32_t mem_addr)
{
    /* L1D cache fields */
    uint32_t l1d_cache_tag = mem_addr >> (LOG2_WORD_SIZE + LOG2_BLOCK_SIZE + L1D_LOG2_NUM_SETS);
    uint16_t l1d_cache_set = (mem_addr >> (LOG2_WORD_SIZE + LOG2_BLOCK_SIZE)) & (L1D_NUM_SETS - 1);
    uint8_t l1d_cache_offset = (mem_addr >> LOG2_WORD_SIZE) & (BLOCK_SIZE - 1);
    uint16_t l1d_cache_way = cache_get_way(&pipe.l1d_cache, l1d_cache_set, l1d_cache_tag);

    /* serve L1D cache miss */
    if (pipe.is_mem_stalled) {
        pipe.is_mem_stalled = 0;
        l1d_cache_way = cache_find_victim(&pipe.l1d_cache, l1d_cache_set);

        /* perform writeback if block is dirty */
        writeback_if_dirty(l1d_cache_set, l1d_cache_way);

        /* access main memory */
        uint32_t l1d_cache_data[BLOCK_SIZE];
        uint32_t address_mask = 0xffffffff << (LOG2_WORD_SIZE + LOG2_BLOCK_SIZE);
        for (uint8_t index = 0; index < BLOCK_SIZE; ++index) {
            l1d_cache_data[index] = mem_read_32((mem_addr & address_mask) + \
                                                (index << LOG2_WORD_SIZE));
        }
        
        cache_insert_data(&pipe.l1d_cache, l1d_cache_set, l1d_cache_way, l1d_cache_tag, \
                          l1d_cache_data);
    }

    /* stall on L1D cache miss */
    if (l1d_cache_way == pipe.l1d_cache.NUM_WAY) {
        pipe.mem_stall = L1D_MISS_STALL_CYCLE_COUNT;
        pipe.is_mem_stalled = 1;
        return 0;
    }
    /* update LRU state on hit */
    else {
        cache_update_lru_state(&pipe.l1d_cache, l1d_cache_set, l1d_cache_way);
    }

    return pipe.l1d_cache.block[l1d_cache_set][l1d_cache_way].data[l1d_cache_offset];
}

void d_cache_store(uint32_t mem_addr, uint32_t data)
{
    /* L1D cache fields */
    uint32_t l1d_cache_tag = mem_addr >> (LOG2_WORD_SIZE + LOG2_BLOCK_SIZE + L1D_LOG2_NUM_SETS);
    uint16_t l1d_cache_set = (mem_addr >> (LOG2_WORD_SIZE + LOG2_BLOCK_SIZE)) & (L1D_NUM_SETS - 1);
    uint8_t l1d_cache_offset = (mem_addr >> LOG2_WORD_SIZE) & (BLOCK_SIZE - 1);
    uint16_t l1d_cache_way = cache_get_way(&pipe.l1d_cache, l1d_cache_set, l1d_cache_tag);

    pipe.l1d_cache.block[l1d_cache_set][l1d_cache_way].dirty = 1;
    pipe.l1d_cache.block[l1d_cache_set][l1d_cache_way].data[l1d_cache_offset] = data;
}

void writeback_if_dirty(uint16_t set, uint16_t way)
{
    if (pipe.l1d_cache.block[set][way].valid && pipe.l1d_cache.block[set][way].dirty) {
        pipe.l1d_cache.block[set][way].dirty = 0;
        uint32_t base_address = (pipe.l1d_cache.block[set][way].tag << \
                                 (LOG2_WORD_SIZE + LOG2_BLOCK_SIZE + L1D_LOG2_NUM_SETS)) + \
                                (set << (LOG2_WORD_SIZE + LOG2_BLOCK_SIZE));

        for (uint8_t offset = 0; offset < BLOCK_SIZE; ++offset) {
            mem_write_32(base_address + (offset << LOG2_WORD_SIZE), \
                         pipe.l1d_cache.block[set][way].data[offset]);
        }
    }
}

void init_branch_pred()
{
    init_gshare(&pipe.gshare_predictor);

    for (int i = 0; i < BTB_SIZE; ++i) {
        pipe.BTB[i].address = 0;
        pipe.BTB[i].branch_target = 0;
        pipe.BTB[i].valid = 0;
        pipe.BTB[i].is_unconditional = 0;
    }
}

uint32_t get_btb_index(uint32_t PC)
{
    return ((PC >> 2) & (BTB_SIZE - 1));
}

uint32_t predict_next_PC(Pipe_Op *op)
{
    uint32_t next_PC = pipe.PC + 4;  // default next predicted PC value
    op->predicted_branch_taken = 0;
    op->predicted_is_branch = 0;
    
    uint32_t btb_index = get_btb_index(pipe.PC);
    uint32_t pht_index = get_pht_index(&pipe.gshare_predictor, pipe.PC);

    if ((pipe.BTB[btb_index].address == pipe.PC) && pipe.BTB[btb_index].valid) {
        op->predicted_is_branch = 1;
        
        if (pipe.BTB[btb_index].is_unconditional || (pipe.gshare_predictor.PHT[pht_index] > 1)) {
            next_PC = pipe.BTB[btb_index].branch_target;
            op->predicted_branch_taken = 1;
        }
    }

    op->predicted_branch_dest = next_PC;

    return next_PC;
}
