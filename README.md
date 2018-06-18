# MIPSIM

This is a suite of simulators for a limited subset of the MIPS32 ISA. It includes:

1. A cycle-accurate instruction level simulator.
1. A timing simulator

This project is derived from [CMU 18-447][course_link]'s labs. The shells have been used as-is. The simulator cores have been created from scratch.

## Core structure

The opcodes have been divided into three categories:

1. Instructions that can be interpreted directly from opcode.
2. "SPECIAL" instructions that have a SHAMT field.
3. "REGIMM" instructions that have a TARGET field.

Separate functions pertaining to each type have been defined.

## Running the simulator

The simulator requires the MIPS assembly code to be converted into a binary file. This can be done with SPIM.

Usage: sim \<input file\>

For a list of commands, type '?' without the quotes into the prompt.

[course_link]: https://www.ece.cmu.edu/~ece447/s15/doku.php?id=start