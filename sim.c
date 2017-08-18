/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   This is the actual MIPS core                              */
/*                                                             */
/*   Author: Sudhanshu Gupta                                   */
/*                                                             */
/***************************************************************/

#include <stdio.h>
#include "shell.h"

// FUNCTION DECLARATIONS
void process_opcode(uint32_t instr);
void process_special(uint32_t instr);
void process_regimm(uint32_t instr);

void process_instruction()
{
        // load current instruction
        uint32_t curr_instr = mem_read_32(CURRENT_STATE.PC);

        // extract opcode from the current instruction
        uint32_t opcode = curr_instr >> 26;

        // preset values for NEXT_STATE
        NEXT_STATE = CURRENT_STATE;
        NEXT_STATE.PC += 4;

        switch(opcode) {
        // opcode = 0 => SPECIAL function
        case 0: process_special(curr_instr);
                break;

        // opcode = 1 => REGIMM rt
        case 1: process_regimm(curr_instr);
                break;

        // else, a regular instruction
        default: process_opcode(curr_instr);
                 break;
        }
}

void process_opcode(uint32_t instr)
{
        // extract the higher order 3 bits of the opcode
        uint32_t opcode_hi = instr >> 29;

        // extract the lower order 3 bits of the opcode
        uint32_t opcode_lo = (instr << 3) >> 29;

	// break the instruction
	uint32_t rs = (instr << 6) >> 27;
	uint32_t rt = (instr << 11) >> 27;
	int32_t rs_value = CURRENT_STATE.REGS[rs];

        // drill down to the instruction
        if (opcode_hi == 0) {
		int32_t target = instr << 16;
		target = target >> 14;

                switch (opcode_lo) {

                // J - Jump
                case 2: NEXT_STATE.PC = (CURRENT_STATE.PC & 0xf0000000)
                                      + ((instr << 2) & 0x0ffffffc);
                        break;

                // JAL - Jump And Link
                case 3: NEXT_STATE.REGS[31] = CURRENT_STATE.PC + 4;
                        NEXT_STATE.PC = (CURRENT_STATE.PC & 0xf0000000)
                                      + ((instr << 2) & 0x0ffffffc);
                        break;

                // BEQ - Branch On Equal
                case 4: if (CURRENT_STATE.REGS[rs] == CURRENT_STATE.REGS[rt]) {
                                NEXT_STATE.PC = CURRENT_STATE.PC + target;
                        }
                        break;

                // BNE - Branch On Not Equal
                case 5: if (CURRENT_STATE.REGS[rs] != CURRENT_STATE.REGS[rt]) {
                                NEXT_STATE.PC = CURRENT_STATE.PC + target;
                        }
                        break;

                // BLEZ - Branch On Less Than Or Equal To Zero
                case 6: if ((rs_value == 0) || ((rs_value >> 31) == -1)) {
                                NEXT_STATE.PC = CURRENT_STATE.PC + target;
                        }
                        break;

                // BGTZ - Branch On Greater Than Zero
                case 7: if ((rs_value != 0) && ((rs_value >> 31) == 0)) {
                                NEXT_STATE.PC = CURRENT_STATE.PC + target;
                        }
                        break;
                }
        }
        else if (opcode_hi == 1) {
		int32_t immediate = instr << 16;
		immediate = immediate >> 16;

                switch (opcode_lo) {

		// ADDI - Add Immediate
		// ADDIU - Add Immediate Unsigned
		case 0:
		case 1:	NEXT_STATE.REGS[rt] = NEXT_STATE.REGS[rs] + immediate;
			break;

		// SLTI - Set On Less Than Immediate
		case 2: if (rs_value < immediate) {
				NEXT_STATE.REGS[rt] = 1;
			}
			else {
				NEXT_STATE.REGS[rt] = 0;
			}
			break;

		// SLTIU - Set On Less Than Immediate Unsigned
		case 3: if ((uint32_t)rs_value < (uint32_t)immediate) {
				NEXT_STATE.REGS[rt] = 1;
			}
			else {
				NEXT_STATE.REGS[rt] = 0;
			}
			break;

		// ANDI - And Immediate
		case 4: NEXT_STATE.REGS[rt] = ((instr << 16) >> 16) & rs_value;
			break;

		// ORI - Or Immediate
		case 5: NEXT_STATE.REGS[rt] = immediate | rs_value;
			break;

		// XORI - Xor Immediate
		case 6: NEXT_STATE.REGS[rt] = ((instr << 16) >> 16) ^ rs_value;
			break;

		// LUI - Load Upper Immediate
		case 7: NEXT_STATE.REGS[rt] = immediate << 16;
                        break;
		}
        }
        else if (opcode_hi == 4) {
                int32_t mem_addr = instr << 16;
                mem_addr = mem_addr >> 16;
                mem_addr += rs_value;
                int32_t mem_value = mem_read_32(mem_addr);

                switch (opcode_lo) {

                // LB - Load Byte
                case 0: mem_value = mem_value << 24;
                        mem_value = mem_value >> 24;
                        NEXT_STATE.REGS[rt] = mem_value;
                        break;

                // LH - Load Halfword
                case 1: mem_value = mem_value << 16;
                        mem_value = mem_value >> 16;
                        NEXT_STATE.REGS[rt] = mem_value;
                        break;

                // LW - Load Word
                case 3: NEXT_STATE.REGS[rt] = mem_value;
                        break;

                // LBU - Load Byte Unsigned
                case 4: mem_value = mem_value << 24;
                        mem_value = (uint32_t)mem_value >> 24;
                        NEXT_STATE.REGS[rt] = mem_value;
                        break;

                // LHU - Load Halfword Unsigned
                case 5: mem_value = mem_value << 16;
                        mem_value = (uint32_t)mem_value >> 16;
                        NEXT_STATE.REGS[rt] = mem_value;
                        break;
                }
        }
        else if (opcode_hi == 5) {
                int32_t mem_addr = instr << 16;
                mem_addr = mem_addr >> 16;
                mem_addr += rs_value;
                uint32_t rt_value = CURRENT_STATE.REGS[rt];
                uint32_t mem_value = mem_read_32(mem_addr);

                switch (opcode_lo) {

                // SB - Store Byte
                case 0: mem_value = (mem_value >> 8) << 8;
                        mem_value += (rt_value << 24) >> 24;
                        mem_write_32(mem_addr, mem_value);
                        break;

                // SH - Store Halfword
                case 1: mem_value = (mem_value >> 16) << 16;
                        mem_value += (rt_value << 16) >> 16;
                        mem_write_32(mem_addr, mem_value);
                        break;

                // SW - Store Word
                case 3: mem_write_32(mem_addr, rt_value);
                        break;
                }
        }
}

void process_special(uint32_t instr)
{
        // extract the higher order 3 bits of the opcode
        uint32_t opcode_hi = (instr << 26) >> 29;

        // extract the lower order 3 bits of the opcode
        uint32_t opcode_lo = (instr << 29) >> 29;

	// break the instruction
	uint32_t rs = (instr << 6) >> 27;
	uint32_t rt = (instr << 11) >> 27;
        uint32_t rd = (instr << 16) >> 27;
        uint32_t shamt = (instr << 21) >> 27;
        uint32_t rs_value = CURRENT_STATE.REGS[rs];

        // drill down to the instruction
        if (opcode_hi == 0) {
                switch (opcode_lo) {

                // SLL - Shift Left Logical
                case 0: NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] << shamt;
                        break;

                // SRL - Shift Right Logical
                case 2: NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >> shamt;
                        break;

                // SRA - Shift Right Arithmetic
                case 3: NEXT_STATE.REGS[rd] = (int32_t)CURRENT_STATE.REGS[rt] >> shamt;
                        break;

                // SLLV - Shift Left Logical Variable
                case 4: NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] << ((rs_value << 27) >> 27);
                        break;

                // SRLV - Shift Right Logical Variable
                case 6: NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >> ((rs_value << 27) >> 27);
                        break;

                // SRAV - Shift Right Arithmetic Variable
                case 7: NEXT_STATE.REGS[rd] = (int32_t)CURRENT_STATE.REGS[rt] >> \
                                              ((rs_value << 27) >> 27);
                        break;
                }
        }
        else if (opcode_hi == 1) {
                switch (opcode_lo) {

                // JR - Jump Register
                case 0: NEXT_STATE.PC = rs_value;
                        break;

                // JALR - Jump And Link Register
                case 1: NEXT_STATE.PC = rs_value;
                        NEXT_STATE.REGS[rd] = CURRENT_STATE.PC + 4;
                        break;

                // SYSCALL - System Call
                case 4: if (CURRENT_STATE.REGS[2] == 10) {
                                RUN_BIT = 0;
                        }
                        break;
                }
        }
        else if (opcode_hi == 2) {
                switch (opcode_lo) {

                // MFHI - Move From Hi
                case 0: NEXT_STATE.REGS[rd] = CURRENT_STATE.HI;
                        break;

                // MTHI - Move To HI
                case 1: NEXT_STATE.HI = rs_value;
                        break;

                // MFLO - Move From LO
                case 2: NEXT_STATE.REGS[rd] = CURRENT_STATE.LO;
                        break;

                // MTLO - Move To LO
                case 3: NEXT_STATE.LO = rs_value;
                        break;
                }
        }
        else if (opcode_hi == 3) {
                int64_t product = 0;

                switch (opcode_lo) {

                // MULT - Multiply
                case 0: product = (int32_t)rs_value * (int32_t)CURRENT_STATE.REGS[rt];
                        NEXT_STATE.HI = product >> 32;
                        NEXT_STATE.LO = product;
                        break;

                // MULTU - Multiply Unsigned
                case 1: product = rs_value * CURRENT_STATE.REGS[rt];
                        NEXT_STATE.HI = product >> 32;
                        NEXT_STATE.LO = product;
                        break;

                // DIV - Divide
                case 2: NEXT_STATE.HI = (int32_t)rs_value % (int32_t)CURRENT_STATE.REGS[rt];
                        NEXT_STATE.LO = (int32_t)rs_value / (int32_t)CURRENT_STATE.REGS[rt];
                        break;

                // DIVU - Divide Unsigned
                case 3: NEXT_STATE.HI = rs_value % CURRENT_STATE.REGS[rt];
                        NEXT_STATE.LO = rs_value / CURRENT_STATE.REGS[rt];
                        break;
                }
        }
        else if (opcode_hi == 4) {
                switch (opcode_lo) {

                // ADD - Add
                // ADDU - Add Unsigned
                case 0:
                case 1: NEXT_STATE.REGS[rd] = rs_value + CURRENT_STATE.REGS[rt];
                        break;

                // SUB - Subtract
                // SUBU - Subtract Unsigned
                case 2:
                case 3: NEXT_STATE.REGS[rd] = rs_value - CURRENT_STATE.REGS[rt];
                        break;

                // AND - And
                case 4: NEXT_STATE.REGS[rd] = rs_value & CURRENT_STATE.REGS[rt];
                        break;

                // OR - Or
                case 5: NEXT_STATE.REGS[rd] = rs_value | CURRENT_STATE.REGS[rt];
                        break;

                // XOR - Xor
                case 6: NEXT_STATE.REGS[rd] = rs_value ^ CURRENT_STATE.REGS[rt];
                        break;

                // NOR - Nor
                case 7: NEXT_STATE.REGS[rd] = !(rs_value | CURRENT_STATE.REGS[rt]);
                        break;
                }
        }
        else if (opcode_hi == 5) {
                switch (opcode_lo) {

                // SLT - Set On Less Than
                case 2: if ((int32_t)rs_value < (int32_t)CURRENT_STATE.REGS[rt]) {
                                NEXT_STATE.REGS[rd] = 1;
                        }
                        break;

                // SLTU - Set On Less Than Unsigned
                case 3: if (rs_value < CURRENT_STATE.REGS[rt]) {
                                NEXT_STATE.REGS[rd] = 1;
                        }
                        break;
                }
        }
}

void process_regimm(uint32_t instr)
{
	// extract the higher order 3 bits of the opcode
        uint32_t opcode_hi = (instr << 11) >> 30;

        // extract the lower order 3 bits of the opcode
        uint32_t opcode_lo = (instr << 13) >> 29;

	// break the instruction
	uint32_t rs = (instr << 6) >> 27;
        int32_t target = instr << 16;
        target = target >> 14;
	int32_t rs_value = CURRENT_STATE.REGS[rs];

        // drill down to the instruction
        if (opcode_hi == 0) {
                switch (opcode_lo) {

                // BLTZ - Branch On Less Than Zero
                case 0: if ((rs_value >> 31) == -1) {
                                NEXT_STATE.PC = CURRENT_STATE.PC + target;
                        }
                        break;

                // BGEZ - Branch On Greater Than Or Equal To Zero
                case 1: if ((rs_value >> 31) == 0) {
                                NEXT_STATE.PC = CURRENT_STATE.PC + target;
                        }
                        break;
                }
        }
        else if (opcode_hi == 2) {
                switch (opcode_lo) {

                // BLTZAL - Branch On Less Than Zero And Link
                case 0: NEXT_STATE.REGS[31] = NEXT_STATE.PC;
                        if ((rs_value >> 31) == -1) {
                                NEXT_STATE.PC = CURRENT_STATE.PC + target;
                        }
                        break;

                // BGEZAL - Branch On Greater Than Or Equal To Zero And Link
                case 1: NEXT_STATE.REGS[31] = NEXT_STATE.PC;
                        if ((rs_value >> 31) == 0) {
                                NEXT_STATE.PC = CURRENT_STATE.PC + target;
                        }
                        break;
                }
        }
}
